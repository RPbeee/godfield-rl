"""Public ABI8 boundaries and real reset/step Light, Darkness and Glory transitions.

Usage: python3 tests/celestial_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from celestial_env import Action, ActionKind, CelestialBatchEnv, _NativeAction
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


class CelestialBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return CelestialBatchEnv(count, max_decisions, library=LIBRARY)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_light_armor_and_mixed_defense(self):
        """Reach an elemental response with actual Light armor through reset/step."""
        with self.create(1) as env:
            for seed in range(8192):
                features, options, actors, _, _ = env.reset(seed)
                base = next((i for i in range(16) if options[0] & (1 << i)
                             and round(features[285 + i] * 6) == 1
                             and features[186 + i] == 0
                             and features[16 + i * 7 + 1] * 30 <= 6), None)
                if base is None:
                    continue
                source = actors[0]
                env.step([Action(ActionKind.ATTACK, base, target=1 - source)])
                features, _, actors, phases, _ = env.observe()
                light = next((i for i in range(16)
                              if round(features[285 + i] * 6) == 5
                              and features[16 + i * 7 + 3] > 0), None)
                neutral = next((i for i in range(16)
                                if features[285 + i] == 0
                                and features[16 + i * 7 + 3] > 0), None)
                if light is None or neutral is None:
                    continue
                self.assertEqual((actors[0], phases[0]), (1 - source, 1))
                self.assertEqual(features[169 + light], 0, "Light leaked into old scalar enum")
                self.assertEqual(features[283], 1)
                self.assertAlmostEqual(features[284], 1 / 6, places=6)
                good = Action(ActionKind.DEFEND, slots=1 << light)
                bad = Action(ActionKind.DEFEND, slots=(1 << light) | (1 << neutral))
                self.assertEqual(env.legal([good]), (True,))
                self.assertEqual(env.legal([bad]), (False,))
                before = observation_copy(env)
                with self.assertRaises(ValueError):
                    env.step([bad])
                self.assertEqual(observation_copy(env), before)
                env.step([good])
                features = env.observe()[0]
                self.assertAlmostEqual(features[0], 40 / 99, places=6)
                self.assertAlmostEqual(features[3], 40 / 99, places=6)
                return
            self.fail("seed search did not find Light armor and neutral armor response")

    def test_glory_from_light_weapon_and_cure(self):
        """Natural ID70 enables Glory without a state-setting test API."""
        with self.create(1, 128) as env:
            for seed in range(32768):
                features, _, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 70 not in ids:
                    continue
                source = actors[0]
                slot = ids.index(70)
                self.assertAlmostEqual(features[285 + slot], 5 / 6, places=6)
                self.assertEqual(features[169 + slot], 0)
                self.assertEqual(features[267 + slot], .5)
                env.step([Action(ActionKind.ATTACK, slot, target=1 - source)])
                features, _, actors, phases, _ = env.observe()
                ids = self.hand_ids(features)
                if not any(card in (235, 236) for card in ids):
                    continue
                self.assertEqual((actors[0], phases[0]), (1 - source, 1))
                self.assertAlmostEqual(features[284], 5 / 6, places=6)
                self.assertEqual(tuple(features[164:169]), (0, 0, 0, 0, 0))
                # This subset has no Darkness DFS. Even large armor cannot stop Light.
                for i in range(16):
                    if features[16 + i * 7 + 3] > 0:
                        self.assertEqual(env.legal([Action(ActionKind.DEFEND, slots=1 << i)]), (False,))
                target = actors[0]
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertEqual(features[258 if actors[0] == target else 260], 1)
                self.assertAlmostEqual(features[0 if actors[0] == target else 3], 38 / 99, places=6)
                if not self.advance_to_owner(env, source):
                    continue
                features, options, _, _, _ = env.observe()
                ordinary = next((i for i in range(16) if options[0] & (1 << i)
                                 and features[285 + i] == 0 and features[186 + i] == 0
                                 and features[16 + i * 7 + 1] * 30 <= 10
                                 and self.hand_ids(features)[i] not in (30, 34, 43)), None)
                if ordinary is None:
                    continue
                env.step([Action(ActionKind.ATTACK, ordinary, target=target)])
                features, options, _, _, _ = env.observe()
                armors = [i for i in range(16) if options[2] & (1 << i)]
                if len(armors) < 2 or not any(card in (235, 236) for card in self.hand_ids(features)):
                    continue
                self.assertAlmostEqual(features[263], 1 / 16, places=6)
                one = Action(ActionKind.DEFEND, slots=1 << armors[0])
                two = Action(ActionKind.DEFEND, slots=(1 << armors[0]) | (1 << armors[1]))
                self.assertEqual(env.legal([one]), (True,))
                self.assertEqual(env.legal([two]), (False,))
                before = observation_copy(env)
                with self.assertRaises(ValueError):
                    env.step([two])
                self.assertEqual(observation_copy(env), before)
                env.step([Action(ActionKind.DEFEND)])
                self.assertTrue(self.advance_to_owner(env, target))
                features = env.observe()[0]
                cure = next(i for i, card in enumerate(self.hand_ids(features)) if card in (235, 236))
                env.step([Action(ActionKind.USE, cure, target=target)])
                features, _, actors, _, _ = env.observe()
                self.assertEqual(features[258 if actors[0] == target else 260], 0)
                return
            self.fail("seed search did not find natural Glory and cure")

    def test_darkness_chance_boundary_and_residual_execution(self):
        """ID101 misses past 50; on hit its residual damage determines HP0."""
        with self.create(1) as env:
            for seed in range(8192):
                features, _, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 101 not in ids:
                    continue
                slot = ids.index(101)
                source = actors[0]
                env.script_hit_rolls(0, [50])
                env.step([Action(ActionKind.ATTACK, slot, target=source)])
                features, options, actors, phases, _ = env.observe()
                armor = next((i for i in range(16) if options[2] & (1 << i)
                              and features[16 + i * 7 + 3] * 30 >= 2), None)
                if armor is not None:
                    break
            else:
                self.fail("seed search did not find dark chance weapon and sufficient armor")
            self.assertEqual((actors[0], phases[0]), (1 - source, 1))
            self.assertEqual(features[284], 1)
            self.assertEqual(tuple(features[164:169]), (0, 0, 0, 0, 0))
            env.step([Action(ActionKind.DEFEND, slots=1 << armor)])
            self.assertAlmostEqual(env.observe()[0][0], 40 / 99, places=6)
            self.assertAlmostEqual(env.observe()[0][3], 40 / 99, places=6)
            for roll, should_hit in ((50, True), (51, False)):
                env.reset(seed)
                env.script_hit_rolls(0, [roll])
                env.step([Action(ActionKind.ATTACK, slot, target=source)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], int(should_hit))
                self.assertEqual(features[264], 0)
                self.assertEqual(features[205], float(should_hit))
                if should_hit:
                    rewards, terminal, truncated = env.step([Action(ActionKind.DEFEND)])
                    features, _, actors, _, _ = env.observe()
                    self.assertEqual((terminal[0], truncated[0]), (1, 0))
                    self.assertEqual(rewards[2 * 0 + source], 1)
                    self.assertEqual(rewards[1 - source], -1)
                    self.assertEqual(features[0 if actors[0] == 1 - source else 3], 0)
                else:
                    self.assertAlmostEqual(features[0], 40 / 99, places=6)
                    self.assertAlmostEqual(features[3], 40 / 99, places=6)

    def test_dark_cloud_forced_hit_cure_and_preserved_roll(self):
        """Real card 43 applies DarkCloud; forced hit leaves roll 100 for after cure."""
        with self.create(1, 128) as env:
            for seed in range(32768):
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                chances = [i for i in range(16) if options[0] & (1 << i)
                           and features[186 + i] > 0 and round(features[285 + i] * 6) != 6
                           and features[16 + i * 7 + 1] * 30 <= 10]
                if 43 in ids and 236 in ids and len(chances) >= 2:
                    source = actors[0]
                    break
            else:
                self.fail("seed search did not find DarkCloud, all-cure and two chance weapons")
            target = 1 - source
            cloud = ids.index(43)
            self.assertEqual(features[267 + cloud], 1)
            self.assertEqual(tuple(features[258:263]), (0, 0, 0, 0, 1))
            env.step([Action(ActionKind.ATTACK, cloud, target=target)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (target, 1))
            self.assertEqual(features[265], 1)
            self.assertAlmostEqual(features[266], 43 / 363, places=6)
            self.assertEqual(features[263], 1)  # Enabled ordinary response limit = 16/16.
            self.assertEqual(features[259], 0, "on-damage status applied before damage")
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertEqual(features[259 if actors[0] == target else 261], 1)
            self.assertAlmostEqual(features[0 if actors[0] == target else 3], 29 / 99, places=6)
            self.assertTrue(self.advance_to_owner(env, source))
            before = observation_copy(env)
            env.script_hit_rolls(0, [100])
            self.assertEqual(observation_copy(env), before, "hidden prefix leaked into features")
            # An invalid replacement must preserve the single installed roll.
            invalid = (c.c_uint8 * 2)(1, 0)
            self.assertEqual(env._lib.gs_script_hit_rolls(env._handle, 0, invalid, 2), -3)
            features, options, _, _, _ = env.observe()
            first = next(i for i in range(16) if options[0] & (1 << i)
                         and features[186 + i] > 0 and round(features[285 + i] * 6) != 6
                         and features[16 + i * 7 + 1] * 30 <= 10)
            env.step([Action(ActionKind.ATTACK, first, target=target)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (target, 1))
            self.assertEqual((features[204], features[205], features[264]), (1, 1, 1))
            env.step([Action(ActionKind.DEFEND)])
            self.assertTrue(self.advance_to_owner(env, source))
            features, _, _, _, _ = env.observe()
            cure = self.hand_ids(features).index(236)
            env.step([Action(ActionKind.USE, cure, target=target)])
            self.assertEqual(env.observe()[3][0], 1)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertEqual(features[259 if actors[0] == target else 261], 0)
            self.assertTrue(self.advance_to_owner(env, source))
            features, options, _, _, _ = env.observe()
            second = next(i for i in range(16) if options[0] & (1 << i)
                          and features[186 + i] > 0)
            target_hp = features[3]
            env.step([Action(ActionKind.ATTACK, second, target=target)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0, "preserved roll 100 should miss after cure")
            self.assertEqual((features[204], features[205], features[264]), (1, 0, 0))
            self.assertAlmostEqual(features[0 if actors[0] == target else 3], target_hp, places=6)

    def test_all_eight_profile_output_boundaries(self):
        """Earlier profiles retain their fixed row strides and complete buffers."""
        profiles = ((LegacyBatchEnv, "gl", 1, 128, 5),
                    (MiraclesBatchEnv, "gm", 2, 164, 6),
                    (ElementsBatchEnv, "ge", 3, 186, 6),
                    (ChanceBatchEnv, "gc", 4, 210, 6),
                    (ReflectionBatchEnv, "gr", 5, 236, 7),
                    (DiseaseBatchEnv, "gd", 6, 258, 7),
                    (HarmsBatchEnv, "gh", 7, 283, 7),
                    (CelestialBatchEnv, "gs", 8, 301, 7))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * 650)(*[12345.0] * 650)
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
                native = getattr(env._lib, "gs_script_" + name)
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
        """Let other seats take harmless discards until the chosen seat acts."""
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
                    if action.kind == ActionKind.CAST:
                        casts += 1
                        old_actor = state[2][i]
                        old_mp = state[0][i * left.observation_size + 1]
                        offset = 1 if next_observation[2][i] == old_actor else 4
                        new_mp = next_observation[0][i * left.observation_size + offset]
                        cost = (6, 8)[action.base]
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
            self.assertEqual(env.ruleset, "legacy-celestial-subset-v8")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 301)
            self.assertEqual(len(options), 2 * 7)
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
            self.assertEqual(env._lib.gs_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.gs_create(0, 64))
            self.assertFalse(lib.gs_create(2, 0))
            self.assertEqual(lib.gs_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gs_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gs_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gs_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gs_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gs_legal(env._handle, 2, env._actions, env._legal), 0)
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
