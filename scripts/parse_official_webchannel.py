#!/usr/bin/env python3
"""Extract sanitized games from complete frames in a private response prefix.

Only Python's standard library is used. Raw input is read-only. Unknown game
fields/string values fail closed rather than accidentally publishing identity
or transport data. HTTP completion cannot be inferred from a content prefix.
"""
from __future__ import annotations

import argparse
import bisect
import codecs
import hashlib
import json
import math
import os
from pathlib import Path
import re
import stat
import sys


class ParseError(Exception):
    """Fixed code in args/str; only allowlisted metadata in detail."""
    def __init__(self, code, detail=None):
        super().__init__(code)
        self.detail = detail or {}


GAME_KEYS = frozenset("""
    players diseasePlayerId guardianPlayerIds gf events tiebreakGF
    attackTurnPlayerId updateCount attacks isOver
    id hp mp cp name isBot dead guardian curses items
    modelId fakeModelId used atk itemModelIds playerId targetPlayerId reversed
    action attack bought curse damage item itemModelId buyingItemModelId natural overflowItem
""".split())
GAME_STRINGS = frozenset("""
    startGame gift advanceGF useAttackItems setTargetPlayer dealDamage addCurse
    disease useDefenseItems removeCurses die endGame boostHP boostMP exchange
    buy canNotBuy setBought useDevilItem removeSomething selfCurse redraw
    replaceItems safe block setGuardian attackByGuardian counterAttack
    nextAttack bounce reflect phenomenon setCurseOfEverybody danger miss hit removeGuardian removeItems removeUsedMiracles sell pray takeCP boostCP boostCPOfEverybody upgradeDisease upgradeHeaven dealDarkDamage revive collectCPOfEverybody
    cold fever hell heaven darkcloud dream fog flash
    mercury venus earth mars jupiter saturn uranus neptune pluto moon
    viewer cpu
""".split())
EVENT_ACTIONS = frozenset("""
    startGame gift advanceGF useAttackItems setTargetPlayer dealDamage addCurse
    disease useDefenseItems removeCurses die endGame boostHP boostMP exchange
    buy canNotBuy setBought useDevilItem removeSomething selfCurse redraw
    replaceItems safe block setGuardian attackByGuardian counterAttack
    nextAttack bounce reflect phenomenon setCurseOfEverybody danger miss hit
    removeGuardian removeItems removeUsedMiracles sell pray takeCP boostCP boostCPOfEverybody upgradeDisease upgradeHeaven dealDarkDamage revive collectCPOfEverybody
""".split())
REDACTED_KEYS = frozenset({"userId", "updatedAt"})


# Only these game fields carry public symbolic values in GAME_STRINGS.
# name is deliberately excluded, including before names are normalized.
ENUM_KEYS = frozenset({"action", "curse", "curses", "guardian"})
KEY_PATTERN = re.compile(r"[A-Za-z][A-Za-z0-9_]{0,63}\Z")
VALUE_PATTERN = re.compile(r"[A-Za-z][A-Za-z0-9_]{0,31}\Z")


def _path(path, key):
    safe = key if ("?" not in path and isinstance(key, str) and KEY_PATTERN.fullmatch(key)
                   and key not in REDACTED_KEYS | {"name"}) else "?"
    return path + "." + safe


def _detail(value, path, key=None):
    result = dict(path=path, value_type=type(value).__name__)
    if isinstance(value, (str, list, dict)):
        result["length"] = len(value)
    if "?" not in path and key in ENUM_KEYS and isinstance(value, str) and VALUE_PATTERN.fullmatch(value):
        result["value"] = value
    if isinstance(value, dict) and path == "game.events[]":
        result.update(_event_keys(value))
    return result


def _event_keys(event):
    safe = sorted(k for k in event if isinstance(k, str) and KEY_PATTERN.fullmatch(k)
                  and k not in REDACTED_KEYS | {"name"})
    result = dict(event_keys=safe[:16])
    action = event.get("action")
    if isinstance(action, str) and VALUE_PATTERN.fullmatch(action):
        result["action"] = action
    if len(event) > len(safe):
        result["event_key_other"] = len(event) - len(safe)
    return result


def _unknown(value, allowed, path):
    keys = set(value) - allowed
    safe = sorted(k for k in keys if "?" not in path and isinstance(k, str) and KEY_PATTERN.fullmatch(k)
                  and k not in REDACTED_KEYS | {"name"})
    result = dict(path=_path(path, next(iter(keys))) if len(keys) == 1 else path,
                  unknown_keys=safe[:16])
    other = len(keys) - len(safe)
    if other:
        result["unknown_key_other"] = other
    return result


def _error(code: str, detail=None) -> None:
    raise ParseError(code, detail)


def _json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            _error("duplicate_json_key")
        result[key] = value
    return result


def _bad_constant(_value):
    _error("nonfinite_json_number")


def _finite_float(value):
    number = float(value)
    if not math.isfinite(number):
        _error("nonfinite_json_number")
    return number


def _valid_string(value, path="game"):
    if not isinstance(value, str) or any(0xD800 <= ord(c) <= 0xDFFF for c in value):
        _error("invalid_string", _detail(value, path))
    return value


def unwrap(value, path="game"):
    """Decode just the supported Firestore Value types, with no coercion."""
    if not isinstance(value, dict) or len(value) != 1:
        _error("invalid_firestore_value", _detail(value, path))
    kind, data = next(iter(value.items()))
    if kind == "mapValue":
        if not isinstance(data, dict) or set(data) - {"fields"}:
            _error("invalid_firestore_value", _detail(data, path))
        fields = data.get("fields", {})
        if not isinstance(fields, dict):
            _error("invalid_firestore_value", _detail(data, path))
        return {_valid_string(k, _path(path, k)): unwrap(v, _path(path, k)) for k, v in fields.items()}
    if kind == "arrayValue":
        if not isinstance(data, dict) or set(data) - {"values"}:
            _error("invalid_firestore_value", _detail(data, path))
        values = data.get("values", [])
        if not isinstance(values, list):
            _error("invalid_firestore_value", _detail(data, path))
        return [unwrap(v, path + "[]") for v in values]
    if kind == "integerValue":
        if type(data) is int:
            number = data
        elif isinstance(data, str) and re.fullmatch(r"-?(?:0|[1-9][0-9]*)", data):
            number = int(data)
        else:
            _error("invalid_firestore_value", _detail(data, path))
        if not -(1 << 63) <= number < (1 << 63):
            _error("invalid_firestore_value", _detail(data, path))
        return number
    if kind == "doubleValue":
        if type(data) not in (int, float) or not math.isfinite(data):
            _error("invalid_firestore_value", _detail(data, path))
        return data
    if kind == "booleanValue":
        if type(data) is not bool:
            _error("invalid_firestore_value", _detail(data, path))
        return data
    if kind in ("stringValue", "timestampValue"):
        return _valid_string(data, path)
    if kind == "nullValue" and data is None:
        return None
    _error("unsupported_firestore_value", _detail(data, path))


def _walk(value):
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from _walk(child)
    elif isinstance(value, list):
        for child in value:
            yield from _walk(child)


def _redact(value):
    if isinstance(value, dict):
        return {k: _redact(v) for k, v in value.items() if k not in REDACTED_KEYS}
    if isinstance(value, list):
        return [_redact(v) for v in value]
    return value


def _validate_game_values(value, path="game", key=None):
    if isinstance(value, dict):
        try:
            allowed = GAME_KEYS | {"dyingPlayerIds"} if path == "game" else GAME_KEYS
            if path == "game.events[]" and value.get("action") in ("counterAttack", "selfCurse"):
                allowed = allowed | {"index"}
            if path == "game.attacks[]":
                allowed = allowed | {"hitRate"}
            if set(value) - allowed:
                _error("unsupported_game_schema", _unknown(value, allowed, path))
            for child_key, child in value.items():
                _validate_game_values(child, _path(path, child_key), child_key)
        except ParseError as error:
            if path == "game.events[]":
                error.detail.update(_event_keys(value))
            raise
    elif isinstance(value, list):
        for child in value:
            _validate_game_values(child, path + "[]", key)
    elif isinstance(value, str):
        if value not in GAME_STRINGS and not re.fullmatch(r"player[0-9]+", value):
            _error("unsupported_game_schema", _detail(value, path, key))
    elif value is not None and type(value) not in (int, float, bool):
        _error("unsupported_game_schema", _detail(value, path))
    elif type(value) is float and not math.isfinite(value):
        _error("nonfinite_json_number", _detail(value, path))


def _sanitize_game(game, viewer_id=None):
    # Validate observed new forms before redaction can remove extra keys.
    if isinstance(game, dict) and isinstance(game.get('events'), list):
        ids = [p.get('id') for p in game.get('players', []) if isinstance(p, dict)]
        def item_ok(item):
            return (isinstance(item, dict) and set(item) == {'id','modelId','fakeModelId','used'}
                    and type(item['id']) is int and item['id'] >= 0
                    and type(item['modelId']) is int and 1 <= item['modelId'] <= 296
                    and type(item['fakeModelId']) is int and 0 <= item['fakeModelId'] <= 296
                    and type(item['used']) is bool)
        def sale_item_ok(item):
            return (isinstance(item, dict) and not set(item) - {'id','modelId','fakeModelId','used'}
                    and type(item.get('modelId')) is int and 1 <= item['modelId'] <= 296
                    and ('id' not in item or (type(item['id']) is int and item['id'] >= 0))
                    and ('fakeModelId' not in item or (type(item['fakeModelId']) is int and 0 <= item['fakeModelId'] <= 296))
                    and ('used' not in item or type(item['used']) is bool))
        response_count = None
        for e in game['events']:
            if not isinstance(e, dict):
                continue
            a = e.get('action'); valid = True
            if a == 'useDefenseItems' and isinstance(e.get('items'), list):
                response_count = len(e['items'])
            if a in ('counterAttack', 'selfCurse'):
                # ks.k/a6 indexes the selected response cards, NOT attacks[].
                # A receipt may begin after the selection marker: inventory cap
                # is then the only known bound. Never infer from final hand.
                bound = 18 if response_count is None else min(18, response_count)
                valid = (set(e) == {'action'} or (set(e) == {'action','index'}
                         and type(e['index']) is int and 0 <= e['index'] < bound))
            elif a in ('upgradeHeaven', 'collectCPOfEverybody'):
                valid = (set(e) == {'action','playerId'} and type(e['playerId']) is int
                         and e['playerId'] in ids)
            elif a == 'dealDarkDamage':
                valid = set(e) == {'action','damage'} and type(e['damage']) is int
            elif a == 'revive':
                valid = (set(e) == {'action','playerId','item'} and type(e['playerId']) is int
                         and e['playerId'] in ids and item_ok(e['item']))
            elif a == 'pray':
                valid = set(e) == {'action'}
            elif a in ('takeCP','boostCP','boostCPOfEverybody'):
                valid = set(e) == {'action','cp'} and type(e['cp']) is int
            elif a == 'upgradeDisease':
                valid = (set(e) in ({'action','curse','playerId'}, {'action','curse','playerId','natural'})
                         and e['curse'] in ('cold','fever','hell','heaven')
                         and type(e['playerId']) is int and e['playerId'] in ids
                         and ('natural' not in e or type(e['natural']) is bool))
            elif a == 'sell':
                # A.to explicitly decodes null overflow as no eviction. Keep
                # the existing compact commodity DTO, with typed optional keys.
                valid = (set(e) in ({'action','item'}, {'action','item','overflowItem'})
                         and sale_item_ok(e['item'])
                         and (e.get('overflowItem') is None or item_ok(e['overflowItem'])))
            if not valid:
                _error('unsupported_game_schema', _detail(e, 'game.events[]'))
    # Check exact event keys before redaction can erase an unexpected payload.
    if isinstance(game, dict) and isinstance(game.get("events"), list):
        for event in game["events"]:
            if isinstance(event, dict) and event.get("action") in ("hit", "miss", "removeGuardian", "removeItems", "removeUsedMiracles"):
                expected = {"action", "playerId"}
                if event["action"] in ("removeItems", "removeUsedMiracles"):
                    expected.add("items")
                    if isinstance(event.get("items"), list) and any(
                            isinstance(item, dict) and set(item) != {"id", "modelId", "fakeModelId", "used"}
                            for item in event["items"]):
                        _error("unsupported_game_schema", _detail(event, "game.events[]"))
                if (set(event) != expected and not (event["action"] == "removeItems"
                        and set(event) == {"action", "items"})):
                    _error("unsupported_game_schema", _detail(event, "game.events[]"))
    game = _redact(game)
    if not isinstance(game, dict):
        _error("invalid_game_schema", _detail(game, "game"))
    if not isinstance(game.get("players"), list):
        _error("invalid_game_schema", _detail(game.get("players"), "game.players"))
    for key in ("updateCount", "gf", "attackTurnPlayerId"):
        if type(game.get(key)) is not int:
            _error("invalid_game_schema", _detail(game.get(key), _path("game", key)))
    players = game["players"]
    if not players:
        _error("invalid_game_schema", _detail(players, "game.players"))
    for player in players:
        if not isinstance(player, dict):
            _error("invalid_game_schema", _detail(player, "game.players[]"))
        if type(player.get("id")) is not int or player["id"] < 0:
            _error("invalid_game_schema", _detail(player.get("id"), "game.players[].id"))
    ids = [p["id"] for p in players]
    if len(set(ids)) != len(ids) or (viewer_id is not None and viewer_id not in ids):
        _error("invalid_game_schema")
    if "dyingPlayerIds" in game:
        # Server death-processing metadata; the pinned client tn decoder ignores
        # it. Accept only roster IDs, without inferring dead/revive/attack order.
        dying = game["dyingPlayerIds"]
        if (not isinstance(dying, list)
                or any(type(pid) is not int or pid not in ids for pid in dying)
                or len(set(dying)) != len(dying)):
            _error("unsupported_game_schema", _detail(dying, "game.dyingPlayerIds"))
    if viewer_id is None and len(players) == 2:
        humans = [p["id"] for p in players if p.get("isBot") is not True]
        bots = [p["id"] for p in players if p.get("isBot") is True]
        if len(humans) == len(bots) == 1:
            viewer_id = humans[0]
    for player in players:
        if "name" in player:
            _valid_string(player["name"], "game.players[].?")
            player["name"] = ("viewer" if player["id"] == viewer_id else
                              "cpu" if viewer_id is not None and player.get("isBot") is True else
                              f"player{player['id']}")
    # Captured client danger dispatch only waits 2000ms; the observed marker
    # carries no target, damage or other payload. Fail closed on new shapes.
    for event in game.get("events", []):
        if isinstance(event, dict) and event.get("action") == "danger":
            if event != {"action": "danger"}:
                _error("unsupported_game_schema", _detail(event, "game.events[]"))
    # Pinned client dispatch uses hit/miss.playerId for public visual effects.
    # Accept exactly the observed recipient marker, never inferred damage.
    for event in game.get("events", []):
        if isinstance(event, dict) and event.get("action") == "sell":
            if (set(event) not in ({"action", "item"}, {"action", "item", "overflowItem"}) or not isinstance(event["item"], dict)
                    or set(event["item"]) - {"id", "modelId", "fakeModelId", "used"}
                    or type(event["item"].get("modelId")) is not int
                    or not 1 <= event["item"]["modelId"] <= 296):
                _error("unsupported_game_schema", _detail(event, "game.events[]"))
        if isinstance(event, dict) and event.get("action") in ("miss", "hit", "removeGuardian"):
            if (set(event) != {"action", "playerId"}
                    or type(event["playerId"]) is not int
                    or event["playerId"] not in ids):
                _error("unsupported_game_schema", _detail(event, "game.events[]"))
        if isinstance(event, dict) and event.get("action") in ("removeItems", "removeUsedMiracles"):
            # Inferred from kY: require every decoded field, with no coercion.
            if (set(event) not in ({"action", "playerId", "items"}, {"action", "items"})
                    or ("playerId" in event and (type(event["playerId"]) is not int or event["playerId"] not in ids))
                    or (event["action"] == "removeUsedMiracles" and "playerId" not in event)
                    or not isinstance(event["items"], list)
                    or any(not isinstance(item, dict)
                           or set(item) != {"id", "modelId", "fakeModelId", "used"}
                           or type(item["id"]) is not int or item["id"] < 0
                           or type(item["modelId"]) is not int or not 1 <= item["modelId"] <= 296
                           or type(item["fakeModelId"]) is not int or not 0 <= item["fakeModelId"] <= 296
                           or type(item["used"]) is not bool for item in event["items"])):
                _error("unsupported_game_schema", _detail(event, "game.events[]"))
    for attack in game.get("attacks", []):
        if isinstance(attack, dict) and "hitRate" in attack:
            # Public attack DTO: integer percentage; zero means catalog fallback.
            rate = attack["hitRate"]
            if type(rate) is not int or not 0 <= rate <= 100:
                _error("unsupported_game_schema", _detail(rate, "game.attacks[].hitRate"))
        if isinstance(attack, dict) and "buyingItemModelId" in attack:
            model = attack["buyingItemModelId"]
            if type(model) is not int or not 1 <= model <= 296:
                _error("unsupported_game_schema", _detail(model, "game.attacks[].buyingItemModelId"))
    _validate_game_values(game)
    for event in game.get("events", []):
        if not isinstance(event, dict) or not isinstance(event.get("action"), str):
            _error("unsupported_game_schema", _detail(event, "game.events[]"))
    return game


def sanitize_game(game, viewer_id=None, *, lenient_events=False, replaced_events=None):
    if not lenient_events or not isinstance(game, dict) or not isinstance(game.get("events"), list):
        return _sanitize_game(game, viewer_id)
    # Validate the entire authoritative state first. Only individual events may
    # be discarded; errors anywhere else retain the strict behavior.
    sanitized = _sanitize_game(dict(game, events=[]), viewer_id)
    response = None
    for event in game["events"]:
        try:
            if (not isinstance(event, dict) or not isinstance(event.get("action"), str)
                    or event["action"] not in EVENT_ACTIONS):
                _error("unsupported_game_schema")
            prefix = [response] if response is not None and event['action'] in ('counterAttack', 'selfCurse') else []
            checked = _sanitize_game(dict(game, events=prefix + [event]), viewer_id)
        except ParseError:
            sanitized["events"].append({"action": "unknownEvent"})
            if replaced_events is not None:
                replaced_events.append(_event_keys(event) if isinstance(event, dict) else {"event_keys": []})
        else:
            sanitized["events"].append(checked["events"][-1])
            if event['action'] == 'useDefenseItems':
                response = event
    return sanitized


def parse_prefix(raw: bytes, viewer_id=None, *, lenient_events=False):
    """Return complete frames; leave an unfinished tail. Strict by default.

    lenient_events replaces only rejected events with empty unknownEvent
    markers and returns safe diagnostics in each record's replaced_events.
    The transport length is a JavaScript string length (UTF-16 code units),
    not a UTF-8 byte length or Python Unicode code-point count.
    """
    decoder = codecs.getincrementaldecoder("utf-8")("strict")
    try:
        text = decoder.decode(raw, final=False)
    except UnicodeDecodeError:
        _error("invalid_utf8")
    pending_utf8 = len(decoder.getstate()[0])
    utf16_offsets, byte_offsets = [0], [0]
    for char in text:
        utf16_offsets.append(utf16_offsets[-1] + (2 if ord(char) > 0xFFFF else 1))
        byte_offsets.append(byte_offsets[-1] + len(char.encode("utf-8")))
    cursor, complete_end = 0, 0
    frames = []
    tail_status = "complete_frame_boundary"
    xssi = ("\x29\x5d\x7d\x27\n", "\x29\x5d\x7d\x27\r\n")
    if text.startswith(")"):
        marker = next((marker for marker in xssi if text.startswith(marker)), None)
        if marker is not None:
            cursor = len(marker)
        elif any(marker.startswith(text) for marker in xssi):
            tail_status = "incomplete_xssi_prefix"
            cursor = len(text)
        else:
            _error("invalid_framing")
    while cursor < len(text):
        while cursor < len(text) and text[cursor] in " \t\r\n":
            cursor += 1
        if cursor == len(text):
            break
        newline = text.find("\n", cursor)
        if newline < 0:
            if not re.fullmatch(r"[0-9]+\r?", text[cursor:]):
                _error("invalid_length_header")
            tail_status = "incomplete_length_header"
            break
        header = text[cursor:newline].removesuffix("\r")
        if not re.fullmatch(r"[0-9]+", header) or len(header) > 16:
            _error("invalid_length_header")
        length = int(header)
        if not 0 < length <= 9007199254740991:
            _error("invalid_length_header")
        start = newline + 1
        wanted = utf16_offsets[start] + length
        if wanted > utf16_offsets[-1]:
            tail_status = "incomplete_frame_payload"
            break
        end = bisect.bisect_left(utf16_offsets, wanted)
        if utf16_offsets[end] != wanted:
            _error("invalid_utf16_boundary")
        try:
            frame = json.loads(text[start:end], object_pairs_hook=_json_object,
                               parse_constant=_bad_constant, parse_float=_finite_float)
        except (json.JSONDecodeError, ValueError):
            _error("invalid_complete_json_frame")
        frames.append((frame, byte_offsets[end]))
        cursor = end
        complete_end = byte_offsets[end]
    if tail_status == "complete_frame_boundary":
        if pending_utf8:
            tail_status = "incomplete_utf8_tail"
        elif complete_end < len(raw):
            tail_status = "no_complete_frames" if not frames else "trailing_framing_whitespace"
    games, document_sequences = [], {}
    for frame_index, (frame, end_byte) in enumerate(frames):
        for obj in _walk(frame):
            change = obj.get("documentChange")
            if not isinstance(change, dict):
                continue
            document = change.get("document", {})
            if not isinstance(document, dict) or not isinstance(document.get("fields", {}), dict):
                _error("invalid_game_document")
            if "game" not in document.get("fields", {}):
                continue
            source_name = document.get("name")
            if not isinstance(source_name, str) or not source_name:
                _error("invalid_game_document")
            decoded = unwrap(document["fields"]["game"])
            replaced = []
            game = (sanitize_game(decoded, viewer_id, lenient_events=True, replaced_events=replaced)
                    if lenient_events else sanitize_game(decoded, viewer_id))
            sequence = document_sequences.setdefault(source_name, len(document_sequences) + 1)
            games.append({"frame_index": frame_index, "frame_end_byte": end_byte,
                          "document_sequence": sequence, "game": game})
            if replaced:
                games[-1]["replaced_events"] = replaced
    return {
        "schema_version": 1,
        "schema": "official-webchannel-prefix-games-v1",
        "source": {
            "capture_extent": "response_prefix",
            "http_response_complete": None,
            "captured_prefix_bytes": len(raw),
            "captured_prefix_sha256": hashlib.sha256(raw).hexdigest(),
            "framing_length_unit": "utf16_code_units",
            "complete_webchannel_frame_count": len(frames),
            "complete_frames_end_byte": complete_end,
            "unparsed_tail_bytes": len(raw) - complete_end,
            "pending_utf8_bytes": pending_utf8,
            "tail_status": tail_status,
        },
        "document_sequence_scope": "Opaque numbers local to this input only;no source document identity is serialized.",
        "game_change_count": len(games),
        "games": games,
    }


class _Arguments(argparse.ArgumentParser):
    def error(self, _message):
        _error("invalid_arguments")


def _private_parent(path):
    missing = []
    cursor = path
    while not cursor.exists():
        missing.append(cursor)
        if cursor.parent == cursor:
            _error("invalid_output_directory")
        cursor = cursor.parent
    for directory in reversed(missing):
        os.mkdir(directory, 0o700)
    info = path.lstat()
    if not stat.S_ISDIR(info.st_mode) or stat.S_IMODE(info.st_mode) != 0o700:
        _error("output_directory_not_private")
    return os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)


def main(argv=None):
    try:
        parser = _Arguments(description=__doc__)
        parser.add_argument("--input", required=True, help="Private immutable response prefix")
        parser.add_argument("--out-file", required=True, help="New JSON in a mode0700 directory")
        parser.add_argument("--viewer-id", type=int, help="Optional numeric in-game viewer ID")
        options = parser.parse_args(argv)
        source = Path(os.path.abspath(options.input))
        output = Path(os.path.abspath(options.out_file))
        if source == output or (output.exists() and os.path.samefile(source, output)):
            _error("input_output_same_file")
        with os.fdopen(os.open(source, os.O_RDONLY | os.O_NOFOLLOW), "rb") as handle:
            if not stat.S_ISREG(os.fstat(handle.fileno()).st_mode):
                _error("input_not_regular_file")
            result = parse_prefix(handle.read(), options.viewer_id)
        encoded = (json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False) + "\n").encode("utf-8")
        parent = _private_parent(output.parent)
        created = False
        try:
            fd = os.open(output.name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                         0o600, dir_fd=parent)
            created = True
            with os.fdopen(fd, "wb") as handle:
                os.fchmod(handle.fileno(), 0o600)
                handle.write(encoded)
        except Exception:
            if created:
                os.unlink(output.name, dir_fd=parent)
            raise
        finally:
            os.close(parent)
        # Metadata is safe;neither the filename nor the game literals go to stdout.
        print(json.dumps({"source": result["source"], "game_change_count": result["game_change_count"]}))
        return 0
    except ParseError as error:
        print("error:" + str(error), file=sys.stderr)
    except OSError:
        print("error:io_error", file=sys.stderr)
    except Exception:
        print("error:parser_error", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
