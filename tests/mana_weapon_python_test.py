"""Public ABI18 boundaries and normal-pool mana-powered weapons.

Usage: python3 tests/mana_weapon_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from mana_weapon_env import Action, ActionKind, ManaWeaponBatchEnv, _NativeAction
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


class ManaWeaponBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return ManaWeaponBatchEnv(count, max_decisions, library=LIBRARY)

    def weapon_fixture(self, env, extra=None):
        for seed in range(131072):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if 57 not in ids:
                continue
            base = ids.index(57)
            if extra is None or extra in ids:
                return actors[0], base, (ids.index(extra) if extra is not None else None)
        self.fail("normal pool did not reach mana weapon fixture")

    def test_base_spends_all_mana_and_publishes_only_resolved_payload(self):
        with self.create(1, 128) as env:
            owner, base, _ = self.weapon_fixture(env)
            features, options, _, _, pray = env.observe()
            self.assertEqual(features[713], 1)
            self.assertEqual(features[714 + base], 1)
            self.assertAlmostEqual(features[16 + base * 7 + 1], 20 / 30, places=6)
            self.assertTrue(options[0] & (1 << base))
            self.assertFalse(pray[0])
            env.step([Action(ActionKind.ATTACK, base, target=1 - owner)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (1 - owner, 1))
            self.assertEqual(features[4], 0)
            self.assertEqual(features[730], 1)
            self.assertAlmostEqual(features[731], 10 / 99, places=6)
            self.assertAlmostEqual(features[8], 20 / 480, places=6)
            self.assertEqual(features[302], 0, "weapon acquired miracle identity")
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[3 if actors[0] == owner else 0], 20 / 99, places=6)

    def test_self_attack_and_attribute_setter_preserve_weapon_rules(self):
        with self.create(1, 128) as env:
            owner, base, setter = self.weapon_fixture(env, 65)
            env.step([Action(ActionKind.ATTACK_SET_FIRE, base, 1 << setter, 1 - owner)])
            features, _, _, phases, _ = env.observe()
            self.assertEqual(phases[0], 1)
            self.assertAlmostEqual(features[284], 1 / 6, places=6)
            self.assertAlmostEqual(features[8], 20 / 480, places=6)  # Setter ATK2 is not added.
            self.assertEqual(features[730], 1)
            self.assertAlmostEqual(features[373], 65 / 363, places=6)
            for seed in range(131072):
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 57 not in ids or not options[1]:
                    continue
                owner, base = actors[0], ids.index(57)
                boost = next(i for i in range(16) if options[1] & (1 << i))
                amount = 20 + round(features[16 + boost * 7 + 2] * 20)
                env.step([Action(ActionKind.ATTACK, base, 1 << boost, 1 - owner)])
                features = env.observe()[0]
                self.assertAlmostEqual(features[8], amount / 480, places=6)
                self.assertAlmostEqual(features[731], 10 / 99, places=6)
                break
            else:
                self.fail("normal pool did not reach mana weapon plus ordinary boost")
            owner, base, _ = self.weapon_fixture(env)
            env.step([Action(ActionKind.ATTACK, base, target=owner)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)
            start = 0 if actors[0] == owner else 3
            self.assertAlmostEqual(features[start], 20 / 99, places=6)
            self.assertEqual(features[start + 1], 0)

    def response_fixture(self, env, reaction):
        for seed in range(131072):
            features, _, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if 57 not in ids:
                continue
            owner = actors[0]
            env.step([Action(ActionKind.ATTACK, ids.index(57), target=1 - owner)])
            features = env.observe()[0]
            ids = self.hand_ids(features)
            if reaction in ids:
                return owner, ids.index(reaction)
        self.fail("normal pool did not reach mana weapon response fixture")

    def test_weapon_reactions_keep_mana_metadata_and_magic_reactions_reject(self):
        for reaction in (39, 196, 40, 210):
            with self.subTest(reaction=reaction), self.create(1, 128) as env:
                owner, slot = self.response_fixture(env, reaction)
                action = Action(ActionKind.DEFEND, slot)
                if reaction in (40, 210):
                    self.assertFalse(env.legal([action])[0])
                    continue
                self.assertTrue(env.legal([action])[0])
                env.step([action])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (owner, 1))
                self.assertEqual(features[1], 0)
                self.assertAlmostEqual(features[4], 10 / 99, places=6)
                self.assertEqual(features[730], 1)
                self.assertAlmostEqual(features[731], 10 / 99, places=6)
                self.assertAlmostEqual(features[8], 20 / 480, places=6)

    def test_generated_counter_does_not_inherit_mana_weapon_provenance(self):
        with self.create(1, 128) as env:
            owner, slot = self.response_fixture(env, 190)
            env.step([Action(ActionKind.DEFEND, slots=1 << slot, counter_order=(slot,))])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (owner, 1))
            self.assertEqual(features[417], 1)
            self.assertEqual((features[730], features[731]), (0, 0))
            self.assertAlmostEqual(features[8], 40 / 480, places=6)

    def advance_to(self, env, owner):
        for _ in range(8):
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(phases[0], 0)
            if actors[0] == owner:
                return True
            discard = next((i for i in range(16) if options[4] & (1 << i)), None)
            if discard is not None:
                env.step([Action(ActionKind.DISCARD, slots=1 << discard)])
            elif pray[0]:
                env.step([Action(ActionKind.PRAY)])
            else:
                return False
        return False

    def test_hand_power_tracks_mp_and_zero_mana_attack_still_responds(self):
        with self.create(1, 128) as env:
            for seed in range(131072):
                features, options, actors, _, _ = env.reset(seed)
                ids = self.hand_ids(features)
                if 57 not in ids or 206 not in ids:
                    continue
                owner = actors[0]
                env.step([Action(ActionKind.USE, ids.index(206), target=owner)])
                if not self.advance_to(env, owner):
                    continue
                features = env.observe()[0]
                base = self.hand_ids(features).index(57)
                self.assertAlmostEqual(features[16 + base * 7 + 1], 10 / 30, places=6)
                env.step([Action(ActionKind.CAST, 8, target=owner)])  # Retained206, another5MP.
                if not self.advance_to(env, owner):
                    continue
                features, options, _, _, pray = env.observe()
                base = self.hand_ids(features).index(57)
                self.assertEqual(features[1], 0)
                self.assertEqual(features[16 + base * 7 + 1], 0)
                self.assertTrue(options[0] & (1 << base))
                self.assertFalse(pray[0])
                env.step([Action(ActionKind.ATTACK, base, target=1 - owner)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (1 - owner, 1))
                self.assertEqual((features[730], features[731], features[8]), (1, 0, 0))
                self.assertAlmostEqual(features[0], 40 / 99, places=6)
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertAlmostEqual(features[3 if actors[0] == owner else 0], 40 / 99, places=6)
                break
            else:
                self.fail("normal flow did not reach retained magic/zero-MP weapon")

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
                self.assertEqual(env._lib.gv_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_eighteen_profile_output_boundaries(self):
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
                    (ManaWeaponBatchEnv, "gv", 18, 732, 11))
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
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 16 if prefix in ("gx", "gy", "gz", "gu", "gv") else 6)
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
                native = getattr(env._lib, "gv_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-mana-weapon-subset-v18")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 732)
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
            self.assertEqual(env._lib.gv_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.gv_create(0, 64))
            self.assertFalse(lib.gv_create(2, 0))
            self.assertEqual(lib.gv_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gv_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gv_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gv_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gv_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gv_legal(env._handle, 2, env._actions, env._legal), 0)
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
