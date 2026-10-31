"""Read-only official C/Python bridge, not a playable simulator.

Usage: python3 tests/official_api_test.py [path/to/libgodfield.dylib]
"""
import ctypes as c
from dataclasses import FrozenInstanceError
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
from official_data import OfficialData, _Card, _Composition

LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None


class OfficialAPITest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = OfficialData(LIBRARY)
        cls.lib = cls.data._lib
        cls.by_image = {card.image_name: card.id for card in cls.data}

    def ids(self, *images):
        return [self.by_image[image] for image in images]

    def test_abi_sizes_offsets_and_scope(self):
        self.assertEqual(c.sizeof(_Card), 44)
        self.assertEqual((_Card.id.offset, _Card.category.offset, _Card.element.offset,
                          _Card.atk.offset, _Card.guardian_attack_rate.offset), (0, 2, 3, 8, 40))
        self.assertEqual(c.sizeof(_Composition), 24)
        self.assertEqual((_Composition.magic_cost.offset, _Composition.attack_element.offset,
                          _Composition.defense_element.offset, _Composition.reserved.offset), (8, 16, 17, 18))
        self.assertEqual(self.lib.gfo_abi_version(), 1)
        self.assertEqual(self.data.ruleset, "official-catalog-client-rules-v1")
        self.assertIn("not-playable", self.data.scope)
        self.assertFalse(hasattr(self.data, "step"))

    def test_catalog_all_fields_match_frozen_source(self):
        source = json.loads((ROOT / "data/official-v1/cards.json").read_text())
        aliases = {"image_name": "imageName", "is_plus_atk": "isPlusAtk", "hit_rate": "hitRate",
                   "defense": "def", "ability_value": "abilityValue", "gift_rate": "giftRate",
                   "appearance_rate": "appearanceRate", "guardian_attack_rate": "guardianAttackRate"}
        self.assertEqual(len(source), len(self.data))
        text_fields = {"name", "image_name", "category", "ability", "curse", "guardian"}
        for original, actual in zip(source, self.data):
            with self.subTest(id=actual.id):
                for field in actual.__dataclass_fields__:
                    default = None if field == "element" else "" if field in text_fields else False if field == "is_plus_atk" else 0
                    self.assertEqual(getattr(actual, field), original.get(aliases.get(field, field), default), field)
        self.assertEqual(sum(card.gift_rate for card in self.data), 500)
        self.assertEqual(self.data.gift_rate, 500)

    def test_hashes_namespace_and_readonly(self):
        manifest = json.loads((ROOT / "data/official-v1/manifest.json").read_text())
        self.assertEqual(self.data.catalog_sha256, manifest["cards_sha256"])
        self.assertEqual(self.data.client_sha256, manifest["client_sha256"])
        # Legacy model2 is EXCHANGE; official model2 is a distinct pseudo-model.
        self.assertEqual(self.data.get_card(2).ability, "sacrifice")
        self.assertEqual(self.data.get_card(3).ability, "exchange")
        self.assertEqual(self.data.get_card(235).cost, 7)
        with self.assertRaises(FrozenInstanceError):
            self.data.get_card(66).atk = 99
        with self.assertRaises(AttributeError):
            self.data.scope = "environment"

    def test_native_string_invalid_and_none(self):
        for value in (0, 297, 65535):
            self.assertIsNone(self.lib.gfo_card_name(value))
            self.assertIsNone(self.lib.gfo_card_image_name(value))
        for name in ("category", "ability", "curse", "guardian"):
            self.assertIsNone(getattr(self.lib, f"gfo_{name}_name")(255))
        self.assertIsNone(self.lib.gfo_element_name(-2))
        self.assertIsNone(self.lib.gfo_element_name(6))
        self.assertEqual(self.lib.gfo_element_name(-1), b"")
        for name in ("ability", "curse", "guardian"):
            self.assertEqual(getattr(self.lib, f"gfo_{name}_name")(0), b"")

    def test_native_get_card_errors_preserve_output(self):
        out = _Card()
        c.memset(c.byref(out), 0xA5, c.sizeof(out))
        before = bytes(out)
        for value in (0, 297, 65535):
            self.assertEqual(self.lib.gfo_get_card(value, c.byref(out)), -2)
            self.assertEqual(bytes(out), before)
        self.assertEqual(self.lib.gfo_get_card(1, None), -1)

    def test_native_compose_errors_preserve_output(self):
        out = _Composition()
        c.memset(c.byref(out), 0xA5, c.sizeof(out))
        before = bytes(out)
        good = (c.c_uint16 * 1)(13)
        bad = (c.c_uint16 * 2)(13, 297)
        double = (c.c_uint16 * 2)(13, self.by_image["aura"])
        cases = [(None, 1, 0, -1), (None, 0, 0, -2), (good, 0, 0, -2),
                 (good, 1, -1, -2), (bad, 2, 0, -2),
                 (double, 2, 0x7FFFFFFFFFFFFFFF, -3),
                 (good, c.c_size_t(-1).value, 0, -2)]
        for ids, count, override, status in cases:
            self.assertEqual(self.lib.gfo_compose(ids, count, override, c.byref(out)), status)
            self.assertEqual(bytes(out), before)
        self.assertEqual(self.lib.gfo_compose(good, 1, 0, None), -1)

    def test_native_buffer_canaries_reserved_zero(self):
        class CardGuard(c.Structure):
            _fields_ = [("before", c.c_uint64), ("card", _Card), ("after", c.c_uint64)]
        class CompositionGuard(c.Structure):
            _fields_ = [("before", c.c_uint64), ("value", _Composition), ("after", c.c_uint64)]
        for value in (1, 66, 296):
            guard = CardGuard()
            c.memset(c.byref(guard), 0xA7, c.sizeof(guard))
            before, after = guard.before, guard.after
            self.assertEqual(self.lib.gfo_get_card(value, c.byref(guard.card)), 0)
            self.assertEqual((guard.before, guard.after), (before, after))
        guard = CompositionGuard()
        c.memset(c.byref(guard), 0xA7, c.sizeof(guard))
        before, after = guard.before, guard.after
        ids = (c.c_uint16 * 2)(13, 66)
        self.assertEqual(self.lib.gfo_compose(ids, 2, 0, c.byref(guard.value)), 0)
        self.assertEqual((guard.before, guard.after), (before, after))
        self.assertEqual(list(guard.value.reserved), [0] * 6)

    def test_current_setter_adds_power_and_replaces_element(self):
        fire = self.data.compose(self.ids("bronze-club", "wand-of-ignition"))
        water = self.data.compose(self.ids("bronze-club", "wand-of-mystic-water"))
        self.assertEqual((fire.attack_power, fire.attack_element, fire.defense_element), (3, "fire", None))
        self.assertEqual((water.attack_power, water.attack_element), (6, "water"))

    def test_doubling_order_and_base_override(self):
        a = self.data.compose(self.ids("bronze-club", "wand-of-ignition", "aura"))
        b = self.data.compose(self.ids("bronze-club", "aura", "wand-of-ignition"))
        self.assertEqual((a.attack_power, b.attack_power), (6, 4))
        self.assertEqual((a.magic_cost, b.magic_cost), (6, 6))
        self.assertEqual(self.data.compose(self.ids("magical-stick", "crossbow"), 20).attack_power, 22)
        self.assertEqual(self.data.compose(self.ids("bronze-club"), 0).attack_power, 1)
        self.assertEqual(self.data.compose([13], 0x7FFFFFFFFFFFFFFF).attack_power, 0x7FFFFFFFFFFFFFFF)

    def test_free_skip_attack_but_not_defense_merge(self):
        result = self.data.compose(self.ids("bronze-club", "wand-of-ignition", "spiritual-staff"))
        self.assertEqual((result.attack_power, result.attack_element), (3, "fire"))
        result = self.data.compose(self.ids("flame-boots", "spiritual-socks"))
        self.assertEqual((result.attack_element, result.defense_element), ("fire", None))

    def test_each_immediate_magic_free_pair_waives_cost(self):
        self.assertEqual(self.data.compose(self.ids("spring")).magic_cost, 7)
        self.assertEqual(self.data.compose(self.ids("spring", "spiritual-doll", "aura", "spiritual-socks")).magic_cost, 0)
        self.assertEqual(self.data.compose(self.ids("spring", "crossbow", "spiritual-doll")).magic_cost, 7)
        self.assertEqual(self.data.compose(self.ids("spring", "spiritual-doll", "spring")).magic_cost, 7)
        self.assertEqual(self.data.compose(self.ids("sell", "spring")).magic_cost, 0)

    def test_element_filter_then_later_setter(self):
        result = self.data.compose(self.ids("wand-of-ignition", "rainbow-curtain", "wand-of-mystic-water"))
        self.assertEqual(result.attack_element, "water")
        self.assertIsNone(result.defense_element)
        self.assertIsNone(self.data.compose(self.ids("shining-high-heels", "pluto-ring")).defense_element)

    def test_duplicate_model_ids_are_physical_copies_not_slots(self):
        self.assertEqual(self.data.compose(self.ids("bronze-club", "crossbow", "crossbow")).attack_power, 5)
        # A composition calculator intentionally accepts selections that the UI
        # would reject; it is not an action validator or a transition engine.
        self.assertEqual(self.data.compose(self.ids("spring", "crossbow")).magic_cost, 7)

    def test_python_bounds_and_overflow(self):
        for bad in (-1, 0, 297, 65536, True, 1.0, "1", None):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.data.get_card(bad)
        for bad in ([], [0], [297], [True], [1.0]):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.data.compose(bad)
        for bad in (-1, 2**63, True, 1.0, None):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.data.compose([13], bad)
        with self.assertRaises(OverflowError):
            self.data.compose(self.ids("bronze-club", "crossbow"), 2**63 - 1)

    def test_frozen_official_javascript_composition_vectors(self):
        # These vectors were executed in the original official JS functions,
        # independently from this C++/Python bridge.
        checked = 0
        elements = [None, "fire", "water", "wood", "stone", "light", "darkness"]
        for line in (ROOT / "data/official-v1/client-vectors.tsv").read_text().splitlines():
            if not line.startswith("C\t"):
                continue
            _, override, ordered, power, atk_element, def_element, cost = line.split("\t")
            result = self.data.compose(map(int, ordered.split(",")), int(override))
            self.assertEqual((result.attack_power, result.attack_element, result.defense_element, result.magic_cost),
                             (int(power), elements[int(atk_element) + 1], elements[int(def_element) + 1], int(cost)))
            checked += 1
        self.assertGreater(checked, 296)


if __name__ == "__main__":
    unittest.main()
