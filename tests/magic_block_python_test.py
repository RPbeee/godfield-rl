"""Public ABI19 boundaries and normal-pool magic blockers.

Usage: python3 tests/magic_block_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from magic_block_env import Action, ActionKind, MagicBlockBatchEnv, _NativeAction
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


class MagicBlockBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return MagicBlockBatchEnv(count, max_decisions, library=LIBRARY)

    BLOCKERS = (27, 54, 55, 152, 165, 182, 183, 185)

    def magic_fixture(self, env, miracle=198, blocker=None, prefix=None):
        for seed in range(131072):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if miracle not in ids or not options[3] & (1 << ids.index(miracle)):
                continue
            owner = actors[0]
            env.script_hit_rolls(0, [1, 100])
            env.step([Action(ActionKind.USE, ids.index(miracle), target=1 - owner)])
            features, options, _, phases, _ = env.observe()
            ids = self.hand_ids(features)
            slots = [i for i, card in enumerate(ids) if card in self.BLOCKERS
                     and (blocker is None or card == blocker)]
            if phases[0] != 1 or not slots or (prefix is not None and prefix not in ids):
                continue
            return owner, slots[0], (ids.index(prefix) if prefix is not None else None)
        self.fail("normal pool did not reach selected MAGIC/block/prefix fixture")

    def test_all_eight_blockers_are_mandatory_specials_for_magic(self):
        for card in self.BLOCKERS:
            with self.subTest(card=card), self.create(1, 128) as env:
                owner, slot, _ = self.magic_fixture(env, blocker=card)
                features, options, _, _, _ = env.observe()
                self.assertEqual(features[732], 1)
                self.assertAlmostEqual(features[733 + slot], (self.BLOCKERS.index(card) + 1) / 8)
                self.assertTrue(options[6] & (1 << slot))
                self.assertFalse(options[2] & (1 << slot))
                self.assertFalse(env.legal([Action(ActionKind.DEFEND, slots=1 << slot)])[0])
                env.step([Action(ActionKind.DEFEND, slot)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], 0)
                self.assertEqual(features[749], 1)
                self.assertEqual(features[354], 1)
                self.assertAlmostEqual(features[230], card / 363, places=6)
                self.assertAlmostEqual(features[0], 40 / 99, places=6)
                self.assertAlmostEqual(features[3], 40 / 99, places=6)
                self.assertAlmostEqual(features[1 if actors[0] == owner else 4], 8 / 99, places=6)
                self.assertAlmostEqual(features[4 if actors[0] == owner else 1], 10 / 99, places=6)

    def test_block_cancels_utility_dark_and_chance_glory_without_refunding_cost(self):
        for miracle, mp in ((216, 4), (220, 2), (207, 5), (200, 7)):
            with self.subTest(miracle=miracle), self.create(1, 128) as env:
                owner, slot, _ = self.magic_fixture(env, miracle)
                before = tuple(env.observe()[0])
                _, terminal, _ = env.step([Action(ActionKind.DEFEND, slot)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], 0)
                self.assertFalse(terminal[0])
                self.assertEqual(features[749], 1)
                self.assertAlmostEqual(features[0], 40 / 99, places=6)
                self.assertAlmostEqual(features[3], 40 / 99, places=6)
                self.assertAlmostEqual(features[2], 20 / 99, places=6)
                self.assertAlmostEqual(features[5], 20 / 99, places=6)
                self.assertAlmostEqual(features[1 if actors[0] == owner else 4], mp / 99, places=6)
                self.assertEqual((features[258], features[260]), (0, 0))
                if miracle == 200:
                    self.assertEqual((features[206], features[207]), (before[206], before[207]))
                if actors[0] == owner and miracle in (216, 220):
                    self.assertAlmostEqual(features[129 + (miracle == 220)], 1 / 128, places=6)

    def test_prefix_counter_is_consumed_but_never_queued_on_magic_block(self):
        with self.create(1, 128) as env:
            _, slot, prefix = self.magic_fixture(env, prefix=190)
            action = Action(ActionKind.DEFEND, slot, 1 << prefix, counter_order=(prefix,))
            self.assertTrue(env.legal([action])[0])
            env.step([action])
            features, _, _, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)
            self.assertEqual(features[749], 1)
            self.assertEqual((features[414], features[415], features[416]), (0, 0, 0))
            self.assertAlmostEqual(features[0], 40 / 99, places=6)
            self.assertAlmostEqual(features[3], 40 / 99, places=6)

    def test_nonmagic_input_keeps_ordinary_armor_and_mana_weapon_category(self):
        for card, defense in ((152, 6), (165, 8), (182, 13), (183, 14), (185, 15)):
            with self.subTest(card=card), self.create(1, 128) as env:
                for seed in range(131072):
                    features, options, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    if 57 not in ids:
                        continue
                    owner = actors[0]
                    env.step([Action(ActionKind.ATTACK, ids.index(57), target=1 - owner)])
                    features, options, _, _, _ = env.observe()
                    ids = self.hand_ids(features)
                    if card not in ids:
                        continue
                    slot = ids.index(card)
                    self.assertTrue(options[2] & (1 << slot))
                    self.assertFalse(options[6] & (1 << slot))
                    self.assertFalse(env.legal([Action(ActionKind.DEFEND, slot)])[0])
                    env.step([Action(ActionKind.DEFEND, slots=1 << slot)])
                    features, _, actors, _, _ = env.observe()
                    defender = 1 - owner
                    self.assertEqual(features[749], 0)
                    self.assertAlmostEqual(features[0 if actors[0] == defender else 3],
                                           (40 - (20 - defense)) / 99, places=6)
                    break
                else:
                    self.fail("normal pool did not reach mana weapon/nonmagic armor fixture")

    def test_block_weapon_attack_values_and_weapon55_boost_are_preserved(self):
        for card, amount in ((27, 7), (54, 15), (55, 15)):
            with self.subTest(card=card), self.create(1, 128) as env:
                for seed in range(131072):
                    features, options, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    if card not in ids:
                        continue
                    slot = ids.index(card)
                    self.assertTrue(options[0] & (1 << slot))
                    self.assertAlmostEqual(features[16 + slot * 7 + 1], amount / 30, places=6)
                    env.step([Action(ActionKind.ATTACK, slot, target=1 - actors[0])])
                    self.assertAlmostEqual(env.observe()[0][8], amount / 480, places=6)
                    break
                else:
                    self.fail("normal pool did not reach block weapon base")
        with self.create(1, 128) as env:
            for seed in range(131072):
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 55 not in ids:
                    continue
                boost = ids.index(55)
                base = next((i for i in range(16) if i != boost and options[0] & (1 << i)
                             and features[285 + i] == 0 and features[186 + i] == 0), None)
                if base is None:
                    continue
                self.assertTrue(options[1] & (1 << boost))
                self.assertAlmostEqual(features[16 + boost * 7 + 2], 15 / 20, places=6)
                amount = round(features[16 + base * 7 + 1] * 30) + 15
                env.step([Action(ActionKind.ATTACK, base, 1 << boost, 1 - actors[0])])
                self.assertAlmostEqual(env.observe()[0][8], amount / 480, places=6)
                break
            else:
                self.fail("normal pool did not reach weapon55 boost fixture")

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
                self.assertEqual(env._lib.gw_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_nineteen_profile_output_boundaries(self):
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
                    (MagicBlockBatchEnv, "gw", 19, 750, 11))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * (2 * dimension + 32))(*[12345.0] * (2 * dimension + 32))
                options = (c.c_uint16 * 32)(*[54321] * 32)
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
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
                native = getattr(env._lib, "gw_script_" + name)
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
                                (6, 8, 2, 3, 3, 4, 4, 4, 5, 5, 7, 8, 12, 12, 30)[action.base])
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
            self.assertEqual(env.ruleset, "legacy-magic-block-subset-v19")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 750)
            self.assertEqual(len(options), 2 * 11)
            self.assertEqual((len(actors), len(phases), len(pray)), (2, 2, 2))
            _, terminal, truncated = env.random_step()
            self.assertEqual(tuple(terminal), (0, 0))
            self.assertEqual(tuple(truncated), (1, 1))
            state = observation_copy(env)
            policy_rng = tuple(env._policy_seeds)
            actions = env.sample_actions()
            self.assertEqual(tuple(env._policy_seeds), policy_rng, "done sampling advanced RNG")
            self.assertFalse(any(env.legal(actions)))
            # The native API must ignore even malformed actions for frozen rows.
            malformed = (_NativeAction * 2)(*[_NativeAction(255, 255, 65535, 255, 255)] * 2)
            self.assertEqual(env._lib.gw_step(env._handle, 2, malformed, env._rewards,
                                             env._terminated, env._truncated), 0)
            self.assertFalse(any(env._rewards))
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(tuple(env._truncated), (1, 1))
            env.reset_at(1, 9)
            actions = env.sample_actions()
            self.assertEqual(env.legal(actions)[0], False)
            self.assertEqual(env.legal(actions)[1], True)
            env.step(actions)

    def test_native_layout_and_errors(self):
        with self.create(2) as env:
            lib = env._lib
            self.assertEqual(c.sizeof(_NativeAction), 16)
            self.assertEqual([getattr(_NativeAction, field).offset for field in
                              ("kind", "base", "slots", "target", "reserved", "counter_count", "reserved2", "counter_order")], [0, 1, 2, 4, 5, 6, 7, 8])
            self.assertFalse(lib.gw_create(0, 64))
            self.assertFalse(lib.gw_create(2, 0))
            self.assertEqual(lib.gw_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gw_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gw_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gw_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gw_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gw_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)

    def test_python_validation_and_close(self):
        for args in ((10,), (True,), (ActionKind.ATTACK, 256),
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
