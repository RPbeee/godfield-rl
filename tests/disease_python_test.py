"""Boundary/transition tests for the public legacy ctypes bridge.

Usage: python3 tests/disease_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not official God Field rules.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from disease_env import Action, ActionKind, DiseaseBatchEnv, _NativeAction
from reflection_env import ReflectionBatchEnv
from chance_env import ChanceBatchEnv
from elements_env import ElementsBatchEnv
from miracles_env import MiraclesBatchEnv
from legacy_env import LegacyBatchEnv

LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None


def observation_copy(env):
    return tuple(tuple(buffer) for buffer in env.observe())


class DiseaseBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return DiseaseBatchEnv(count, max_decisions, library=LIBRARY)

    def advance_to_owner(self, env, owner):
        """Let other seats take harmless discards until the chosen seat acts."""
        for _ in range(4):
            _, options, actors, phases, pray = env.observe()
            self.assertEqual(phases[0], 0)
            if actors[0] == owner:
                return True
            if options[4]:
                env.step([Action(ActionKind.DISCARD, slots=options[4] & -options[4])])
            elif pray[0]:
                env.step([Action(ActionKind.PRAY)])
            else:
                return False
        return False

    @staticmethod
    def disease_of(observation, seat):
        features, _, actors, _, _ = observation
        start = 236 if actors[0] == seat else 241
        return next(i for i, value in enumerate(features[start:start + 5]) if value > .5)

    def test_cold_application_worsening_tick_and_cure(self):
        """Reach Cold through a real self attack, then worsen and cure it."""
        with self.create(1, 128) as env:
            for seed in range(4096):
                features, options, actors, _, _ = env.reset(seed)
                ids = [round(features[16 + i * 7] * 363) for i in range(16)]
                cold = next((i for i, card in enumerate(ids) if card == 34), None)
                cure = next((i for i, card in enumerate(ids) if card in (235, 236)), None)
                extra = next((i for i, card in enumerate(ids) if options[4] & (1 << i)
                              and card not in (235, 236)), None)
                if cold is None or cure is None or extra is None:
                    continue
                owner = actors[0]
                before = observation_copy(env)
                env.script_disease_rolls(0, [99, 0])
                self.assertEqual(observation_copy(env), before, "outcomes leaked into observation")
                invalid = (c.c_uint8 * 2)(99, 100)
                self.assertEqual(env._lib.gd_script_disease_rolls(env._handle, 0, invalid, 2), -3)
                env.step([Action(ActionKind.ATTACK, cold, 0, owner)])
                observed = env.observe()
                self.assertEqual(self.disease_of(observed, owner), 1)
                features, _, actors, _, _ = observed
                self.assertEqual(features[248], 1)
                self.assertEqual(features[249], 1)
                self.assertEqual(features[250], 0)  # Initial legacy counter is zero.
                self.assertAlmostEqual(features[252], 1 / 4, places=6)
                self.assertAlmostEqual(features[253], 1 / 4, places=6)
                self.assertAlmostEqual(features[254], 1 / 99, places=6)
                self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 30 / 99, places=6)
                if not self.advance_to_owner(env, owner):
                    continue
                features, options, _, _, _ = env.observe()
                ids = [round(features[16 + i * 7] * 363) for i in range(16)]
                extra = next((i for i, card in enumerate(ids) if options[4] & (1 << i)
                              and card not in (235, 236)), None)
                if extra is None:
                    continue
                env.step([Action(ActionKind.DISCARD, slots=1 << extra)])
                observed = env.observe()
                self.assertEqual(self.disease_of(observed, owner), 2)
                features, _, actors, _, _ = observed
                self.assertEqual(features[250], 1)
                self.assertAlmostEqual(features[252], 1 / 4, places=6)
                self.assertAlmostEqual(features[253], 2 / 4, places=6)
                self.assertAlmostEqual(features[254], 2 / 99, places=6)
                self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 28 / 99, places=6)
                if not self.advance_to_owner(env, owner):
                    continue
                features, _, _, _, _ = env.observe()
                ids = [round(features[16 + i * 7] * 363) for i in range(16)]
                cure = next(i for i, card in enumerate(ids) if card in (235, 236))
                action = Action(ActionKind.USE, cure, 0, owner)
                self.assertEqual(env.legal([action]), (True,))
                env.step([action])
                observed = env.observe()
                self.assertEqual(self.disease_of(observed, owner), 0)
                features, _, actors, _, _ = observed
                self.assertAlmostEqual(features[0 if actors[0] == owner else 3], 28 / 99, places=6)
                return
            self.fail("seed search could not reach complete Cold/worsening/cure scenario")

    def test_disease_roll_prefix_validation(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            for values in ((-1,), (100,), (True,), (0.5,), (0,) * 65):
                with self.assertRaises(ValueError):
                    env.script_disease_rolls(0, values)
            with self.assertRaises(ValueError):
                env.script_disease_rolls(1, [0])
            self.assertEqual(env._lib.gd_script_disease_rolls(None, 0, None, 0), -1)
            self.assertEqual(env._lib.gd_script_disease_rolls(env._handle, 1, None, 0), -2)
            self.assertEqual(env._lib.gd_script_disease_rolls(env._handle, 0, None, 1), -1)
            self.assertEqual(env._lib.gd_script_disease_rolls(env._handle, 0, None, 65), -2)
            self.assertEqual(env._lib.gd_script_disease_rolls(env._handle, 0, None, 0), 0)
            env.script_disease_rolls(0, [0, 99] * 32)
            env.script_disease_rolls(0, [])
            self.assertEqual(observation_copy(env), before)

    def reaction_fixture(self, env, reaction_kind, need_prefix=False):
        """Reach a real response through reset/attack, without a restore backdoor."""
        for seed in range(2048):
            features, options, actors, _, _ = env.reset(seed)
            base = next((i for i in range(16) if options[0] & (1 << i)
                         and features[169 + i] == 0 and features[186 + i] == 0
                         and round(features[16 + i * 7] * 363) not in (30, 34, 49, 61)), None)
            if base is None:
                continue
            owner = actors[0]
            damage = features[16 + base * 7 + 1] * 30
            env.step([Action(ActionKind.ATTACK, base, 0, 1 - owner)])
            features, options, _, phases, _ = env.observe()
            self.assertEqual(phases[0], 1)
            special = next((i for i in range(16) if options[6] & (1 << i)
                            and round(features[210 + i] * 3) == reaction_kind), None)
            ordinary = next((i for i in range(16) if options[2] & (1 << i)
                             and features[16 + i * 7 + 3] > 0), None)
            if special is not None and (ordinary is not None or not need_prefix):
                return owner, damage, special, ordinary
        self.fail("seed search did not reach a neutral weapon reaction fixture")

    def test_reflection_preserves_owner_and_original_damage(self):
        with self.create(1) as env:
            owner, damage, special, ordinary = self.reaction_fixture(env, 1, need_prefix=True)
            defender = 1 - owner
            before = observation_copy(env)
            initial_count = sum(before[0][16 + i * 7] > 0 for i in range(16))
            action = Action(ActionKind.DEFEND, special, 1 << ordinary)
            self.assertEqual(env.legal([action]), (True,))
            overlap = Action(ActionKind.DEFEND, special, (1 << ordinary) | (1 << special))
            self.assertEqual(env.legal([overlap]), (False,))
            with self.assertRaises(ValueError):
                env.step([overlap])
            self.assertEqual(observation_copy(env), before)
            env.step([action])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (owner, 1))
            self.assertEqual(features[10], 1)  # Actor is still the original turn owner.
            self.assertAlmostEqual(features[8] * 480, damage, places=5)
            self.assertAlmostEqual(features[11], (initial_count - 2) / 16, places=6)
            self.assertEqual(features[226], 1)
            self.assertAlmostEqual(features[227], 1 / 32, places=6)
            self.assertEqual(features[228], 0)  # Current source is no longer the turn owner.
            self.assertEqual(features[229], 1)
            self.assertAlmostEqual(features[230], 39 / 363, places=6)
            self.assertEqual(tuple(features[231:236]), (1, 0, 0, 1, 0))
            self.assertAlmostEqual(features[0], 40 / 99, places=6)
            self.assertAlmostEqual(features[3], 40 / 99, places=6)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)
            source_hp = features[0 if actors[0] == owner else 3]
            defender_hp = features[0 if actors[0] == defender else 3]
            self.assertAlmostEqual(source_hp, (40 - damage) / 99, places=6)
            self.assertAlmostEqual(defender_hp, 40 / 99, places=6)

    def test_scripted_bounce_self_and_other_targets(self):
        for bounce_self in (False, True):
            with self.create(1) as env:
                owner, damage, special, _ = self.reaction_fixture(env, 2)
                defender = 1 - owner
                target = defender if bounce_self else owner
                before = observation_copy(env)
                env.script_bounce_targets(0, [target])
                invalid = (c.c_uint8 * 2)(0, 2)
                self.assertEqual(env._lib.gd_script_bounce_targets(env._handle, 0, invalid, 2), -3)
                self.assertEqual(observation_copy(env), before)
                env.step([Action(ActionKind.DEFEND, special)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], 0 if bounce_self else 1)
                self.assertEqual(features[229], 1)
                self.assertAlmostEqual(features[230], 20 / 363, places=6)
                self.assertEqual(features[231], 0)
                self.assertEqual(features[232], 1)
                self.assertEqual(features[233], float(actors[0] == defender))
                self.assertEqual(features[234], float(actors[0] == target))
                self.assertEqual(features[235], float(bounce_self))
                if not bounce_self:
                    self.assertEqual(actors[0], owner)
                    self.assertAlmostEqual(features[8] * 480, damage, places=5)
                    env.step([Action(ActionKind.DEFEND)])
                    features, _, actors, _, _ = env.observe()
                damaged_hp = features[0 if actors[0] == target else 3]
                other_hp = features[0 if actors[0] != target else 3]
                self.assertAlmostEqual(damaged_hp, (40 - damage) / 99, places=6)
                self.assertAlmostEqual(other_hp, 40 / 99, places=6)

    def test_bounce_target_prefix_validation(self):
        with self.create(1) as env:
            env.reset(42)
            state = observation_copy(env)
            for values in ((-1,), (2,), (True,), (0.5,), (0,) * 65):
                with self.assertRaises(ValueError):
                    env.script_bounce_targets(0, values)
            with self.assertRaises(ValueError):
                env.script_bounce_targets(1, [0])
            self.assertEqual(env._lib.gd_script_bounce_targets(None, 0, None, 0), -1)
            self.assertEqual(env._lib.gd_script_bounce_targets(env._handle, 1, None, 0), -2)
            self.assertEqual(env._lib.gd_script_bounce_targets(env._handle, 0, None, 1), -1)
            self.assertEqual(env._lib.gd_script_bounce_targets(env._handle, 0, None, 65), -2)
            self.assertEqual(env._lib.gd_script_bounce_targets(env._handle, 0, None, 0), 0)
            env.script_bounce_targets(0, [0, 1] * 32)
            env.script_bounce_targets(0, [])
            self.assertEqual(observation_copy(env), state)

    def test_scripted_threshold_hit_and_miss_without_extra_response(self):
        """Use the same seeded weapon at its hit boundary and one past it."""
        with self.create(1) as finder:
            for seed in range(512):
                features, options, actors, _, _ = finder.reset(seed)
                slot = next((i for i in range(16) if options[0] & (1 << i)
                             and features[186 + i] > 0), None)
                if slot is not None:
                    rate = round(features[186 + slot] * 100)
                    amount = features[16 + slot * 7 + 1] * 30
                    card_id = round(features[16 + slot * 7] * 363)
                    source = actors[0]
                    break
            else:
                self.fail("seed search did not find an initial probability weapon")

        for roll, should_hit in ((rate, True), (rate + 1, False)):
            with self.create(1) as env:
                env.reset(seed)
                before = observation_copy(env)
                env.script_hit_rolls(0, [roll])
                self.assertEqual(observation_copy(env), before, "fixture outcome leaked into observation")
                # Invalid replacement must preserve the previously installed outcome.
                invalid = (c.c_uint8 * 2)(1, 0)
                self.assertEqual(env._lib.gd_script_hit_rolls(env._handle, 0, invalid, 2), -3)
                self.assertEqual(observation_copy(env), before)
                env.step([Action(ActionKind.ATTACK, slot, 0, 1 - source)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], int(should_hit))
                self.assertEqual(features[203], 1)
                self.assertEqual(features[204], 1)
                self.assertEqual(features[205], float(should_hit))
                self.assertAlmostEqual(features[206], card_id / 363, places=6)
                self.assertAlmostEqual(features[207], rate / 100, places=6)
                self.assertEqual(features[208], float(actors[0] == source))
                self.assertEqual(features[209], float(actors[0] == 1 - source))
                self.assertAlmostEqual(features[0], 40 / 99, places=6)
                self.assertAlmostEqual(features[3], 40 / 99, places=6)
                if should_hit:
                    self.assertEqual(actors[0], 1 - source)
                    self.assertAlmostEqual(features[202], rate / 100, places=6)
                    env.step([Action(ActionKind.DEFEND)])
                    features, _, actors, phases, _ = env.observe()
                    self.assertEqual(phases[0], 0)
                    source_hp = features[0 if actors[0] == source else 3]
                    target_hp = features[0 if actors[0] == 1 - source else 3]
                    self.assertAlmostEqual(source_hp, 40 / 99, places=6)
                    self.assertAlmostEqual(target_hp, (40 - amount) / 99, places=6)
                else:
                    self.assertEqual(features[202], 0)
                    self.assertFalse(env.legal([Action(ActionKind.DEFEND)])[0])

    def test_script_outcome_boundary_validation(self):
        with self.create(1) as env:
            env.reset(42)
            state = observation_copy(env)
            for values in ((0,), (101,), (True,), (1.5,), (1,) * 65):
                with self.assertRaises(ValueError):
                    env.script_hit_rolls(0, values)
            with self.assertRaises(ValueError):
                env.script_hit_rolls(-1, [1])
            self.assertEqual(env._lib.gd_script_hit_rolls(None, 0, None, 0), -1)
            self.assertEqual(env._lib.gd_script_hit_rolls(env._handle, 1, None, 0), -2)
            self.assertEqual(env._lib.gd_script_hit_rolls(env._handle, 0, None, 1), -1)
            self.assertEqual(env._lib.gd_script_hit_rolls(env._handle, 0, None, 65), -2)
            self.assertEqual(env._lib.gd_script_hit_rolls(env._handle, 0, None, 0), 0)
            env.script_hit_rolls(0, [1] * 64)
            env.script_hit_rolls(0, [])
            self.assertEqual(observation_copy(env), state)

    def test_all_profile_output_boundaries(self):
        """All six profiles retain separate fixed output extents."""
        with LegacyBatchEnv(2,64,library=LIBRARY) as old, MiraclesBatchEnv(2,64,library=LIBRARY) as miracles, ElementsBatchEnv(2,64,library=LIBRARY) as elements, ChanceBatchEnv(2,64,library=LIBRARY) as chance, ReflectionBatchEnv(2,64,library=LIBRARY) as reflection, self.create(2) as disease:
            for env,prefix,version,dimension,width in ((old,"gl",1,128,5),(miracles,"gm",2,164,6),(elements,"ge",3,186,6),(chance,"gc",4,210,6),(reflection,"gr",5,236,7),(disease,"gd",6,258,7)):
                self.assertEqual((env.observation_size,env.options_width),(dimension,width))
                self.assertEqual(getattr(env._lib,prefix+"_abi_version")(),version)
                env.reset(42)
                features=(c.c_float*600)(*[12345.0]*600)
                options=(c.c_uint16*24)(*[54321]*24)
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
                                 if bases & (1 << i) and features[169 + i] > 0 and features[186 + i] == 0), None)
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
            self.assertEqual(env.ruleset, "legacy-disease-subset-v6")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 258)
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
            self.assertEqual(env._lib.gd_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.gd_create(0, 64))
            self.assertFalse(lib.gd_create(2, 0))
            self.assertEqual(lib.gd_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gd_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gd_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gd_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gd_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gd_legal(env._handle, 2, env._actions, env._legal), 0)
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
