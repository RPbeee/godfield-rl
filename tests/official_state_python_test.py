"""Strict import, privacy projection, visible-only queries and ownership tests."""
import copy
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_state import OfficialState, ResponseContext, MAX_WIRE_INTEGER
LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else None

def fixture():
    return {"gf": 7, "tiebreakGF": 100, "isOver": False, "updateCount": 123,
        "attackTurnPlayerId": 900, "attacks": [{"playerId": 900, "custom": "first"}, {"playerId": 77, "custom": "tail"}],
        "events": [{"action": "unknown-future-event", "value": 0.25}], "extra": {"retained": True},
        "players": [
            {"id": 0, "userId": "u\0i", "name": "Alice\0!", "alias": None, "team": 1, "hp": 40, "mp": 0, "cp": 20,
             "curses": ["fog"], "guardian": "mars", "confusionCount": 2, "dead": False, "isBot": False, "rating": 1200, "ratingGain": -5,
             "items": [{"id": 0, "modelId": 6}, {"id": 2, "modelId": 235, "fakeModelId": 13}, {"id": 7, "modelId": 237, "used": True}, {"id": 8, "modelId": 9}]},
            {"id": 77, "userId": "ally", "name": "Ally", "alias": "", "team": 1, "hp": 31, "mp": 4, "cp": 8, "items": []},
            {"id": 900, "userId": "enemy", "name": "Real enemy", "alias": "Enemy alias", "team": 2, "hp": 91, "mp": 7, "cp": 50,
             "curses": ["cold"], "guardian": "venus", "items": [{"id": 0, "modelId": 235, "fakeModelId": 13, "used": True}, {"id": 22, "modelId": 94}]}]}

class OfficialStateTests(unittest.TestCase):
    def state(self, raw=None):
        state = OfficialState(fixture() if raw is None else raw, LIBRARY)
        self.addCleanup(state.close)
        return state

    def test_projection_ownership_tokens_and_raw_preservation(self):
        raw = fixture(); saved = copy.deepcopy(raw); state = self.state(raw)
        raw["players"][0]["hp"] = 1
        self.assertEqual(state.raw_snapshot, saved)
        copied = state.raw_snapshot; copied["attacks"].clear()
        self.assertEqual(len(state.raw_snapshot["attacks"]), 2)
        view = state.view(user_id="u\0i")
        self.assertEqual(view["viewer"], 0)
        self.assertEqual(view["players"][0]["name"], "Alice\0!")
        self.assertEqual([x["token"] for x in view["hand"]], [0, 8, 2, 7])
        disguised = next(x for x in view["hand"] if x["token"] == 2)
        self.assertEqual((disguised["face"], disguised["reference"], disguised["disguised"]), (13, 13, True))
        self.assertEqual(view["players"][1]["name"], "")
        self.assertEqual(view["players"][1]["status"]["hp"], 31)
        self.assertEqual(view["players"][2]["name"], "Enemy alias")
        self.assertTrue(view["players"][2]["fogged"])
        self.assertIsNone(view["players"][2]["status"])
        self.assertEqual(view["players"][2]["used_models"], [13])
        for key in ("events", "attacks", "updateCount", "userId", "rating", "extra"):
            self.assertNotIn(key, json.dumps(view))
        self.assertEqual(state.raw_snapshot["players"][0]["ratingGain"], -5)

    def test_foreign_hidden_noninterference_and_unbounded_foreign_inventory(self):
        raw = fixture(); original = self.state(raw).view(player_id=0)
        enemy = raw["players"][2]; enemy.update(hp=-123, mp=1000, cp=99999, curses=["hell", "heaven"], guardian="moon")
        enemy["items"] = [{"id": 0, "modelId": 235, "fakeModelId": 13, "used": True}] + [{"id": i + 1, "modelId": 6 + i % 10} for i in range(45)]
        self.assertEqual(self.state(raw).view(player_id=0), original)
        with self.assertRaises(ValueError): self.state(raw).view(player_id=900)
        spectator = self.state(raw).view(spectator=True)
        self.assertEqual(spectator["hand"], [])
        self.assertFalse(spectator["players"][2]["fogged"])

    def test_death_and_game_over_fresh_reference(self):
        for ended in ("dead", "over"):
            raw = fixture()
            if ended == "dead": raw["players"][0]["dead"] = True
            else: raw["isOver"] = True
            view = self.state(raw).view(player_id=0)
            self.assertFalse(view["players"][2]["fogged"])
            self.assertEqual(next(x for x in view["hand"] if x["token"] == 2)["reference"], 235)
            if ended == "dead": self.assertFalse(view["players"][0]["status"]["confused"])

    def test_visible_selection_does_not_use_hidden_true_card(self):
        state = self.state()
        self.assertTrue(state.can_append_hand([], 2, player_id=0))
        self.assertTrue(state.selection_ready([2], player_id=0))  # MP0, visible punch rather than true Spring.
        self.assertTrue(state.can_append_hand([0], 8, player_id=0))
        self.assertFalse(state.can_append_hand([0], 0, player_id=0))
        self.assertFalse(state.selection_ready([999], player_id=0))
        self.assertFalse(state.selection_ready([], player_id=0))  # Visible WEAPON exists.
        self.assertFalse(state.selection_ready([7], player_id=0))  # True held tone not affordable.
        self.assertEqual(state.raw_snapshot, fixture())

    def test_response_flash_cannot_be_spoofed(self):
        raw = {"players": [{"id": 3, "mp": 10, "curses": [], "items": [{"id": 1, "modelId": 113}, {"id": 2, "modelId": 114}]}]}
        # Both are ordinary NE armor, so two-card response is allowed without flash.
        normal = self.state(raw)
        self.assertTrue(normal.can_append_hand([1], 2, player_id=3, response=ResponseContext(flash=True)))
        raw["players"][0]["curses"] = ["flash"]
        cursed = self.state(raw)
        self.assertFalse(cursed.can_append_hand([1], 2, player_id=3, response=ResponseContext(flash=False)))

    def test_explicit_viewer_unknown_ambiguous_and_zero(self):
        state = self.state()
        for kwargs in ({}, {"player_id": 0, "spectator": True}, {"user_id": "missing"}, {"player_id": 1}, {"player_id": True}, {"spectator": 1}):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError): state.view(**kwargs)
        raw = fixture(); raw["players"][1]["userId"] = "u\0i"
        with self.assertRaises(ValueError): self.state(raw).view(user_id="u\0i")
        raw = {"players": [{"id": 0, "userId": "", "alias": "", "team": None}]}
        self.assertEqual(self.state(raw).view(user_id="")["viewer"], 0)

    def test_strict_integer_bool_model_enum_shapes(self):
        mutations = [lambda r: r.update(players={}), lambda r: r.update(players=[{}]),
            lambda r: r["players"].append(copy.deepcopy(r["players"][0])),
            lambda r: r.update(players=[{"id": i} for i in range(10)]),
            lambda r: r.update(gf=True), lambda r: r.update(updateCount=-1), lambda r: r.update(gf=MAX_WIRE_INTEGER + 1),
            lambda r: r["players"][0].update(hp=1.0), lambda r: r["players"][0].update(team=5), lambda r: r["players"][0].update(team=False),
            lambda r: r["players"][0].update(dead=0), lambda r: r["players"][0].update(guardian="unknown"),
            lambda r: r["players"][0].update(curses=["unknown"]), lambda r: r["players"][0].update(curses=[""]),
            lambda r: r["players"][0].update(name="\ud800"), lambda r: r["players"][0].update(userId=3),
            lambda r: r["players"][0]["items"][0].update(modelId=0), lambda r: r["players"][0]["items"][0].update(fakeModelId=None),
            lambda r: r["players"][0]["items"][0].update(id=True), lambda r: r["players"][0]["items"][0].update(used=1),
            lambda r: r["players"][0]["items"].append({"id": 0, "modelId": 6}), lambda r: r.update(attacks=[None])]
        for mutate in mutations:
            raw = fixture(); mutate(raw)
            with self.subTest(raw=raw), self.assertRaises(ValueError): self.state(raw)

    def test_json_duplicate_nonfinite_and_close(self):
        for raw in ['{"players":[],"players":[]}', '{"players":[{"id":0,"id":1}]}', '{"players":[],"extra":NaN}', '{"players":[],"extra":1e999}', '{"players":[],"extra":9007199254740992}', '{']:
            with self.subTest(raw=raw), self.assertRaises(ValueError): self.state(raw)
        state = self.state(json.dumps(fixture()).encode())
        self.assertEqual(state.view(player_id=0)["gf"], 7)
        state.close(); state.close(); self.assertTrue(state.closed)
        with self.assertRaises(ValueError): state.view(spectator=True)
        with self.assertRaises(ValueError): state.selection_ready([], player_id=0)
        with self.state() as context: self.assertFalse(context.closed)
        self.assertTrue(context.closed)

    def test_selected_bound_and_context_validation(self):
        state = self.state()
        for selected in ([0] * 19, [True], [MAX_WIRE_INTEGER + 1]):
            with self.subTest(selected=selected), self.assertRaises(ValueError): state.selection_ready(selected, player_id=0)
        for response in ({}, ResponseContext(incoming_element="neutral"), ResponseContext(incoming_is_attack=1)):
            with self.subTest(response=response), self.assertRaises(ValueError): state.can_append_hand([], 0, player_id=0, response=response)

if __name__ == "__main__": unittest.main()
