"""Public ABI20 boundaries, ordered extra miracles and88-byte actions.

Usage: python3 tests/additive_miracles_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from additive_miracles_env import Action, ActionKind, AdditiveMiraclesBatchEnv, _NativeAction
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


class AdditiveMiraclesBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return AdditiveMiraclesBatchEnv(count, max_decisions, library=LIBRARY)

    def fixture(self, env, magic=197, base_id=None, need_boost=False):
        for seed in range(131072):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            if magic not in ids:
                continue
            base = next((i for i, card in enumerate(ids) if options[0] & (1 << i)
                         and features[186 + i] == 0 and features[285 + i] == 0
                         and (card == base_id if base_id is not None else card != 57)
                         and 0 < round(features[16 + i * 7 + 1] * 30) <= 20), None)
            if base is None:
                continue
            boost = next((i for i in range(16) if i != base and options[1] & (1 << i)), None)
            if need_boost and boost is None:
                continue
            return seed, actors[0], base, ids.index(magic), boost
        self.fail("normal pool did not reach additive miracle/weapon fixture")

    def advance_to(self, env, owner):
        for _ in range(8):
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(phases[0], 0)
            if actors[0] == owner:
                return True
            slot = next((i for i in range(16) if options[4] & (1 << i)), None)
            if slot is not None:
                env.step([Action(ActionKind.DISCARD, slots=1 << slot)])
            elif pray[0]:
                env.step([Action(ActionKind.PRAY)])
            else:
                return False
        return False

    def test_single_magic_and_new_cast_have_separate_identity(self):
        for card, value, cost, local in ((197, 2, 2, 0), (217, 10, 7, 1)):
            with self.subTest(card=card), self.create(1, 128) as env:
                _, owner, _, slot, _ = self.fixture(env, magic=card)
                features, options, _, _, _ = env.observe()
                self.assertEqual(features[750], 1)
                self.assertAlmostEqual(features[751 + slot], (local + 1) / 2)
                self.assertTrue(options[11] & (1 << slot))
                self.assertFalse(env.legal([Action(ActionKind.ATTACK_ORDERED, slot,
                                                 target=1 - owner, extras=(slot,))])[0])
                env.step([Action(ActionKind.USE, slot, target=1 - owner)])
                features, _, _, phases, _ = env.observe()
                self.assertEqual(phases[0], 1)
                self.assertAlmostEqual(features[4], (10 - cost) / 99, places=6)
                self.assertAlmostEqual(features[8], value / 480, places=6)
                self.assertAlmostEqual(features[774], (local + 1) / 2)
                self.assertEqual(features[769], 0)
                self.assertAlmostEqual(features[303], card / 363, places=6)
                env.step([Action(ActionKind.DEFEND)])
                if card == 197:
                    self.assertTrue(self.advance_to(env, owner))
                    features, options, _, _, _ = env.observe()
                    self.assertTrue(options[12] & 1)
                    self.assertAlmostEqual(features[767], 1 / (15 * 128), places=7)
                    env.step([Action(ActionKind.CAST_ADDITIVE, 0, target=owner)])
                    features, _, actors, _, _ = env.observe()
                    start = 0 if actors[0] == owner else 3
                    self.assertAlmostEqual(features[start + 1], 6 / 99, places=6)

    def test_same_action_acquisition_repeated_retained_refs_and_gifts(self):
        with self.create(1, 128) as env:
            _, owner, base, hand, _ = self.fixture(env)
            features, options, _, _, _ = env.observe()
            value = round(features[16 + base * 7 + 1] * 30)
            self.assertFalse(options[12] & 1)
            before = observation_copy(env)
            invalid = (Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(16, hand)),
                       Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(hand, hand)),
                       Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(base, hand)),
                       Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(hand,) + (16,) * 5))
            for action in invalid:
                self.assertFalse(env.legal([action])[0])
                with self.assertRaises(ValueError):
                    env.step([action])
                self.assertEqual(observation_copy(env), before)
            action = Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(hand, 16, 16))
            self.assertTrue(env.legal([action])[0])
            env.step([action])
            features, _, _, phases, _ = env.observe()
            self.assertEqual(phases[0], 1)
            self.assertAlmostEqual(features[4], 4 / 99, places=6)
            self.assertEqual(features[769], 1)
            self.assertAlmostEqual(features[770], self.hand_ids(before[0])[base] / 363, places=6)
            self.assertAlmostEqual(features[771], 3 / 64, places=6)
            self.assertAlmostEqual(features[772], 10 / 99, places=6)
            self.assertAlmostEqual(features[773], 6 / 99, places=6)
            self.assertAlmostEqual(features[8], (value + 6) / 480, places=6)
            for i, held in enumerate((0, 1, 1)):
                self.assertAlmostEqual(features[775 + 2 * i], 197 / 363, places=6)
                self.assertEqual(features[776 + 2 * i], held)
            self.assertTrue(all(x == 0 for x in features[781:903]))
            env.step([Action(ActionKind.DEFEND)])
            self.assertTrue(self.advance_to(env, owner))
            features, options, _, _, _ = env.observe()
            self.assertAlmostEqual(features[767], 1 / (15 * 128), places=7)
            self.assertTrue(options[12] & 1)
            self.assertEqual(sum(card != 0 for card in self.hand_ids(features)), 12)

    def test_mana_weapon_order_resets_earlier_boost_and_preserves_split_cost(self):
        with self.create(1, 128) as left, self.create(1, 128) as right:
            seed, owner, base, hand, boost = self.fixture(left, base_id=57, need_boost=True)
            right.reset(seed)
            features = left.observe()[0]
            power = round(features[16 + boost * 7 + 2] * 20)
            left.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(boost, hand))])
            right.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner, extras=(hand, boost))])
            for env, amount in ((left, 18), (right, 18 + power)):
                features = env.observe()[0]
                self.assertAlmostEqual(features[8], amount / 480, places=6)
                self.assertEqual(features[4], 0)
                self.assertEqual(features[730], 1)
                self.assertAlmostEqual(features[731], 8 / 99, places=6)
                self.assertAlmostEqual(features[773], 2 / 99, places=6)

    def test_ordered_weapon_keeps_weapon_reactions_and_trace_after_reflection(self):
        for reaction in (39, 40, 210, 152):
            with self.subTest(reaction=reaction), self.create(1, 128) as env:
                for seed in range(131072):
                    features, options, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    if 197 not in ids:
                        continue
                    base = next((i for i in range(16) if options[0] & (1 << i)
                                 and ids[i] != 57 and features[285 + i] == 0
                                 and features[186 + i] == 0), None)
                    if base is None:
                        continue
                    owner = actors[0]
                    env.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner,
                                     extras=(ids.index(197),))])
                    before = tuple(env.observe()[0])
                    ids = self.hand_ids(before)
                    if reaction not in ids:
                        continue
                    slot = ids.index(reaction)
                    action = Action(ActionKind.DEFEND, slot)
                    self.assertEqual(env.legal([action])[0], reaction == 39)
                    self.assertEqual(before[302], 0)
                    if reaction == 39:
                        env.step([action])
                        features, _, actors, phases, _ = env.observe()
                        self.assertEqual((actors[0], phases[0]), (owner, 1))
                        self.assertEqual(features[8], before[8])
                        self.assertEqual(tuple(features[769:903]), before[769:903])
                    break
                else:
                    self.fail("normal pool did not reach ordered weapon/reaction fixture")

    def test_88_byte_reserved_tail_count_and_typed_reference_validation(self):
        for extras in ((-1,), (18,), (True,), tuple([16] * 65)):
            with self.assertRaises(ValueError):
                Action(ActionKind.ATTACK_ORDERED, 0, target=1, extras=extras)
        with self.create(2) as env:
            env.reset(42)
            actions = env.sample_actions()
            before = observation_copy(env)
            for mutate in (lambda row: setattr(row, 'extra_count', 65),
                           lambda row: row.reserved_extra.__setitem__(6, 1),
                           lambda row: row.extras.__setitem__(63, 17)):
                env._encode(actions)
                mutate(env._actions[1])
                self.assertEqual(env._lib.gp_step(env._handle, 2, env._actions, env._rewards,
                                                env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

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
                self.assertEqual(env._lib.gp_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_twenty_profile_output_boundaries(self):
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
                    (AdditiveMiraclesBatchEnv, "gp", 20, 903, 13))
        for cls, prefix, version, dimension, width in profiles:
            with self.subTest(profile=prefix), cls(2, 64, library=LIBRARY) as env:
                self.assertEqual((env.observation_size, env.options_width), (dimension, width))
                self.assertEqual(getattr(env._lib, prefix + "_abi_version")(), version)
                env.reset(42)
                features = (c.c_float * (2 * dimension + 32))(*[12345.0] * (2 * dimension + 32))
                options = (c.c_uint16 * 32)(*[54321] * 32)
                status = getattr(env._lib, prefix + "_observe")(
                    env._handle, 2, features, options, env._actors, env._phases, env._pray)
                self.assertEqual(status, 0)
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 88 if prefix == "gp" else 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
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
                native = getattr(env._lib, "gp_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-additive-miracles-subset-v20")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 903)
            self.assertEqual(len(options), 2 * 13)
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
            self.assertEqual(env._lib.gp_step(env._handle, 2, malformed, env._rewards,
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
            self.assertEqual(c.sizeof(_NativeAction), 88)
            self.assertEqual([getattr(_NativeAction, field).offset for field in
                              ("kind", "base", "slots", "target", "reserved", "counter_count", "reserved2", "counter_order", "extra_count", "reserved_extra", "extras")], [0, 1, 2, 4, 5, 6, 7, 8, 16, 17, 24])
            self.assertFalse(lib.gp_create(0, 64))
            self.assertFalse(lib.gp_create(2, 0))
            self.assertEqual(lib.gp_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gp_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gp_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gp_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gp_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gp_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)
            guarded = (c.c_uint8 * (2 * 88 + 32))(*[165] * (2 * 88 + 32))
            sampled = c.cast(guarded, c.POINTER(_NativeAction))
            self.assertEqual(lib.gp_sample_actions(env._handle, 2, env._policy_seeds, sampled), 0)
            self.assertTrue(all(byte == 165 for byte in guarded[2 * 88:]))
            self.assertEqual(lib.gp_legal(env._handle, 2, sampled, env._legal), 0)
            self.assertTrue(all(env._legal))
            for i in range(2):
                self.assertTrue(all(byte == 0 for byte in sampled[i].reserved_extra))
                self.assertTrue(all(byte == 0 for byte in sampled[i].extras[sampled[i].extra_count:]))

    def test_python_validation_and_close(self):
        for args in ((12,), (True,), (ActionKind.ATTACK, 256),
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
