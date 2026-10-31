"""ctypes bridge for legacy-attribute-removal-subset-v12, separate from the synthetic API.

Observation buffers are reused: copy them before storing in replay memory.
DEFEND uses base=the special reaction slot (255 for ordinary), slots=preceding armor.
CAST uses base=the retained main miracle index (0..14), slots=0, target=seat.
DEFEND_CAST uses local defensive index (0=210, 1=215), prefix slots and target=255.
ATTACK_SET_* chooses a reachable final attribute with all extra boost/setter slots.
This represents equivalent attack orders; it does not preserve an XML selection order.
Removal195 is only in DEFEND/DEFEND_CAST prefix slots, before any special base.
Conditional mask components must be validated together with legal().
Options are component masks, not complete legal-action masks. Validate composed
actions with legal(). Rewards are [fixed player 0, fixed player 1] for each row.
Light/Darkness use the new suffix features, preserving older feature layouts.
Glory and DarkCloud are public state flags; hidden outcome prefixes are never observed.
This is an old-version subset profile, not current official God Field fidelity.
"""
from __future__ import annotations

import ctypes as c
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
import sys


class ActionKind(IntEnum):
    PRAY = 0
    ATTACK = 1
    DEFEND = 2
    USE = 3
    DISCARD = 4
    CAST = 5
    DEFEND_CAST = 6
    ATTACK_SET_FIRE = 7
    ATTACK_SET_WATER = 8
    ATTACK_SET_NEUTRAL = 9


@dataclass(frozen=True)
class Action:
    kind: ActionKind
    base: int = 255
    slots: int = 0
    target: int = 255

    def __post_init__(self):
        for name, value, limit in (("kind", self.kind, 9), ("base", self.base, 255),
                                   ("slots", self.slots, 65535), ("target", self.target, 255)):
            if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= limit:
                raise ValueError(f"{name} must be an integer in [0,{limit}]")


class _NativeAction(c.Structure):
    _fields_ = [("kind", c.c_uint8), ("base", c.c_uint8), ("slots", c.c_uint16),
                ("target", c.c_uint8), ("reserved", c.c_uint8)]


class AttributeRemovalBatchEnv:
    def __init__(self, count: int = 64, max_decisions: int = 512,
                 library: str | None = None):
        self._handle = None
        if type(count) is not int or not 1 <= count <= c.c_size_t(-1).value:
            raise ValueError("count must be a positive size_t integer")
        if type(max_decisions) is not int or not 1 <= max_decisions <= 0xFFFFFFFF:
            raise ValueError("max_decisions must be in [1,2**32-1]")
        ext = "dylib" if sys.platform == "darwin" else "so"
        path = Path(library) if library else Path(__file__).resolve().parents[1] / "build" / f"libgodfield.{ext}"
        lib = self._lib = c.CDLL(str(path))
        for name in ("gn_abi_version", "gn_observation_size", "gn_options_width", "gn_hand_capacity"):
            func = getattr(lib, name)
            func.argtypes = []
            func.restype = c.c_int
        lib.gn_action_size.argtypes = []
        lib.gn_action_size.restype = c.c_size_t
        lib.gn_ruleset.argtypes = []
        lib.gn_ruleset.restype = c.c_char_p
        if (lib.gn_abi_version() != 12 or lib.gn_action_size() != c.sizeof(_NativeAction)
                or lib.gn_observation_size() != 393 or lib.gn_options_width() != 10
                or lib.gn_hand_capacity() != 16):
            raise RuntimeError("unsupported legacy native ABI or dimensions")
        lib.gn_create.argtypes = [c.c_size_t, c.c_uint32]
        lib.gn_create.restype = c.c_void_p
        lib.gn_destroy.argtypes = [c.c_void_p]
        lib.gn_destroy.restype = None
        lib.gn_reset_at.argtypes = [c.c_void_p, c.c_size_t, c.c_uint64]
        lib.gn_reset_at.restype = c.c_int
        lib.gn_script_hit_rolls.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.gn_script_hit_rolls.restype = c.c_int
        lib.gn_script_bounce_targets.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.gn_script_bounce_targets.restype = c.c_int
        lib.gn_script_disease_rolls.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.gn_script_disease_rolls.restype = c.c_int
        lib.gn_observe.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_float),
                                  c.POINTER(c.c_uint16), c.POINTER(c.c_uint8),
                                  c.POINTER(c.c_uint8), c.POINTER(c.c_uint8)]
        lib.gn_observe.restype = c.c_int
        lib.gn_legal.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(_NativeAction), c.POINTER(c.c_uint8)]
        lib.gn_legal.restype = c.c_int
        lib.gn_sample_actions.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint64), c.POINTER(_NativeAction)]
        lib.gn_sample_actions.restype = c.c_int
        lib.gn_step.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(_NativeAction),
                               c.POINTER(c.c_float), c.POINTER(c.c_uint8), c.POINTER(c.c_uint8)]
        lib.gn_step.restype = c.c_int
        self.count = count
        self.observation_size = lib.gn_observation_size()
        self.options_width = lib.gn_options_width()
        self.hand_capacity = lib.gn_hand_capacity()
        self.ruleset = lib.gn_ruleset().decode("ascii")
        self._obs = (c.c_float * (count * self.observation_size))()
        self._options = (c.c_uint16 * (count * self.options_width))()
        self._actors = (c.c_uint8 * count)()
        self._phases = (c.c_uint8 * count)()
        self._pray = (c.c_uint8 * count)()
        self._actions = (_NativeAction * count)()
        self._legal = (c.c_uint8 * count)()
        self._rewards = (c.c_float * (count * 2))()
        self._terminated = (c.c_uint8 * count)()
        self._truncated = (c.c_uint8 * count)()
        self._policy_seeds = (c.c_uint64 * count)(*range(count))
        self._handle = lib.gn_create(count, max_decisions)
        if not self._handle:
            raise MemoryError("native batch allocation failed")

    @staticmethod
    def _check(status):
        if status:
            raise ValueError(f"native legacy environment error {status}")

    def _open(self):
        if not self._handle:
            raise RuntimeError("environment is closed")

    def reset_at(self, index: int, seed: int):
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        if type(seed) is not int or not 0 <= seed <= 0xFFFFFFFFFFFFFFFF:
            raise ValueError("seed must be uint64")
        self._check(self._lib.gn_reset_at(self._handle, index, seed))
        self._policy_seeds[index] = seed ^ 0xA0761D6478BD642F

    def reset(self, seed: int = 0):
        self._open()
        if type(seed) is not int or not 0 <= seed <= 0xFFFFFFFFFFFFFFFF - self.count + 1:
            raise ValueError("seed range overflow")
        for i in range(self.count):
            self.reset_at(i, seed + i)
        return self.observe()

    def script_hit_rolls(self, index: int, rolls):
        """Fixture-only prefix of at most 64 integer outcomes in [1,100].

        Empty input clears the prefix. This controls future draws, so never give
        the prefix to a learning policy or treat it as observed game information.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(rolls)
        if len(values) > 64:
            raise ValueError("at most 64 scripted hit outcomes are supported")
        if any(type(value) is not int or not 1 <= value <= 100 for value in values):
            raise ValueError("hit outcomes must be integers in [1,100]")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.gn_script_hit_rolls(self._handle, index, native, len(values)))

    def observe(self):
        """(features[n,393], options[n,10], actor[n], phase[n], pray[n]).

        All are flat reusable ctypes buffers. phase: 0 main, 1 response.
        Options order: weapon bases, boosts, defenses, utilities, discards, retained main abilities, hand reactions, retained defensive reactions, setter hand slots, removal hand slots.
        """
        self._open()
        self._check(self._lib.gn_observe(self._handle, self.count, self._obs,
                                        self._options, self._actors, self._phases, self._pray))
        return self._obs, self._options, self._actors, self._phases, self._pray

    def script_bounce_targets(self, index: int, targets):
        """Fixture-only prefix of at most 64 fixed seat targets (0 or 1).

        Empty input clears the prefix; future targets are hidden from the policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(targets)
        if len(values) > 64:
            raise ValueError("at most 64 scripted bounce targets are supported")
        if any(type(value) is not int or value not in (0, 1) for value in values):
            raise ValueError("bounce targets must be integer seats 0 or 1")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.gn_script_bounce_targets(self._handle, index, native, len(values)))

    def script_disease_rolls(self, index: int, rolls):
        """Fixture-only prefix of up to 64 legacy disease outcomes in [0,99].

        An empty prefix clears it. Unlike hit rolls, zero is a valid outcome.
        Future outcomes and the internal worsening counter are hidden from policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(rolls)
        if len(values) > 64:
            raise ValueError("at most 64 scripted disease outcomes are supported")
        if any(type(value) is not int or not 0 <= value <= 99 for value in values):
            raise ValueError("disease outcomes must be integers in [0,99]")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.gn_script_disease_rolls(self._handle, index, native, len(values)))

    def _encode(self, actions):
        if len(actions) != self.count:
            raise ValueError("expected one semantic Action per environment")
        for i, action in enumerate(actions):
            if not isinstance(action, Action):
                raise TypeError("actions must contain Action objects")
            self._actions[i] = _NativeAction(action.kind, action.base, action.slots, action.target, 0)

    def legal(self, actions):
        """Validate composed actions; unlike options, this checks all constraints."""
        self._open()
        self._encode(actions)
        self._check(self._lib.gn_legal(self._handle, self.count, self._actions, self._legal))
        return tuple(bool(x) for x in self._legal)

    def sample_actions(self):
        """Hierarchical native random policy; advances only private policy seeds."""
        self._open()
        self._check(self._lib.gn_sample_actions(self._handle, self.count,
                                               self._policy_seeds, self._actions))
        return [Action(ActionKind(a.kind), a.base, a.slots, a.target) for a in self._actions]

    def step(self, actions):
        self._open()
        self._encode(actions)
        self._check(self._lib.gn_step(self._handle, self.count, self._actions,
                                     self._rewards, self._terminated, self._truncated))
        return self._rewards, self._terminated, self._truncated

    def random_step(self):
        return self.step(self.sample_actions())

    def close(self):
        if self._handle:
            self._lib.gn_destroy(self._handle)
            self._handle = None

    def __enter__(self):
        self._open()
        return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        self.close()


if __name__ == "__main__":
    episodes = 0
    with AttributeRemovalBatchEnv(16, max_decisions=64) as env:
        env.reset(42)
        for tick in range(128):
            env.observe()
            actions = env.sample_actions()
            assert all(env.legal(actions))
            _, terminal, truncated = env.step(actions)
            for i in range(env.count):
                if terminal[i] or truncated[i]:
                    episodes += 1
                    env.reset_at(i, 100000 + tick * env.count + i)
    print(f"Attribute removal native batch smoke passed: 2048 decisions, {episodes} episodes")
