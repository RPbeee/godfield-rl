"""Received UI projection tests; no Match/server-state equivalence is asserted."""
import copy
from dataclasses import FrozenInstanceError
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"python"))
from official_state import OfficialState, MAX_WIRE_INTEGER
from official_display import ActiveDisplay, NextAttack, SetTargetPlayer, AdvanceGF
from official_match import Match, InvalidMatchError
LIBRARY = sys.argv.pop(1) if len(sys.argv)>1 else None

def fixture():
    return {"gf": 17, "attackTurnPlayerId": 0, "players": [
        {"id": 0, "name": "private owner", "hp": 40, "mp": 10, "cp": 20, "curses": ["fog"],
         "items": [{"id": 0, "modelId": 6}]},
        {"id": 7, "name": "private enemy", "hp": 91, "mp": 99, "cp": 78, "guardian": "earth"},
        {"id": 9, "hp": 0, "dead": True}]}

class ActiveDisplayTests(unittest.TestCase):
    def display(self, raw=None, turn=None):
        with OfficialState(fixture() if raw is None else raw, LIBRARY) as state:
            value = ActiveDisplay(state, turn)
        self.addCleanup(value.close); return value

    def applied(self, value, events):
        result = value.apply(events); self.addCleanup(result.close); return result

    def test_initialization_copy_default_turn_and_private_noninterference(self):
        raw = fixture(); state = OfficialState(raw, LIBRARY); initial_board = state.view(player_id=0)
        value = ActiveDisplay(state); self.addCleanup(value.close)
        self.assertEqual(value.view(), {"schema": "official-active-display-view-v1", "gf": 17, "reversed": False,
            "panes": {"left": 0, "right": None}, "selected_side": None, "attack": None})
        self.assertEqual(state.view(player_id=0), initial_board)
        state.close(); raw["players"].clear()
        self.assertEqual(value.view()["panes"]["left"], 0)
        changed = fixture(); changed["players"][0].update(hp=-100, mp=MAX_WIRE_INTEGER, cp=-300, name="other private", curses=["dream"])
        changed["players"][1]["items"] = [{"id": i, "modelId": 235, "used": True} for i in range(40)]
        self.assertEqual(self.display(changed).view(), value.view())
        self.assertEqual(self.display(turn=7).view()["panes"], {"left": 7, "right": None})
        with self.assertRaises(ValueError): self.display(turn=999)
        with self.assertRaises(ValueError): self.display(turn=True)
        for invalid in ({}, None, value):
            with self.assertRaises(ValueError): ActiveDisplay(invalid)

    def test_ordered_batch_matches_incremental_and_displays_real_target_zero(self):
        value = self.display(); original = value.view()
        models = [58]; event = NextAttack(source=7, models=models, guardian="earth", atk=20,
            target=999, buying_model=235, reversed=True, mp=-123, cp=123)
        models[0] = 6; self.assertEqual(event.models, (58,))
        with self.assertRaises(FrozenInstanceError): event.source = 0
        first = self.applied(value, [event]); drawn = first.view()
        self.assertEqual((drawn["reversed"], drawn["panes"], drawn["selected_side"]), (True, {"left": None, "right": 7}, None))
        self.assertIsNone(drawn["attack"]["target"]); self.assertIsNone(drawn["attack"]["buying_model"])
        self.assertEqual(drawn["attack"]["summary"]["power"], 20)
        self.assertTrue(drawn["attack"]["cards"][0]["from_mp"])
        self.assertNotIn("mp", drawn["attack"])
        second = self.applied(first, [SetTargetPlayer(0)]); selected = second.view()
        self.assertEqual((selected["panes"], selected["selected_side"], selected["attack"]["target"]),
                         ({"left": 0, "right": 7}, "left", 0))
        same_source = self.applied(second, [SetTargetPlayer(7)]).view()
        self.assertEqual((same_source["panes"], same_source["selected_side"]), ({"left": 0, "right": 7}, "right"))
        third = self.applied(second, [AdvanceGF(7)])
        batch = self.applied(value, [event, SetTargetPlayer(0), AdvanceGF(7)])
        self.assertEqual(batch.view(), third.view())
        self.assertEqual(third.view(), {"schema": "official-active-display-view-v1", "gf": 18, "reversed": False,
            "panes": {"left": 7, "right": None}, "selected_side": None, "attack": None})
        self.assertEqual(value.view(), original)

    def test_fresh_active_head_and_received_next_attack_have_distinct_target_rules(self):
        raw = fixture(); raw["attacks"] = [{"playerId": 0, "itemModelIds": [6], "reversed": True,
            "targetPlayerId": 7, "buyingItemModelId": 235}]
        with OfficialState(raw, LIBRARY) as state:
            value = ActiveDisplay(state); self.addCleanup(value.close)
            self.assertEqual(value.view()["attack"], state.active_attack())
        initial = value.view()
        self.assertEqual((initial["panes"], initial["selected_side"]), ({"left": 7, "right": 0}, "left"))
        changed = self.applied(value, [SetTargetPlayer(0)]).view()
        self.assertEqual((changed["panes"], changed["attack"]["target"], changed["attack"]["buying_model"]),
                         ({"left": 7, "right": 0}, 0, 235))
        next_frame = self.applied(value, [NextAttack(source=0, models=(6,), target=7, buying_model=235)])
        self.assertEqual(next_frame.view()["panes"], {"left": 0, "right": None})
        self.assertIsNone(next_frame.view()["attack"]["target"]); self.assertIsNone(next_frame.view()["attack"]["buying_model"])
        with Match(library=LIBRARY) as match:
            with self.assertRaises(InvalidMatchError): match.load_scene(value, 0)

    def test_strict_events_and_whole_batch_failure_preserve_source(self):
        value = self.display(); before = value.view()
        bad = [None, {}, "nextAttack", NextAttack(source=True), NextAttack(source=-1), NextAttack(source=MAX_WIRE_INTEGER+1),
            NextAttack(models=(0,)), NextAttack(models=(297,)), NextAttack(models=(True,)), NextAttack(guardian=""),
            NextAttack(guardian="Mars"), NextAttack(guardian=9), NextAttack(atk=1.0), NextAttack(hit_rate=True),
            NextAttack(mp=MAX_WIRE_INTEGER+1), NextAttack(cp=None), NextAttack(reversed=1),
            NextAttack(target=-1), NextAttack(buying_model=297), SetTargetPlayer(True), AdvanceGF(-1)]
        for event in bad:
            with self.subTest(event=event), self.assertRaises(ValueError): value.apply([event])
            self.assertEqual(value.view(), before)
        for events in (None, {}, "events", [AdvanceGF(7), SetTargetPlayer(999)],
                       [SetTargetPlayer(7), NextAttack(source=999)], [AdvanceGF(999)]):
            with self.assertRaises(ValueError): value.apply(events)
            self.assertEqual(value.view(), before)
        with self.assertRaises(ValueError): NextAttack(models=None)
        with self.assertRaises(ValueError): NextAttack(models="6")
        self.assertEqual(NextAttack(models=(i for i in (6, 9))).models, (6, 9))
        raw = fixture(); raw["gf"] = MAX_WIRE_INTEGER; exhausted = self.display(raw)
        with self.assertRaises(ValueError): exhausted.apply([AdvanceGF(7)])
        self.assertEqual(exhausted.view()["gf"], MAX_WIRE_INTEGER)

    def test_clone_ending_lifetimes_and_no_fixed_hand_bound_for_public_columns(self):
        value = self.display(); clone = self.applied(value, [])
        expected = value.view(); value.close(); value.close()
        self.assertTrue(value.closed); self.assertEqual(clone.view(), expected)
        public = self.applied(clone, [NextAttack(source=9, models=(6,)*19, target=MAX_WIRE_INTEGER)])
        self.assertEqual(public.view()["attack"]["source"], 9)  # display lookup, not alive/actor permission
        self.assertEqual(public.view()["attack"]["models"], [6]*19)
        raw = fixture(); raw["isOver"] = True
        ending = self.display(raw, turn=999)
        self.assertEqual((ending.view()["panes"], ending.view()["attack"]), ({"left": None, "right": None}, None))
        for call in (value.view, lambda: value.apply([])):
            with self.assertRaises(ValueError): call()
        with self.display() as scoped: self.assertFalse(scoped.closed)
        self.assertTrue(scoped.closed)

if __name__ == "__main__": unittest.main()
