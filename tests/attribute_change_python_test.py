"""Public ABI11 boundaries and real attribute setter attack transitions.

Usage: python3 tests/attribute_change_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from attribute_change_env import Action, ActionKind, AttributeChangeBatchEnv, _NativeAction
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


class AttributeChangeBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return AttributeChangeBatchEnv(count, max_decisions, library=LIBRARY)

    def test_wand_is_a_base_weapon_without_additive_boost(self):
        for card, amount, element in ((65, 2, 1), (85, 5, 2)):
            with self.subTest(card=card), self.create(1) as env:
                for seed in range(8192):
                    features, options, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    if card in ids:
                        slot, source = ids.index(card), actors[0]
                        break
                else:
                    self.fail("seed search did not find attribute wand")
                self.assertEqual(features[356], 1)
                self.assertAlmostEqual(features[357 + slot], element / 6, places=6)
                self.assertAlmostEqual(features[16 + slot * 7 + 1], amount / 30, places=6)
                self.assertEqual(features[16 + slot * 7 + 2], 0)
                self.assertTrue(options[8] & (1 << slot))
                self.assertFalse(options[1] & (1 << slot), "setter advertised as an additive boost")
                self.assertFalse(options[4] & (1 << slot), "weapon advertised as discardable")
                action = Action(ActionKind.ATTACK, slot, target=1 - source)
                self.assertEqual(env.legal([action]), (True,))
                self.assertEqual(env.legal([Action(ActionKind.ATTACK_SET_FIRE, slot, target=source)]), (False,))
                env.step([action])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (1 - source, 1))
                self.assertAlmostEqual(features[284], element / 6, places=6)
                self.assertAlmostEqual(features[8], amount / 480, places=6)
                self.assertEqual((features[373], features[374]), (0, 0))
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertAlmostEqual(features[0 if actors[0] == 1 - source else 3], (40 - amount) / 99, places=6)

    def test_same_extras_reach_fire_water_or_neutral_without_extra_wand_damage(self):
        """A single mask represents three old attack-order outcomes through explicit kinds."""
        with self.create(1) as env:
            for seed in range(65536):
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 65 not in ids or 85 not in ids:
                    continue
                base = next((i for i in range(16) if options[0] & (1 << i)
                             and features[285 + i] == 0 and features[186 + i] == 0
                             and ids[i] not in (30, 34, 43, 49)
                             and features[16 + i * 7 + 1] * 30 <= 14), None)
                boost = next((i for i in range(16) if options[1] & (1 << i)
                              and features[285 + i] == 0 and i != base), None)
                if base is not None and boost is not None:
                    fire, water, source = ids.index(65), ids.index(85), actors[0]
                    amount = features[16 + base * 7 + 1] * 30 + features[16 + boost * 7 + 2] * 20
                    break
            else:
                self.fail("seed search did not find both setters and a neutral ordinary boost")
            for kind, element, representative in (
                    (ActionKind.ATTACK_SET_FIRE, 1, 65),
                    (ActionKind.ATTACK_SET_WATER, 2, 85),
                    (ActionKind.ATTACK_SET_NEUTRAL, 0, 65 if fire < water else 85)):
                with self.subTest(kind=kind):
                    env.reset(seed)
                    slots = (1 << fire) | (1 << water) | (1 << boost)
                    action = Action(kind, base, slots, 1 - source)
                    self.assertEqual(env.legal([action]), (True,))
                    # Missing the final setter's color cannot be repaired by base color.
                    missing = 1 << (water if kind == ActionKind.ATTACK_SET_FIRE else fire)
                    if kind != ActionKind.ATTACK_SET_NEUTRAL:
                        self.assertEqual(env.legal([Action(kind, base, missing, source)]), (False,))
                    before = observation_copy(env)
                    impossible = Action(ActionKind.ATTACK_SET_NEUTRAL, base, (1 << fire) | (1 << water), source)
                    self.assertEqual(env.legal([impossible]), (False,))
                    with self.assertRaises(ValueError):
                        env.step([impossible])
                    self.assertEqual(observation_copy(env), before)
                    env.step([action])
                    features, _, actors, phases, _ = env.observe()
                    self.assertEqual((actors[0], phases[0]), (1 - source, 1))
                    self.assertAlmostEqual(features[284], element / 6, places=6)
                    self.assertAlmostEqual(features[8] * 480, amount, places=4)
                    self.assertAlmostEqual(features[373], representative / 363, places=6)
                    self.assertEqual(features[374], 1)
                    env.step([Action(ActionKind.DEFEND)])
                    features, _, actors, _, _ = env.observe()
                    self.assertAlmostEqual(features[0 if actors[0] == 1 - source else 3], (40 - amount) / 99, places=5)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_eleven_profile_output_boundaries(self):
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
                    (AttributeChangeBatchEnv, "ga", 11, 375, 9))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * 850)(*[12345.0] * 850)
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
                native = getattr(env._lib, "ga_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-attribute-change-subset-v11")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 375)
            self.assertEqual(len(options), 2 * 9)
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
            self.assertEqual(env._lib.ga_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.ga_create(0, 64))
            self.assertFalse(lib.ga_create(2, 0))
            self.assertEqual(lib.ga_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.ga_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.ga_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.ga_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.ga_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.ga_legal(env._handle, 2, env._actions, env._legal), 0)
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
