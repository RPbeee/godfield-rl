"""Dependency-free ctypes bridge for synthetic-neutral-duel-v1. Not Gymnasium.

observe(): (flat float features[n,73], uint8 masks[n,521], uint8 actors[n])
step(): (flat fixed-seat rewards[n,2], terminated[n], truncated[n])
The acting seat can stay the same across defense -> main transitions.
"""
from __future__ import annotations

import ctypes as c
from pathlib import Path
import sys


class BatchEnv:
    def __init__(self, count: int = 64, max_decisions: int = 512,
                 library: str | None = None):
        if not isinstance(count, int) or count <= 0:
            raise ValueError("count must be a positive integer")
        if not isinstance(max_decisions, int) or not 1 <= max_decisions <= 0xFFFFFFFF:
            raise ValueError("max_decisions must be in [1, 2**32-1]")
        ext = "dylib" if sys.platform == "darwin" else "so"
        path = Path(library) if library else Path(__file__).resolve().parents[1] / "build" / f"libgodfield.{ext}"
        lib = self._lib = c.CDLL(str(path))
        self._handle = None
        lib.gf_abi_version.restype = c.c_int
        if lib.gf_abi_version() != 1:
            raise RuntimeError("unsupported native ABI")
        lib.gf_create.argtypes = [c.c_size_t, c.c_uint32]
        lib.gf_create.restype = c.c_void_p
        lib.gf_destroy.argtypes = [c.c_void_p]
        lib.gf_destroy.restype = None
        lib.gf_reset_at.argtypes = [c.c_void_p, c.c_size_t, c.c_uint64]
        lib.gf_reset_at.restype = c.c_int
        lib.gf_observe.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_float), c.POINTER(c.c_uint8), c.POINTER(c.c_uint8)]
        lib.gf_observe.restype = c.c_int
        lib.gf_step.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_int32), c.POINTER(c.c_float), c.POINTER(c.c_uint8), c.POINTER(c.c_uint8)]
        lib.gf_step.restype = c.c_int
        self.count = count
        self.action_count = lib.gf_action_count()
        self.observation_size = lib.gf_observation_size()
        self._obs = (c.c_float * (count * self.observation_size))()
        self._masks = (c.c_uint8 * (count * self.action_count))()
        self._actors = (c.c_uint8 * count)()
        self._actions = (c.c_int32 * count)()
        self._rewards = (c.c_float * (count * 2))()
        self._terminated = (c.c_uint8 * count)()
        self._truncated = (c.c_uint8 * count)()
        self._handle = lib.gf_create(count, max_decisions)
        if not self._handle:
            raise MemoryError("native batch allocation failed")

    def _check(self, status):
        if status:
            raise ValueError(f"native environment error {status}")

    def _open(self):
        if not self._handle:
            raise RuntimeError("environment is closed")

    def reset_at(self, index: int, seed: int):
        self._open()
        if not 0 <= index < self.count or not 0 <= seed <= 0xFFFFFFFFFFFFFFFF:
            raise ValueError("index or seed out of range")
        self._check(self._lib.gf_reset_at(self._handle, index, seed))

    def reset(self, seed: int = 0):
        if not 0 <= seed <= 0xFFFFFFFFFFFFFFFF - self.count + 1:
            raise ValueError("seed range overflow")
        for i in range(self.count):
            self.reset_at(i, seed + i)
        return self.observe()

    def observe(self):
        """Return reusable ctypes buffers; copy before storing in replay memory."""
        self._open()
        self._check(self._lib.gf_observe(self._handle, self.count, self._obs, self._masks, self._actors))
        return self._obs, self._masks, self._actors

    def step(self, actions):
        self._open()
        if len(actions) != self.count:
            raise ValueError("expected one action per environment")
        for i, action in enumerate(actions):
            if not isinstance(action, int) or not 0 <= action < self.action_count:
                raise ValueError("action out of range")
            self._actions[i] = action
        self._check(self._lib.gf_step(self._handle, self.count, self._actions,
                                     self._rewards, self._terminated, self._truncated))
        return self._rewards, self._terminated, self._truncated

    def close(self):
        if self._handle:
            self._lib.gf_destroy(self._handle)
            self._handle = None

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


if __name__ == "__main__":
    import random
    rng = random.Random(42)
    episodes = 0
    with BatchEnv(64) as env:
        env.reset(42)
        for tick in range(256):
            _, masks, _ = env.observe()
            actions = [rng.choice([a for a in range(env.action_count)
                                   if masks[i * env.action_count + a]])
                       for i in range(env.count)]
            _, terminal, truncated = env.step(actions)
            for i in range(env.count):
                if terminal[i] or truncated[i]:
                    episodes += 1
                    env.reset_at(i, 100000 + tick * env.count + i)
    print(f"Native batch smoke passed: 16384 decisions, {episodes} episodes")
