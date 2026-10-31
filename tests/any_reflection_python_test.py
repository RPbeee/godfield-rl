"""Public ABI17 boundaries and normal-pool universal reflection.

Usage: python3 tests/any_reflection_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from any_reflection_env import Action, ActionKind, AnyReflectionBatchEnv, _NativeAction
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


class AnyReflectionBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return AnyReflectionBatchEnv(count, max_decisions, library=LIBRARY)

    def counter_fixture(self, env, counter=194, fatal=False, prefix=False):
        for seed in range(131072):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if 196 not in ids:
                continue
            base = next((i for i in range(16) if options[0] & (1 << i)
                         and round(features[285 + i] * 6) == (6 if fatal else 0)
                         and features[186 + i] == 0 and ids[i] not in (30, 34, 43, 49)
                         and 0 < features[16 + i * 7 + 1] * 30 <= 10), None)
            if base is None:
                continue
            if prefix and not any(card == 195 or (features[16 + i * 7 + 3] > 0 and card != 175)
                                  for i, card in enumerate(ids)):
                continue
            owner = actors[0]
            amount = round(features[16 + base * 7 + 1] * 30)
            env.script_hit_rolls(0, [1, 100])
            env.step([Action(ActionKind.ATTACK, base, target=1 - owner)])
            features, options, _, _, _ = env.observe()
            ids = self.hand_ids(features)
            if counter not in ids:
                continue
            slot = ids.index(counter)
            action = Action(ActionKind.DEFEND, slots=1 << slot, counter_order=(slot,))
            if not env.legal([action])[0]:
                continue
            env.step([action])
            features, _, actors, phases, _ = env.observe()
            if phases[0] == 1 and actors[0] == owner:
                return owner, amount, self.hand_ids(features).index(196)
        self.fail("seed search did not reach selected counter plus196")

    def test_money_counter_reflection_changes_beneficiary_and_keeps_origin(self):
        with self.create(1, 128) as env:
            owner, amount, slot = self.counter_fixture(env, prefix=True)
            features, options, _, _, _ = env.observe()
            self.assertEqual(features[696], 1)
            self.assertEqual(features[697 + slot], 1)
            self.assertTrue(options[6] & (1 << slot))
            self.assertEqual(features[411], 0)  # Reservation belongs to the other seat.
            for prefix in range(16):
                if options[2] & (1 << prefix) and prefix != slot:
                    self.assertFalse(env.legal([Action(ActionKind.DEFEND, slot, 1 << prefix)])[0])
            env.step([Action(ActionKind.DEFEND, slot)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (1 - owner, 1))
            self.assertEqual(features[411], 1, "reflection replaced immutable reservation origin")
            self.assertEqual(features[678], 1)
            self.assertAlmostEqual(features[8], amount / 480, places=6)
            env.step([Action(ActionKind.DEFEND)])
            features, _, actors, _, _ = env.observe()
            own = actors[0] == owner
            self.assertAlmostEqual(features[2 if own else 5], (20 + amount) / 99, places=6)
            self.assertAlmostEqual(features[5 if own else 2], (20 - amount) / 99, places=6)
            self.assertAlmostEqual(features[3 if own else 0], (40 - amount) / 99, places=6)

    def test_counter_returned_to_dead_source_has_no_effect(self):
        with self.create(1, 128) as env:
            owner, _, slot = self.counter_fixture(env, fatal=True)
            features = env.observe()[0]
            self.assertEqual((features[412], features[413]), (0, 1))
            rewards, terminal, _ = env.step([Action(ActionKind.DEFEND, slot)])
            self.assertTrue(terminal[0])
            features, _, actors, _, _ = env.observe()
            self.assertAlmostEqual(features[2], 20 / 99, places=6)
            self.assertAlmostEqual(features[5], 20 / 99, places=6)
            self.assertEqual(rewards[owner], 1)
            self.assertEqual(rewards[1 - owner], -1)

    def test_percent_and_stone_counters_reflect_without_relaunch(self):
        for counter, multiplier in ((187, 1), (190, 2)):
            with self.subTest(counter=counter), self.create(1, 128) as env:
                owner, amount, slot = self.counter_fixture(env, counter)
                before = tuple(env.observe()[0])
                env.step([Action(ActionKind.DEFEND, slot)])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (1 - owner, 1))
                self.assertEqual(features[411], 1)
                self.assertEqual(features[417], 1)
                self.assertAlmostEqual(features[8], amount * multiplier / 480, places=6)
                if counter == 187:
                    self.assertEqual((features[202], features[206], features[207]),
                                     (before[202], before[206], before[207]))
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                target = 1 - owner
                self.assertAlmostEqual(features[0 if actors[0] == target else 3],
                                       (40 - amount * (1 + multiplier)) / 99, places=6)

    def magic_fixture(self, env, miracle=207, removal=False):
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
            if phases[0] != 1 or 196 not in ids or (removal and 195 not in ids):
                continue
            armor = next((i for i, card in enumerate(ids) if options[2] & (1 << i)
                          and features[16 + i * 7 + 3] > 0 and card != 175), None)
            if armor is not None:
                return owner, ids.index(196), armor
        self.fail("seed search did not reach MAGIC plus196/prefix")

    def test_dark_magic_allows_prefix_and_keeps_raw_payload(self):
        with self.create(1, 128) as env:
            owner, slot, armor = self.magic_fixture(env)
            before = tuple(env.observe()[0])
            env.step([Action(ActionKind.DEFEND, slot, 1 << armor)])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual((actors[0], phases[0]), (owner, 1))
            self.assertEqual(features[8], before[8])
            self.assertEqual(features[284], 1)  # DARK enum6 / 6.
            self.assertAlmostEqual(features[303], 207 / 363, places=6)
            self.assertAlmostEqual(features[4], 10 / 99, places=6)
            _, terminal, _ = env.step([Action(ActionKind.DEFEND)])
            self.assertTrue(terminal[0])
            self.assertEqual(env._rewards[owner], -1)

    def test_colored_weapon_prefix_requires_actual_removal195(self):
        with self.create(1, 128) as env:
            for seed in range(131072):
                features, options, actors, _, _ = env.reset(seed)
                base = next((i for i in range(16) if options[0] & (1 << i)
                             and features[285 + i] > 0 and features[186 + i] == 0
                             and 0 < features[16 + i * 7 + 1] * 30 <= 10), None)
                if base is None:
                    continue
                owner = actors[0]
                env.step([Action(ActionKind.ATTACK, base, target=1 - owner)])
                features, options, _, _, _ = env.observe()
                ids = self.hand_ids(features)
                if 196 not in ids or 195 not in ids:
                    continue
                slot = ids.index(196)
                removal = ids.index(195)
                armor = next((i for i, card in enumerate(ids) if card not in (175, 195, 196)
                              and features[16 + i * 7 + 3] > 0), None)
                if armor is None:
                    continue
                self.assertTrue(env.legal([Action(ActionKind.DEFEND, slot)])[0])
                self.assertFalse(env.legal([Action(ActionKind.DEFEND, slot, 1 << armor)])[0])
                action = Action(ActionKind.DEFEND, slot, 1 << removal)
                self.assertTrue(env.legal([action])[0])
                amount = features[8]
                env.step([action])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (owner, 1))
                self.assertEqual((features[284], features[392]), (0, 1))
                self.assertEqual(features[8], amount)
                break
            else:
                self.fail("normal pool did not reach weapon/195/196/prefix fixture")

    def test_non_magic_heal_mana_and_cure_are_reflected(self):
        for card, hp, mp in ((227, 45, 10), (231, 40, 15), (235, 40, 10)):
            with self.subTest(card=card), self.create(1, 128) as env:
                for seed in range(131072):
                    features, options, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    if card not in ids or not options[3] & (1 << ids.index(card)):
                        continue
                    owner = actors[0]
                    action = Action(ActionKind.USE, ids.index(card), target=1 - owner)
                    if not env.legal([action])[0]:
                        continue
                    env.step([action])
                    features, _, _, phases, _ = env.observe()
                    ids = self.hand_ids(features)
                    if phases[0] != 1 or 196 not in ids:
                        continue
                    env.step([Action(ActionKind.DEFEND, ids.index(196))])
                    features, _, actors, phases, _ = env.observe()
                    self.assertEqual((actors[0], phases[0]), (owner, 1))
                    self.assertEqual(features[302], 0, "SUNDRY became a retained miracle")
                    env.step([Action(ActionKind.DEFEND)])
                    features, _, actors, _, _ = env.observe()
                    start = 0 if actors[0] == owner else 3
                    self.assertAlmostEqual(features[start], hp / 99, places=6)
                    self.assertAlmostEqual(features[start + 1], mp / 99, places=6)
                    break
                else:
                    self.fail("normal pool did not reach SUNDRY plus196 fixture")

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
                self.assertEqual(env._lib.gu_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_seventeen_profile_output_boundaries(self):
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
                    (AnyReflectionBatchEnv, "gu", 17, 713, 11))
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
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 16 if prefix in ("gx", "gy", "gz", "gu") else 6)
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
                native = getattr(env._lib, "gu_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-any-reflection-subset-v17")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 713)
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
            self.assertEqual(env._lib.gu_step(env._handle, 2, malformed, env._rewards,
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
            self.assertFalse(lib.gu_create(0, 64))
            self.assertFalse(lib.gu_create(2, 0))
            self.assertEqual(lib.gu_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gu_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gu_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gu_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gu_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gu_legal(env._handle, 2, env._actions, env._legal), 0)
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
