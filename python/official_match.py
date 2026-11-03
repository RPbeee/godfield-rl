"""Experimental executable subset using official public arithmetic and local scheduling.

NOT authoritative official-server equivalence. No account/network access,
checkpoint replay or learning implementation is provided. load_scene is an
explicit privileged initializer from an immutable OfficialState, not a replay of
server attacks/events. Ordinary Earth/Moon payloads must be supplied externally.
All model generation is restricted to the native advertised supported pool.
"""
from __future__ import annotations
import ctypes as c
from dataclasses import dataclass
import json
from pathlib import Path
import sys
try:
    from .official_data import OfficialData
except ImportError:
    from official_data import OfficialData

MAX_WIRE_INTEGER = 9007199254740991
KINDS = ("play", "defend", "discard", "pray", "exchange", "sell", "buy", "accept", "decline")
PHASES = ("main", "response", "finished", "purchase")
_GUARDIAN_NAMES = ("mars", "mercury", "jupiter", "saturn", "uranus", "pluto", "neptune", "venus", "earth", "moon")

class MatchError(ValueError):
    def __init__(self, message, status=-2):
        super().__init__(message); self.status = status
class InvalidMatchError(MatchError):
    pass
class UnsupportedMatchError(MatchError):
    def __init__(self, message, status=-5): super().__init__(message, status)

def _check(status):
    if status == -5: raise UnsupportedMatchError("unsupported experimental match operation")
    if status in (-1, -2): raise InvalidMatchError(f"invalid match operation (status {status})", status)
    if status: raise MatchError(f"match operation failed (status {status})", status)

def _integer(value, label, minimum=0, maximum=MAX_WIRE_INTEGER):
    if type(value) is not int or not minimum <= value <= maximum:
        raise InvalidMatchError(f"{label}: expected an integer in [{minimum}, {maximum}]")
    return value

def _text(value):
    if type(value) is not str: raise InvalidMatchError("player name must be a string")
    try: return value.encode("utf-8", errors="strict")
    except UnicodeError as error: raise InvalidMatchError("player name is not valid Unicode") from error

@dataclass(frozen=True)
class Config:
    max_decisions: int = 512
    initial_hand: int = 9
    hand_limit: int = 18
    initial_hp: int = 40
    initial_mp: int = 10
    initial_cp: int = 20

@dataclass(frozen=True)
class PlayerSetup:
    id: int
    name: str = ""
    team: int = 0

@dataclass(frozen=True)
class Command:
    actor: int
    kind: str
    items: tuple[int, ...] = ()
    target: int | None = None
    allocation: tuple[int, int, int] | None = None

@dataclass(frozen=True)
class GuardianAction:
    source: int
    model: int
    target: int | None = None

@dataclass(frozen=True)
class GuardianPayloadAction:
    source: int
    guardian: str
    models: tuple[int, ...]
    target: int | None = None

@dataclass(frozen=True)
class PhenomenonAction:
    model: int
    recipient: int | None = None
    guardians: tuple[str, ...] = ()

@dataclass(frozen=True)
class ItemSource:
    owner: int
    token: int

@dataclass(frozen=True)
class ItemShuffleAction:
    order: tuple[ItemSource, ...] = ()

@dataclass(frozen=True)
class MatchStatus:
    decisions: int
    owner: int
    actor: int | None
    phase: str
    player_ids: tuple[int, ...]
    rewards: tuple[float, ...]
    winners: tuple[int, ...]
    terminated: bool
    truncated: bool

    @property
    def done(self): return self.phase == "finished"

    @property
    def reward_by_player(self): return dict(zip(self.player_ids, self.rewards))

class _Config(c.Structure):
    _fields_ = [("max_decisions", c.c_uint32), ("initial_hand", c.c_uint8), ("hand_limit", c.c_uint8),
        ("reserved", c.c_uint8 * 2), ("initial_hp", c.c_int32), ("initial_mp", c.c_int32), ("initial_cp", c.c_int32), ("reserved_tail", c.c_uint8 * 4)]
class _Player(c.Structure):
    _fields_ = [("id", c.c_uint64), ("name", c.c_char_p), ("name_size", c.c_size_t), ("team", c.c_uint8), ("reserved", c.c_uint8 * 7)]
class _Command(c.Structure):
    _fields_ = [("actor", c.c_uint64), ("target", c.c_uint64), ("kind", c.c_uint8), ("has_target", c.c_uint8),
        ("item_count", c.c_uint8), ("reserved", c.c_uint8 * 5), ("items", c.c_uint64 * 18),
        ("hp", c.c_int32), ("mp", c.c_int32), ("cp", c.c_int32), ("has_allocation", c.c_uint8), ("reserved_tail", c.c_uint8 * 3)]
class _GuardianAction(c.Structure):
    _fields_ = [("source", c.c_uint64), ("target", c.c_uint64), ("model", c.c_uint16),
                ("has_target", c.c_uint8), ("reserved", c.c_uint8 * 5)]
class _GuardianPayloadAction(c.Structure):
    _fields_ = [("source", c.c_uint64), ("target", c.c_uint64), ("models", c.c_uint16 * 18),
                ("guardian", c.c_uint8), ("model_count", c.c_uint8), ("has_target", c.c_uint8), ("reserved", c.c_uint8 * 9)]
class _PhenomenonAction(c.Structure):
    _fields_ = [("recipient", c.c_uint64), ("model", c.c_uint16), ("has_recipient", c.c_uint8),
                ("guardian_count", c.c_uint8), ("guardians", c.c_uint8 * 9), ("reserved", c.c_uint8 * 3)]
class _ItemSource(c.Structure):
    _fields_ = [("owner", c.c_uint64), ("token", c.c_uint64)]
class _ItemShuffleAction(c.Structure):
    _fields_ = [("order", c.POINTER(_ItemSource)), ("count", c.c_size_t)]
class _Status(c.Structure):
    _fields_ = [("decisions", c.c_uint64), ("owner", c.c_uint64), ("actor", c.c_uint64),
        ("player_ids", c.c_uint64 * 9), ("rewards", c.c_double * 9), ("winners", c.c_uint64 * 9),
        ("player_count", c.c_uint8), ("winner_count", c.c_uint8), ("phase", c.c_uint8), ("has_actor", c.c_uint8),
        ("terminated", c.c_uint8), ("truncated", c.c_uint8), ("reserved", c.c_uint8 * 2)]

def _config(value):
    if not isinstance(value, Config): raise InvalidMatchError("config must be Config")
    result = _Config(max_decisions=_integer(value.max_decisions, "max_decisions", 0, 2**32 - 1),
        initial_hand=_integer(value.initial_hand, "initial_hand", 0, 255), hand_limit=_integer(value.hand_limit, "hand_limit", 0, 255))
    for name in ("initial_hp", "initial_mp", "initial_cp"):
        setattr(result, name, _integer(getattr(value, name), name, -(2**31), 2**31 - 1))
    return result

def _players(values):
    if isinstance(values, dict): raise UnsupportedMatchError("snapshot/state import is not supported")
    try: iterator = iter(values)
    except TypeError as error: raise InvalidMatchError("players must be a sequence") from error
    records, copied = [], []
    for value in iterator:
        if len(records) == 9: raise InvalidMatchError("at most9 players")
        if type(value) is dict:
            if set(value) - {"id", "name", "team"}: raise UnsupportedMatchError("only player setup id/name/team can be imported")
            try: value = PlayerSetup(**value)
            except TypeError as error: raise InvalidMatchError("invalid player setup") from error
        if not isinstance(value, PlayerSetup): raise InvalidMatchError("players require PlayerSetup records")
        id = _integer(value.id, "player ID"); team = _integer(value.team, "team", 0, 4); name = _text(value.name)
        records.append(_Player(id=id, name=name, name_size=len(name), team=team))
        copied.append(PlayerSetup(id, value.name, team))
    return (_Player * len(records))(*records), tuple(copied)

def _command(value):
    if not isinstance(value, Command): raise InvalidMatchError("step/legal requires Command")
    if type(value.kind) is not str or value.kind not in KINDS: raise InvalidMatchError("unknown command kind")
    result = _Command(actor=_integer(value.actor, "actor"), kind=KINDS.index(value.kind))
    if value.target is not None:
        result.target = _integer(value.target, "target"); result.has_target = 1
    try: iterator = iter(value.items)
    except TypeError as error: raise InvalidMatchError("items must be item tokens") from error
    for token in iterator:
        if result.item_count == 18: raise InvalidMatchError("at most18 selected tokens")
        result.items[result.item_count] = _integer(token, "item token"); result.item_count += 1
    if value.allocation is not None:
        if type(value.allocation) not in (tuple, list) or len(value.allocation) != 3:
            raise InvalidMatchError("allocation needs a tuple or list of three integer resources")
        for name, amount in zip(("hp", "mp", "cp"), value.allocation): setattr(result, name, _integer(amount, name, 0, 99))
        result.has_allocation = 1
    return result

def _guardian_action(value):
    if not isinstance(value, GuardianAction): raise InvalidMatchError("guardian operation requires GuardianAction")
    result = _GuardianAction(source=_integer(value.source, "guardian source"),
                             model=_integer(value.model, "guardian model", 0, 65535))
    if value.target is not None:
        result.target = _integer(value.target, "guardian target"); result.has_target = 1
    return result

def _guardian_payload_action(value):
    if not isinstance(value, GuardianPayloadAction): raise InvalidMatchError("guardian payload operation requires GuardianPayloadAction")
    if type(value.guardian) is not str or value.guardian not in ("earth", "moon"):
        raise InvalidMatchError("guardian payload requires earth or moon")
    if type(value.models) is not tuple or not 1 <= len(value.models) <= 18:
        raise InvalidMatchError("guardian payload requires a tuple of1..18 models")
    result = _GuardianPayloadAction(source=_integer(value.source, "guardian payload source"),
                                    guardian=9 if value.guardian == "earth" else 10, model_count=len(value.models))
    for index, model in enumerate(value.models): result.models[index] = _integer(model, "guardian payload model", 0, 65535)
    if value.target is not None:
        result.target = _integer(value.target, "guardian payload target"); result.has_target = 1
    return result

def _phenomenon_action(value):
    if not isinstance(value, PhenomenonAction): raise InvalidMatchError("phenomenon operation requires PhenomenonAction")
    if type(value.guardians) is not tuple or len(value.guardians) > 9:
        raise InvalidMatchError("phenomenon guardians require a tuple of at most9 names")
    result = _PhenomenonAction(model=_integer(value.model, "phenomenon model", 0, 65535), guardian_count=len(value.guardians))
    if value.recipient is not None:
        result.recipient = _integer(value.recipient, "phenomenon recipient"); result.has_recipient = 1
    for index, name in enumerate(value.guardians):
        if type(name) is not str or name not in _GUARDIAN_NAMES: raise InvalidMatchError("unknown phenomenon guardian")
        result.guardians[index] = _GUARDIAN_NAMES.index(name) + 1
    return result

def _item_shuffle_action(value):
    if not isinstance(value, ItemShuffleAction): raise InvalidMatchError("item shuffle requires ItemShuffleAction")
    if type(value.order) is not tuple or len(value.order) > 162:
        raise InvalidMatchError("item shuffle requires a tuple of at most162 ItemSource records")
    records = []
    for source in value.order:
        if not isinstance(source, ItemSource): raise InvalidMatchError("item shuffle references require ItemSource")
        records.append(_ItemSource(_integer(source.owner, "item source owner"), _integer(source.token, "item source token")))
    copied = (_ItemSource * len(records))(*records)
    encoded = _ItemShuffleAction(count=len(records))
    if records: encoded.order = copied
    return encoded, copied

def _decode_status(value):
    if value.player_count > 9 or value.winner_count > 9 or value.phase > 3 or value.has_actor > 1:
        raise MatchError("invalid native status descriptor", -3)
    return MatchStatus(value.decisions, value.owner, value.actor if value.has_actor else None, PHASES[value.phase],
        tuple(value.player_ids[:value.player_count]), tuple(value.rewards[:value.player_count]),
        tuple(value.winners[:value.winner_count]), bool(value.terminated), bool(value.truncated))

def _decode_command(value):
    if value.item_count > 18 or value.kind >= len(KINDS) or value.has_target > 1 or value.has_allocation > 1:
        raise MatchError("invalid native command descriptor", -3)
    if (value.kind == 4) != bool(value.has_allocation) or any(value.reserved_tail) or (not value.has_allocation and (value.hp or value.mp or value.cp)):
        raise MatchError("invalid native allocation descriptor", -3)
    allocation = (value.hp, value.mp, value.cp) if value.has_allocation else None
    if allocation is not None and any(amount < 0 or amount > 99 for amount in allocation): raise MatchError("invalid native resource allocation", -3)
    return Command(value.actor, KINDS[value.kind], tuple(value.items[:value.item_count]), value.target if value.has_target else None, allocation)

class Match:
    """Owned mutable experimental match. Successful commands commit individually.

    Observe(None) is spectator; observe(player_id) includes that player's own hand.
    sample_action(rng) returns (Command, new_rng), changing neither game state nor
    the caller's integer. Rewards always follow status.player_ids, not actor.
    Do not operate the same handle concurrently from several threads.
    """
    def __init__(self, players=None, seed=42, config=Config(), library: str | Path | None = None):
        self._handle = c.c_void_p()
        self._data = OfficialData(library); self._lib = lib = self._data._lib
        try:
            for name in ("abi_version", "catalog_count", "gift_weight"):
                fn = getattr(lib, "gfo_match_" + name); fn.argtypes = []; fn.restype = c.c_int
            for name in ("ruleset", "scope"):
                fn = getattr(lib, "gfo_match_" + name); fn.argtypes = []; fn.restype = c.c_char_p
            for name in ("config_size", "player_size", "command_size", "status_size", "guardian_action_size", "guardian_payload_action_size", "phenomenon_action_size", "item_source_size", "item_shuffle_action_size"):
                fn = getattr(lib, "gfo_match_" + name); fn.argtypes = []; fn.restype = c.c_size_t
        except AttributeError as error: raise MatchError("library does not support experimental match ABI") from error
        self.ruleset = lib.gfo_match_ruleset().decode(); self.scope = lib.gfo_match_scope().decode()
        if (lib.gfo_match_abi_version() != 2 or self.ruleset != "official-execution-subset-v37"
            or not self.scope.startswith("experimental:") or "not-official-server-equivalence" not in self.scope
            or lib.gfo_match_config_size() != c.sizeof(_Config) or lib.gfo_match_player_size() != c.sizeof(_Player)
            or lib.gfo_match_command_size() != c.sizeof(_Command) or lib.gfo_match_status_size() != c.sizeof(_Status)
            or lib.gfo_match_guardian_action_size() != c.sizeof(_GuardianAction)
            or lib.gfo_match_guardian_payload_action_size() != c.sizeof(_GuardianPayloadAction)
            or _GuardianPayloadAction.models.offset != 16 or _GuardianPayloadAction.guardian.offset != 52
            or _GuardianPayloadAction.model_count.offset != 53 or _GuardianPayloadAction.has_target.offset != 54
            or _GuardianPayloadAction.reserved.offset != 55
            or lib.gfo_match_phenomenon_action_size() != c.sizeof(_PhenomenonAction)
            or _PhenomenonAction.model.offset != 8 or _PhenomenonAction.has_recipient.offset != 10
            or _PhenomenonAction.guardian_count.offset != 11 or _PhenomenonAction.guardians.offset != 12
            or _PhenomenonAction.reserved.offset != 21
            or lib.gfo_match_item_source_size() != c.sizeof(_ItemSource) or _ItemSource.token.offset != 8
            or lib.gfo_match_item_shuffle_action_size() != c.sizeof(_ItemShuffleAction) or _ItemShuffleAction.count.offset != 8):
            raise MatchError("unsupported match ABI/scope")
        lib.gfo_match_supported.argtypes = [c.c_uint16]; lib.gfo_match_supported.restype = c.c_int
        lib.gfo_match_create.argtypes = [c.POINTER(_Config), c.POINTER(_Player), c.c_size_t, c.c_uint64, c.POINTER(c.c_void_p)]
        lib.gfo_match_create.restype = c.c_int
        lib.gfo_match_destroy.argtypes = [c.c_void_p]; lib.gfo_match_destroy.restype = None
        lib.gfo_match_reset.argtypes = [c.c_void_p, c.POINTER(_Player), c.c_size_t, c.c_uint64, c.POINTER(_Status)]
        lib.gfo_match_reset.restype = c.c_int
        lib.gfo_match_load_scene.argtypes = [c.c_void_p, c.c_void_p, c.c_uint64, c.c_uint64, c.POINTER(_Status)]
        lib.gfo_match_load_scene.restype = c.c_int
        lib.gfo_match_get_status.argtypes = [c.c_void_p, c.POINTER(_Status)]; lib.gfo_match_get_status.restype = c.c_int
        lib.gfo_match_legal.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(c.c_uint8)]; lib.gfo_match_legal.restype = c.c_int
        lib.gfo_match_step.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(_Status)]; lib.gfo_match_step.restype = c.c_int
        lib.gfo_match_confusion_legal.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(c.c_uint8)]
        lib.gfo_match_confusion_legal.restype = c.c_int
        lib.gfo_match_confusion_step.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(_Status)]
        lib.gfo_match_confusion_step.restype = c.c_int
        lib.gfo_match_guardian_supported.argtypes = [c.c_uint16]; lib.gfo_match_guardian_supported.restype = c.c_int
        lib.gfo_match_guardian_legal.argtypes = [c.c_void_p, c.POINTER(_GuardianAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_guardian_legal.restype = c.c_int
        lib.gfo_match_guardian_step.argtypes = [c.c_void_p, c.POINTER(_GuardianAction), c.POINTER(_Status)]
        lib.gfo_match_guardian_step.restype = c.c_int
        lib.gfo_match_guardian_payload_supported.argtypes = [c.c_uint8, c.c_uint16]; lib.gfo_match_guardian_payload_supported.restype = c.c_int
        lib.gfo_match_guardian_payload_legal.argtypes = [c.c_void_p, c.POINTER(_GuardianPayloadAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_guardian_payload_legal.restype = c.c_int
        lib.gfo_match_guardian_payload_step.argtypes = [c.c_void_p, c.POINTER(_GuardianPayloadAction), c.POINTER(_Status)]
        lib.gfo_match_guardian_payload_step.restype = c.c_int
        lib.gfo_match_phenomenon_supported.argtypes = [c.c_uint16]; lib.gfo_match_phenomenon_supported.restype = c.c_int
        lib.gfo_match_phenomenon_legal.argtypes = [c.c_void_p, c.POINTER(_PhenomenonAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_phenomenon_legal.restype = c.c_int
        lib.gfo_match_phenomenon_step.argtypes = [c.c_void_p, c.POINTER(_PhenomenonAction), c.POINTER(_Status)]
        lib.gfo_match_phenomenon_step.restype = c.c_int
        lib.gfo_match_item_shuffle_supported.argtypes = [c.c_uint16]; lib.gfo_match_item_shuffle_supported.restype = c.c_int
        lib.gfo_match_item_shuffle_legal.argtypes = [c.c_void_p, c.POINTER(_ItemShuffleAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_item_shuffle_legal.restype = c.c_int
        lib.gfo_match_item_shuffle_step.argtypes = [c.c_void_p, c.POINTER(_ItemShuffleAction), c.POINTER(_Status)]
        lib.gfo_match_item_shuffle_step.restype = c.c_int
        lib.gfo_match_sample_action.argtypes = [c.c_void_p, c.POINTER(c.c_uint64), c.POINTER(_Command)]; lib.gfo_match_sample_action.restype = c.c_int
        lib.gfo_match_observe.argtypes = [c.c_void_p, c.c_uint8, c.c_uint64, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
        lib.gfo_match_observe.restype = c.c_int
        self.supported_models = tuple(card.id for card in self._data if lib.gfo_match_supported(card.id))
        self.supported_guardian_models = tuple(card.id for card in self._data if lib.gfo_match_guardian_supported(card.id))
        self.supported_guardian_payload_models = {
            name: tuple(card.id for card in self._data if lib.gfo_match_guardian_payload_supported(kind, card.id))
            for name, kind in (("earth", 9), ("moon", 10))}
        self.supported_phenomenon_models = tuple(card.id for card in self._data if lib.gfo_match_phenomenon_supported(card.id))
        self.supported_item_shuffle_models = tuple(card.id for card in self._data if lib.gfo_match_item_shuffle_supported(card.id))
        self.gift_weight = lib.gfo_match_gift_weight()
        if (len(self.supported_models) != lib.gfo_match_catalog_count() or not self.supported_models
            or self.gift_weight != sum(self._data.get_card(id).gift_rate for id in self.supported_models)):
            raise MatchError("inconsistent match catalog descriptor")
        if self.supported_guardian_models != tuple(range(245, 287)):
            raise MatchError("inconsistent guardian action descriptor")
        config_record = _config(config)
        setups, copied = _players((PlayerSetup(0, "Player0"), PlayerSetup(1, "Player1")) if players is None else players)
        seed = _integer(seed, "seed", 0, 2**64 - 1)
        _check(lib.gfo_match_create(c.byref(config_record), setups, len(setups), seed, c.byref(self._handle)))
        self._players = copied; self.config = config

    @property
    def closed(self): return not bool(self._handle.value)

    def _open(self):
        if self.closed: raise InvalidMatchError("match is closed")

    def close(self):
        if getattr(self, "_handle", None) and self._handle.value:
            self._lib.gfo_match_destroy(self._handle); self._handle = c.c_void_p()

    def __enter__(self): self._open(); return self
    def __exit__(self, *unused): self.close()
    def __del__(self):
        try: self.close()
        except Exception: pass

    @property
    def status(self):
        self._open(); result = _Status()
        _check(self._lib.gfo_match_get_status(self._handle, c.byref(result)))
        return _decode_status(result)

    @property
    def actor(self): return self.status.actor
    @property
    def done(self): return self.status.done

    def reset(self, seed=42, players=None):
        self._open(); setups, copied = _players(self._players if players is None else players)
        seed = _integer(seed, "seed", 0, 2**64 - 1); result = _Status()
        _check(self._lib.gfo_match_reset(self._handle, setups, len(setups), seed, c.byref(result)))
        self._players = copied
        return _decode_status(result)

    def legal(self, command):
        self._open(); encoded = _command(command); result = c.c_uint8()
        _check(self._lib.gfo_match_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def load_scene(self, state, owner, seed=42):
        """Initialize a privileged quiescent local scene, retaining this Config.

        A live same-library OfficialState is copied independently. Pending server
        attacks cannot be imported; events and metadata do not reconstruct engine
        timing, RNG or checkpoints. reset() subsequently uses the imported roster.
        """
        self._open()
        try:
            from .official_state import OfficialState
        except ImportError:
            from official_state import OfficialState
        if not isinstance(state, OfficialState): raise InvalidMatchError("load_scene requires OfficialState")
        if state.closed: raise InvalidMatchError("official state is closed")
        if state._lib._handle != self._lib._handle:
            raise InvalidMatchError("scene and match must use the same loaded library")
        if state._raw.get("attacks"):
            raise UnsupportedMatchError("pending server attacks cannot initialize a quiescent scene")
        owner = _integer(owner, "scene owner"); seed = _integer(seed, "scene seed", 0, 2**64 - 1)
        # Allocate Python bookkeeping before the native atomic commit.
        copied = tuple(PlayerSetup(p["id"], p.get("name", ""), p.get("team") or 0)
                       for p in state._raw.get("players", []))
        result = _Status()
        _check(self._lib.gfo_match_load_scene(self._handle, state._handle, owner, seed, c.byref(result)))
        self._players = copied
        return _decode_status(result)

    def guardian_legal(self, action):
        self._open(); encoded = _guardian_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_guardian_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def guardian_step(self, action):
        """Execute one externally selected action; no autonomous guardian policy."""
        self._open(); encoded = _guardian_action(action); result = _Status()
        _check(self._lib.gfo_match_guardian_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def guardian_payload_legal(self, action):
        self._open(); encoded = _guardian_payload_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_guardian_payload_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def guardian_payload_step(self, action):
        """Execute one supplied Earth/Moon model composition; no selector distribution is implied."""
        self._open(); encoded = _guardian_payload_action(action); result = _Status()
        _check(self._lib.gfo_match_guardian_payload_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def phenomenon_legal(self, action):
        self._open(); encoded = _phenomenon_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_phenomenon_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def phenomenon_step(self, action):
        """Apply one supplied phenomenon; targeted effects await responses. No selector distribution is implied."""
        self._open(); encoded = _phenomenon_action(action); result = _Status()
        _check(self._lib.gfo_match_phenomenon_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def item_shuffle_legal(self, action):
        """Validate a privileged full-inventory manifest; not a normal policy query."""
        self._open(); encoded, copied = _item_shuffle_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_item_shuffle_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def item_shuffle_step(self, action):
        """Apply a supplied permutation; no selection distribution is implied."""
        self._open(); encoded, copied = _item_shuffle_action(action); result = _Status()
        _check(self._lib.gfo_match_item_shuffle_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def step(self, command):
        self._open(); encoded = _command(command); result = _Status()
        _check(self._lib.gfo_match_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def confusion_legal(self, command):
        """Validate an externally resolved confused-owner main command; no selector."""
        self._open(); encoded = _command(command); result = c.c_uint8()
        _check(self._lib.gfo_match_confusion_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def confusion_step(self, command):
        """Execute one supplied confused-owner action and consume one confusion count."""
        self._open(); encoded = _command(command); result = _Status()
        _check(self._lib.gfo_match_confusion_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def sample_action(self, policy_rng):
        self._open(); rng = c.c_uint64(_integer(policy_rng, "policy_rng", 0, 2**64 - 1)); result = _Command()
        _check(self._lib.gfo_match_sample_action(self._handle, c.byref(rng), c.byref(result)))
        return _decode_command(result), rng.value

    def observe(self, viewer=None):
        self._open(); present = viewer is not None; id = 0 if viewer is None else _integer(viewer, "viewer")
        needed = c.c_size_t()
        _check(self._lib.gfo_match_observe(self._handle, present, id, None, 0, c.byref(needed)))
        data = c.create_string_buffer(needed.value)
        _check(self._lib.gfo_match_observe(self._handle, present, id, data, len(data), c.byref(needed)))
        return json.loads(data.raw[:-1].decode("utf-8"))

    def random_rollout(self, policy_rng=1, max_steps=None):
        """Execute sampled legal commands; return (final/current status, new RNG).

        A rollout is a sequence of atomic commands, not one transactional batch.
        max_steps=0 performs no command. This method never trains a model.
        """
        policy_rng = _integer(policy_rng, "policy_rng", 0, 2**64 - 1)
        if max_steps is not None: max_steps = _integer(max_steps, "max_steps", 0, 2**64 - 1)
        steps = 0
        while not self.done and (max_steps is None or steps < max_steps):
            command, policy_rng = self.sample_action(policy_rng); self.step(command); steps += 1
        return self.status, policy_rng

if __name__ == "__main__":
    library = sys.argv[1] if len(sys.argv) > 1 else None
    with Match(seed=42, library=library) as match:
        status, rng = match.random_rollout(12345)
        print(json.dumps({"ruleset": match.ruleset, "scope": match.scope, "catalog_count": len(match.supported_models),
            "gift_weight": match.gift_weight, "decisions": status.decisions, "winners": status.winners,
            "rewards": status.reward_by_player, "terminated": status.terminated, "truncated": status.truncated, "policy_rng": rng}))
