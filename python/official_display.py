"""Immutable received active-display updates, not a game engine or server replay.

Only nextAttack/setTargetPlayer/advanceGF are typed here. The input OfficialState
is copied. Existing fresh view/active-attack queries and Match APIs are unchanged.
No hidden model, outcome, actor legality, event fanout or random selector is inferred.
"""
from __future__ import annotations
import ctypes as c
from dataclasses import dataclass
import json
try:
    from .official_state import OfficialState, _ActiveAttack, _integer, _boolean, GUARDIANS
except ImportError:
    from official_state import OfficialState, _ActiveAttack, _integer, _boolean, GUARDIANS

@dataclass(frozen=True)
class NextAttack:
    source: int = 0
    models: tuple[int, ...] = ()
    guardian: str | None = None
    atk: int = 0
    hit_rate: int = 0
    mp: int = 0
    cp: int = 0
    reversed: bool = False
    target: int = 0
    buying_model: int = 0

    def __post_init__(self):
        if isinstance(self.models, (str, bytes, dict)): raise ValueError("models must be an iterable of catalog IDs")
        try: object.__setattr__(self, "models", tuple(self.models))
        except TypeError as error: raise ValueError("models must be an iterable of catalog IDs") from error

@dataclass(frozen=True)
class SetTargetPlayer:
    player_id: int

@dataclass(frozen=True)
class AdvanceGF:
    player_id: int

class _Event(c.Structure):
    _fields_ = [("attack", _ActiveAttack), ("player_id", c.c_uint64),
                ("kind", c.c_uint8), ("reserved", c.c_uint8 * 7)]

def _encode(event, model_count):
    wire, storage = _Event(), None
    if isinstance(event, NextAttack):
        values = [_integer(model, "attack model", 1, model_count) for model in event.models]
        storage = (c.c_uint16 * len(values))(*values)
        if event.guardian is None: guardian = 0
        elif type(event.guardian) is str and event.guardian in GUARDIANS[1:]: guardian = GUARDIANS.index(event.guardian)
        else: raise ValueError("guardian must be a known name or None")
        wire.attack = _ActiveAttack(models=storage if values else None, model_count=len(values),
            player_id=_integer(event.source, "attack source", 0), target_player_id=_integer(event.target, "attack target", 0),
            atk=_integer(event.atk, "attack atk"), hit_rate=_integer(event.hit_rate, "attack hit_rate"),
            mp=_integer(event.mp, "attack mp"), cp=_integer(event.cp, "attack cp"),
            guardian=guardian, reversed=_boolean(event.reversed, "attack reversed"),
            buying_model=_integer(event.buying_model, "buying model", 0, model_count))
    elif isinstance(event, SetTargetPlayer): wire.kind = 1; wire.player_id = _integer(event.player_id, "target player", 0)
    elif isinstance(event, AdvanceGF): wire.kind = 2; wire.player_id = _integer(event.player_id, "GF player", 0)
    else: raise ValueError("unsupported received display event")
    return wire, storage

class ActiveDisplay:
    """Owned immutable display. apply returns a new owner; close each owner."""
    def __init__(self, state: OfficialState, turn_player=None):
        self._handle = c.c_void_p()
        if not isinstance(state, OfficialState): raise ValueError("state must be an OfficialState")
        state._open(); self._lib = lib = state._lib; self._model_count = len(state._data)
        try:
            lib.gfo_display_api_version.argtypes = []; lib.gfo_display_api_version.restype = c.c_int
            lib.gfo_display_event_size.argtypes = []; lib.gfo_display_event_size.restype = c.c_size_t
        except AttributeError as error: raise RuntimeError("library does not support active display API") from error
        if (lib.gfo_display_api_version() != 1 or lib.gfo_display_event_size() != c.sizeof(_Event)
            or c.sizeof(_Event) != 88 or _Event.player_id.offset != 72 or _Event.kind.offset != 80 or _Event.reserved.offset != 81):
            raise RuntimeError("unsupported active display API layout")
        lib.gfo_display_create.argtypes = [c.c_void_p, c.c_uint64, c.POINTER(c.c_void_p)]; lib.gfo_display_create.restype = c.c_int
        lib.gfo_display_apply.argtypes = [c.c_void_p, c.POINTER(_Event), c.c_size_t, c.POINTER(c.c_void_p)]; lib.gfo_display_apply.restype = c.c_int
        lib.gfo_display_destroy.argtypes = [c.c_void_p]; lib.gfo_display_destroy.restype = None
        lib.gfo_display_view_json.argtypes = [c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]; lib.gfo_display_view_json.restype = c.c_int
        if turn_player is None: turn_player = state._raw.get("attackTurnPlayerId", 0)
        turn_player = _integer(turn_player, "turn player", 0)
        self._check(lib.gfo_display_create(state._handle, turn_player, c.byref(self._handle)))

    @staticmethod
    def _check(status):
        if status: raise ValueError(f"received display operation rejected (status {status})")

    @property
    def closed(self): return not bool(self._handle.value)

    def _open(self):
        if self.closed: raise ValueError("active display is closed")

    def close(self):
        if getattr(self, "_handle", None) and self._handle.value:
            self._lib.gfo_display_destroy(self._handle); self._handle = c.c_void_p()

    def __enter__(self): self._open(); return self
    def __exit__(self, *unused): self.close()
    def __del__(self):
        try: self.close()
        except Exception: pass

    def apply(self, events):
        """Copy and apply the complete supplied sequence atomically, without RNG.

        Unknown events are rejected rather than replayed or silently skipped.
        Arrays are copied; they can be mutated/destroyed after this call returns.
        """
        self._open()
        if isinstance(events, (str, bytes, dict)): raise ValueError("events must be a sequence of typed events")
        try: iterator = iter(events)
        except TypeError as error: raise ValueError("events must be iterable") from error
        encoded, storage = [], []
        for event in iterator:
            wire, models = _encode(event, self._model_count); encoded.append(wire); storage.append(models)
        array = (_Event * len(encoded))(*encoded)
        result = object.__new__(type(self)); result._handle = c.c_void_p(); result._lib = self._lib; result._model_count = self._model_count
        self._check(self._lib.gfo_display_apply(self._handle, array, len(array), c.byref(result._handle)))
        return result

    def view(self):
        self._open(); size = c.c_size_t()
        self._check(self._lib.gfo_display_view_json(self._handle, None, 0, c.byref(size)))
        buffer = c.create_string_buffer(size.value)
        self._check(self._lib.gfo_display_view_json(self._handle, buffer, len(buffer), c.byref(size)))
        value = json.loads(buffer.raw[:-1].decode("utf-8"))
        if value.get("schema") != "official-active-display-view-v1": raise RuntimeError("unsupported active display JSON schema")
        return value
