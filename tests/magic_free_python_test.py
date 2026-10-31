"""Public ABI23 boundaries and shared attack/response MP waivers.

Usage: python3 tests/magic_free_python_test.py [path/to/libgodfield.dylib]
These verify the bridge contract, not current official God Field fidelity.
"""
import ctypes as c
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from magic_free_env import Action, ActionKind, MagicFreeBatchEnv, _NativeAction
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


class MagicFreeBridgeTests(unittest.TestCase):
    def create(self, count=16, max_decisions=64):
        return MagicFreeBatchEnv(count, max_decisions, library=LIBRARY)

    FREE = (47, 153, 172, 180, 239)

    def fixture(self, env, magic=216, free=None, mana=False, boost=False, two_free=False):
        for seed in range(262144):
            features, options, actors, _, _ = env.reset(seed)
            ids = self.hand_ids(features)
            carriers = [i for i, card in enumerate(ids) if card in self.FREE and (free is None or card == free)]
            if magic not in ids or not carriers or (two_free and len(carriers) < 2):
                continue
            base = next((i for i in range(16) if options[0] & (1 << i) and ids[i] == 57), None)
            extra = next((i for i in range(16) if options[1] & (1 << i)), None)
            if (mana and base is None) or (boost and extra is None):
                continue
            return seed, actors[0], ids.index(magic), carriers, base, extra
        self.fail("normal pool did not reach MAGIC_FREE fixture")

    def advance_to(self, env, owner):
        for _ in range(8):
            _, options, actors, phases, pray = env.observe()
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

    def test_all_five_carriers_free_main_cast_and_are_consumed(self):
        for local, card in enumerate(self.FREE, 1):
            with self.subTest(card=card), self.create(1, 128) as env:
                _, owner, magic, carriers, _, _ = self.fixture(env, free=card)
                free = carriers[0]
                features, options, _, _, _ = env.observe()
                self.assertEqual(features[945], 1)
                self.assertAlmostEqual(features[946 + free], local / 5, places=6)
                self.assertTrue(options[17] & (1 << free))
                action = Action(ActionKind.CAST_ORDERED, magic, target=1 - owner, extras=(free,))
                self.assertTrue(env.legal([action])[0])
                env.step([action])
                features, _, actors, phases, _ = env.observe()
                self.assertEqual((actors[0], phases[0]), (1 - owner, 1))
                self.assertAlmostEqual(features[4], 10 / 99, places=6)
                self.assertAlmostEqual(features[962], card / 363, places=6)
                self.assertEqual(features[974], 1)
                self.assertAlmostEqual(features[975], 1 / 16, places=6)
                self.assertAlmostEqual(features[976], 216 / 363, places=6)
                self.assertAlmostEqual(features[977], card / 363, places=6)
                self.assertAlmostEqual(features[11], 8 / 16, places=6)
                env.step([Action(ActionKind.DEFEND)])
                features, _, actors, _, _ = env.observe()
                self.assertAlmostEqual(features[0 if actors[0] != owner else 3], 50 / 99, places=6)

    def test_unaffordable_spell_multiple_free_and_retained_main_cast(self):
        with self.create(1, 128) as env:
            _, owner, magic, carriers, _, _ = self.fixture(env, magic=226, two_free=True)
            features, options, _, _, _ = env.observe()
            self.assertFalse(options[3] & (1 << magic))
            self.assertTrue(options[18] & (1 << magic))
            env.script_hit_rolls(0, [1])
            env.step([Action(ActionKind.CAST_ORDERED, magic, target=1 - owner, extras=tuple(carriers[:2]))])
            features = env.observe()[0]
            self.assertAlmostEqual(features[4], 10 / 99, places=6)
            self.assertAlmostEqual(features[11], 7 / 16, places=6)
            self.assertAlmostEqual(features[975], 2 / 16, places=6)
        with self.create(1, 128) as env:
            _, owner, magic, _, _, _ = self.fixture(env)
            env.step([Action(ActionKind.USE, magic, target=owner)])
            self.assertTrue(self.advance_to(env, owner))
            features, options, _, _, _ = env.observe()
            free = next(i for i, card in enumerate(self.hand_ids(features)) if card in self.FREE)
            self.assertFalse(options[5] & 1)  # Only4MP remain, while216 normally costs6.
            self.assertTrue(options[19] & 1)
            env.step([Action(ActionKind.CAST_ORDERED, 32, target=1 - owner, extras=(free,))])
            self.assertAlmostEqual(env.observe()[0][4], 4 / 99, places=6)

    def test_free_magic_does_not_reset_mana_weapon_earlier_boost(self):
        with self.create(1, 128) as env:
            _, owner, magic, carriers, base, boost = self.fixture(env, magic=197, mana=True, boost=True)
            free = carriers[0]
            features = env.observe()[0]
            extra = round(features[16 + boost * 7 + 2] * 20)
            carrier = self.hand_ids(features)[free]
            env.step([Action(ActionKind.ATTACK_ORDERED, base, target=1 - owner,
                             extras=(boost, magic, free))])
            features = env.observe()[0]
            self.assertAlmostEqual(features[8], (20 + extra + 2) / 480, places=6)
            self.assertEqual(features[4], 0)
            self.assertAlmostEqual(features[731], 10 / 99, places=6)
            self.assertEqual(features[773], 0)
            self.assertAlmostEqual(features[973], 2 / 99, places=6)
            self.assertAlmostEqual(features[965], carrier / 363, places=6)
            self.assertGreater(features[964], 0)

    def test_free_defensive_magic_keeps_unused_suffix_and_pays_no_mp(self):
        for special in (210, 215):
            with self.subTest(special=special), self.create(1, 128) as env:
                for seed in range(262144):
                    features, options, actors, _, _ = env.reset(seed)
                    ids = self.hand_ids(features)
                    owner = actors[0]
                    if special == 210:
                        if 198 not in ids:
                            continue
                        attack = Action(ActionKind.USE, ids.index(198), target=1 - owner)
                    else:
                        base = next((i for i in range(16) if options[0] & (1 << i)
                                     and ids[i] != 57 and features[285 + i] == 0 and features[186 + i] == 0), None)
                        if base is None:
                            continue
                        attack = Action(ActionKind.ATTACK, base, target=1 - owner)
                    env.step([attack])
                    features, options, _, _, _ = env.observe()
                    ids = self.hand_ids(features)
                    free = next((i for i, card in enumerate(ids) if card in self.FREE), None)
                    if special not in ids or free is None:
                        continue
                    card = ids[free]
                    env.script_bounce_targets(0, [owner])
                    action = Action(ActionKind.DEFEND_ORDERED, extras=(ids.index(special), free))
                    self.assertTrue(env.legal([action])[0])
                    env.step([action])
                    features, _, actors, phases, _ = env.observe()
                    self.assertAlmostEqual(features[963], card / 363, places=6)
                    self.assertAlmostEqual(features[1 if actors[0] == 1 - owner else 4], 10 / 99, places=6)
                    if special == 210:
                        self.assertEqual((actors[0], phases[0]), (owner, 1))
                        self.assertAlmostEqual(features[11], 9 / 16, places=6)
                    else:
                        self.assertEqual(phases[0], 0)
                    break
                else:
                    self.fail("normal pool did not reach special plus unused Free suffix")

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
                self.assertEqual(env._lib.gi_step(env._handle, 2, env._actions, env._rewards, env._terminated, env._truncated), -3)
                self.assertEqual(observation_copy(env), before)

    @staticmethod
    def hand_ids(features):
        return [round(features[16 + i * 7] * 363) for i in range(16)]

    def test_all_twenty_three_profile_output_boundaries(self):
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
                    (MagicFreeBatchEnv, "gi", 23, 993, 23))
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
                self.assertEqual(getattr(env._lib, prefix + "_action_size")(), 88 if prefix in ("gp", "gq", "gt", "gi") else 16 if prefix in ("gx", "gy", "gz", "gu", "gv", "gw") else 6)
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
                native = getattr(env._lib, "gi_script_" + name)
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
            self.assertEqual(env.ruleset, "legacy-magic-free-subset-v23")
            features, options, actors, phases, pray = env.observe()
            self.assertEqual(len(features), 2 * 993)
            self.assertEqual(len(options), 2 * 23)
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
            self.assertEqual(env._lib.gi_step(env._handle, 2, malformed, env._rewards,
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
                              ("kind", "base", "slots", "target", "reserved", "counter_count", "reserved2", "counter_order", "extra_count", "extra_tail", "reserved_extra", "extras")], [0, 1, 2, 4, 5, 6, 7, 8, 16, 17, 18, 24])
            self.assertFalse(lib.gi_create(0, 64))
            self.assertFalse(lib.gi_create(2, 0))
            self.assertEqual(lib.gi_reset_at(env._handle, 2, 42), -2)
            self.assertEqual(lib.gi_reset_at(None, 0, 42), -1)
            self.assertEqual(lib.gi_observe(env._handle, 1, env._obs, env._options,
                                           env._actors, env._phases, env._pray), -2)
            self.assertEqual(lib.gi_observe(env._handle, 2, None, env._options,
                                           env._actors, env._phases, env._pray), -1)
            env.reset(42)
            actions = env.sample_actions()
            env._encode(actions)
            state = observation_copy(env)
            env._actions[1].reserved = 1
            self.assertEqual(lib.gi_step(env._handle, 2, env._actions, env._rewards,
                                        env._terminated, env._truncated), -3)
            self.assertEqual(observation_copy(env), state)
            self.assertEqual(lib.gi_legal(env._handle, 2, env._actions, env._legal), 0)
            self.assertEqual(env._legal[1], 0)
            guarded = (c.c_uint8 * (2 * 88 + 32))(*[165] * (2 * 88 + 32))
            sampled = c.cast(guarded, c.POINTER(_NativeAction))
            self.assertEqual(lib.gi_sample_actions(env._handle, 2, env._policy_seeds, sampled), 0)
            self.assertTrue(all(byte == 165 for byte in guarded[2 * 88:]))
            self.assertEqual(lib.gi_legal(env._handle, 2, sampled, env._legal), 0)
            self.assertTrue(all(env._legal))
            for i in range(2):
                self.assertTrue(all(byte == 0 for byte in sampled[i].reserved_extra))
                self.assertTrue(all(byte == 0 for byte in sampled[i].extras[sampled[i].extra_count:]))
                if sampled[i].extra_count < 65:
                    self.assertEqual(sampled[i].extra_tail, 0)

    def test_python_validation_and_close(self):
        for args in ((14,), (True,), (ActionKind.ATTACK, 256),
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
