"""Public ABI16 boundaries and normal-pool mandatory MAGIC reflection.

Usage: python3 tests/magic_reflection_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from magic_reflection_env import Action, ActionKind, MagicReflectionBatchEnv, _NativeAction
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


class MagicReflectionBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return MagicReflectionBatchEnv(count, max_decisions, library=LIBRARY)

    def magic_fixture(self, env, miracle=198, reflector=None, removal=False, armor=False):
        """Reach both hands through ordinary reset/first-use public decisions."""
        for seed in range(65536):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if miracle not in ids:
                continue
            slot = ids.index(miracle)
            if not options[3] & (1 << slot):
                continue
            source = actors[0]
            env.script_hit_rolls(0, [1, 100])
            env.step([Action(ActionKind.USE, slot, target=1 - source)])
            features, options, _, phases, _ = env.observe()
            if phases[0] != 1:
                continue
            ids = self.hand_ids(features)
            choices = [i for i, card in enumerate(ids) if card in (40, 171, 174, 178)
                       and (reflector is None or card == reflector)]
            if not choices or (removal and 195 not in ids):
                continue
            prefix = next((i for i, card in enumerate(ids) if options[2] & (1 << i)
                           and features[16 + i * 7 + 3] > 0
                           and card not in (175, 40, 171, 174, 178)), None)
            if armor and prefix is None:
                continue
            return source, choices[0], prefix
        self.fail("seed search failed to reach MAGIC reflection fixture")

    def test_all_four_cards_are_mandatory_magic_reactions(self):
        for card in (40, 171, 174, 178):
            with self.subTest(card=card), self.create(1, 128) as env:
                source, slot, armor = self.magic_fixture(env, reflector=card, armor=True)
                features, options, actors, _, _ = env.observe()
                self.assertEqual(features[679], 1)
                self.assertAlmostEqual(features[680 + slot], (1, 2, 3, 4)[(40, 171, 174, 178).index(card)] / 4)
                self.assertTrue(options[6] & (1 << slot))
                self.assertFalse(options[2] & (1 << slot))
                self.assertFalse(env.legal([Action(ActionKind.DEFEND, slots=1 << slot)])[0])
                raw = features[8]
                env.step([Action(ActionKind.DEFEND, slot, 1 << armor)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (source, 1))
                self.assertEqual(features[8], raw, "prefix DEF leaked into raw reflected payload")
                self.assertAlmostEqual(features[230], card / 363, places=6)
                self.assertEqual((features[231], features[233], features[234]), (1, 0, 1))
                self.assertAlmostEqual(features[303], 198 / 363, places=6)
                self.assertAlmostEqual(features[4], 10 / 99, places=6)
                self.assertAlmostEqual(features[11] * 16, 8, places=5)
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertAlmostEqual(features[0 if actors[0] == source else 3], 36 / 99, places=6)

    def test_utility_heal_and_money_reflect_without_mp_or_retention_cost(self):
        for miracle, expected_hp, expected_money, mp in ((216, 50, 20, 4), (220, 40, 35, 2)):
            with self.subTest(miracle=miracle), self.create(1, 128) as env:
                source, slot, _ = self.magic_fixture(env, miracle)
                env.step([Action(ActionKind.DEFEND, slot)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (source, 1))
                self.assertAlmostEqual(features[1], mp / 99, places=6)
                self.assertAlmostEqual(features[4], 10 / 99, places=6)
                self.assertAlmostEqual(features[129 + (miracle == 220)], 1 / 128, places=6)
                self.assertEqual(features[304], 0)
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                start = 0 if actors[0] == source else 3
                self.assertAlmostEqual(features[start], expected_hp / 99, places=6)
                self.assertAlmostEqual(features[start + 2], expected_money / 99, places=6)

    def test_percent_glory_and_dark_magic_keep_payload_and_hit_result(self):
        for miracle in (200, 207):
            with self.subTest(miracle=miracle), self.create(1, 128) as env:
                source, slot, _ = self.magic_fixture(env, miracle)
                before = tuple(env.observe()[0])
                env.step([Action(ActionKind.DEFEND, slot)])
                features = env.observe()[0]
                self.assertEqual((features[202], features[206], features[207]),
                                 (before[202], before[206], before[207]))
                self.assertAlmostEqual(features[303], miracle / 363, places=6)
                _, terminated, _ = env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                start = 0 if actors[0] == source else 3
                if miracle == 200:
                    self.assertFalse(terminated[0])
                    self.assertAlmostEqual(features[start], 39 / 99, places=6)
                    self.assertEqual(features[258 if actors[0] == source else 260], 1)
                else:
                    self.assertTrue(terminated[0])
                    self.assertEqual(features[start], 0)
                    self.assertEqual(env._rewards[source], -1)

    def test_removal_prefix_preserves_magic_origin_and_raw_amount(self):
        with self.create(1, 128) as env:
            source, slot, _ = self.magic_fixture(env, removal=True)
            features = env.observe()[0]
            removal = self.hand_ids(features).index(195)
            amount = features[8]
            env.step([Action(ActionKind.DEFEND, slot, 1 << removal)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (source, 1))
            self.assertEqual(features[392], 1)
            self.assertEqual(features[284], 0)
            self.assertEqual(features[8], amount)
            self.assertAlmostEqual(features[303], 198 / 363, places=6)

    def test_non_magic_input_uses_armor_printed_defense(self):
        for card, defense in ((171, 9), (174, 10), (178, 11)):
            with self.subTest(card=card), self.create(1, 128) as env:
                for seed in range(65536):
                    features, options, actors, _, _ = env.reset(seed)
                    base = next((i for i in range(16) if options[0] & (1 << i)
                                 and features[285 + i] == 0 and features[186 + i] == 0
                                 and self.hand_ids(features)[i] not in (30, 34, 43, 49)), None)
                    if base is None:
                        continue
                    source = actors[0]
                    env.step([Action(ActionKind.ATTACK, base, target=1 - source)])
                    features, options, _, _, _ = env.observe()
                    ids = self.hand_ids(features)
                    if card not in ids:
                        continue
                    slot = ids.index(card)
                    self.assertTrue(options[2] & (1 << slot))
                    self.assertFalse(options[6] & (1 << slot))
                    self.assertFalse(env.legal([Action(ActionKind.DEFEND, slot)])[0])
                    amount = round(features[8] * 480)
                    env.step([Action(ActionKind.DEFEND, slots=1 << slot)])
                    features, _, actors, _, _ = env.observe()
                    defender = 1 - source
                    self.assertAlmostEqual(features[0 if actors[0] == defender else 3],
                                           (40 - max(0, amount - defense)) / 99, places=6)
                    break
                else:
                    self.fail("normal pool did not reach non-MAGIC armor fixture")

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
                self.assertEqual(env._lib.gz_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_sixteen_profile_output_boundaries(self):
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
                    (MagicReflectionBatchEnv, "gz", 16, 696, 11))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * 1500)(*[12345.0] * 1500)
                options = (c.c_uint16 * 32)(*[54321] * 32)
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 16 if prefix in ("gx", "gy", "gz") else 6)
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
                native = getattr(env._lib, "gz_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-magic-reflection-subset-v16")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 696)
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
            self.assertEqual(env._lib.gz_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.gz_create(0, 64))
            self.assertFalse(lib.gz_create(2, 0))
            self.assertEqual(lib.gz_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gz_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gz_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gz_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gz_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gz_legal(env._handle, 2, env._actions, env._legal), 0)
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
