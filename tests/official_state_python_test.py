"""Strict import, privacy projection, visible-only queries and ownership tests."""
import copy
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_state import OfficialState, ReceivedState, EmptySlot, ResponseContext, MAX_WIRE_INTEGER
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

    def received(self, raw=None):
        state = ReceivedState(fixture() if raw is None else raw, LIBRARY)
        self.addCleanup(state.close)
        return state

    def test_received_separates_empty_rows_without_losing_real_zero_token(self):
        physical = fixture(); raw = copy.deepcopy(physical)
        raw["players"][0]["items"].insert(0, {"id": 0, "modelId": 0})
        raw["players"][0]["items"].append({"id": 0, "modelId": 0, "fakeModelId": 0, "used": False})
        raw["players"][2]["items"].append({"id": 0, "modelId": 0})
        received = self.received(raw); strict = self.state(physical)
        self.assertEqual(received.raw_snapshot, raw)
        self.assertEqual(received.physical_snapshot, physical)
        self.assertEqual(received.empty_slots, (EmptySlot(0, 0), EmptySlot(0, 5), EmptySlot(900, 2)))
        self.assertEqual(received.view(player_id=0), strict.view(player_id=0))
        self.assertEqual(received.active_attack(), strict.active_attack())
        self.assertTrue(received.can_append_hand([], 0, player_id=0))
        self.assertEqual(received.selection_ready([0], player_id=0), strict.selection_ready([0], player_id=0))
        self.assertEqual(sum(i["token"] == 0 for i in received.view(player_id=0)["hand"]), 1)
        for key in ("empty_slots", "wire_index", "events", "userId"):
            self.assertNotIn(key, json.dumps(received.view(player_id=0)))
        with self.assertRaises(ValueError): self.state(raw)  # Existing strict constructor is unchanged.
        absent = {"players": [{"id": 0}], "events": []}
        self.assertEqual(self.received(absent).physical_snapshot, absent)

    def test_received_canonical_empty_strict_types_and_physical_duplicates(self):
        invalid = [{"id": i, "modelId": 0} for i in (True, -1, 1, MAX_WIRE_INTEGER + 1)]
        invalid += [{"id": 0, "modelId": v} for v in (False, None, 0.0, -1, 297)]
        invalid += [{"id": 0, "modelId": 0, "fakeModelId": v} for v in (True, None, 1, -1)]
        invalid += [{"id": 0, "modelId": 0, "used": v} for v in (True, 0, "false", None)]
        invalid += [{"modelId": 0}, None, {"id": 0, "modelId": 0, "opaque": MAX_WIRE_INTEGER + 1}]
        for item in invalid:
            with self.subTest(item=item), self.assertRaises(ValueError):
                self.received({"players": [{"id": 0, "items": [item]}]})
        duplicate = {"players": [{"id": 0, "items": [{"id": 0, "modelId": 0}, {"id": 0, "modelId": 6}, {"id": 0, "modelId": 7}]}]}
        with self.assertRaises(ValueError): self.received(duplicate)
        for raw in ({"players": {}}, {"players": [{"id": False}]}, {"players": [{"id": 0, "items": None}]},
                    {"players": [{"id": 0}, {"id": 0, "items": [{"id": 0, "modelId": 0}]}]}):
            with self.subTest(raw=raw), self.assertRaises(ValueError): self.received(raw)
        with self.assertRaises(ValueError): self.received('{"players":[],"players":[]}')

    def test_received_owns_raw_physical_and_metadata_independently(self):
        raw = fixture(); raw["players"][0]["items"].append({"id": 0, "modelId": 0})
        saved = copy.deepcopy(raw); state = self.received(raw)
        raw["players"][0]["items"].clear(); self.assertEqual(state.raw_snapshot, saved)
        exposed = state.raw_snapshot; exposed["players"].clear()
        physical = state.physical_snapshot; physical["attacks"].clear(); physical["players"][0]["items"].clear()
        self.assertEqual(state.raw_snapshot, saved)
        self.assertEqual(len(state.physical_snapshot["players"][0]["items"]), 4)
        self.assertEqual(len(state.physical_snapshot["attacks"]), 2)
        with self.assertRaises(AttributeError): state.empty_slots = ()
        with self.assertRaises(AttributeError): state.empty_slots[0].wire_index = 0
        for args in ((True, 0), (0, -1), (0, MAX_WIRE_INTEGER + 1)):
            with self.assertRaises(ValueError): EmptySlot(*args)
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "received.json"; path.write_text(json.dumps(saved))
            with ReceivedState.from_file(path, LIBRARY) as loaded:
                self.assertEqual(loaded.raw_snapshot, saved)
            self.assertTrue(loaded.closed)

    def test_received_projection_privacy_and_physical_renderer_bound(self):
        raw = fixture(); raw["players"][0]["items"].append({"id": 0, "modelId": 0})
        state = self.received(raw); changed = copy.deepcopy(raw)
        changed["players"][2]["items"] += [{"id": 0, "modelId": 0}] * 20
        self.assertNotEqual(self.received(changed).empty_slots, state.empty_slots)
        self.assertEqual(self.received(changed).view(player_id=0), state.view(player_id=0))
        for count in (18, 19):
            items = [{"id": i, "modelId": 6} for i in range(count)] + [{"id": 0, "modelId": 0}] * 5
            received = self.received({"players": [{"id": 0, "items": items}]})
            if count == 18: self.assertEqual(len(received.view(player_id=0)["hand"]), 18)
            else:
                with self.assertRaises(ValueError): received.view(player_id=0)
        dead = self.received({"isOver": True, "players": [{"id": 0, "dead": True, "hp": 0, "items": [{"id": 0, "modelId": 0}]}]})
        self.assertEqual(dead.empty_slots, (EmptySlot(0, 0),))  # Empty is not a gift-debt claim.
        self.assertEqual(dead.view(player_id=0)["hand"], [])

    def test_received_active_tail_pending_boundary_and_closed_queries(self):
        from official_match import Match, Config, InvalidMatchError, UnsupportedMatchError
        raw = fixture(); raw["players"][0]["items"].insert(0, {"id": 0, "modelId": 0})
        raw["attacks"] = [{"playerId": 900, "targetPlayerId": 0, "itemModelIds": [6]}, {"playerId": False, "itemModelIds": "opaque-tail"}]
        state = self.received(raw)
        self.assertEqual(state.physical_snapshot["attacks"], raw["attacks"])
        self.assertEqual(state.physical_snapshot["events"], raw["events"])
        attack = state.active_attack(); attack["models"].clear()
        self.assertEqual(state.active_attack()["models"], [6])
        with Match(config=Config(initial_hand=0), library=LIBRARY) as match:
            before = (match.status, match.observe(0))
            with self.assertRaises(InvalidMatchError): match.load_scene(state, 0)
            self.assertEqual((match.status, match.observe(0)), before)
            with OfficialState(state.physical_snapshot, LIBRARY) as board:
                with self.assertRaises(UnsupportedMatchError): match.load_scene(board, 0)
        state.close(); state.close(); self.assertTrue(state.closed)
        for query in (lambda: state.view(spectator=True), state.active_attack,
                      lambda: state.can_append_hand([], 0, player_id=0), lambda: state.selection_ready([], player_id=0), state.__enter__):
            with self.assertRaises(ValueError): query()
        self.assertEqual(state.raw_snapshot, raw)  # Owned evidence remains available after close.

    def test_received_two_actual_sanitized_v60_snapshots(self):
        report = json.loads((Path(__file__).resolve().parents[1] / "data/official-v2/wire-game-updates-v60.json").read_text())
        self.assertEqual(len(report["updates"]), 2)
        for entry in report["updates"]:
            raw = entry["game"]; physical = copy.deepcopy(raw); slots = []
            for player in physical["players"]:
                original = player["items"]
                slots.extend(EmptySlot(player["id"], i) for i, item in enumerate(original) if item["modelId"] == 0)
                player["items"] = [item for item in original if item["modelId"] > 0]
            with self.subTest(update=raw["updateCount"]):
                received = self.received(raw); strict = self.state(physical)
                self.assertEqual(received.raw_snapshot, raw)
                self.assertEqual(received.physical_snapshot, physical)
                self.assertEqual(received.empty_slots, tuple(slots)); self.assertTrue(slots)
                self.assertEqual(received.view(spectator=True), strict.view(spectator=True))
                self.assertEqual(received.active_attack(), strict.active_attack())
                for player in raw["players"]:
                    self.assertEqual(received.view(player_id=player["id"]), strict.view(player_id=player["id"]))

    def test_received_v66_self_weapon_roles_slots_and_pending_buy(self):
        for scenario, viewer, before_model, after_model in (("absorb", 1, 45, 161), ("twice", 2, 14, 19)):
            report = json.loads((Path(__file__).resolve().parents[1] / f"data/official-v2/wire-game-updates-v66-{scenario}.json").read_text())
            self.assertEqual(report["roles"]["viewer_player_id"], viewer)
            for index, entry in enumerate(report["updates"]):
                with self.subTest(scenario=scenario, update=index+1):
                    raw = entry["game"]; state = self.received(raw); own = state.view(player_id=viewer)
                    player = next(p for p in own["players"] if p["id"] == viewer)
                    item = next(i for i in own["hand"] if i["token"] == 6)
                    self.assertEqual((item["face"], item["disguised"], item["used"]), (after_model if index else before_model, False, False))
                    self.assertEqual((player["status"]["hp"], player["status"]["mp"], player["status"]["cp"]),
                                     (34 if scenario == "twice" and index else 40, 10, 20))
                    self.assertEqual(len(own["hand"]), 9)
                    self.assertEqual(state.raw_snapshot, raw)
                    self.assertEqual(len(state.empty_slots), int(scenario == "absorb" and index == 1))
                    self.assertEqual(state.view(spectator=True)["hand"], [])
                    if scenario == "absorb" and index:
                        self.assertEqual(state.active_attack()["models"], [5])
                        self.assertEqual(state.active_attack()["target"], 1)
                        self.assertEqual(sum(i["face"] == 161 for i in own["hand"]), 2)
                    else:
                        self.assertIsNone(state.active_attack())

    def test_received_v65_fog_privacy_guardian_and_used_miracles(self):
        for scenario, hp, mp, counts, empty in (
                ("guardian", (40, 40, 45), (10, 10, 4), (9, 9, 10), (0, 2, 0)),
                ("fate", (40, 40, 40), (10, 10, 10), (9, 9, 9), (0, 1, 0))):
            report = json.loads((Path(__file__).resolve().parents[1] / f"data/official-v2/wire-game-updates-v65-{scenario}.json").read_text())
            for index, entry in enumerate(report["updates"]):
                with self.subTest(scenario=scenario, update=index+1):
                    raw = entry["game"]; state = self.received(raw); own = state.view(player_id=1)
                    self.assertEqual(state.raw_snapshot, raw)
                    self.assertEqual((raw["updateCount"], own["gf"]), (index+1, index+1))
                    self.assertEqual((own["players"][0]["status"]["hp"], own["players"][0]["status"]["mp"]), (hp[index], mp[index]))
                    self.assertEqual((len(own["hand"]), len(state.empty_slots)), (counts[index], empty[index]))
                    self.assertEqual(state.view(spectator=True)["hand"], [])
                    if scenario == "fate" and index:
                        self.assertTrue(own["players"][1]["fogged"])
                        self.assertIsNone(own["players"][1]["status"])
                    if scenario == "guardian" and index:
                        self.assertEqual(own["players"][0]["status"]["guardian"], "neptune")
                        cpu = state.view(player_id=2)
                        self.assertTrue(next(item for item in cpu["hand"] if item["token"] == 5)["used"])
                    if scenario == "guardian" and index == 2:
                        self.assertTrue(next(item for item in own["hand"] if item["token"] == 4)["used"])

    def test_received_v64_dream_presence_visible_faces_and_privileged_evidence(self):
        report = json.loads((Path(__file__).resolve().parents[1] / "data/official-v2/wire-game-updates-v64.json").read_text())
        expected = ((4, 4, 35, 15, 195), (5, 5, 34, 15, 195),
                    (6, 7, 34, 20, 197), (7, 9, 34, 35, 13))
        self.assertEqual(len(report["updates"]), len(expected))
        for entry, (update, gf, hp, mp, actual) in zip(report["updates"], expected):
            with self.subTest(update=update):
                raw = entry["game"]; received = self.received(raw); own = received.view(player_id=1)
                self.assertEqual((raw["updateCount"], own["gf"]), (update, gf))
                self.assertEqual((own["players"][0]["status"]["hp"], own["players"][0]["status"]["mp"]), (hp, mp))
                self.assertEqual(received.raw_snapshot, raw)
                self.assertEqual(received.empty_slots, (EmptySlot(2, 8),) if update == 4 else ())
                hand = {item["token"]: item for item in own["hand"]}
                self.assertEqual(len(hand), 9)
                expected_face = 13 if update == 7 else 195
                self.assertEqual((hand[3]["face"], hand[3]["reference"], hand[3]["disguised"], hand[3]["used"]),
                                 (expected_face, expected_face, True, False))
                evidence = next(item for item in received.physical_snapshot["players"][0]["items"] if item["id"] == 3)
                self.assertEqual((evidence["modelId"], evidence["fakeModelId"]), (actual, expected_face))
                if update == 4:
                    self.assertEqual((hand[1]["face"], hand[1]["reference"], hand[1]["disguised"]), (135, 135, True))
                    self.assertEqual(received.active_attack()["models"], [23])
                    self.assertEqual(received.active_attack()["summary"]["power"], 6)
                else:
                    self.assertEqual((hand[1]["face"], hand[1]["disguised"]), (213, False))
                    self.assertIsNone(received.active_attack())
                self.assertEqual(received.view(spectator=True)["hand"], [])
                self.assertNotIn("events", own)
                self.assertNotIn("empty_slots", own)

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

    def test_active_attack_is_separate_owned_public_head_projection(self):
        raw = fixture()
        raw["attacks"] = [{"playerId": 0, "guardian": "earth", "itemModelIds": [6], "atk": 7,
            "hitRate": 75, "mp": -3, "cp": 4, "targetPlayerId": 900, "buyingItemModelId": 235, "reversed": True,
            "unknown": {"kept": True}}, {"playerId": False, "itemModelIds": "opaque-tail", "atk": 0.5}]
        saved = copy.deepcopy(raw); state = self.state(raw)
        raw["attacks"][0]["itemModelIds"][0] = 235
        projection = state.active_attack()
        self.assertEqual({key: projection[key] for key in ("source", "guardian", "models", "target", "buying_model", "reversed")},
                         {"source": 0, "guardian": "earth", "models": [6],
                          "target": 900, "buying_model": 235, "reversed": True})
        self.assertEqual(projection["summary"], {"models": [6], "prayer": False, "is_attack": True, "element": None, "power": 7, "hit_rate": 75})
        self.assertEqual(projection["response"], {"incoming_category": "weapons", "incoming_element": None, "incoming_is_attack": True})
        self.assertNotIn("unknown", projection)
        for key in ("atk", "hit_rate", "mp", "cp"): self.assertNotIn(key, projection)
        unused_changed = copy.deepcopy(saved)
        unused_changed["attacks"][0].update(mp=12345, cp=-9876)
        self.assertEqual(self.state(unused_changed).active_attack(), projection)
        self.assertEqual(state.raw_snapshot, saved)
        projection["models"].clear()
        self.assertEqual(state.active_attack()["models"], [6])
        board_only = copy.deepcopy(saved); board_only.pop("attacks")
        self.assertEqual(state.view(player_id=0), self.state(board_only).view(player_id=0))
        self.assertTrue(state.can_append_hand([0], 8, player_id=0))
        state.close()
        with self.assertRaises(ValueError): state.active_attack()

    def test_active_attack_defaults_summary_and_over_suppression(self):
        self.assertIsNone(self.state({"players": []}).active_attack())
        self.assertIsNone(self.state({"players": [], "attacks": []}).active_attack())
        prayer = self.state({"players": [{"id": 0}], "attacks": [{}]}).active_attack()
        self.assertEqual((prayer["source"], prayer["guardian"], prayer["target"], prayer["buying_model"]), (0, None, None, None))
        self.assertEqual(prayer["summary"], {"models": [], "prayer": True, "is_attack": False, "element": None, "power": None, "hit_rate": None})
        self.assertIsNone(prayer["response"])
        for models, category in (([207, 6], "weapons"), ([264], "weapons"), ([286], "weapons"), ([223], "miracles")):
            raw = {"players": [{"id": 0}], "attacks": [{"itemModelIds": models, "buyingItemModelId": 235}]}
            attack = self.state(raw).active_attack()
            self.assertEqual(attack["models"], models)
            self.assertEqual(attack["summary"]["models"], models[1:] if models[0] == 207 else models)
            self.assertEqual(attack["response"]["incoming_category"], category)
            self.assertIsNone(attack["buying_model"])
            if models == [223]:
                self.assertIsNone(attack["summary"]["element"])
                self.assertEqual(attack["response"], {"incoming_category": "miracles", "incoming_element": "darkness", "incoming_is_attack": False})
            raw["isOver"] = True
            self.assertIsNone(self.state(raw).active_attack())
        long_frame = self.state({"players": [{"id": 0}], "attacks": [{"itemModelIds": [6] * 19}]})
        self.assertEqual(long_frame.active_attack()["summary"]["models"], [6] * 19)
        for endpoint in ({"playerId": 999}, {"targetPlayerId": 999}):
            raw = {"players": [{"id": 0}], "attacks": [endpoint]}
            state = self.state(raw)
            self.assertEqual(state.view(spectator=True)["players"][0]["id"], 0)
            with self.assertRaises(ValueError): state.active_attack()
            raw["isOver"] = True
            self.assertIsNone(self.state(raw).active_attack())

    def test_active_attack_strict_first_frame_and_opaque_tail(self):
        mutations = [lambda a: a.update(playerId=True), lambda a: a.update(playerId=-1),
            lambda a: a.update(targetPlayerId=MAX_WIRE_INTEGER+1), lambda a: a.update(atk=1.0),
            lambda a: a.update(hitRate=False), lambda a: a.update(mp=MAX_WIRE_INTEGER+1), lambda a: a.update(cp=None),
            lambda a: a.update(guardian="unknown"), lambda a: a.update(guardian=9), lambda a: a.update(reversed=1),
            lambda a: a.update(itemModelIds={}), lambda a: a.update(itemModelIds=[0]), lambda a: a.update(itemModelIds=[297]),
            lambda a: a.update(itemModelIds=[True]), lambda a: a.update(buyingItemModelId=None), lambda a: a.update(buyingItemModelId=297)]
        for mutate in mutations:
            raw = {"attacks": [{}]}; mutate(raw["attacks"][0])
            with self.subTest(raw=raw), self.assertRaises(ValueError): self.state(raw)
        # Invalid known fields after the head are evidence, not another projected frame.
        raw = {"players": [{"id": MAX_WIRE_INTEGER}], "attacks": [{"playerId": MAX_WIRE_INTEGER, "atk": -MAX_WIRE_INTEGER, "hitRate": MAX_WIRE_INTEGER,
                            "mp": -MAX_WIRE_INTEGER, "cp": MAX_WIRE_INTEGER}, {"itemModelIds": [0], "guardian": "unknown"}]}
        state = self.state(raw)
        self.assertEqual(state.active_attack()["source"], MAX_WIRE_INTEGER)
        self.assertEqual(state.raw_snapshot, raw)

if __name__ == "__main__": unittest.main()
