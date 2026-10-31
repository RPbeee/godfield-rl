"""Public ABI26 boundaries and passive revival without replacement gifts.

Usage: python3 tests/revival_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from revival_env import Action, ActionKind, RevivalBatchEnv, _NativeAction
from exchange_env import ExchangeBatchEnv
from forgetting_env import ForgettingBatchEnv
from magic_free_env import MagicFreeBatchEnv
from wide_miracles_env import WideMiraclesBatchEnv
from doubling_miracles_env import DoublingMiraclesBatchEnv
from additive_miracles_env import AdditiveMiraclesBatchEnv
from magic_block_env import MagicBlockBatchEnv
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


class RevivalBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return RevivalBatchEnv(count, max_decisions, library=LIBRARY)

    def fixture(self, env):
        for seed in range(131072):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if 2 in ids and ids.count(244) == 1:
                return seed, actors[0], ids.index(2), ids.index(244)
        self.fail("normal pool did not reach exchange2 plus one244")

    def advance_to(self, env, owner):
        for _ in range(32):
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(phases[0], 0)
            if actors[0] == owner:
                return
            slot = next((i for i in range(16) if options[4] & (1 << i)), None)
            if slot is not None:
                env.step([Action(ActionKind.DISCARD, slots=1 << slot)])
            else:
                self.assertTrue(pray[0])
                env.step([Action(ActionKind.PRAY)])
        self.fail("owner did not regain a main turn")

    def test_passive_revival_after_zero_hp_exchange_earns_no_gift(self):
        with self.create(1, 128) as env:
            _, owner, trade, revival = self.fixture(env)
            features = env.observe()[0]
            self.assertEqual(features[1043], 1)
            self.assertEqual(features[1044 + revival], 1)
            env.step([Action(ActionKind.EXCHANGE, trade, extras=(0, 0, 70))])
            features, _, actors, phases, _ = env.observe()
            self.assertEqual(phases[0], 0)
            self.assertFalse(env._terminated[0])
            offset = 0 if actors[0] == owner else 3
            self.assertAlmostEqual(features[offset], 10 / 99, places=6)
            self.assertEqual(features[offset + 1], 0)
            self.assertAlmostEqual(features[offset + 2], 70 / 99, places=6)
            self.assertEqual(features[1060], 1)
            self.assertEqual(features[1061], int(actors[0] == owner))
            self.assertEqual(features[1039], 0)  # Historical allocation precedes revival.
            self.assertAlmostEqual(self.hand_fraction(features, actors[0], owner), 9 / 16, places=6)

    def test_manual_discard_removes_protection(self):
        with self.create(1, 128) as env:
            _, owner, _, revival = self.fixture(env)
            action = Action(ActionKind.DISCARD, slots=1 << revival)
            self.assertTrue(env.legal([action])[0])
            env.step([action])
            self.advance_to(env, owner)
            features = env.observe()[0]
            ids = self.hand_ids(features)
            self.assertNotIn(244, ids)
            self.assertEqual(features[1060], 0)
            env.step([Action(ActionKind.EXCHANGE, ids.index(2), extras=(0, 70, 0))])
            self.assertTrue(env._terminated[0])
            self.assertEqual(env.observe()[0][1060], 0)

    def test_revival_is_not_an_active_spell_or_attack_modifier(self):
        with self.create(1, 128) as env:
            _, owner, _, revival = self.fixture(env)
            features, options, _, _, _ = env.observe()
            self.assertTrue(options[4] & (1 << revival))
            self.assertFalse(options[0] & (1 << revival))
            self.assertFalse(options[3] & (1 << revival))
            for action in (Action(ActionKind.USE, revival, target=owner),
                           Action(ActionKind.ATTACK, revival, target=1 - owner),
                           Action(ActionKind.CAST_ORDERED, revival, target=owner, extras=(revival,))):
                self.assertFalse(env.legal([action])[0])
            base = next((i for i in range(16) if options[0] & (1 << i)), None)
            if base is not None:
                self.assertFalse(env.legal([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(revival,))])[0])

    def test_public_revival_event_persists_and_is_actor_relative(self):
        with self.create(1, 128) as env:
            _, owner, trade, _ = self.fixture(env)
            env.step([Action(ActionKind.EXCHANGE, trade, extras=(0, 70, 0))])
            self.advance_to(env, owner)
            features, options, actors, _, _ = env.observe()
            self.assertEqual(actors[0], owner)
            self.assertEqual((features[1060], features[1061]), (1, 1))
            self.assertAlmostEqual(features[0], 10 / 99, places=6)
            slot = next((i for i in range(16) if options[4] & (1 << i)), None)
            if slot is not None:
                env.step([Action(ActionKind.DISCARD, slots=1 << slot)])
            elif options[0]:
                base = next(i for i in range(16) if options[0] & (1 << i))
                env.step([Action(ActionKind.ATTACK, base, target=1 - owner)])
            else:
                env.step([Action(ActionKind.PRAY)])
            features, _, actors, _, _ = env.observe()
            self.assertEqual(features[1060], 1)
            self.assertEqual(features[1061], int(actors[0] == owner))

    def test_forget_ticket_validation_and_hidden_values(self):
        with self.create(1) as env:
            env.reset(42)
            before = observation_copy(env)
            env.script_forget_tickets(0, [0, 2**64 - 1])
            self.assertEqual(observation_copy(env), before)
            for values in ((-1,), (2**64,), (True,), (0.5,), (0,) * 65):
                with self.assertRaises(ValueError):
                    env.script_forget_tickets(0, values)
            native = env._lib.g26_script_forget_tickets
            self.assertEqual(native(None, 0, None, 0), -1)
            self.assertEqual(native(env._handle, 1, None, 0), -2)
            self.assertEqual(native(env._handle, 0, None, 1), -1)
            self.assertEqual(native(env._handle, 0, None, 65), -2)
            self.assertEqual(observation_copy(env), before)
            env.script_forget_tickets(0, [])
            self.assertEqual(observation_copy(env), before)

    def test_tail_layout_and_old_api_rejects_65th_reference(self):
        action = Action(ActionKind.ATTACK_ORDERED, 0, target=1, extras=(16,) * 64 + (19,))
        with self.create(1) as env:
            env._encode([action])
            self.assertEqual(env._actions[0].extra_count, 65)
            self.assertEqual(env._actions[0].extra_tail, 19)
            self.assertEqual(tuple(env._actions[0].extras), (16,) * 64)
        from wide_miracles_env import Action as OldAction, ActionKind as OldKind
        with self.assertRaises(ValueError):
            OldAction(OldKind.ATTACK_ORDERED, 0, target=1, extras=(16,) * 65)
        for extras in ((22,), (True,), (16,) * 66):
            with self.assertRaises(ValueError):
                Action(ActionKind.ATTACK_ORDERED, 0, target=1, extras=extras)

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
                self.assertEqual(env._lib.g26_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    def hand_fraction(self, features, actor, seat):
        # Feature10 identifies the turn owner; only feature11 is a hand count.
        return sum(card != 0 for card in self.hand_ids(features)) / 16 if actor == seat else features[11]

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_twenty_six_profile_output_boundaries(self):
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
                    (MagicBlockBatchEnv, "gw", 19, 750, 11),
                    (AdditiveMiraclesBatchEnv, "gp", 20, 903, 13),
                    (DoublingMiraclesBatchEnv, "gq", 21, 924, 15),
                    (WideMiraclesBatchEnv, "gt", 22, 945, 17),
                    (MagicFreeBatchEnv, "gi", 23, 993, 23),
                    (ForgettingBatchEnv, "gj", 24, 1017, 24),
                    (ExchangeBatchEnv, "gg", 25, 1043, 25),
                    (RevivalBatchEnv, "g26", 26, 1062, 25))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * (2 * dimension + 32))(*[12345.0] * (2 * dimension + 32))
                options = (c.c_uint16 * (2 * width + 16))(*[54321] * (2 * width + 16))
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 88 if prefix in ("gp", "gq", "gt", "gi", "gj", "gg", "g26") else 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
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
                native = getattr(env._lib, "g26_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-revival-subset-v26")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 1062)
            self.assertEqual(len(options), 2 * 25)
            self.assertEqual((len(actors), len(phases), len(pray)), (2, 2, 2))
            _, terminal, truncated = env.random_step()
            end_terminated, end_truncated = tuple(terminal), tuple(truncated)
            self.assertTrue(all(t or x for t, x in zip(terminal, truncated)))
            state = observation_copy(env)
            policy_rng = tuple(env._policy_seeds)
            actions = env.sample_actions()
            self.assertEqual(tuple(env._policy_seeds), policy_rng, "done sampling advanced RNG")
            self.assertFalse(any(env.legal(actions)))
            # The native API must ignore even malformed actions for frozen rows.
            malformed = (_NativeAction * 2)(*[_NativeAction(255, 255, 65535, 255, 255)] * 2)
            self.assertEqual(env._lib.g26_step(env._handle, 2, malformed, env._rewards,
                                             env._terminated, env._truncated), 0)
            self.assertFalse(any(env._rewards))
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(tuple(env._terminated), end_terminated)
            self.assertEqual(tuple(env._truncated), end_truncated)
            env.reset_at(1, 9)
            actions = env.sample_actions()
            self.assertEqual(env.legal(actions)[0], False)
            self.assertEqual(env.legal(actions)[1], True)
            env.step(actions)

    def test_native_layout_and_errors(self):
        with self.create(2) as env:
            lib = env._lib
            self.assertEqual(c.sizeof(_NativeAction), 88)
            self.assertEqual([getattr(_NativeAction, field).offset for field in
                              ("kind", "base", "slots", "target", "reserved", "counter_count", "reserved2", "counter_order", "extra_count", "extra_tail", "reserved_extra", "extras")], [0, 1, 2, 4, 5, 6, 7, 8, 16, 17, 18, 24])
            self.assertFalse(lib.g26_create(0, 64))
            self.assertFalse(lib.g26_create(2, 0))
            self.assertEqual(lib.g26_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.g26_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.g26_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.g26_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.g26_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.g26_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)
            guarded = (c.c_uint8 * (2 * 88 + 32))(*[165] * (2 * 88 + 32))
            sampled = c.cast(guarded, c.POINTER(_NativeAction))
            self.assertEqual(lib.g26_sample_actions(env._handle, 2, env._policy_seeds, sampled), 0)
            self.assertTrue(all(byte == 165 for byte in guarded[2 * 88:]))
            self.assertEqual(lib.g26_legal(env._handle, 2, sampled, env._legal), 0)
            self.assertTrue(all(env._legal))
            for i in range(2):
                self.assertTrue(all(byte == 0 for byte in sampled[i].reserved_extra))
                self.assertTrue(all(byte == 0 for byte in sampled[i].extras[sampled[i].extra_count:]))
                if sampled[i].extra_count < 65:
                    self.assertEqual(sampled[i].extra_tail, 0)

    def test_python_validation_and_close(self):
        for args in ((15,), (True,), (ActionKind.ATTACK, 256),
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
