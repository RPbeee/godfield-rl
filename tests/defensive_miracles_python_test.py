"""Public ABI10 boundaries and real defensive miracle acquisition/reuse.

Usage: python3 tests/defensive_miracles_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from defensive_miracles_env import Action, ActionKind, DefensiveMiraclesBatchEnv, _NativeAction
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


class DefensiveMiraclesBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return DefensiveMiraclesBatchEnv(count, max_decisions, library=LIBRARY)

    def response_fixture(self, env, incoming_id, defensive_id, need_armor=False):
        """Only normal reset, ownership-visible card IDs and public actions are used."""
        for seed in range(32768):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if incoming_id is None:
                base = next((i for i in range(16) if options[0] & (1 << i)
                             and features[285 + i] == 0 and features[186 + i] == 0
                             and features[16 + i * 7 + 1] * 30 <= 10
                             and ids[i] not in (30, 34, 43)), None)
            else:
                base = ids.index(incoming_id) if incoming_id in ids else None
            if base is None:
                continue
            source = actors[0]
            kind = ActionKind.ATTACK if incoming_id is None else ActionKind.USE
            env.step([Action(kind, base, target=1 - source)])
            features, options, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (1 - source, 1))
            ids = self.hand_ids(features)
            if defensive_id in ids and (not need_armor or options[2]):
                return source, ids.index(defensive_id)
        self.fail("seed search did not reach requested defensive miracle response")

    def test_block_acquisition_and_mana_guard(self):
        with self.create(1, 128) as env:
            source, block = self.response_fixture(env, None, 215)
            defender = 1 - source
            features, options, _, _, _ = env.observe()
            self.assertEqual(features[334], 1)
            self.assertEqual(features[337 + block], 1)
            self.assertTrue(options[6] & (1 << block))
            self.assertEqual(options[7], 0)
            self.assertEqual(env.legal([Action(ActionKind.DEFEND_CAST, 1)]), (False,))
            env.step([Action(ActionKind.DEFEND, block)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)
            self.assertEqual((features[353], features[354]), (0, 1))
            self.assertEqual(features[229], 1)
            self.assertAlmostEqual(features[230], 215 / 363, places=6)
            self.assertAlmostEqual(features[1 if actors[0] == defender else 4], 4 / 99, places=6)
            self.assertAlmostEqual(features[0], 40 / 99, places=6)
            self.assertAlmostEqual(features[3], 40 / 99, places=6)
            self.assertTrue(self.advance_to_owner(env, defender))
            features = env.observe()[0]
            self.assertAlmostEqual(features[336], 1 / 128, places=6)
            self.assertFalse(env.legal([Action(ActionKind.CAST, 15, target=defender)])[0])
            self.assertFalse(env.legal([Action(ActionKind.DEFEND_CAST, 1)])[0])

    def test_bounce_first_use_and_retained_reuse(self):
        with self.create(1, 128) as env:
            source, bounce = self.response_fixture(env, 198, 210)
            defender = 1 - source
            features, options, _, _, _ = env.observe()
            self.assertEqual(features[337 + bounce], .5)
            self.assertTrue(options[6] & (1 << bounce))
            env.script_bounce_targets(0, [source, defender])
            env.step([Action(ActionKind.DEFEND, bounce)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (source, 1))
            self.assertEqual((features[353], features[354]), (0, 0))
            self.assertEqual(features[232], 1)
            self.assertAlmostEqual(features[355], 1 / 128, places=6)
            self.assertAlmostEqual(features[303], 198 / 363, places=6)
            self.assertAlmostEqual(features[4], 5 / 99, places=6)
            env.step([Action(ActionKind.DEFEND)])
            self.assertTrue(self.advance_to_owner(env, source))
            env.step([Action(ActionKind.CAST, 2, target=defender)])
            features, options, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (defender, 1))
            self.assertAlmostEqual(features[335], 1 / 128, places=6)
            self.assertTrue(options[7] & 1)
            hand_before = sum(card != 0 for card in self.hand_ids(features))
            before = observation_copy(env)
            bad = Action(ActionKind.DEFEND_CAST, 0, target=source)
            self.assertEqual(env.legal([bad]), (False,))
            with self.assertRaises(ValueError):
                env.step([bad])
            self.assertEqual(observation_copy(env), before)
            env.step([Action(ActionKind.DEFEND_CAST, 0)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)  # Scripted self bounce resolves immediately.
            self.assertEqual((features[353], features[354]), (1, 0))
            self.assertAlmostEqual(features[1 if actors[0] == defender else 4], 0, places=6)
            self.assertAlmostEqual(features[0 if actors[0] == defender else 3], 36 / 99, places=6)
            self.assertTrue(self.advance_to_owner(env, defender))
            features = env.observe()[0]
            self.assertAlmostEqual(features[335], 1 / 128, places=6)
            self.assertEqual(sum(card != 0 for card in self.hand_ids(features)), hand_before + 1)

    def test_utility_armor_and_bounce_preserve_full_effect(self):
        with self.create(1, 128) as env:
            source, bounce = self.response_fixture(env, 220, 210, need_armor=True)
            features, options, actors, _, _ = env.observe()
            ordinary = next((i for i in range(16) if options[2] & (1 << i)), None)
            if ordinary is None:
                self.fail("fixture needs normal armor on non-damage response")
            defender = actors[0]
            self.assertEqual(env.legal([Action(ActionKind.DEFEND, slots=1 << ordinary)]), (True,))
            initial_hand = sum(card != 0 for card in self.hand_ids(features))
            env.script_bounce_targets(0, [defender])
            action = Action(ActionKind.DEFEND, bounce, slots=1 << ordinary)
            self.assertEqual(env.legal([action]), (True,))
            env.step([action])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)
            self.assertAlmostEqual(features[2 if actors[0] == defender else 5], 35 / 99, places=6)
            self.assertTrue(self.advance_to_owner(env, defender))
            self.assertEqual(sum(card != 0 for card in self.hand_ids(env.observe()[0])), initial_hand)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_ten_profile_output_boundaries(self):
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
                    (DefensiveMiraclesBatchEnv, "gb", 10, 356, 8))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * 750)(*[12345.0] * 750)
                options = (c.c_uint16 * 24)(*[54321] * 24)
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
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
                native = getattr(env._lib, "gb_script_" + name)
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

    def advance_to_owner(self, env, owner):
        """Advance other seats without changing the chosen caster's resources."""
        for _ in range(4):
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(phases[0], 0)
            if actors[0] == owner:
                return True
            discard = next((i for i, card in enumerate(self.hand_ids(features))
                            if options[4] & (1 << i) and card not in (235, 236)), None)
            if discard is not None:
                env.step([Action(ActionKind.DISCARD, slots=1 << discard)])
            elif pray[0]:
                env.step([Action(ActionKind.PRAY)])
            else:
                utility = next((i for i in range(16) if options[3] & (1 << i)
                                and (features[16 + i * 7 + 4] > 0
                                     or features[16 + i * 7 + 5] > 0)), None)
                safe_weapon = next((i for i in range(16) if options[0] & (1 << i)
                                    and features[186 + i] == 0 and features[285 + i] == 0
                                    and 0 < features[16 + i * 7 + 1] * 30 <= 3
                                    and features[0] * 99 > features[16 + i * 7 + 1] * 30), None)
                if utility is not None:
                    env.step([Action(ActionKind.USE, utility, target=actors[0])])
                elif safe_weapon is not None:
                    env.step([Action(ActionKind.ATTACK, safe_weapon, target=actors[0])])
                else:
                    return False
        return False

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
            self.assertEqual(env.ruleset, "legacy-defensive-miracles-subset-v10")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 356)
            self.assertEqual(len(options), 2 * 8)
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
            self.assertEqual(env._lib.gb_step(env._handle, 2, malformed, env._rewards,
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
            self.assertEqual(c.sizeof(_NativeAction), 6)
            self.assertEqual([getattr(_NativeAction, field).offset for field in
                              ("kind", "base", "slots", "target", "reserved")], [0, 1, 2, 4, 5])
            self.assertFalse(lib.gb_create(0, 64))
            self.assertFalse(lib.gb_create(2, 0))
            self.assertEqual(lib.gb_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gb_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gb_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gb_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gb_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gb_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)

    def test_python_validation_and_close(self):
        for args in ((9,), (True,), (ActionKind.ATTACK, 256),
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
