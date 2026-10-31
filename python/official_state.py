"""Strict public wire-snapshot import and immutable fresh-view projection.

NOT an Env: no reset/step, server transitions, RNG or match connection. Known wire
fields use exact JSON types and JS-safe integers; malformed values are rejected
rather than silently coerced like the browser decoder. Missing optional fields
use its documented defaults. Extra keys/attacks/events/ratings are deep-copied in
raw_snapshot, never replayed, inferred as timing context or included in the view.
raw_snapshot is privileged input evidence: never expose it to an agent as a view.

Physical item modelId0 is an empty UI slot, not an imported inventory instance.
Tokens/player IDs may be0 and need not equal seats. Players<=9; foreign inventory
has no guessed16/18 cap. Own fresh view/selection uses the proven18 renderer bound.
"""
from __future__ import annotations
import copy
import ctypes as c
from dataclasses import dataclass
import json
import math
from pathlib import Path
import sys
try:
    from .official_data import OfficialData
except ImportError:
    from official_data import OfficialData

MAX_WIRE_INTEGER = 9007199254740991
CURSES = ("", "cold", "fever", "hell", "heaven", "fog", "dream", "flash", "darkcloud")
GUARDIANS = ("", "mars", "mercury", "jupiter", "saturn", "uranus", "pluto", "neptune", "venus", "earth", "moon")
CATEGORIES = ("trade", "weapons", "armor", "sundries", "miracles", "devils", "guardians", "phenomena")
ELEMENTS = ("fire", "water", "wood", "stone", "light", "darkness")

class _Item(c.Structure):
    _fields_ = [("token", c.c_uint64), ("model", c.c_uint16), ("fake_model", c.c_uint16), ("used", c.c_uint8), ("reserved", c.c_uint8 * 3)]
class _Player(c.Structure):
    _fields_ = [("id", c.c_uint64), ("user_id", c.c_char_p), ("user_id_size", c.c_size_t),
        ("name", c.c_char_p), ("name_size", c.c_size_t), ("alias", c.c_char_p), ("alias_size", c.c_size_t),
        ("hp", c.c_int64), ("mp", c.c_int64), ("cp", c.c_int64), ("confusion_count", c.c_int64),
        ("item_offset", c.c_size_t), ("item_count", c.c_size_t), ("curse_offset", c.c_size_t), ("curse_count", c.c_size_t),
        ("team", c.c_uint8), ("guardian", c.c_uint8), ("dead", c.c_uint8), ("reserved", c.c_uint8 * 5)]
class _Meta(c.Structure):
    _fields_ = [("gf", c.c_int64), ("tiebreak_gf", c.c_int64), ("update_count", c.c_uint64),
                ("has_tiebreak", c.c_uint8), ("is_over", c.c_uint8), ("reserved", c.c_uint8 * 6)]
class _Response(c.Structure):
    _fields_ = [("flash", c.c_uint8), ("incoming_category", c.c_uint8), ("incoming_element", c.c_int8),
                ("incoming_is_attack", c.c_uint8), ("reserved", c.c_uint8 * 4)]

@dataclass(frozen=True)
class ResponseContext:
    """Visible response context, not inferred from raw attacks or server timing.

    flash is accepted for ABI symmetry but ignored by C++: the viewer's snapshot
    curse determines it, so callers cannot bypass/introduce flash selection limits.
    """
    incoming_category: str = "weapons"
    incoming_element: str | None = None
    incoming_is_attack: bool = True
    flash: bool = False

def _integer(value, label, minimum=-MAX_WIRE_INTEGER, maximum=MAX_WIRE_INTEGER):
    if type(value) is not int or not minimum <= value <= maximum:
        raise ValueError(f"{label}: expected an integer in [{minimum}, {maximum}]")
    return value

def _boolean(value, label):
    if type(value) is not bool:
        raise ValueError(f"{label}: expected a boolean")
    return value

def _string(value, label, optional=False):
    if optional and value is None:
        return None
    if type(value) is not str:
        raise ValueError(f"{label}: expected {'string or null' if optional else 'string'}")
    try:
        value.encode("utf-8", errors="strict")
    except UnicodeError as error:
        raise ValueError(f"{label}: invalid Unicode") from error
    return value

def _list(value, label):
    if type(value) is not list:
        raise ValueError(f"{label}: expected an array")
    return value

def _object(value, label):
    if type(value) is not dict:
        raise ValueError(f"{label}: expected an object")
    return value

def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result

def _bad_constant(value):
    raise ValueError(f"non-JSON numeric constant: {value}")

def _json_value(value):
    # Extras remain opaque finite JSON (including fractional numbers). Every
    # integer value is JS-safe; projected integer fields additionally reject floats.
    if value is None or type(value) is bool:
        return
    if type(value) is str:
        _string(value, "JSON string"); return
    if type(value) is int:
        _integer(value, "JSON integer")
        return
    if type(value) is float:
        if not math.isfinite(value):
            raise ValueError("nonfinite JSON number")
        return
    if type(value) is list:
        for item in value: _json_value(item)
        return
    if type(value) is dict:
        for key, item in value.items():
            _string(key, "JSON object key"); _json_value(item)
        return
    raise ValueError("snapshot contains a non-JSON value")

def _load(source):
    if isinstance(source, (str, bytes, bytearray)):
        try:
            raw = json.loads(source, object_pairs_hook=_pairs, parse_constant=_bad_constant)
        except (UnicodeError, json.JSONDecodeError) as error:
            raise ValueError("invalid snapshot JSON") from error
    elif type(source) is dict:
        _json_value(source)
        raw = copy.deepcopy(source)
    else:
        raise ValueError("snapshot must be a JSON object or JSON text")
    _object(raw, "snapshot"); _json_value(raw)
    return raw

def _enum(value, values, label, optional=False):
    if optional and value is None:
        return 0
    if type(value) is not str or value not in values:
        raise ValueError(f"{label}: unknown name")
    return values.index(value)

def _normalize(raw, model_count):
    players = _list(raw.get("players", []), "players")
    if len(players) > 9:
        raise ValueError("players: at most9")
    meta = _Meta()
    meta.gf = _integer(raw.get("gf", 0), "gf")
    tie = raw.get("tiebreakGF")
    if tie is not None:
        meta.tiebreak_gf = _integer(tie, "tiebreakGF"); meta.has_tiebreak = 1
    meta.is_over = _boolean(raw.get("isOver", False), "isOver")
    meta.update_count = _integer(raw.get("updateCount", 0), "updateCount", 0)
    if "attackTurnPlayerId" in raw:
        _integer(raw["attackTurnPlayerId"], "attackTurnPlayerId", 0)
    # These lists are preserved in full, not reduced to the browser's head attack.
    for key in ("attacks", "events"):
        if key in raw:
            for entry in _list(raw[key], key): _object(entry, key + " entry")
    flat_players, flat_items, flat_curses, ids = [], [], [], set()
    users = []
    for index, source in enumerate(players):
        p = _object(source, f"players[{index}]")
        value = _Player(); value.id = _integer(p.get("id"), "player.id", 0)
        if value.id in ids: raise ValueError("duplicate player ID")
        ids.add(value.id)
        user = _string(p.get("userId"), "userId", optional=True)
        alias = _string(p.get("alias"), "alias", optional=True)
        name = _string(p.get("name", ""), "name")
        for key, text in (("user_id", user), ("alias", alias), ("name", name)):
            data = None if text is None else text.encode("utf-8")
            setattr(value, key, data); setattr(value, key + "_size", 0 if data is None else len(data))
        users.append((user, value.id))
        team = p.get("team", 0)
        value.team = _integer(0 if team is None else team, "team", 0, 4)
        value.guardian = _enum(p.get("guardian"), GUARDIANS, "guardian", optional=True)
        value.hp = _integer(p.get("hp", 0), "hp")
        value.mp = _integer(p.get("mp", 0), "mp")
        value.cp = _integer(p.get("cp", 0), "cp")
        value.confusion_count = _integer(p.get("confusionCount", 0), "confusionCount")
        value.dead = _boolean(p.get("dead", False), "dead")
        for key in ("rating", "ratingGain"):
            if key in p: _integer(p[key], key)
        if "isBot" in p: _boolean(p["isBot"], "isBot")
        value.curse_offset = len(flat_curses)
        for curse in _list(p.get("curses", []), "curses"):
            enum = _enum(curse, CURSES, "curse")
            if not enum: raise ValueError("empty curse is not an active curse")
            flat_curses.append(enum)
        value.curse_count = len(flat_curses) - value.curse_offset
        value.item_offset = len(flat_items); tokens = set()
        for source_item in _list(p.get("items", []), "items"):
            item = _object(source_item, "item")
            token = _integer(item.get("id"), "item.id", 0)
            if token in tokens: raise ValueError("duplicate per-player item token")
            tokens.add(token)
            model = _integer(item.get("modelId"), "modelId", 1, model_count)
            fake = _integer(item.get("fakeModelId", 0), "fakeModelId", 0, model_count)
            used = _boolean(item.get("used", False), "used")
            flat_items.append(_Item(token=token, model=model, fake_model=fake, used=used))
        value.item_count = len(flat_items) - value.item_offset
        flat_players.append(value)
    return meta, flat_players, flat_items, flat_curses, users

class OfficialState:
    """Owned immutable frame. raw_snapshot is evidence, view() is the projection.

    Explicitly choose view(player_id=...), view(user_id=...) or spectator=True.
    Unknown/ambiguous user IDs fail rather than silently granting spectator view.
    """
    def __init__(self, snapshot, library: str | Path | None = None):
        self._handle = c.c_void_p()
        self._data = OfficialData(library)
        self._lib = lib = self._data._lib
        try:
            for name in ("abi_version", "item_size", "player_size", "meta_size", "response_size"):
                fn = getattr(lib, "gfo_state_" + name); fn.argtypes = []; fn.restype = c.c_int if name == "abi_version" else c.c_size_t
        except AttributeError as error:
            raise RuntimeError("library does not support official state extension") from error
        if (lib.gfo_state_abi_version() != 1 or lib.gfo_state_item_size() != c.sizeof(_Item)
            or lib.gfo_state_player_size() != c.sizeof(_Player) or lib.gfo_state_meta_size() != c.sizeof(_Meta)
            or lib.gfo_state_response_size() != c.sizeof(_Response)):
            raise RuntimeError("unsupported official state ABI")
        lib.gfo_state_create.argtypes = [c.POINTER(_Meta), c.POINTER(_Player), c.c_size_t, c.POINTER(_Item), c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t, c.POINTER(c.c_void_p)]
        lib.gfo_state_create.restype = c.c_int
        lib.gfo_state_destroy.argtypes = [c.c_void_p]; lib.gfo_state_destroy.restype = None
        lib.gfo_state_view_json.argtypes = [c.c_void_p, c.c_uint8, c.c_uint64, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
        lib.gfo_state_view_json.restype = c.c_int
        lib.gfo_state_can_append.argtypes = [c.c_void_p, c.c_uint64, c.POINTER(c.c_uint64), c.c_size_t, c.c_uint64, c.POINTER(_Response), c.POINTER(c.c_uint8)]
        lib.gfo_state_can_append.restype = c.c_int
        lib.gfo_state_selection_ready.argtypes = [c.c_void_p, c.c_uint64, c.POINTER(c.c_uint64), c.c_size_t, c.POINTER(_Response), c.POINTER(c.c_uint8)]
        lib.gfo_state_selection_ready.restype = c.c_int
        raw = _load(snapshot)
        meta, players, items, curses, users = _normalize(raw, len(self._data))
        pa = (_Player * len(players))(*players); ia = (_Item * len(items))(*items); ca = (c.c_uint8 * len(curses))(*curses)
        self._check(lib.gfo_state_create(c.byref(meta), pa, len(pa), ia, len(ia), ca, len(ca), c.byref(self._handle)))
        self._raw = raw; self._users = users; self._ids = {p.id for p in players}

    @classmethod
    def from_file(cls, path: str | Path, library: str | Path | None = None):
        return cls(Path(path).read_bytes(), library)

    @staticmethod
    def _check(status):
        if status: raise ValueError(f"official state operation rejected (status {status})")

    @property
    def raw_snapshot(self):
        return copy.deepcopy(self._raw)

    @property
    def closed(self):
        return not bool(self._handle.value)

    def _open(self):
        if self.closed: raise ValueError("official state is closed")

    def close(self):
        if getattr(self, "_handle", None) and self._handle.value:
            self._lib.gfo_state_destroy(self._handle); self._handle = c.c_void_p()

    def __enter__(self):
        self._open(); return self

    def __exit__(self, *unused):
        self.close()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def _viewer(self, player_id, user_id, spectator=False):
        if type(spectator) is not bool: raise ValueError("spectator must be boolean")
        if sum((player_id is not None, user_id is not None, spectator)) != 1:
            raise ValueError("choose exactly one player_id, user_id or explicit spectator")
        if spectator: return 0, 0
        if user_id is not None:
            user_id = _string(user_id, "viewer user_id")
            matches = [id for user, id in self._users if user is not None and user == user_id]
            if len(matches) != 1: raise ValueError("unknown or ambiguous viewer user_id")
            player_id = matches[0]
        player_id = _integer(player_id, "viewer player_id", 0)
        if player_id not in self._ids: raise ValueError("unknown viewer player_id")
        return 1, player_id

    def view(self, *, player_id=None, user_id=None, spectator=False):
        self._open(); present, viewer = self._viewer(player_id, user_id, spectator)
        size = c.c_size_t()
        self._check(self._lib.gfo_state_view_json(self._handle, present, viewer, None, 0, c.byref(size)))
        buffer = c.create_string_buffer(size.value)
        self._check(self._lib.gfo_state_view_json(self._handle, present, viewer, buffer, len(buffer), c.byref(size)))
        return json.loads(buffer.raw[:-1].decode("utf-8"))

    @staticmethod
    def _selected(tokens):
        try: iterator = iter(tokens)
        except TypeError as error: raise ValueError("selected must be an iterable of item tokens") from error
        ordered = []
        for token in iterator:
            ordered.append(token)
            if len(ordered) > 18: raise ValueError("at most18 selected tokens")
        return (c.c_uint64 * len(ordered))(*[_integer(t, "selected token", 0) for t in ordered])

    @staticmethod
    def _response(response):
        if response is None: return None
        if not isinstance(response, ResponseContext): raise ValueError("response must be ResponseContext or None")
        category = _enum(response.incoming_category, CATEGORIES, "incoming_category")
        element = -1 if response.incoming_element is None else _enum(response.incoming_element, ELEMENTS, "incoming_element")
        return _Response(flash=_boolean(response.flash, "response.flash"), incoming_category=category,
                         incoming_element=element, incoming_is_attack=_boolean(response.incoming_is_attack, "incoming_is_attack"))

    def can_append_hand(self, selected, next_token, *, player_id=None, user_id=None, response=None):
        self._open(); _, viewer = self._viewer(player_id, user_id)
        tokens = self._selected(selected); next_token = _integer(next_token, "next token", 0)
        context = self._response(response); out = c.c_uint8()
        self._check(self._lib.gfo_state_can_append(self._handle, viewer, tokens, len(tokens), next_token,
                                                 None if context is None else c.byref(context), c.byref(out)))
        return bool(out.value)

    def selection_ready(self, selected, *, player_id=None, user_id=None, response=None):
        self._open(); _, viewer = self._viewer(player_id, user_id)
        tokens = self._selected(selected); context = self._response(response); out = c.c_uint8()
        self._check(self._lib.gfo_state_selection_ready(self._handle, viewer, tokens, len(tokens),
                                                      None if context is None else c.byref(context), c.byref(out)))
        return bool(out.value)

if __name__ == "__main__":
    library = sys.argv[1] if len(sys.argv) > 1 else None
    example = {"players": [{"id": 40, "userId": "local-demo", "name": "預言者", "hp": 40, "mp": 10, "cp": 20,
                 "items": [{"id": 0, "modelId": 6}, {"id": 99, "modelId": 235, "fakeModelId": 13}]}],
               "gf": 1, "events": [], "attacks": []}
    with OfficialState(example, library) as state:
        print(json.dumps(state.view(user_id="local-demo"), ensure_ascii=False))
        print("selection_ready=" + str(state.selection_ready([0], player_id=40)))
