"""Public ABI14 boundaries and explicit FIFO mixed-counter responses.

Usage: python3 tests/counter_extensions_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from counter_extensions_env import Action, ActionKind, CounterExtensionsBatchEnv, _NativeAction
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


class CounterExtensionsBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return CounterExtensionsBatchEnv(count, max_decisions, library=LIBRARY)

    def response_fixture(self, env, cards, dark=False, need_chance=False):
        for seed in range(131072):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            base = next((i for i in range(16) if options[0] & (1 << i)
                         and round(features[285 + i] * 6) == (6 if dark else 0)
                         and features[186 + i] == 0
                         and features[16 + i * 7 + 1] * 30 <= 5
                         and ids[i] not in (30, 34, 43, 49)), None)
            if base is None or (need_chance and not any(options[0] & (1 << i) and features[186 + i] > 0
                                                       for i in range(16))):
                continue
            source = actors[0]
            amount = features[16 + base * 7 + 1] * 30
            env.step([Action(ActionKind.ATTACK, base, target=1 - source)])
            ids = self.hand_ids(env.observe()[0])
            if all(card in ids for card in cards):
                return seed, source, amount, tuple(ids.index(card) for card in cards)
        self.fail("seed search did not find requested mixed counter response")

    def test_counter_order_changes_forced_hit_hp_and_future_roll(self):
        with self.create(1, 128) as finder:
            seed, source, amount, slots = self.response_fixture(finder, (192, 187), need_chance=True)
        for cloud_first in (True, False):
            with self.subTest(cloud_first=cloud_first), self.create(1, 128) as env:
                # Reconstruct the exact same ordinary attack and response from the public seed.
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                base = next(i for i in range(16) if options[0] & (1 << i) and features[285 + i] == 0
                            and features[186 + i] == 0 and features[16 + i * 7 + 1] * 30 <= 5
                            and ids[i] not in (30, 34, 43, 49))
                env.step([Action(ActionKind.ATTACK, base, target=1 - source)])
                cloud, fire = slots
                mask = (1 << cloud) | (1 << fire)
                order = (cloud, fire) if cloud_first else (fire, cloud)
                before = observation_copy(env)
                env.script_hit_rolls(0, [76, 1])
                self.assertEqual(observation_copy(env), before)
                for bad_order in ((), (cloud,), (cloud, cloud)):
                    bad = Action(ActionKind.DEFEND, slots=mask, counter_order=bad_order)
                    self.assertFalse(env.legal([bad])[0])
                    with self.assertRaises(ValueError):
                        env.step([bad])
                    self.assertEqual(observation_copy(env), before)
                env.step([Action(ActionKind.DEFEND, slots=mask, counter_order=order)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (source, 1))
                if cloud_first:
                    self.assertEqual(features[532], 1)
                    self.assertAlmostEqual(features[410], 192 / 363, places=6)
                    self.assertAlmostEqual(features[533], 187 / 363, places=6)
                    self.assertAlmostEqual(features[535], .75, places=6)
                    env.step([Action(ActionKind.DEFEND)])
                    features, _, _, phases, _ = env.observe()
                    self.assertEqual(phases[0], 1)
                    self.assertEqual(features[264], 1)
                    self.assertEqual(features[205], 1)
                    env.step([Action(ActionKind.DEFEND)])
                else:
                    # Fire roll76 missed; the next item is the direct DarkCloud response.
                    self.assertEqual(features[205], 0)
                    self.assertEqual(features[264], 0)
                    self.assertEqual(features[532], 1)
                    self.assertAlmostEqual(features[410], 192 / 363, places=6)
                    env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertEqual(features[259 if actors[0] == source else 261], 1)
                self.assertAlmostEqual(features[0 if actors[0] == source else 3], (40 - amount if cloud_first else 40) / 99, places=6)
                self.assertTrue(self.advance_to_owner(env, source))
                features, options, _, _, _ = env.observe()
                chance = next(i for i in range(16) if options[0] & (1 << i) and features[186 + i] > 0)
                env.step([Action(ActionKind.ATTACK, chance, target=1 - source)])
                features, _, _, phases, _ = env.observe()
                self.assertEqual(phases[0], int(not cloud_first))
                self.assertEqual(features[205], float(not cloud_first))

    def test_mana_counter_is_self_auto_and_skips_dead_owner(self):
        for dark in (False, True):
            with self.subTest(dark=dark), self.create(1, 128) as env:
                _, source, amount, (mana,) = self.response_fixture(env, (193,), dark=dark)
                defender = 1 - source
                env.step([Action(ActionKind.DEFEND, slots=1 << mana, counter_order=(mana,))])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual(phases[0], 0, "self MP counter requested an external response")
                expected = 10 if dark else min(99, 10 + amount * 2)
                self.assertAlmostEqual(features[1 if actors[0] == defender else 4], expected / 99, places=6)
                self.assertAlmostEqual(features[0 if actors[0] == defender else 3], 0 if dark else (40 - amount) / 99, places=6)

    def test_glory_counter_is_direct_harm_without_hp_damage(self):
        with self.create(1, 128) as env:
            _, source, _, (glory,) = self.response_fixture(env, (191,))
            env.step([Action(ActionKind.DEFEND, slots=1 << glory, counter_order=(glory,))])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (source, 1))
            self.assertEqual(features[532], 1)
            self.assertEqual(features[265], .5)
            self.assertEqual(features[417], 1)
            self.assertAlmostEqual(features[410], 191 / 363, places=6)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertEqual(features[258 if actors[0] == source else 260], 1)
            self.assertAlmostEqual(features[0 if actors[0] == source else 3], 40 / 99, places=6)

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
                self.assertEqual(env._lib.gx_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_fourteen_profile_output_boundaries(self):
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
                    (CounterExtensionsBatchEnv, "gx", 14, 661, 11))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * 1450)(*[12345.0] * 1450)
                options = (c.c_uint16 * 32)(*[54321] * 32)
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 16 if prefix == "gx" else 6)
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
                native = getattr(env._lib, "gx_script_" + name)
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
                            if options[4] & (1 << i) and card not in (195, 235, 236)), None)
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
            self.assertEqual(env.ruleset, "legacy-counter-extensions-subset-v14")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 661)
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
            self.assertEqual(env._lib.gx_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.gx_create(0, 64))
            self.assertFalse(lib.gx_create(2, 0))
            self.assertEqual(lib.gx_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gx_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gx_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gx_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gx_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gx_legal(env._handle, 2, env._actions, env._legal), 0)
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
