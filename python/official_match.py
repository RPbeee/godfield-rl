"""Experimental executable subset using official public arithmetic and local scheduling.

NOT authoritative official-server equivalence. No account/network access,
checkpoint replay or learning implementation is provided. load_scene is an
explicit privileged initializer from an immutable OfficialState, not a replay of
server attacks/events. Earth/Moon payloads have explicit external entry points
and opt-in local automatic selection; the default keeps canonical285/286.
Artifact generation uses the native advertised configured gift pool.
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
KINDS = ("play", "defend", "discard", "pray", "exchange", "sell", "buy", "accept", "decline", "sacrifice")
PHASES = ("main", "response", "finished", "purchase", "grant")
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
    external_grants: bool = False
    include_fate: bool = False
    local_guardian_payloads: bool = False
    external_artifacts: bool = False

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
class FateRequest:
    actor: int
    token: int

PlayerChoice = Command | FateRequest

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
class FateAction:
    actor: int
    token: int
    outcome: PhenomenonAction
    shuffle: ItemShuffleAction | None = None

@dataclass(frozen=True)
class DevilAction:
    target: int
    model: int
    resource: str | None = None
    removed: tuple[int, ...] = ()

@dataclass(frozen=True)
class GrantAction:
    """Privileged grant result: 0 draws, a configured ordinary ID is supplied, or 240..244 resolves a devil.

    Explicit ordinary IDs consume no game RNG and currently require a non-Dream
    recipient. Model207 additionally requires include_fate; this is not a selector.
    """
    target: int
    model: int = 0
    resource: str | None = None
    removed: tuple[int, ...] = ()

@dataclass(frozen=True)
class ArtifactGrantAction:
    """Privileged supplied ordinary artifact; None means explicitly unmasked.

    A face requires Dream and either a distinct compatible model, the same
    supported non207 model, or the observed directed actual19/face27 pair.
    The reverse pair is unsupported. Presence stays distinct from None; this
    supplied input does not change the local random mask selector.
    Actual207 needs include_fate and cannot carry a face. No game RNG is used.
    """
    target: int
    model: int
    fake_model: int | None = None

@dataclass(frozen=True)
class RedrawItem:
    """Supplied fresh unused replacement; None is an explicit absent disguise."""
    model: int
    fake_model: int | None = None

    def __post_init__(self):
        _integer(self.model, "redraw model", 1, 65535)
        if self.fake_model is not None: _integer(self.fake_model, "redraw face", 1, 65535)

@dataclass(frozen=True)
class RedrawResponseAction:
    """One ordinary response plus one owned result list per selected actual178.

    Results replace all remaining physical copies before the attack body.
    No token, used flag or owner is supplied; ordinary reserved gifts are separate.
    """
    response: Command
    results: tuple[tuple[RedrawItem, ...], ...]

    def __post_init__(self):
        object.__setattr__(self, "response", _decode_command(_command(self.response)))
        copied = []
        try:
            for result in self.results:
                if len(copied) == 18: raise InvalidMatchError("at most18 redraw results")
                items = []
                for item in result:
                    if len(items) == 18: raise InvalidMatchError("at most18 redraw items per result")
                    if type(item) is not RedrawItem: raise InvalidMatchError("redraw items require RedrawItem")
                    items.append(item)
                copied.append(tuple(items))
        except TypeError as error: raise InvalidMatchError("redraw results must be item sequences") from error
        object.__setattr__(self, "results", tuple(copied))

@dataclass(frozen=True)
class PurchaseResponseAction:
    """Privileged BUY result, using an exact current unused seller token.

    None denotes an empty eligible inventory; token0 remains an actual copy.
    This supplies a chance result, not the buyer's later accept/decline command.
    """
    response: Command
    commodity: int | None = None

    def __post_init__(self):
        object.__setattr__(self, "response", _decode_command(_command(self.response)))
        if self.commodity is not None: _integer(self.commodity, "purchase commodity")

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

@dataclass(frozen=True)
class RolloutResult:
    status: MatchStatus
    policy_rng: int
    chance_rng: int
    steps: int

class RolloutError(MatchError):
    """A failed decision, with seeds/status after the last successful decision."""
    def __init__(self, cause: MatchError, progress: RolloutResult):
        super().__init__(f"chance rollout stopped after {progress.steps} successful steps: {cause}", cause.status)
        self.cause = cause
        self.progress = progress

class _Config(c.Structure):
    _fields_ = [("max_decisions", c.c_uint32), ("initial_hand", c.c_uint8), ("hand_limit", c.c_uint8),
        ("external_grants", c.c_uint8), ("include_fate", c.c_uint8), ("initial_hp", c.c_int32), ("initial_mp", c.c_int32), ("initial_cp", c.c_int32),
        ("local_guardian_payloads", c.c_uint8), ("external_artifacts", c.c_uint8), ("reserved_tail", c.c_uint8 * 2)]
class _Player(c.Structure):
    _fields_ = [("id", c.c_uint64), ("name", c.c_char_p), ("name_size", c.c_size_t), ("team", c.c_uint8), ("reserved", c.c_uint8 * 7)]
class _Command(c.Structure):
    _fields_ = [("actor", c.c_uint64), ("target", c.c_uint64), ("kind", c.c_uint8), ("has_target", c.c_uint8),
        ("item_count", c.c_uint8), ("reserved", c.c_uint8 * 5), ("items", c.c_uint64 * 18),
        ("hp", c.c_int32), ("mp", c.c_int32), ("cp", c.c_int32), ("has_allocation", c.c_uint8), ("reserved_tail", c.c_uint8 * 3)]
class _Choice(c.Structure):
    _fields_ = [("command", _Command), ("fate_actor", c.c_uint64), ("fate_token", c.c_uint64),
                ("kind", c.c_uint8), ("reserved", c.c_uint8 * 7)]
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
class _FateAction(c.Structure):
    _fields_ = [("actor", c.c_uint64), ("token", c.c_uint64), ("outcome", _PhenomenonAction),
                ("shuffle", _ItemShuffleAction), ("has_shuffle", c.c_uint8), ("reserved", c.c_uint8 * 7)]
class _ArtifactGrantAction(c.Structure):
    _fields_ = [("target", c.c_uint64), ("model", c.c_uint16), ("fake_model", c.c_uint16), ("reserved", c.c_uint8 * 4)]
class _RedrawItem(c.Structure):
    _fields_ = [("model", c.c_uint16), ("fake_model", c.c_uint16), ("has_fake", c.c_uint8), ("reserved", c.c_uint8 * 3)]
class _RedrawResult(c.Structure):
    _fields_ = [("items", c.POINTER(_RedrawItem)), ("item_count", c.c_size_t)]
class _PurchaseSelection(c.Structure):
    _fields_ = [("commodity", c.c_uint64), ("has_commodity", c.c_uint8), ("reserved", c.c_uint8 * 7)]
class _DevilAction(c.Structure):
    _fields_ = [("target", c.c_uint64), ("removed", c.c_uint64 * 2), ("model", c.c_uint16),
                ("resource", c.c_uint8), ("has_resource", c.c_uint8), ("removed_count", c.c_uint8), ("reserved", c.c_uint8 * 3)]
class _Status(c.Structure):
    _fields_ = [("decisions", c.c_uint64), ("owner", c.c_uint64), ("actor", c.c_uint64),
        ("player_ids", c.c_uint64 * 9), ("rewards", c.c_double * 9), ("winners", c.c_uint64 * 9),
        ("player_count", c.c_uint8), ("winner_count", c.c_uint8), ("phase", c.c_uint8), ("has_actor", c.c_uint8),
        ("terminated", c.c_uint8), ("truncated", c.c_uint8), ("reserved", c.c_uint8 * 2)]

def _config(value):
    if not isinstance(value, Config): raise InvalidMatchError("config must be Config")
    if type(value.external_grants) is not bool: raise InvalidMatchError("external_grants must be bool")
    if type(value.include_fate) is not bool: raise InvalidMatchError("include_fate must be bool")
    if type(value.local_guardian_payloads) is not bool: raise InvalidMatchError("local_guardian_payloads must be bool")
    if type(value.external_artifacts) is not bool: raise InvalidMatchError("external_artifacts must be bool")
    result = _Config(external_grants=value.external_grants, include_fate=value.include_fate, local_guardian_payloads=value.local_guardian_payloads, external_artifacts=value.external_artifacts, max_decisions=_integer(value.max_decisions, "max_decisions", 0, 2**32 - 1),
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

def _fate_action(value):
    if not isinstance(value, FateAction): raise InvalidMatchError("fate operation requires FateAction")
    encoded = _FateAction(actor=_integer(value.actor, "fate actor"), token=_integer(value.token, "fate token"),
                          outcome=_phenomenon_action(value.outcome))
    if (value.shuffle is not None) != (encoded.outcome.model == 295):
        raise InvalidMatchError("only phenomenon 295 requires a shuffle manifest")
    copied = None
    if value.shuffle is not None:
        encoded.shuffle, copied = _item_shuffle_action(value.shuffle); encoded.has_shuffle = 1
    return encoded, copied

def _devil_action(value): return _resource_action(value, DevilAction, 1)

def _grant_action(value): return _resource_action(value, GrantAction, 0)

def _artifact_grant_action(value):
    if not isinstance(value, ArtifactGrantAction): raise InvalidMatchError("operation requires ArtifactGrantAction")
    return _ArtifactGrantAction(target=_integer(value.target, "artifact recipient"),
        model=_integer(value.model, "artifact model", 1, 65535),
        fake_model=0 if value.fake_model is None else _integer(value.fake_model, "artifact face", 1, 65535))

def _redraw_response_action(value):
    if type(value) is not RedrawResponseAction: raise InvalidMatchError("operation requires RedrawResponseAction")
    copied, records = [], []
    for items in value.results:
        storage = (_RedrawItem * len(items))(*(_RedrawItem(model=item.model,
            fake_model=0 if item.fake_model is None else item.fake_model, has_fake=item.fake_model is not None) for item in items))
        copied.append(storage)
        records.append(_RedrawResult(items=storage if items else None, item_count=len(items)))
    return _command(value.response), (_RedrawResult * len(records))(*records), copied

def _purchase_response_action(value):
    if type(value) is not PurchaseResponseAction: raise InvalidMatchError("operation requires PurchaseResponseAction")
    return _command(value.response), _PurchaseSelection(commodity=0 if value.commodity is None else value.commodity,
        has_commodity=value.commodity is not None)

def _resource_action(value, record_type, minimum):
    if not isinstance(value, record_type): raise InvalidMatchError(f"operation requires {record_type.__name__}")
    encoded = _DevilAction(target=_integer(value.target, "recipient"), model=_integer(value.model, "model", minimum, 65535))
    if (value.resource is not None) != (encoded.model == 244):
        raise InvalidMatchError("only devil 244 requires a resource")
    if value.resource is not None:
        if type(value.resource) is not str or value.resource not in ("hp", "mp", "cp"):
            raise InvalidMatchError("devil resource must be hp, mp or cp")
        encoded.resource = ("hp", "mp", "cp").index(value.resource); encoded.has_resource = 1
    if type(value.removed) is not tuple or len(value.removed) > 2 or (value.removed and encoded.model != 243):
        raise InvalidMatchError("only devil 243 accepts at most two removed tokens")
    encoded.removed_count = len(value.removed)
    for i, token in enumerate(value.removed): encoded.removed[i] = _integer(token, "removed token")
    return encoded

def _decode_status(value):
    if value.player_count > 9 or value.winner_count > 9 or value.phase >= len(PHASES) or value.has_actor > 1:
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

def _choice(value):
    result = _Choice()
    if isinstance(value, Command): result.command = _command(value)
    elif isinstance(value, FateRequest):
        result.kind = 1; result.fate_actor = _integer(value.actor, "fate request actor"); result.fate_token = _integer(value.token, "fate request token")
    else: raise InvalidMatchError("choice must be Command or FateRequest")
    return result

def _decode_choice(value):
    try:
        if value.kind == 0: result = _decode_command(value.command)
        elif value.kind == 1: result = FateRequest(value.fate_actor, value.fate_token)
        else: raise MatchError("unknown native choice kind", -3)
        canonical = _choice(result)
    except MatchError as error: raise MatchError("invalid native choice descriptor", -3) from error
    if bytes(value) != bytes(canonical): raise MatchError("noncanonical native choice descriptor", -3)
    return result

def _decode_grant(value):
    if (value.model not in (0, 240, 241, 242, 243, 244) or value.has_resource > 1 or value.removed_count > 2
        or any(value.reserved) or any(value.removed[value.removed_count:])
        or (not value.has_resource and value.resource) or (value.removed_count == 2 and value.removed[0] == value.removed[1])):
        raise MatchError("invalid native grant descriptor", -3)
    if value.has_resource and value.resource > 2: raise MatchError("invalid native grant resource", -3)
    action = GrantAction(value.target, value.model, ("hp", "mp", "cp")[value.resource] if value.has_resource else None,
                         tuple(value.removed[:value.removed_count]))
    try: _grant_action(action)
    except InvalidMatchError as error: raise MatchError("invalid native grant shape", -3) from error
    return action

def _decode_fate(value, storage, expected_token):
    outcome = value.outcome; count = value.shuffle.count
    pointer = c.cast(value.shuffle.order, c.c_void_p).value
    if (value.has_shuffle > 1 or any(value.reserved) or value.token != expected_token
        or not 287 <= outcome.model <= 296 or bool(value.has_shuffle) != (outcome.model == 295)
        or count > len(storage) or (not value.has_shuffle and count)
        or pointer != (c.addressof(storage) if count else None)
        or outcome.has_recipient > 1 or any(outcome.reserved) or outcome.guardian_count > 9
        or any(outcome.guardians[outcome.guardian_count:]) or (not outcome.has_recipient and outcome.recipient)
        or bool(outcome.has_recipient) != (291 <= outcome.model <= 294)
        or (outcome.model != 296 and outcome.guardian_count)):
        raise MatchError("invalid native Fate descriptor", -3)
    names = []
    for guardian in outcome.guardians[:outcome.guardian_count]:
        if not 1 <= guardian <= 10: raise MatchError("invalid native Fate guardian", -3)
        names.append(_GUARDIAN_NAMES[guardian-1])
    order = tuple(ItemSource(record.owner, record.token) for record in storage[:count])
    if len(set(order)) != count: raise MatchError("duplicate native Fate source", -3)
    result = FateAction(value.actor, value.token, PhenomenonAction(outcome.model,
        outcome.recipient if outcome.has_recipient else None, tuple(names)), ItemShuffleAction(order) if value.has_shuffle else None)
    try: _fate_action(result)
    except InvalidMatchError as error: raise MatchError("invalid native Fate shape", -3) from error
    return result

def _decode_guardian_payload(value, expected_source):
    if (value.source != expected_source or value.guardian not in (9, 10) or value.has_target != 1
        or not 1 <= value.model_count <= 18 or any(value.reserved) or any(value.models[value.model_count:])
        or any(not 1 <= model <= 296 for model in value.models[:value.model_count])):
        raise MatchError("invalid native guardian payload descriptor", -3)
    result = GuardianPayloadAction(value.source, "earth" if value.guardian == 9 else "moon",
                                   tuple(value.models[:value.model_count]), value.target)
    try: _guardian_payload_action(result)
    except InvalidMatchError as error: raise MatchError("invalid native guardian payload shape", -3) from error
    return result

class Match:
    """Owned mutable experimental match. Successful commands commit individually.

    Observe(None) is spectator; observe(player_id) includes that player's own hand.
    sample_action(rng) returns (Command, new_rng), changing neither game state nor
    the caller's integer. Rewards always follow status.player_ids, not actor.
    supported_models describes ordinary Command capability; gift_models and
    gift_weight describe the configured artifact distribution.
    Do not operate the same handle concurrently from several threads.
    """
    def __init__(self, players=None, seed=42, config=Config(), library: str | Path | None = None):
        self._handle = c.c_void_p()
        self._data = OfficialData(library); self._lib = lib = self._data._lib
        try:
            for name in ("abi_version", "catalog_count", "gift_weight", "driver_api_version"):
                fn = getattr(lib, "gfo_match_" + name); fn.argtypes = []; fn.restype = c.c_int
            for name in ("ruleset", "scope", "choice_rule", "driver_rule", "self_attack_rule", "artifact_grant_rule", "normal_grant_rule", "redraw_response_rule", "purchase_response_rule"):
                fn = getattr(lib, "gfo_match_" + name); fn.argtypes = []; fn.restype = c.c_char_p
            for name in ("config_size", "player_size", "command_size", "choice_size", "status_size", "guardian_action_size", "guardian_payload_action_size", "phenomenon_action_size", "item_source_size", "item_shuffle_action_size", "fate_action_size", "devil_action_size", "grant_action_size", "artifact_grant_action_size", "redraw_item_size", "redraw_result_size", "purchase_selection_size"):
                fn = getattr(lib, "gfo_match_" + name); fn.argtypes = []; fn.restype = c.c_size_t
        except AttributeError as error: raise MatchError("library does not support experimental match ABI") from error
        self.ruleset = lib.gfo_match_ruleset().decode(); self.scope = lib.gfo_match_scope().decode()
        self.self_attack_rule = lib.gfo_match_self_attack_rule().decode()
        self.artifact_grant_rule = lib.gfo_match_artifact_grant_rule().decode()
        self.normal_grant_rule = lib.gfo_match_normal_grant_rule().decode()
        self.redraw_response_rule = lib.gfo_match_redraw_response_rule().decode()
        self.purchase_response_rule = lib.gfo_match_purchase_response_rule().decode()
        self.driver_api_version = lib.gfo_match_driver_api_version()
        self.choice_rule = lib.gfo_match_choice_rule().decode(); self.driver_rule = lib.gfo_match_driver_rule().decode()
        if (lib.gfo_match_abi_version() != 2 or self.ruleset != "official-execution-subset-v65"
            or not self.scope.startswith("experimental:") or "not-official-server-equivalence" not in self.scope
            or lib.gfo_match_config_size() != c.sizeof(_Config) or _Config.external_grants.offset != 6 or _Config.include_fate.offset != 7
            or _Config.local_guardian_payloads.offset != 20 or _Config.external_artifacts.offset != 21
            or _Config.reserved_tail.offset != 22
            or lib.gfo_match_player_size() != c.sizeof(_Player)
            or lib.gfo_match_command_size() != c.sizeof(_Command) or lib.gfo_match_status_size() != c.sizeof(_Status)
            or self.driver_api_version != 1 or lib.gfo_match_choice_size() != c.sizeof(_Choice)
            or c.sizeof(_Choice) != 208 or _Choice.fate_actor.offset != 184 or _Choice.fate_token.offset != 192
            or _Choice.kind.offset != 200 or _Choice.reserved.offset != 201
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
            or lib.gfo_match_item_shuffle_action_size() != c.sizeof(_ItemShuffleAction) or _ItemShuffleAction.count.offset != 8
            or lib.gfo_match_fate_action_size() != c.sizeof(_FateAction) or _FateAction.token.offset != 8
            or _FateAction.outcome.offset != 16 or _FateAction.shuffle.offset != 40
            or _FateAction.has_shuffle.offset != 56 or _FateAction.reserved.offset != 57
            or lib.gfo_match_devil_action_size() != c.sizeof(_DevilAction) or _DevilAction.removed.offset != 8
            or _DevilAction.model.offset != 24 or _DevilAction.resource.offset != 26
            or _DevilAction.has_resource.offset != 27 or _DevilAction.removed_count.offset != 28 or _DevilAction.reserved.offset != 29
            or lib.gfo_match_grant_action_size() != c.sizeof(_DevilAction)
            or lib.gfo_match_purchase_selection_size() != c.sizeof(_PurchaseSelection)
            or c.sizeof(_PurchaseSelection) != 16 or _PurchaseSelection.has_commodity.offset != 8 or _PurchaseSelection.reserved.offset != 9
            or lib.gfo_match_redraw_item_size() != c.sizeof(_RedrawItem)
            or c.sizeof(_RedrawItem) != 8 or _RedrawItem.fake_model.offset != 2 or _RedrawItem.has_fake.offset != 4 or _RedrawItem.reserved.offset != 5
            or lib.gfo_match_redraw_result_size() != c.sizeof(_RedrawResult)
            or c.sizeof(_RedrawResult) != 16 or _RedrawResult.item_count.offset != 8
            or lib.gfo_match_artifact_grant_action_size() != c.sizeof(_ArtifactGrantAction)
            or c.sizeof(_ArtifactGrantAction) != 16 or _ArtifactGrantAction.model.offset != 8
            or _ArtifactGrantAction.fake_model.offset != 10 or _ArtifactGrantAction.reserved.offset != 12):
            raise MatchError("unsupported match ABI/scope")
        lib.gfo_match_supported.argtypes = [c.c_uint16]; lib.gfo_match_supported.restype = c.c_int
        lib.gfo_match_gift_supported.argtypes = [c.c_void_p, c.c_uint16, c.POINTER(c.c_uint8)]
        lib.gfo_match_gift_supported.restype = c.c_int
        lib.gfo_match_get_gift_weight.argtypes = [c.c_void_p, c.POINTER(c.c_uint16)]
        lib.gfo_match_get_gift_weight.restype = c.c_int
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
        lib.gfo_match_choice_legal.argtypes = [c.c_void_p, c.POINTER(_Choice), c.POINTER(c.c_uint8)]
        lib.gfo_match_choice_legal.restype = c.c_int
        lib.gfo_match_sample_choice.argtypes = [c.c_void_p, c.POINTER(c.c_uint64), c.POINTER(_Choice)]
        lib.gfo_match_sample_choice.restype = c.c_int
        lib.gfo_match_choice_step.argtypes = [c.c_void_p, c.POINTER(_Choice), c.POINTER(c.c_uint64), c.POINTER(_Status)]
        lib.gfo_match_choice_step.restype = c.c_int
        lib.gfo_match_rollout_step.argtypes = [c.c_void_p, c.POINTER(c.c_uint64), c.POINTER(c.c_uint64), c.POINTER(_Status)]
        lib.gfo_match_rollout_step.restype = c.c_int
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
        lib.gfo_match_fate_supported.argtypes = [c.c_uint16]; lib.gfo_match_fate_supported.restype = c.c_int
        lib.gfo_match_fate_legal.argtypes = [c.c_void_p, c.POINTER(_FateAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_fate_legal.restype = c.c_int
        lib.gfo_match_fate_step.argtypes = [c.c_void_p, c.POINTER(_FateAction), c.POINTER(_Status)]
        lib.gfo_match_fate_step.restype = c.c_int
        lib.gfo_match_devil_supported.argtypes = [c.c_uint16]; lib.gfo_match_devil_supported.restype = c.c_int
        lib.gfo_match_devil_legal.argtypes = [c.c_void_p, c.POINTER(_DevilAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_devil_legal.restype = c.c_int
        lib.gfo_match_devil_step.argtypes = [c.c_void_p, c.POINTER(_DevilAction), c.POINTER(_Status)]
        lib.gfo_match_devil_step.restype = c.c_int
        lib.gfo_match_grant_supported.argtypes = [c.c_uint16]; lib.gfo_match_grant_supported.restype = c.c_int
        lib.gfo_match_grant_legal.argtypes = [c.c_void_p, c.POINTER(_DevilAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_grant_legal.restype = c.c_int
        lib.gfo_match_grant_step.argtypes = [c.c_void_p, c.POINTER(_DevilAction), c.POINTER(_Status)]
        lib.gfo_match_grant_step.restype = c.c_int
        lib.gfo_match_artifact_grant_supported.argtypes = [c.c_uint16]; lib.gfo_match_artifact_grant_supported.restype = c.c_int
        lib.gfo_match_artifact_grant_legal.argtypes = [c.c_void_p, c.POINTER(_ArtifactGrantAction), c.POINTER(c.c_uint8)]
        lib.gfo_match_artifact_grant_legal.restype = c.c_int
        lib.gfo_match_artifact_grant_step.argtypes = [c.c_void_p, c.POINTER(_ArtifactGrantAction), c.POINTER(_Status)]
        lib.gfo_match_artifact_grant_step.restype = c.c_int
        lib.gfo_match_redraw_response_legal.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(_RedrawResult), c.c_size_t, c.POINTER(c.c_uint8)]
        lib.gfo_match_redraw_response_legal.restype = c.c_int
        lib.gfo_match_redraw_response_step.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(_RedrawResult), c.c_size_t, c.POINTER(_Status)]
        lib.gfo_match_redraw_response_step.restype = c.c_int
        lib.gfo_match_purchase_response_legal.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(_PurchaseSelection), c.POINTER(c.c_uint8)]
        lib.gfo_match_purchase_response_legal.restype = c.c_int
        lib.gfo_match_purchase_response_step.argtypes = [c.c_void_p, c.POINTER(_Command), c.POINTER(_PurchaseSelection), c.POINTER(_Status)]
        lib.gfo_match_purchase_response_step.restype = c.c_int

        lib.gfo_match_sample_action.argtypes = [c.c_void_p, c.POINTER(c.c_uint64), c.POINTER(_Command)]; lib.gfo_match_sample_action.restype = c.c_int
        lib.gfo_match_sample_confusion.argtypes = [c.c_void_p, c.POINTER(c.c_uint64), c.POINTER(_Command)]
        lib.gfo_match_sample_confusion.restype = c.c_int
        lib.gfo_match_sample_guardian_payload.argtypes = [c.c_void_p, c.c_uint64, c.POINTER(c.c_uint64), c.POINTER(_GuardianPayloadAction)]
        lib.gfo_match_sample_guardian_payload.restype = c.c_int
        lib.gfo_match_sample_grant.argtypes = [c.c_void_p, c.POINTER(c.c_uint64), c.POINTER(_DevilAction)]
        lib.gfo_match_sample_grant.restype = c.c_int
        lib.gfo_match_sample_fate.argtypes = [c.c_void_p, c.c_uint64, c.POINTER(c.c_uint64), c.POINTER(_ItemSource), c.c_size_t, c.POINTER(_FateAction)]
        lib.gfo_match_sample_fate.restype = c.c_int
        lib.gfo_match_observe.argtypes = [c.c_void_p, c.c_uint8, c.c_uint64, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
        lib.gfo_match_observe.restype = c.c_int
        self.supported_models = tuple(card.id for card in self._data if lib.gfo_match_supported(card.id))
        self.supported_guardian_models = tuple(card.id for card in self._data if lib.gfo_match_guardian_supported(card.id))
        self.supported_guardian_payload_models = {
            name: tuple(card.id for card in self._data if lib.gfo_match_guardian_payload_supported(kind, card.id))
            for name, kind in (("earth", 9), ("moon", 10))}
        self.supported_phenomenon_models = tuple(card.id for card in self._data if lib.gfo_match_phenomenon_supported(card.id))
        self.supported_item_shuffle_models = tuple(card.id for card in self._data if lib.gfo_match_item_shuffle_supported(card.id))
        self.supported_fate_models = tuple(card.id for card in self._data if lib.gfo_match_fate_supported(card.id))
        self.supported_devil_models = tuple(card.id for card in self._data if lib.gfo_match_devil_supported(card.id))
        # Static capability includes237 ordinary IDs; config and recipient conditions remain legal() checks.
        self.supported_grant_models = tuple(model for model in (0, *(card.id for card in self._data)) if lib.gfo_match_grant_supported(model))
        self.supported_artifact_grant_models = tuple(card.id for card in self._data if lib.gfo_match_artifact_grant_supported(card.id))
        base_weight = lib.gfo_match_gift_weight()
        if (len(self.supported_models) != lib.gfo_match_catalog_count() or not self.supported_models
            or base_weight != sum(self._data.get_card(id).gift_rate for id in self.supported_models)):
            raise MatchError("inconsistent match catalog descriptor")
        if self.supported_guardian_models != tuple(range(245, 287)):
            raise MatchError("inconsistent guardian action descriptor")
        config_record = _config(config)
        setups, copied = _players((PlayerSetup(0, "Player0"), PlayerSetup(1, "Player1")) if players is None else players)
        seed = _integer(seed, "seed", 0, 2**64 - 1)
        _check(lib.gfo_match_create(c.byref(config_record), setups, len(setups), seed, c.byref(self._handle)))
        self._players = copied; self.config = config
        gift_models = []
        for card in self._data:
            allowed = c.c_uint8()
            _check(lib.gfo_match_gift_supported(self._handle, card.id, c.byref(allowed)))
            if allowed.value > 1: raise MatchError("invalid configured gift membership", -3)
            if allowed.value: gift_models.append(card.id)
        current_weight = c.c_uint16()
        _check(lib.gfo_match_get_gift_weight(self._handle, c.byref(current_weight)))
        self.gift_models = tuple(gift_models); self.gift_weight = current_weight.value
        expected_models = tuple(sorted((*self.supported_models, 207))) if config.include_fate else self.supported_models
        if self.gift_models != expected_models or self.gift_weight != sum(self._data.get_card(id).gift_rate for id in self.gift_models):
            raise MatchError("inconsistent configured gift pool", -3)

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

    def fate_legal(self, action):
        """Validate owned207 and its supplied outcome as one atomic action; no selector."""
        self._open(); encoded, copied = _fate_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_fate_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def fate_step(self, action):
        self._open(); encoded, copied = _fate_action(action); result = _Status()
        _check(self._lib.gfo_match_fate_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def devil_legal(self, action):
        """Validate an externally supplied devil outcome, not an appearance selector."""
        self._open(); encoded = _devil_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_devil_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def devil_step(self, action):
        self._open(); encoded = _devil_action(action); result = _Status()
        _check(self._lib.gfo_match_devil_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def grant_legal(self, action):
        """Validate one externally supplied reserved-gift resolution; no selector."""
        self._open(); encoded = _grant_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_grant_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def grant_step(self, action):
        """Resolve one reserved gift with a supplied result; positive ordinary IDs do not sample.

        Use model0 for the existing weighted draw, including its Dream policy.
        Explicit ordinary results carry neither a resource nor removed tokens.
        """
        self._open(); encoded = _grant_action(action); result = _Status()
        _check(self._lib.gfo_match_grant_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def artifact_grant_legal(self, action):
        """Validate one explicit ordinary result, including an explicit absent disguise."""
        self._open(); encoded = _artifact_grant_action(action); result = c.c_uint8()
        _check(self._lib.gfo_match_artifact_grant_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def artifact_grant_step(self, action):
        """Fill one reserved debt without sampling a model or a Dream disguise."""
        self._open(); encoded = _artifact_grant_action(action); result = _Status()
        _check(self._lib.gfo_match_artifact_grant_step(self._handle, c.byref(encoded), c.byref(result)))
        return _decode_status(result)

    def redraw_response_legal(self, action):
        """Validate a complete supplied178 replacement response without drawing."""
        self._open(); response, results, copied = _redraw_response_action(action); out = c.c_uint8()
        _check(self._lib.gfo_match_redraw_response_legal(self._handle, c.byref(response), results if len(results) else None, len(results), c.byref(out)))
        return bool(out.value)

    def redraw_response_step(self, action):
        """Commit one response and all replacements atomically; no model selector.

        Supplied replacement rows use no game RNG. Body/guardian/followup effects
        retain their existing RNG behavior, and reserved gifts are not replaced.
        """
        self._open(); response, results, copied = _redraw_response_action(action); out = _Status()
        _check(self._lib.gfo_match_redraw_response_step(self._handle, c.byref(response), results if len(results) else None, len(results), c.byref(out)))
        return _decode_status(out)

    def purchase_response_legal(self, action):
        """Validate an externally selected BUY commodity without sampling it."""
        self._open(); response, selection = _purchase_response_action(action); out = c.c_uint8()
        _check(self._lib.gfo_match_purchase_response_legal(self._handle, c.byref(response), c.byref(selection), c.byref(out)))
        return bool(out.value)

    def purchase_response_step(self, action):
        """Resolve the response and supplied BUY selection as one transaction."""
        self._open(); response, selection = _purchase_response_action(action); out = _Status()
        _check(self._lib.gfo_match_purchase_response_step(self._handle, c.byref(response), c.byref(selection), c.byref(out)))
        return _decode_status(out)

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

    def choice_legal(self, choice: PlayerChoice):
        """Validate a Command or an owned207 request without selecting its outcome."""
        self._open(); encoded = _choice(choice); result = c.c_uint8()
        _check(self._lib.gfo_match_choice_legal(self._handle, c.byref(encoded), c.byref(result)))
        return bool(result.value)

    def sample_choice(self, policy_rng):
        """Return (choice,new_policy) without a play or chance draw.

        For separate choice_step, keep the old policy seed until that step succeeds.
        rollout_step combines both operations and commits their streams together.
        """
        self._open(); rng = c.c_uint64(_integer(policy_rng, "policy_rng", 0, 2**64-1)); result = _Choice()
        _check(self._lib.gfo_match_sample_choice(self._handle, c.byref(rng), c.byref(result)))
        return _decode_choice(result), rng.value

    def choice_step(self, choice: PlayerChoice, chance_rng):
        """Apply one external choice atomically; return (status,new_chance)."""
        self._open(); encoded = _choice(choice)
        rng = c.c_uint64(_integer(chance_rng, "chance_rng", 0, 2**64-1)); result = _Status()
        _check(self._lib.gfo_match_choice_step(self._handle, c.byref(encoded), c.byref(rng), c.byref(result)))
        return _decode_status(result), rng.value

    def rollout_step(self, policy_rng, chance_rng):
        """Execute exactly one decision; commit Match and both streams together.

        This local driver handles grant, confused main, ordinary player choices
        including207, responses and purchases. It does not claim server selectors.
        """
        self._open(); policy = c.c_uint64(_integer(policy_rng, "policy_rng", 0, 2**64-1))
        chance = c.c_uint64(_integer(chance_rng, "chance_rng", 0, 2**64-1)); result = _Status()
        _check(self._lib.gfo_match_rollout_step(self._handle, c.byref(policy), c.byref(chance), c.byref(result)))
        return _decode_status(result), policy.value, chance.value

    def sample_confusion(self, chance_rng):
        """Return a local visible-policy Command and new chance RNG for confusion.

        Sampling preserves the count and match. Use confusion_step separately
        to commit the command and debit one count; no server selector is implied.
        """
        self._open(); rng = c.c_uint64(_integer(chance_rng, "chance_rng", 0, 2**64-1)); result = _Command()
        _check(self._lib.gfo_match_sample_confusion(self._handle, c.byref(rng), c.byref(result)))
        return _decode_command(result), rng.value

    def sample_guardian_payload(self, source, chance_rng):
        """Return a local Earth/Moon payload and new chance RNG without a play.

        The source's guardian determines its domain; MP/hand do not filter the
        choice. This privileged local driver does not claim server selectors.
        """
        self._open(); source = _integer(source, "guardian source")
        rng = c.c_uint64(_integer(chance_rng, "chance_rng", 0, 2**64-1)); result = _GuardianPayloadAction()
        _check(self._lib.gfo_match_sample_guardian_payload(self._handle, source, c.byref(rng), c.byref(result)))
        return _decode_guardian_payload(result, source), rng.value

    def sample_grant(self, chance_rng):
        """Privileged local chance: returns (GrantAction,new_rng), without changing the match.

        Returns model0 or a devil, never an explicit ordinary-model result.
        Without authorized apocalypse devils, normal-only mode returns0 and
        preserves chance_rng; the normal artifact itself uses game RNG at step.
        Uses actual current tokens for243. This is separate from policy/game RNG
        and does not claim the server's resource or copy selection distribution.
        """
        self._open(); rng = c.c_uint64(_integer(chance_rng, "chance_rng", 0, 2**64 - 1)); result = _DevilAction()
        _check(self._lib.gfo_match_sample_grant(self._handle, c.byref(rng), c.byref(result)))
        return _decode_grant(result), rng.value

    def sample_fate(self, token, chance_rng):
        """Return a supplied local Fate choice and new chance RNG; never commit a play.

        The returned immutable manifest owns its references. This privileged
        driver uses all alive inventories for295; server selectors are unknown.
        """
        self._open(); token = _integer(token, "fate token")
        rng = c.c_uint64(_integer(chance_rng, "chance_rng", 0, 2**64-1)); storage = (_ItemSource * 162)(); result = _FateAction()
        _check(self._lib.gfo_match_sample_fate(self._handle, token, c.byref(rng), storage, len(storage), c.byref(result)))
        return _decode_fate(result, storage, token), rng.value

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

    def chance_rollout(self, policy_rng=1, chance_rng=2, max_steps=None):
        """Run a sequence of atomic decisions, returning immutable RolloutResult.

        The sequence is not one transaction. RolloutError.progress keeps status,
        both RNGs and the step count after the last successful decision, so a
        failed decision can be retried after its cause is resolved. Its cause
        and __cause__ preserve the original MatchError. No training is performed.
        """
        self._open(); policy_rng = _integer(policy_rng, "policy_rng", 0, 2**64-1)
        chance_rng = _integer(chance_rng, "chance_rng", 0, 2**64-1)
        if max_steps is not None: max_steps = _integer(max_steps, "max_steps", 0, 2**64-1)
        progress = RolloutResult(self.status, policy_rng, chance_rng, 0)
        while not progress.status.done and (max_steps is None or progress.steps < max_steps):
            try: status, policy_rng, chance_rng = self.rollout_step(progress.policy_rng, progress.chance_rng)
            except MatchError as error: raise RolloutError(error, progress) from error
            progress = RolloutResult(status, policy_rng, chance_rng, progress.steps+1)
        return progress

if __name__ == "__main__":
    library = sys.argv[1] if len(sys.argv) > 1 else None
    with Match(seed=42, library=library) as match:
        status, rng = match.random_rollout(12345)
        print(json.dumps({"ruleset": match.ruleset, "scope": match.scope, "catalog_count": len(match.supported_models),
            "gift_weight": match.gift_weight, "decisions": status.decisions, "winners": status.winners,
            "rewards": status.reward_by_player, "terminated": status.terminated, "truncated": status.truncated, "policy_rng": rng}))
