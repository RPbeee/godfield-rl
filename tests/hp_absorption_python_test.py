"""Public ABI28 boundaries and source-first HP absorption.

Usage: python3 tests/hp_absorption_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from hp_absorption_env import Action, ActionKind, HPAbsorptionBatchEnv, _NativeAction
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


class HPAbsorptionBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return HPAbsorptionBatchEnv(count, max_decisions, library=LIBRARY)

    LIMIT = 480 << 17
    FREE = (47, 153, 172, 180, 239)

    def fixture(self, env, card, free=False):
        for seed in range(131072):
            features, _, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            carrier = next((i for i, item in enumerate(ids) if item in self.FREE), None)
            if card in ids and (not free or carrier is not None):
                return seed, actors[0], ids.index(card), carrier
        self.fail("normal pool did not reach HP-absorption fixture")

    def advance_to(self, env, owner):
        for _ in range(32):
            _, options, actors, phases, pray = env.observe()
            self.assertEqual(phases[0], 0)
            if actors[0] == owner:
                return
            slot = next((i for i in range(16) if options[4] & (1 << i)), None)
            if slot is not None:
                env.step([Action(ActionKind.DISCARD, slots=1 << slot)])
            elif pray[0]:
                env.step([Action(ActionKind.PRAY)])
            else:
                # Initial hands normally contain a discardable utility/armor.
                self.fail("fixture opponent cannot pass without attacking")
        self.fail("owner did not regain a main turn")

    def test_both_guaranteed_weapons_self_resolve_with_source_healing_event(self):
        for local, card, amount in ((1, 26, 7), (2, 42, 11)):
            with self.subTest(card=card), self.create(1, 128) as env:
                _, owner, base, _ = self.fixture(env, card)
                features, options, _, _, _ = env.observe()
                self.assertEqual(features[1085], 1)
                self.assertAlmostEqual(features[1086 + base], local / 4, places=6)
                self.assertTrue(options[26] & (1 << base))
                env.step([Action(ActionKind.ATTACK, base, target=owner)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], 0)
                self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 40 / 99, places=6)
                self.assertEqual(features[1104], 1)
                self.assertEqual(features[1105], int(actors[0] == owner))
                self.assertEqual(features[1106], int(actors[0] == owner))
                self.assertAlmostEqual(features[1107], card / 363, places=6)
                self.assertAlmostEqual(features[1108], amount / self.LIMIT, places=7)
                self.assertAlmostEqual(features[1109], amount / 99, places=6)

    def test_normal_armor_changes_absorption_to_final_residual(self):
        with self.create(1, 128) as env:
            for seed in range(131072):
                features, _, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 42 not in ids:
                    continue
                owner = actors[0]
                env.step([Action(ActionKind.ATTACK, ids.index(42), target=1 - owner)])
                features, options, _, _, _ = env.observe()
                armor = next((i for i in range(16) if options[2] & (1 << i)
                              and not options[10] & (1 << i)
                              and 0 < round(features[16 + i * 7 + 3] * 30) < 11
                              and env.legal([Action(ActionKind.DEFEND, slots=1 << i)])[0]), None)
                if armor is None:
                    continue
                residual = 11 - round(features[16 + armor * 7 + 3] * 30)
                self.assertAlmostEqual(features[1103], 42 / 363, places=6)
                env.step([Action(ActionKind.DEFEND, slots=1 << armor)])
                features, _, actors, _, _ = env.observe()
                self.assertAlmostEqual(features[0 if actors[0] == owner else 3], (40 + residual) / 99, places=6)
                self.assertAlmostEqual(features[3 if actors[0] == owner else 0], (40 - residual) / 99, places=6)
                self.assertAlmostEqual(features[1108], residual / self.LIMIT, places=7)
                self.assertAlmostEqual(features[1109], residual / 99, places=6)
                break
            else:
                self.fail("normal pool did not reach partial armor against42")

    def test_chance103_threshold_and_miss_without_absorption(self):
        with self.create(1, 128) as env:
            seed, owner, base, _ = self.fixture(env, 103)
            for roll, hit in ((75, True), (76, False)):
                env.reset(seed)
                env.script_hit_rolls(0, [roll])
                env.step([Action(ActionKind.ATTACK, base, target=owner)])
                features, _, actors, phases, _ = env.observe()
                if hit:
                    self.assertEqual((actors[0], phases[0]), (1 - owner, 1))
                    self.assertAlmostEqual(features[1103], 103 / 363, places=6)
                    env.step([Action(ActionKind.DEFEND)])
                    features, _, actors, _, _ = env.observe()
                    self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 43 / 99, places=6)
                    self.assertAlmostEqual(features[1109], 3 / 99, places=6)
                else:
                    self.assertEqual(phases[0], 0)
                    self.assertEqual(features[1104], 0)
                    self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 40 / 99, places=6)

    def test_magic221_retains_slot15_and_free_casts_from_base47(self):
        with self.create(1, 128) as env:
            _, owner, base, _ = self.fixture(env, 221, free=True)
            env.step([Action(ActionKind.USE, base, target=1 - owner)])
            features, _, _, phases, _ = env.observe()
            self.assertEqual(phases[0], 1)
            self.assertEqual(features[4], 0)
            self.assertAlmostEqual(features[1103], 221 / 363, places=6)
            env.step([Action(ActionKind.DEFEND)])
            self.advance_to(env, owner)
            features, options, _, _, _ = env.observe()
            self.assertAlmostEqual(features[1102], 1 / 128, places=6)
            self.assertFalse(options[5] & (1 << 15))
            self.assertTrue(options[19] & (1 << 15))
            free = next(i for i, card in enumerate(self.hand_ids(features)) if card in self.FREE)
            action = Action(ActionKind.CAST_ORDERED, 47, target=1 - owner, extras=(free,))
            self.assertTrue(env.legal([action])[0])
            env.step([action])
            self.assertEqual(env.observe()[0][4], 0)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 60 / 99, places=6)
            self.assertAlmostEqual(features[1109], 10 / 99, places=6)
            with ItemRemovalBatchEnv(1, 128, library=LIBRARY) as old:
                old.reset(42)
                old._actions[0].kind = 5
                old._actions[0].base = 15
                old._actions[0].target = 1 - old.observe()[2][0]
                self.assertEqual(old._lib.g27_legal(old._handle, 1, old._actions, old._legal), 0)
                self.assertFalse(old._legal[0])

    def test_hidden_ticket_prefixes_remain_private_and_checked(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            for name in ("forget_tickets", "item_removal_tickets"):
                setter = getattr(env, "script_" + name)
                native = getattr(env._lib, "g28_script_" + name)
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
                self.assertEqual(env._lib.g28_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    def hand_fraction(self, features, actor, seat):
        # Feature10 identifies the turn owner; only feature11 is a hand count.
        return sum(card != 0 for card in self.hand_ids(features)) / 16 if actor == seat else features[11]

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_twenty_eight_profile_output_boundaries(self):
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
                    (HPAbsorptionBatchEnv, "g28", 28, 1110, 27))
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
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 88 if prefix in ("gp", "gq", "gt", "gi", "gj", "gg", "g26", "g27", "g28") else 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
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
                    ("disease_rolls", (0, 99), 100, (0, 99))):
                setter = getattr(env, "script_" + name)
                native = getattr(env._lib, "g28_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-hp-absorption-subset-v28")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 1110)
            self.assertEqual(len(options), 2 * 27)
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
            self.assertEqual(env._lib.g28_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.g28_create(0, 64))
            self.assertFalse(lib.g28_create(2, 0))
            self.assertEqual(lib.g28_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.g28_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.g28_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.g28_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.g28_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.g28_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)
            guarded = (c.c_uint8 * (2 * 88 + 32))(*[165] * (2 * 88 + 32))
            sampled = c.cast(guarded, c.POINTER(_NativeAction))
            self.assertEqual(lib.g28_sample_actions(env._handle, 2, env._policy_seeds, sampled), 0)
            self.assertTrue(all(byte == 165 for byte in guarded[2 * 88:]))
            self.assertEqual(lib.g28_legal(env._handle, 2, sampled, env._legal), 0)
            self.assertTrue(all(env._legal))
            for i in range(2):
                self.assertTrue(all(byte == 0 for byte in sampled[i].reserved_extra))
                self.assertTrue(all(byte == 0 for byte in sampled[i].extras[sampled[i].extra_count:]))
                if sampled[i].extra_count < 65:
                    self.assertEqual(sampled[i].extra_tail, 0)

    def test_python_validation_and_close(self):
        for args in ((15,), (True,), (ActionKind.ATTACK, 256),
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
