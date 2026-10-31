"""Boundary/transition tests for the public legacy ctypes bridge.

Usage: python3 tests/elements_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not official God Field rules.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from elements_env import Action, ActionKind, ElementsBatchEnv, _NativeAction
from miracles_env import MiraclesBatchEnv
from legacy_env import LegacyBatchEnv

LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None


def observation_copy(env):
    return tuple(tuple(buffer) for buffer in env.observe())


class ElementsBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return ElementsBatchEnv(count, max_decisions, library=LIBRARY)

    def test_all_profile_output_boundaries(self):
        """All three APIs must respect their independently versioned buffers."""
        with LegacyBatchEnv(2,64,library=LIBRARY) as old, MiraclesBatchEnv(2,64,library=LIBRARY) as miracles, self.create(2) as elements:
            for env,prefix,version,dimension,width in ((old,"gl",1,128,5),(miracles,"gm",2,164,6),(elements,"ge",3,186,6)):
                self.assertEqual((env.observation_size,env.options_width),(dimension,width))
                self.assertEqual(getattr(env._lib,prefix+"_abi_version")(),version)
                env.reset(42)
                features=(c.c_float*420)(*[12345.0]*420)
                options=(c.c_uint16*20)(*[54321]*20)
                status=getattr(env._lib,prefix+"_observe")(env._handle,2,features,options,env._actors,env._phases,env._pray)
                self.assertEqual(status,0)
                self.assertEqual(tuple(features[:2*dimension]),tuple(env.observe()[0]))
                self.assertTrue(all(x==12345.0 for x in features[2*dimension:]))
                self.assertTrue(all(x==54321 for x in options[2*width:]))

    def test_mixed_defense_is_rejected_without_mutation(self):
        """Reach an elemental attack through the API, then mix matching and NE armor."""
        counter_element = {1: 2, 2: 1, 3: 4, 4: 3}
        with self.create(1, 64) as env:
            for seed in range(512):
                features, options, actors, _, _ = env.reset(seed)
                bases = options[0]
                selected = next((i for i in range(16)
                                 if bases & (1 << i) and features[169 + i] > 0), None)
                if selected is None:
                    continue
                element = round(features[169 + selected] * 4)
                attacker = actors[0]
                env.step([Action(ActionKind.ATTACK, selected, 0, 1 - attacker)])
                features, _, _, phases, _ = env.observe()
                self.assertEqual(phases[0], 1)
                self.assertEqual(tuple(features[164:169]),
                                 tuple(float(i == element) for i in range(5)))
                matching = neutral = None
                for slot in range(16):
                    if features[16 + slot * 7 + 3] <= 0:
                        continue
                    defense_element = round(features[169 + slot] * 4)
                    if defense_element == counter_element[element]:
                        matching = slot
                    elif defense_element == 0:
                        neutral = slot
                if matching is None or neutral is None:
                    continue
                compatible = Action(ActionKind.DEFEND, slots=1 << matching)
                incompatible = Action(ActionKind.DEFEND, slots=(1 << matching) | (1 << neutral))
                self.assertEqual(env.legal([compatible]), (True,))
                self.assertEqual(env.legal([incompatible]), (False,))
                self.assertEqual(env.legal([Action(ActionKind.DEFEND)]), (True,))
                before = observation_copy(env)
                with self.assertRaises(ValueError):
                    env.step([incompatible])
                self.assertEqual(observation_copy(env), before)
                env.step([compatible])
                return
            self.fail("seed search did not reach matching plus NE defense fixture")

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
            self.assertEqual(env.ruleset, "legacy-elements-miracles-subset-v3")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 186)
            self.assertEqual(len(options), 2 * 6)
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
            self.assertEqual(env._lib.ge_step(env._handle, 2, malformed, env._rewards,
                                             env._terminated, env._truncated), 0)
            self.assertFalse(any(env._rewards))
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(tuple(env._truncated), (1, 1))
            env.reset_at(1, 9)
            actions = env.sample_actions()
            self.assertEqual(env.legal(actions)[0], False)
            self.assertEqual(env.legal(actions)[1], True)
            env.step(actions)  # Mix a frozen row and an active row.

    def test_native_layout_and_errors(self):
        with self.create(2) as env:
            lib = env._lib
            self.assertEqual(c.sizeof(_NativeAction), 6)
            self.assertEqual([getattr(_NativeAction, field).offset for field in
                              ("kind", "base", "slots", "target", "reserved")], [0, 1, 2, 4, 5])
            self.assertFalse(lib.ge_create(0, 64))
            self.assertFalse(lib.ge_create(2, 0))
            self.assertEqual(lib.ge_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.ge_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.ge_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.ge_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.ge_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.ge_legal(env._handle, 2, env._actions, env._legal), 0)
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
