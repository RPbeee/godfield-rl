"""Public ABI35 boundaries and physical sundry boost composition.

Usage: python3 tests/sundry_boost_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from sundry_boost_env import Action, ActionKind, SundryBoostBatchEnv, Phase, _NativeAction
from magic_bounce_env import MagicBounceBatchEnv
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


class SundryBoostBridgeTests(unittest.TestCase):
    _seeds = {}

    def create(self, count=16, max_decisions=64):
        return SundryBoostBatchEnv(count, max_decisions, library=LIBRARY)

    def fixture(self, env, required):
        key = tuple(sorted(required))
        seeds = (self._seeds[key],) if key in self._seeds else range(1048576)
        for seed in seeds:
            features, _, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if all(ids.count(card) >= required.count(card) for card in required):
                self._seeds[key] = seed
                return seed, actors[0], ids
        self.fail(f"normal pool did not reach {required}")

    def test_normal_pool_membership_and_boost_only_roles(self):
        with self.create(1, 128) as env:
            _, owner, ids = self.fixture(env, (238,))
            features, options, _, _, _ = env.observe()
            slot = ids.index(238)
            self.assertEqual(features[1300], 1)
            self.assertEqual(features[1301 + slot], 1)
            self.assertTrue(options[36] & (1 << slot))
            self.assertTrue(options[1] & (1 << slot))
            self.assertFalse(options[0] & (1 << slot))
            self.assertFalse(options[3] & (1 << slot))
            self.assertAlmostEqual(features[16 + slot * 7 + 2], 10 / 20, places=6)
            self.assertEqual(features[16 + slot * 7 + 1], 0)
            self.assertAlmostEqual(features[1192 + slot], 15 / 30, places=6)
            before = observation_copy(env)
            for action in (Action(ActionKind.USE, slot, target=owner),
                           Action(ActionKind.ATTACK, slot, target=owner),
                           Action(ActionKind.ATTACK_ORDERED, slot, target=owner, extras=(16,))):
                self.assertFalse(env.legal([action])[0])
                with self.assertRaises(ValueError):
                    env.step([action])
                self.assertEqual(observation_copy(env), before)

    def test_prayer_remains_legal_with238_and_no_weapon(self):
        with self.create(1, 128) as env:
            for seed in range(262144):
                features, options, _, _, pray = env.reset(seed)
                if 238 in self.hand_ids(features) and options[0] == 0:
                    break
            else:
                self.fail("normal pool did not reach boost-only/no-weapon hand")
            self.assertTrue(pray[0])
            self.assertTrue(env.legal([Action(ActionKind.PRAY)])[0])
            env.step([Action(ActionKind.PRAY)])

    def test_unordered_and_ordered_boost_add10_with_one_gift_each(self):
        with self.create(1, 128) as env:
            seed, owner, ids = self.fixture(env, (5, 238))
            base, boost = ids.index(5), ids.index(238)
            for action in (Action(ActionKind.ATTACK, base, slots=1 << boost, target=owner),
                           Action(ActionKind.ATTACK_ORDERED, base, target=owner, extras=(boost,))):
                env.reset(seed)
                env.step([action])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], Phase.MAIN)
                self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 29 / 99, places=6)
                self.assertAlmostEqual(features[1 if actors[0] == owner else 4], 10 / 99, places=6)
                self.assertAlmostEqual(self.hand_fraction(features, actors[0], owner), 10 / 16, places=6)

    def test_multiple_physical_copies_each_add10_and_duplicate_ref_rejects(self):
        with self.create(1, 128) as env:
            _, owner, ids = self.fixture(env, (5, 238, 238))
            boosts = tuple(i for i, card in enumerate(ids) if card == 238)[:2]
            before = observation_copy(env)
            with self.assertRaises(ValueError):
                env.step([Action(ActionKind.ATTACK_ORDERED, ids.index(5), target=owner, extras=(boosts[0], boosts[0]))])
            self.assertEqual(observation_copy(env), before)
            env.step([Action(ActionKind.ATTACK, ids.index(5), slots=sum(1 << i for i in boosts), target=owner)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 19 / 99, places=6)
            self.assertAlmostEqual(self.hand_fraction(features, actors[0], owner), 10 / 16, places=6)

    def test_setter_order_changes_final_color_without_adding_setter_attack(self):
        with self.create(1, 128) as env:
            seed, owner, ids = self.fixture(env, (5, 65, 238))
            base, setter, boost = ids.index(5), ids.index(65), ids.index(238)
            for extras, element in (((boost, setter), 1), ((setter, boost), 0)):
                env.reset(seed)
                env.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=extras)])
                features, options, _, phases, _ = env.observe()
                self.assertEqual(phases[0], Phase.RESPONSE)
                self.assertEqual(options[36], 0)
                self.assertAlmostEqual(features[284], element / 6, places=6)
                self.assertAlmostEqual(features[8], 11 / 480, places=6)
            for kind, element in ((ActionKind.ATTACK_SET_FIRE, 1), (ActionKind.ATTACK_SET_NEUTRAL, 0)):
                env.reset(seed)
                env.step([Action(kind, base, slots=(1 << setter) | (1 << boost), target=1 - owner)])
                self.assertAlmostEqual(env.observe()[0][284], element / 6, places=6)

    def test_doubling_before_and_after_boost_keeps_order(self):
        with self.create(1, 128) as env:
            seed, owner, ids = self.fixture(env, (5, 214, 238))
            base, double, boost = ids.index(5), ids.index(214), ids.index(238)
            for extras, amount in (((boost, double), 22), ((double, boost), 12)):
                env.reset(seed)
                env.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=extras)])
                features = env.observe()[0]
                self.assertAlmostEqual(features[8], amount / 480, places=6)
                self.assertAlmostEqual(features[4], 4 / 99, places=6)

    def test_mana_weapon_paid_reset_erases_only_earlier_boost(self):
        with self.create(1, 128) as env:
            seed, owner, ids = self.fixture(env, (57, 197, 238))
            base, magic, boost = ids.index(57), ids.index(197), ids.index(238)
            for extras, amount in (((boost, magic), 18), ((magic, boost), 28)):
                env.reset(seed)
                env.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=extras)])
                features = env.observe()[0]
                self.assertAlmostEqual(features[8], amount / 480, places=6)
                self.assertEqual(features[4], 0)
                self.assertAlmostEqual(features[731], 8 / 99, places=6)

    def test_free_after238_does_not_waive_an_earlier_magic(self):
        with self.create(1, 128) as env:
            for seed in range(1048576):
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if not all(card in ids for card in (197, 238, 239)):
                    continue
                base = next((i for i, card in enumerate(ids) if options[0] & (1 << i)
                             and card not in (53, 57, 99)
                             and features[186 + i] == 0 and features[285 + i] == 0), None)
                if base is not None:
                    break
            else:
                self.fail("normal pool did not reach magic/boost/Free with NE weapon")
            owner = actors[0]
            magic, boost, free = ids.index(197), ids.index(238), ids.index(239)
            amount = round(features[16 + base * 7 + 1] * 30) + 12
            for extras, mp, saved in (((magic, boost, free), 8, 0), ((boost, magic, free), 10, 2)):
                env.reset(seed)
                env.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=extras)])
                features = env.observe()[0]
                self.assertAlmostEqual(features[8], amount / 480, places=6)
                self.assertAlmostEqual(features[4], mp / 99, places=6)
                self.assertAlmostEqual(features[973], saved / 99, places=6)

    def test_free_without_any_prior_magic_is_invalid(self):
        with self.create(1, 128) as env:
            _, owner, ids = self.fixture(env, (5, 238, 239))
            action = Action(ActionKind.ATTACK_ORDERED, ids.index(5), target=owner,
                            extras=(ids.index(238), ids.index(239)))
            before = observation_copy(env)
            self.assertFalse(env.legal([action])[0])
            with self.assertRaises(ValueError):
                env.step([action])
            self.assertEqual(observation_copy(env), before)

    def test_wide_before_boost_keeps_weapon_type_and_rate100(self):
        with self.create(1, 128) as env:
            _, owner, ids = self.fixture(env, (5, 209, 238))
            env.script_hit_rolls(0, [100])
            env.step([Action(ActionKind.ATTACK_ORDERED, ids.index(5), target=owner,
                             extras=(ids.index(209), ids.index(238)))])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (1 - owner, Phase.RESPONSE))
            self.assertAlmostEqual(features[8], 11 / 480, places=6)
            self.assertEqual(features[202], 1)
            self.assertEqual(features[302], 0)
            self.assertEqual(features[942], 1)
            self.assertEqual(features[943], 1)

    def test_original_percent_and_magic_bases_reject238(self):
        with self.create(1, 128) as env:
            for card in (103, 206):
                _, owner, ids = self.fixture(env, (card, 238))
                before = observation_copy(env)
                for action in (Action(ActionKind.ATTACK, ids.index(card), slots=1 << ids.index(238), target=owner),
                               Action(ActionKind.ATTACK_ORDERED, ids.index(card), target=owner, extras=(ids.index(238),))):
                    self.assertFalse(env.legal([action])[0])
                    with self.assertRaises(ValueError):
                        env.step([action])
                    self.assertEqual(observation_copy(env), before)

    def test_response238_is_excluded_from_canonical_defense(self):
        with self.create(1, 128) as env:
            for seed in range(262144):
                features, _, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 5 not in ids:
                    continue
                owner = actors[0]
                env.step([Action(ActionKind.ATTACK, ids.index(5), target=1 - owner)])
                ids = self.hand_ids(env.observe()[0])
                if 238 in ids:
                    break
            else:
                self.fail("normal pool did not reach238 defense exclusion")
            slot = ids.index(238)
            features, options, _, _, _ = env.observe()
            self.assertEqual(features[1301 + slot], 1)
            self.assertEqual(options[36], 0)
            self.assertFalse(options[2] & (1 << slot))
            before = observation_copy(env)
            for action in (Action(ActionKind.DEFEND, slots=1 << slot), Action(ActionKind.DEFEND, slot)):
                self.assertFalse(env.legal([action])[0])
                with self.assertRaises(ValueError):
                    env.step([action])
                self.assertEqual(observation_copy(env), before)

    def test_trade_price15_is_used_without_attack_or_mp_role(self):
        with self.create(1, 128) as env:
            _, owner, ids = self.fixture(env, (3, 238))
            env.step([Action(ActionKind.SELL, ids.index(3), slots=1 << ids.index(238), target=owner)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[1214], 238 / 363, places=6)
            self.assertEqual(features[1215], .5)
            offset = 0 if actors[0] == owner else 3
            self.assertAlmostEqual(features[offset], 40 / 99, places=6)
            self.assertAlmostEqual(features[offset + 1], 10 / 99, places=6)
            self.assertAlmostEqual(features[offset + 2], 20 / 99, places=6)

    def test_pestle_target_tape_validation_and_hidden_values(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            env.script_pestle_targets(0, [0, 1])
            self.assertEqual(observation_copy(env), before)
            for values in ((2,), (True,), (0.5,), (0,) * 65):
                with self.assertRaises(ValueError):
                    env.script_pestle_targets(0, values)
            native = env._lib.g35_script_pestle_targets
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
                native = getattr(env._lib, "g35_script_" + name)
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
                self.assertEqual(env._lib.g35_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    def hand_fraction(self, features, actor, seat):
        # Feature10 identifies the turn owner; only feature11 is a hand count.
        return sum(card != 0 for card in self.hand_ids(features)) / 16 if actor == seat else features[11]

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_thirty_five_profile_output_boundaries(self):
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
                    (MagicBounceBatchEnv, "g34", 34, 1300, 36),
                    (SundryBoostBatchEnv, "g35", 35, 1317, 37))
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
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 88 if prefix in ("gp", "gq", "gt", "gi", "gj", "gg", "g26", "g27", "g28", "g29", "g30", "g31", "g32", "g33", "g34", "g35") else 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
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
                native = getattr(env._lib, "g35_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-sundry-boost-subset-v35")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 1317)
            self.assertEqual(len(options), 2 * 37)
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
            self.assertEqual(env._lib.g35_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.g35_create(0, 64))
            self.assertFalse(lib.g35_create(2, 0))
            self.assertEqual(lib.g35_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.g35_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.g35_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.g35_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.g35_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.g35_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)
            guarded = (c.c_uint8 * (2 * 88 + 32))(*[165] * (2 * 88 + 32))
            sampled = c.cast(guarded, c.POINTER(_NativeAction))
            self.assertEqual(lib.g35_sample_actions(env._handle, 2, env._policy_seeds, sampled), 0)
            self.assertTrue(all(byte == 165 for byte in guarded[2 * 88:]))
            self.assertEqual(lib.g35_legal(env._handle, 2, sampled, env._legal), 0)
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
