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
    """Only fixed codes, never raw values, are safe to show to the caller."""


GAME_KEYS = frozenset("""
    players diseasePlayerId guardianPlayerIds gf events tiebreakGF
    attackTurnPlayerId updateCount attacks isOver
    id hp mp cp name isBot dead guardian curses items
    modelId fakeModelId used atk itemModelIds playerId targetPlayerId reversed
    action attack bought curse damage item itemModelId
""".split())
GAME_STRINGS = frozenset("""
    startGame gift advanceGF useAttackItems setTargetPlayer dealDamage addCurse
    disease useDefenseItems removeCurses die endGame boostHP boostMP exchange
    buy canNotBuy setBought useDevilItem removeSomething selfCurse redraw
    replaceItems safe block setGuardian attackByGuardian counterAttack
    nextAttack bounce reflect phenomenon setCurseOfEverybody danger miss
    cold fever hell heaven darkcloud dream fog flash
    mercury venus earth mars jupiter saturn uranus neptune pluto moon
    viewer cpu
""".split())
REDACTED_KEYS = frozenset({"userId", "updatedAt"})


def _error(code: str) -> None:
    raise ParseError(code)


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


def _valid_string(value):
    if not isinstance(value, str) or any(0xD800 <= ord(c) <= 0xDFFF for c in value):
        _error("invalid_string")
    return value


def unwrap(value):
    """Decode just the supported Firestore Value types, with no coercion."""
    if not isinstance(value, dict) or len(value) != 1:
        _error("invalid_firestore_value")
    kind, data = next(iter(value.items()))
    if kind == "mapValue":
        if not isinstance(data, dict) or set(data) - {"fields"}:
            _error("invalid_firestore_value")
        fields = data.get("fields", {})
        if not isinstance(fields, dict):
            _error("invalid_firestore_value")
        return {_valid_string(k): unwrap(v) for k, v in fields.items()}
    if kind == "arrayValue":
        if not isinstance(data, dict) or set(data) - {"values"}:
            _error("invalid_firestore_value")
        values = data.get("values", [])
        if not isinstance(values, list):
            _error("invalid_firestore_value")
        return [unwrap(v) for v in values]
    if kind == "integerValue":
        if type(data) is int:
            number = data
        elif isinstance(data, str) and re.fullmatch(r"-?(?:0|[1-9][0-9]*)", data):
            number = int(data)
        else:
            _error("invalid_firestore_value")
        if not -(1 << 63) <= number < (1 << 63):
            _error("invalid_firestore_value")
        return number
    if kind == "doubleValue":
        if type(data) not in (int, float) or not math.isfinite(data):
            _error("invalid_firestore_value")
        return data
    if kind == "booleanValue":
        if type(data) is not bool:
            _error("invalid_firestore_value")
        return data
    if kind in ("stringValue", "timestampValue"):
        return _valid_string(data)
    if kind == "nullValue" and data is None:
        return None
    _error("unsupported_firestore_value")


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


def _validate_game_values(value):
    if isinstance(value, dict):
        if set(value) - GAME_KEYS:
            _error("unsupported_game_schema")
        for child in value.values():
            _validate_game_values(child)
    elif isinstance(value, list):
        for child in value:
            _validate_game_values(child)
    elif isinstance(value, str):
        if value not in GAME_STRINGS and not re.fullmatch(r"player[0-9]+", value):
            _error("unsupported_game_schema")
    elif value is not None and type(value) not in (int, float, bool):
        _error("unsupported_game_schema")
    elif type(value) is float and not math.isfinite(value):
        _error("nonfinite_json_number")


def sanitize_game(game, viewer_id=None):
    game = _redact(game)
    if not isinstance(game, dict) or not isinstance(game.get("players"), list):
        _error("invalid_game_schema")
    for key in ("updateCount", "gf", "attackTurnPlayerId"):
        if type(game.get(key)) is not int:
            _error("invalid_game_schema")
    players = game["players"]
    if not players or any(not isinstance(p, dict) or type(p.get("id")) is not int
                          or p["id"] < 0 for p in players):
        _error("invalid_game_schema")
    ids = [p["id"] for p in players]
    if len(set(ids)) != len(ids) or (viewer_id is not None and viewer_id not in ids):
        _error("invalid_game_schema")
    if viewer_id is None and len(players) == 2:
        humans = [p["id"] for p in players if p.get("isBot") is not True]
        bots = [p["id"] for p in players if p.get("isBot") is True]
        if len(humans) == len(bots) == 1:
            viewer_id = humans[0]
    for player in players:
        if "name" in player:
            _valid_string(player["name"])
            player["name"] = ("viewer" if player["id"] == viewer_id else
                              "cpu" if viewer_id is not None and player.get("isBot") is True else
                              f"player{player['id']}")
    # Captured client danger dispatch only waits 2000ms; the observed marker
    # carries no target, damage or other payload. Fail closed on new shapes.
    for event in game.get("events", []):
        if isinstance(event, dict) and event.get("action") == "danger":
            if event != {"action": "danger"}:
                _error("unsupported_game_schema")
    # Pinned client dispatch reads only miss.playerId for its visual effect.
    # Accept exactly the observed recipient marker, never inferred damage.
    for event in game.get("events", []):
        if isinstance(event, dict) and event.get("action") == "miss":
            if (set(event) != {"action", "playerId"}
                    or type(event["playerId"]) is not int
                    or event["playerId"] not in ids):
                _error("unsupported_game_schema")
    _validate_game_values(game)
    return game


def parse_prefix(raw: bytes, viewer_id=None):
    """Return only complete, strictly parsed frames; leave an unfinished tail.

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
            game = sanitize_game(unwrap(document["fields"]["game"]), viewer_id)
            sequence = document_sequences.setdefault(source_name, len(document_sequences) + 1)
            games.append({"frame_index": frame_index, "frame_end_byte": end_byte,
                          "document_sequence": sequence, "game": game})
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
