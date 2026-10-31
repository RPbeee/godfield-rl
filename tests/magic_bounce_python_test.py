"""Public ABI34 boundaries and mandatory physical magic bounce.

Usage: python3 tests/magic_bounce_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from magic_bounce_env import Action, ActionKind, MagicBounceBatchEnv, Phase, _NativeAction
from hp_gamble_env import HPGambleBatchEnv
from buying_env import BuyingBatchEnv
from selling_env import SellingBatchEnv
from pestle_mortar_env import PestleMortarBatchEnv
from self_damage_env import SelfDamageBatchEnv
from hp_absorption_env import HPAbsorptionBatchEnv
from item_removal_env import ItemRemovalBatchEnv
from revival_env import RevivalBatchEnv
from exchange_env import ExchangeBatchEnv
from forgetting_env import ForgettingBatchEnv
from magic_free_env import MagicFreeBatchEnv
from wide_miracles_env import WideMiraclesBatchEnv
from doubling_miracles_env import DoublingMiraclesBatchEnv
from additive_miracles_env import AdditiveMiraclesBatchEnv
from magic_block_env import MagicBlockBatchEnv
from mana_weapon_env import ManaWeaponBatchEnv
from any_reflection_env import AnyReflectionBatchEnv
from magic_reflection_env import MagicReflectionBatchEnv
from money_counter_env import MoneyCounterBatchEnv
from counter_extensions_env import CounterExtensionsBatchEnv
from counter_env import CounterBatchEnv
from attribute_removal_env import AttributeRemovalBatchEnv
from attribute_change_env import AttributeChangeBatchEnv
from defensive_miracles_env import DefensiveMiraclesBatchEnv
from offensive_miracles_env import OffensiveMiraclesBatchEnv
from celestial_env import CelestialBatchEnv
from harms_env import HarmsBatchEnv
from disease_env import DiseaseBatchEnv
from reflection_env import ReflectionBatchEnv
from chance_env import ChanceBatchEnv
from elements_env import ElementsBatchEnv
from miracles_env import MiraclesBatchEnv
from legacy_env import LegacyBatchEnv

LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None


def observation_copy(env):
    return tuple(tuple(buffer) for buffer in env.observe())


class MagicBounceBridgeTests(unittest.TestCase):
    IDS = (21, 119, 121, 127, 134, 140)

    def create(self, count=16, max_decisions=64):
        return MagicBounceBatchEnv(count, max_decisions, library=LIBRARY)

    def main_fixture(self, env, required):
        for seed in range(262144):
            features, _, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if all(card in ids for card in required):
                return seed, actors[0], ids
        self.fail(f"normal pool did not reach {required}")

    def magic_response(self, env, spell=206, carrier=None, prefix=None, glory=False):
        for seed in range(262144):
            features, _, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if spell not in ids or (glory and 70 not in ids):
                continue
            owner = actors[0]
            if glory:
                env.step([Action(ActionKind.ATTACK, ids.index(70), target=1 - owner)])
                env.step([Action(ActionKind.DEFEND)])
                ids = self.hand_ids(env.observe()[0])
                bounce = next((i for i, card in enumerate(ids) if card in self.IDS), None)
                if bounce is None:
                    continue
                discard = next(i for i, card in enumerate(ids) if card and i != bounce)
                env.step([Action(ActionKind.DISCARD, slots=1 << discard)])
                ids = self.hand_ids(env.observe()[0])
                if spell not in ids:
                    continue
            env.script_hit_rolls(0, [1, 100])
            env.step([Action(ActionKind.USE, ids.index(spell), target=1 - owner)])
            features, options, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (1 - owner, Phase.RESPONSE))
            ids = self.hand_ids(features)
            bounce = next((i for i, card in enumerate(ids) if card in self.IDS
                           and (carrier is None or card == carrier)), None)
            if bounce is None:
                continue
            extra = next((i for i, card in enumerate(ids) if card == prefix), None) if prefix else None
            if prefix and extra is None:
                continue
            if glory:
                extra = next((i for i in range(16) if i != bounce and options[2] & (1 << i)), None)
                if extra is None:
                    continue
            return seed, owner, bounce, extra
        self.fail("normal pool did not reach magic bounce response")

    def test_all_six_cards_reach_normal_pool_with_exact_membership(self):
        seen = set()
        with self.create(1) as env:
            for seed in range(8192):
                features, options, _, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                expected = sum(1 << i for i, card in enumerate(ids) if card in self.IDS)
                self.assertEqual(options[35], expected)
                self.assertEqual(features[1278], 1)
                for i, card in enumerate(ids):
                    self.assertEqual(features[1279 + i], int(card in self.IDS))
                seen.update(card for card in ids if card in self.IDS)
                if seen == set(self.IDS):
                    break
            self.assertEqual(seen, set(self.IDS))

    def test_weapon21_has_base_attack5_and_additive_boost5(self):
        with self.create(1, 128) as env:
            seed, owner, ids = self.main_fixture(env, (21, 5))
            features, options, _, _, _ = env.observe()
            slot = ids.index(21)
            self.assertAlmostEqual(features[16 + slot * 7 + 1], 5 / 30, places=6)
            self.assertAlmostEqual(features[16 + slot * 7 + 2], 5 / 20, places=6)
            self.assertTrue(options[0] & (1 << slot))
            self.assertTrue(options[1] & (1 << slot))
            env.step([Action(ActionKind.ATTACK, slot, target=owner)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 35 / 99, places=6)
            env.reset(seed)
            env.step([Action(ActionKind.ATTACK, ids.index(5), slots=1 << slot, target=owner)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 34 / 99, places=6)
            self.assertEqual(features[1295], 0)

    def test_five_armors_retain_nonmagic_dfs_including_zero(self):
        with self.create(1, 128) as env:
            for card, defense in zip(self.IDS[1:], range(5)):
                for seed in range(262144):
                    features, _, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    if 5 not in ids:
                        continue
                    owner = actors[0]
                    env.step([Action(ActionKind.ATTACK, ids.index(5), target=1 - owner)])
                    features, options, _, _, _ = env.observe()
                    ids = self.hand_ids(features)
                    if card in ids:
                        break
                else:
                    self.fail(f"no ordinary armor fixture {card}")
                slot = ids.index(card)
                self.assertTrue(options[2] & (1 << slot))
                env.step([Action(ActionKind.DEFEND, slots=1 << slot)])
                features, _, actors, _, _ = env.observe()
                offset = 0 if actors[0] != owner else 3
                self.assertAlmostEqual(features[offset], (40 - max(0, 1 - defense)) / 99, places=6)
                self.assertEqual(features[1295], 0)

    def test_magic_requires_special_and_self_or_other_bounce_uses_full_raw(self):
        with self.create(1, 128) as env:
            seed, owner, carrier, _ = self.magic_response(env, carrier=140)
            for target in (owner, 1 - owner):
                env.reset(seed)
                ids = self.hand_ids(env.observe()[0])
                env.step([Action(ActionKind.USE, ids.index(206), target=1 - owner)])
                carrier = self.hand_ids(env.observe()[0]).index(140)
                before = observation_copy(env)
                self.assertFalse(env.legal([Action(ActionKind.DEFEND, slots=1 << carrier)])[0])
                with self.assertRaises(ValueError):
                    env.step([Action(ActionKind.DEFEND, slots=1 << carrier)])
                self.assertEqual(observation_copy(env), before)
                env.script_bounce_targets(0, [target])
                env.step([Action(ActionKind.DEFEND, carrier)])
                if target == owner:
                    features, _, actors, phases, _ = env.observe()
                    self.assertEqual((actors[0], phases[0]), (owner, Phase.RESPONSE))
                    self.assertAlmostEqual(features[8], 10 / 480, places=6)
                    env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertAlmostEqual(features[0 if actors[0] == target else 3], 30 / 99, places=6)
                self.assertEqual(features[1295], 1)
                self.assertAlmostEqual(features[1296], 140 / 363, places=6)
                self.assertEqual(features[1297], int(actors[0] != owner))
                self.assertEqual(features[1298], int(actors[0] == target))
                self.assertEqual(features[1299], int(target != owner))
                self.assertAlmostEqual(features[1 if actors[0] != owner else 4], 10 / 99, places=6)

    def test_ordinary_prefix_is_consumed_but_reduction_does_not_survive_bounce(self):
        with self.create(1, 128) as env:
            _, owner, carrier, prefix = self.magic_response(env, prefix=126)
            defender = 1 - owner
            env.script_bounce_targets(0, [owner])
            env.step([Action(ActionKind.DEFEND, carrier, slots=1 << prefix)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (owner, Phase.RESPONSE))
            self.assertAlmostEqual(features[8], 10 / 480, places=6)
            self.assertAlmostEqual(self.hand_fraction(features, actors[0], defender), 8 / 16, places=6)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 30 / 99, places=6)
            self.assertAlmostEqual(self.hand_fraction(features, actors[0], defender), 10 / 16, places=6)

    def test_195_changes_dark_magic_to_ne_before_bounce(self):
        with self.create(1, 128) as env:
            _, owner, carrier, prefix = self.magic_response(env, spell=207, prefix=195)
            env.script_bounce_targets(0, [owner])
            env.step([Action(ActionKind.DEFEND, carrier, slots=1 << prefix)])
            features = env.observe()[0]
            self.assertEqual(features[392], 1)
            self.assertAlmostEqual(features[8], 5 / 480, places=6)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 35 / 99, places=6)

    def test_percent_magic_does_not_reroll_hit_after_bounce(self):
        with self.create(1, 128) as env:
            _, owner, carrier, _ = self.magic_response(env, spell=205)
            env.script_bounce_targets(0, [owner])
            env.step([Action(ActionKind.DEFEND, carrier)])
            features = env.observe()[0]
            self.assertAlmostEqual(features[202], .25, places=6)
            self.assertEqual(tuple(features[204:206]), (1, 1))
            self.assertAlmostEqual(features[206], 205 / 363, places=6)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 30 / 99, places=6)

    def test_utility_heal_bounces_without_mp_cost_or_new_holding(self):
        with self.create(1, 128) as env:
            _, owner, carrier, _ = self.magic_response(env, spell=216)
            defender = 1 - owner
            env.script_bounce_targets(0, [defender])
            env.step([Action(ActionKind.DEFEND, carrier)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == defender else 3], 50 / 99, places=6)
            self.assertAlmostEqual(features[1 if actors[0] == defender else 4], 10 / 99, places=6)
            if actors[0] == defender:
                self.assertEqual(features[129], 0)

    def test_bounce_works_at_zero_mp(self):
        with self.create(1, 128) as env:
            for seed in range(262144):
                features, _, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 2 not in ids or not any(card in self.IDS for card in ids):
                    continue
                defender = actors[0]
                env.step([Action(ActionKind.EXCHANGE, ids.index(2), extras=(40, 0, 30))])
                ids = self.hand_ids(env.observe()[0])
                if 206 not in ids:
                    continue
                env.step([Action(ActionKind.USE, ids.index(206), target=defender)])
                ids = self.hand_ids(env.observe()[0])
                carrier = next(i for i, card in enumerate(ids) if card in self.IDS)
                break
            else:
                self.fail("no zero-MP magic bounce fixture")
            env.script_bounce_targets(0, [defender])
            self.assertTrue(env.legal([Action(ActionKind.DEFEND, carrier)])[0])
            env.step([Action(ActionKind.DEFEND, carrier)])
            features, _, actors, _, _ = env.observe()
            self.assertEqual(features[1 if actors[0] == defender else 4], 0)

    def test_glory_limits_prefix_plus_bounce_to_one_selected_card(self):
        with self.create(1, 128) as env:
            _, _, carrier, prefix = self.magic_response(env, glory=True)
            self.assertEqual(env.observe()[0][258], 1)
            self.assertTrue(env.legal([Action(ActionKind.DEFEND, carrier)])[0])
            before = observation_copy(env)
            with self.assertRaises(ValueError):
                env.step([Action(ActionKind.DEFEND, carrier, slots=1 << prefix)])
            self.assertEqual(observation_copy(env), before)

    def test_free_suffix_is_not_consumed_and_does_not_waive_original_spell(self):
        with self.create(1, 128) as env:
            _, owner, carrier, free = self.magic_response(env, prefix=239)
            defender = 1 - owner
            env.script_bounce_targets(0, [defender])
            env.step([Action(ActionKind.DEFEND_ORDERED, extras=(carrier, free))])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[1 if actors[0] == owner else 4], 5 / 99, places=6)
            self.assertAlmostEqual(features[1 if actors[0] == defender else 4], 10 / 99, places=6)
            self.assertAlmostEqual(self.hand_fraction(features, actors[0], defender), 10 / 16, places=6)
            if actors[0] == defender:
                self.assertIn(239, self.hand_ids(features))

    def test_matching_bounce_prefix_cannot_opt_out_into_another_special(self):
        with self.create(1, 128) as env:
            _, _, carrier, any_slot = self.magic_response(env, prefix=196)
            before = observation_copy(env)
            for action in (Action(ActionKind.DEFEND, slots=1 << carrier),
                           Action(ActionKind.DEFEND, any_slot, slots=1 << carrier)):
                self.assertFalse(env.legal([action])[0])
                with self.assertRaises(ValueError):
                    env.step([action])
                self.assertEqual(observation_copy(env), before)

    def test_pestle_target_tape_validation_and_hidden_values(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            env.script_pestle_targets(0, [0, 1])
            self.assertEqual(observation_copy(env), before)
            for values in ((2,), (True,), (0.5,), (0,) * 65):
                with self.assertRaises(ValueError):
                    env.script_pestle_targets(0, values)
            native = env._lib.g34_script_pestle_targets
            self.assertEqual(native(None, 0, None, 0), -1)
            self.assertEqual(native(env._handle, 1, None, 0), -2)
            self.assertEqual(native(env._handle, 0, None, 1), -1)
            self.assertEqual(native(env._handle, 0, None, 65), -2)
            bad = (c.c_uint8 * 2)(0, 2)
            self.assertEqual(native(env._handle, 0, bad, 2), -3)
            self.assertEqual(observation_copy(env), before)
            env.script_pestle_targets(0, [])

    def test_hidden_ticket_prefixes_remain_private_and_checked(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            for name in ("forget_tickets", "item_removal_tickets", "buy_tickets"):
                setter = getattr(env, "script_" + name)
                native = getattr(env._lib, "g34_script_" + name)
                setter(0, [0, 2**64 - 1])
                self.assertEqual(observation_copy(env), before)
                for values in ((-1,), (2**64,), (True,), (0.5,), (0,) * 65):
                    with self.assertRaises(ValueError):
                        setter(0, values)
                self.assertEqual(native(None, 0, None, 0), -1)
                self.assertEqual(native(env._handle, 1, None, 0), -2)
                self.assertEqual(native(env._handle, 0, None, 1), -1)
                self.assertEqual(native(env._handle, 0, None, 65), -2)
                setter(0, [])
            self.assertEqual(observation_copy(env), before)

    def test_tail_layout_and_old_api_rejects_65th_reference(self):
        action = Action(ActionKind.ATTACK_ORDERED, 0, target=1, extras=(16,) * 64 + (19,))
        with self.create(1) as env:
            env._encode([action])
            self.assertEqual(env._actions[0].extra_count, 65)
            self.assertEqual(env._actions[0].extra_tail, 19)
            self.assertEqual(tuple(env._actions[0].extras), (16,) * 64)
        from wide_miracles_env import Action as OldAction, ActionKind as OldKind
        with self.assertRaises(ValueError):
            OldAction(OldKind.ATTACK_ORDERED, 0, target=1, extras=(16,) * 65)
        for extras in ((22,), (True,), (16,) * 66):
            with self.assertRaises(ValueError):
                Action(ActionKind.ATTACK_ORDERED, 0, target=1, extras=extras)

    def test_extended_layout_padding_and_order_validation(self):
        for order in ((-1,), (16,), (True,), tuple(range(16)) + (0,)):
            with self.assertRaises(ValueError):
                Action(ActionKind.DEFEND, counter_order=order)
        with self.create(2) as env:
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            before = observation_copy(env)
            for field, value in (("reserved2", 1), ("counter_count", 17), ("counter_order", 1 << 60)):
                env._encode(actions)
                setattr(env._actions[1], field, value)
                self.assertEqual(env._lib.g34_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    def hand_fraction(self, features, actor, seat):
        # Feature10 identifies the turn owner; only feature11 is a hand count.
        return sum(card != 0 for card in self.hand_ids(features)) / 16 if actor == seat else features[11]

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_thirty_four_profile_output_boundaries(self):
        """Earlier profiles retain their fixed row strides and complete buffers."""
        profiles = ((LegacyBatchEnv, "gl", 1, 128, 5),
                    (MiraclesBatchEnv, "gm", 2, 164, 6),
                    (ElementsBatchEnv, "ge", 3, 186, 6),
                    (ChanceBatchEnv, "gc", 4, 210, 6),
                    (ReflectionBatchEnv, "gr", 5, 236, 7),
                    (DiseaseBatchEnv, "gd", 6, 258, 7),
                    (HarmsBatchEnv, "gh", 7, 283, 7),
                    (CelestialBatchEnv, "gs", 8, 301, 7),
                    (OffensiveMiraclesBatchEnv, "go", 9, 334, 7),
                    (DefensiveMiraclesBatchEnv, "gb", 10, 356, 8),
                    (AttributeChangeBatchEnv, "ga", 11, 375, 9),
                    (AttributeRemovalBatchEnv, "gn", 12, 393, 10),
                    (CounterBatchEnv, "gk", 13, 515, 11),
                    (CounterExtensionsBatchEnv, "gx", 14, 661, 11),
                    (MoneyCounterBatchEnv, "gy", 15, 679, 11),
                    (MagicReflectionBatchEnv, "gz", 16, 696, 11),
                    (AnyReflectionBatchEnv, "gu", 17, 713, 11),
                    (ManaWeaponBatchEnv, "gv", 18, 732, 11),
                    (MagicBlockBatchEnv, "gw", 19, 750, 11),
                    (AdditiveMiraclesBatchEnv, "gp", 20, 903, 13),
                    (DoublingMiraclesBatchEnv, "gq", 21, 924, 15),
                    (WideMiraclesBatchEnv, "gt", 22, 945, 17),
                    (MagicFreeBatchEnv, "gi", 23, 993, 23),
                    (ForgettingBatchEnv, "gj", 24, 1017, 24),
                    (ExchangeBatchEnv, "gg", 25, 1043, 25),
                    (RevivalBatchEnv, "g26", 26, 1062, 25),
                    (ItemRemovalBatchEnv, "g27", 27, 1085, 26),
                    (HPAbsorptionBatchEnv, "g28", 28, 1110, 27),
                    (SelfDamageBatchEnv, "g29", 29, 1135, 28),
                    (PestleMortarBatchEnv, "g30", 30, 1175, 30),
                    (SellingBatchEnv, "g31", 31, 1221, 32),
                    (BuyingBatchEnv, "g32", 32, 1251, 34),
                    (HPGambleBatchEnv, "g33", 33, 1278, 35),
                    (MagicBounceBatchEnv, "g34", 34, 1300, 36))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * (2 * dimension + 32))(*[12345.0] * (2 * dimension + 32))
                options = (c.c_uint16 * (2 * width + 16))(*[54321] * (2 * width + 16))
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 88 if prefix in ("gp", "gq", "gt", "gi", "gj", "gg", "g26", "g27", "g28", "g29", "g30", "g31", "g32", "g33", "g34") else 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
                self.assertEqual(tuple(features[:2 * dimension]), tuple(env.observe()[0]))
                self.assertTrue(all(x == 12345.0 for x in features[2 * dimension:]))
                self.assertTrue(all(x == 54321 for x in options[2 * width:]))

    def test_script_validation_and_hidden_prefixes(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            for name, values, bad, valid_range in (
                    ("hit_rolls", (1, 100), 0, (1, 100)),
                    ("bounce_targets", (0, 1), 2, (0, 1)),
                    ("disease_rolls", (0, 99), 100, (0, 99)),
                    ("hp_gamble_rolls", (0, 1), 2, (0, 1))):
                setter = getattr(env, "script_" + name)
                native = getattr(env._lib, "g34_script_" + name)
                setter(0, values)
                self.assertEqual(observation_copy(env), before)
                for invalid_values in ((bad,), (True,), (0.5,), values * 33):
                    with self.assertRaises(ValueError):
                        setter(0, invalid_values)
                self.assertEqual(native(None, 0, None, 0), -1)
                self.assertEqual(native(env._handle, 1, None, 0), -2)
                self.assertEqual(native(env._handle, 0, None, 1), -1)
                self.assertEqual(native(env._handle, 0, None, 65), -2)
                invalid = (c.c_uint8 * 2)(valid_range[0], bad)
                self.assertEqual(native(env._handle, 0, invalid, 2), -3)
                setter(0, [])
            self.assertEqual(observation_copy(env), before)

    def test_seeded_batch_replay_and_atomic_rejection(self):
        """Rejected last rows must not advance earlier rows or hidden draw RNG."""
        with self.create() as left, self.create() as right:
            left.reset(42)
            right.reset(42)
            episodes = 0
            casts = 0
            for tick in range(128):
                self.assertEqual(observation_copy(left), observation_copy(right))
                state = observation_copy(left)
                actions = left.sample_actions()
                self.assertEqual(observation_copy(left), state, "sampling changed game state")
                self.assertEqual(actions, right.sample_actions())
                self.assertTrue(all(left.legal(actions)))
                self.assertEqual(observation_copy(left), state, "legality check changed game state")
                invalid = actions.copy()
                invalid[-1] = Action(ActionKind.ATTACK)  # No base or target, invalid in both phases.
                with self.assertRaises(ValueError):
                    left.step(invalid)
                self.assertEqual(observation_copy(left), state, "rejected batch changed a row")
                lr, lt, lx = left.step(actions)
                rr, rt, rx = right.step(actions)
                next_observation = left.observe()
                for i, action in enumerate(actions):
                    if action.kind in (ActionKind.CAST, ActionKind.DEFEND_CAST):
                        casts += 1
                        old_actor = state[2][i]
                        old_mp = state[0][i * left.observation_size + 1]
                        offset = 1 if next_observation[2][i] == old_actor else 4
                        new_mp = next_observation[0][i * left.observation_size + offset]
                        cost = ((5, 6)[action.base] if action.kind == ActionKind.DEFEND_CAST else
                                (6, 8, 2, 3, 3, 4, 4, 4, 5, 5, 7, 8, 12, 12, 30, 10)[action.base])
                        self.assertAlmostEqual(old_mp - new_mp, cost / 99, places=6)
                self.assertEqual(tuple(lr), tuple(rr))
                self.assertEqual(tuple(lt), tuple(rt))
                self.assertEqual(tuple(lx), tuple(rx))
                for i in range(left.count):
                    self.assertEqual(lr[2 * i] + lr[2 * i + 1], 0)
                    if lt[i] or lx[i]:
                        episodes += 1
                        seed = 100000 + tick * left.count + i
                        left.reset_at(i, seed)
                        right.reset_at(i, seed)
            self.assertGreater(episodes, 0)
            self.assertGreater(casts, 0, "seeded scenario failed to exercise retained-miracle casting")

    def test_done_rows_and_buffer_contract(self):
        with self.create(2, 1) as env:
            env.reset(4)
            self.assertEqual(env.ruleset, "legacy-magic-bounce-subset-v34")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 1300)
            self.assertEqual(len(options), 2 * 36)
            self.assertEqual((len(actors), len(phases), len(pray)), (2, 2, 2))
            _, terminal, truncated = env.random_step()
            end_terminated, end_truncated = tuple(terminal), tuple(truncated)
            self.assertTrue(all(t or x for t, x in zip(terminal, truncated)))
            state = observation_copy(env)
            policy_rng = tuple(env._policy_seeds)
            actions = env.sample_actions()
            self.assertEqual(tuple(env._policy_seeds), policy_rng, "done sampling advanced RNG")
            self.assertFalse(any(env.legal(actions)))
            # The native API must ignore even malformed actions for frozen rows.
            malformed = (_NativeAction * 2)(*[_NativeAction(255, 255, 65535, 255, 255)] * 2)
            self.assertEqual(env._lib.g34_step(env._handle, 2, malformed, env._rewards,
                                             env._terminated, env._truncated), 0)
            self.assertFalse(any(env._rewards))
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(tuple(env._terminated), end_terminated)
            self.assertEqual(tuple(env._truncated), end_truncated)
            env.reset_at(1, 9)
            actions = env.sample_actions()
            self.assertEqual(env.legal(actions)[0], False)
            self.assertEqual(env.legal(actions)[1], True)
            env.step(actions)

    def test_native_layout_and_errors(self):
        with self.create(2) as env:
            lib = env._lib
            self.assertEqual(c.sizeof(_NativeAction), 88)
            self.assertEqual([getattr(_NativeAction, field).offset for field in
                              ("kind", "base", "slots", "target", "reserved", "counter_count", "reserved2", "counter_order", "extra_count", "extra_tail", "reserved_extra", "extras")], [0, 1, 2, 4, 5, 6, 7, 8, 16, 17, 18, 24])
            self.assertFalse(lib.g34_create(0, 64))
            self.assertFalse(lib.g34_create(2, 0))
            self.assertEqual(lib.g34_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.g34_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.g34_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.g34_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.g34_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.g34_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)
            guarded = (c.c_uint8 * (2 * 88 + 32))(*[165] * (2 * 88 + 32))
            sampled = c.cast(guarded, c.POINTER(_NativeAction))
            self.assertEqual(lib.g34_sample_actions(env._handle, 2, env._policy_seeds, sampled), 0)
            self.assertTrue(all(byte == 165 for byte in guarded[2 * 88:]))
            self.assertEqual(lib.g34_legal(env._handle, 2, sampled, env._legal), 0)
            self.assertTrue(all(env._legal))
            for i in range(2):
                self.assertTrue(all(byte == 0 for byte in sampled[i].reserved_extra))
                self.assertTrue(all(byte == 0 for byte in sampled[i].extras[sampled[i].extra_count:]))
                if sampled[i].extra_count < 65:
                    self.assertEqual(sampled[i].extra_tail, 0)

    def test_python_validation_and_close(self):
        for args in ((19,), (True,), (ActionKind.ATTACK, 256),
                     (ActionKind.ATTACK, 0, 65536), (ActionKind.ATTACK, 0, 0, -1)):
            with self.assertRaises(ValueError):
                Action(*args)
        for count in (0, -1, True):
            with self.assertRaises(ValueError):
                self.create(count)
        env = self.create(2)
        with env:
            with self.assertRaises(ValueError):
                env.reset(-1)
            with self.assertRaises(ValueError):
                env.reset_at(-1, 0)
            with self.assertRaises(ValueError):
                env.step([])
            with self.assertRaises(TypeError):
                env.step([0, 0])
        env.close()
        for operation in (env.observe, env.sample_actions, env.random_step, env.reset):
            with self.assertRaises(RuntimeError):
                operation()


if __name__ == "__main__":
    unittest.main()
