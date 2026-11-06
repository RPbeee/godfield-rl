"""Finite framing,privacy and filesystem regressions for the private parser."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "parse_official_webchannel.py"
SPEC = importlib.util.spec_from_file_location("official_webchannel_parser", SCRIPT)
parser = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(parser)


def frame(value):
    payload = json.dumps(value, ensure_ascii=False, separators=(",", ":"))
    return (str(len(payload.encode("utf-16-le")) // 2) + "\n" + payload).encode()


def firestore(value):
    if value is None:
        return {"nullValue": None}
    if type(value) is bool:
        return {"booleanValue": value}
    if type(value) is int:
        return {"integerValue": str(value)}
    if type(value) is float:
        return {"doubleValue": value}
    if isinstance(value, str):
        return {"stringValue": value}
    if isinstance(value, list):
        return {"arrayValue": {"values": [firestore(v) for v in value]}}
    return {"mapValue": {"fields": {k: firestore(v) for k, v in value.items()}}}


def game():
    return {"updateCount": 1, "gf": 1, "attackTurnPlayerId": 1,
            "updatedAt": "PRIVATE_TIMESTAMP", "events": [{"action": "startGame"}],
            "players": [
                {"id": 1, "name": "秘密の名前😃", "userId": "PRIVATE_USER",
                 "hp": 40, "mp": 10, "cp": 20,
                 "items": [{"id": 6, "modelId": 30}]},
                {"id": 2, "name": "非公開CPU", "userId": "PRIVATE_BOT", "isBot": True,
                 "hp": 40, "mp": 10, "cp": 20, "items": []}]}


def change(value=None, document="PRIVATE_ROOM_DOCUMENT"):
    return {"documentChange": {"document": {"name": document,
            "fields": {"game": firestore(game() if value is None else value),
                       "authentication": {"stringValue": "PRIVATE_AUTH"}}}}}


class ParserTests(unittest.TestCase):
    def test_utf16_japanese_emoji_redaction_and_source_hash(self):
        raw = frame([change()])
        result = parser.parse_prefix(raw)
        source = result["source"]
        self.assertEqual(source["captured_prefix_bytes"], len(raw))
        self.assertEqual(source["captured_prefix_sha256"], hashlib.sha256(raw).hexdigest())
        self.assertEqual(source["complete_frames_end_byte"], len(raw))
        self.assertEqual(source["unparsed_tail_bytes"], 0)
        self.assertEqual(source["complete_webchannel_frame_count"], 1)
        self.assertIsNone(source["http_response_complete"])
        got = result["games"][0]["game"]
        self.assertEqual([p["name"] for p in got["players"]], ["viewer", "cpu"])
        self.assertEqual(got["players"][0]["items"], [{"id": 6, "modelId": 30}])
        self.assertNotIn("curses", got["players"][0])
        encoded = json.dumps(result, ensure_ascii=False)
        for secret in ("PRIVATE", "秘密", "非公開", "userId", "updatedAt", "authentication"):
            self.assertNotIn(secret, encoded)

    def test_partial_utf8_header_and_frame_preserve_only_complete_prefix(self):
        first = frame([change()])
        for suffix, status, pending in [(b"123", "incomplete_length_header", 0),
                                        (b"12\n[", "incomplete_frame_payload", 0),
                                        (b"\xf0\x9f\x98", "incomplete_utf8_tail", 3)]:
            with self.subTest(status=status):
                result = parser.parse_prefix(first + suffix)
                self.assertEqual(result["game_change_count"], 1)
                self.assertEqual(result["source"]["complete_frames_end_byte"], len(first))
                self.assertEqual(result["source"]["unparsed_tail_bytes"], len(suffix))
                self.assertEqual(result["source"]["pending_utf8_bytes"], pending)
                self.assertEqual(result["source"]["tail_status"], status)
        second = frame([change()])
        emoji = second.index("😃".encode())
        result = parser.parse_prefix(first + second[:emoji + 2])
        self.assertEqual(result["game_change_count"], 1)
        self.assertEqual(result["source"]["pending_utf8_bytes"], 2)
        self.assertEqual(result["source"]["tail_status"], "incomplete_frame_payload")

    def test_complete_malformed_frame_cannot_be_swallowed_as_tail(self):
        for raw in (b"4\n[! ]", b"3\nNaN", b"5\n1e999", b"13\n{\"x\":1,\"x\":2}",
                    b"2\n\xf0\x9f\x98\x83", b"1\n\xff"):
            with self.subTest(raw=raw):
                with self.assertRaises(parser.ParseError):
                    parser.parse_prefix(raw)

    def test_xssi_only_at_start_and_opaque_local_documents(self):
        raw = b")]}'\n" + frame([change(), change()]) + frame([change(document="OTHER_PRIVATE_ROOM")])
        result = parser.parse_prefix(raw)
        self.assertEqual([g["document_sequence"] for g in result["games"]], [1, 1, 2])
        self.assertEqual([g["frame_index"] for g in result["games"]], [0, 0, 1])
        self.assertEqual(result["source"]["complete_frames_end_byte"], len(raw))
        self.assertEqual(parser.parse_prefix(b")]}")["source"]["tail_status"], "incomplete_xssi_prefix")
        self.assertEqual(parser.parse_prefix(b")]}'\n")["source"]["tail_status"], "no_complete_frames")
        self.assertEqual(parser.parse_prefix(frame([]) + b"\n")["source"]["unparsed_tail_bytes"], 1)
        with self.assertRaises(parser.ParseError):
            parser.parse_prefix(frame([]) + b")]}'\n")

    def test_all_firestore_value_types_and_explicit_viewer(self):
        values = {"integerValue": "-9"}, {"doubleValue": 1.25}, {"booleanValue": False}, {"nullValue": None}, {"timestampValue": "2026-11-06T00:00:00Z"}
        self.assertEqual([parser.unwrap(v) for v in values], [-9, 1.25, False, None, "2026-11-06T00:00:00Z"])
        self.assertEqual(parser.unwrap({"mapValue": {}}), {})
        self.assertEqual(parser.unwrap({"arrayValue": {}}), [])
        g = game()
        g["players"][1].pop("isBot")
        result = parser.parse_prefix(frame([change(g)]))
        self.assertEqual([p["name"] for p in result["games"][0]["game"]["players"]], ["player1", "player2"])
        result = parser.parse_prefix(frame([change(g)]), viewer_id=1)
        self.assertEqual(result["games"][0]["game"]["players"][0]["name"], "viewer")

    def test_unknown_identity_field_is_rejected_without_serialization(self):
        g = game()
        g["roomId"] = "PRIVATE_ROOM"
        with self.assertRaises(parser.ParseError):
            parser.parse_prefix(frame([change(g)]))


class CliTests(unittest.TestCase):
    def invoke(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), *map(str, args)],
                              text=True, capture_output=True, check=False)

    def test_exclusive_private_output_and_readonly_source(self):
        with tempfile.TemporaryDirectory() as directory:
            parent = Path(directory)
            source = parent / "raw.bin"
            raw = frame([change()])
            source.write_bytes(raw)
            source.chmod(0o400)
            before = source.stat()
            output = parent / "created" / "private.json"
            result = self.invoke("--input", source, "--out-file", output, "--viewer-id", 1)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(source.read_bytes(), raw)
            self.assertEqual(source.stat().st_mtime_ns, before.st_mtime_ns)
            self.assertEqual(stat.S_IMODE(source.stat().st_mode), 0o400)
            self.assertEqual(stat.S_IMODE(output.stat().st_mode), 0o600)
            self.assertEqual(stat.S_IMODE(output.parent.stat().st_mode), 0o700)
            self.assertNotIn("PRIVATE", result.stdout + result.stderr)
            stored = output.read_bytes()
            result = self.invoke("--input", source, "--out-file", output)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), stored)
            self.assertRegex(result.stderr, r"^error:[a-z_]+\n$")
            result = self.invoke("--input", source, "--out-file", source)
            self.assertEqual(result.stderr, "error:input_output_same_file\n")
            alias = parent / "same-inode.bin"
            os.link(source, alias)
            result = self.invoke("--input", source, "--out-file", alias)
            self.assertEqual(result.stderr, "error:input_output_same_file\n")

    def test_public_parent_and_raw_errors_are_generic(self):
        with tempfile.TemporaryDirectory() as directory:
            parent = Path(directory)
            source = parent / "SECRET_INPUT.bin"
            source.write_bytes(frame([]))
            public = parent / "public"
            public.mkdir(mode=0o755)
            result = self.invoke("--input", source, "--out-file", public / "out.json")
            self.assertEqual(result.stderr, "error:output_directory_not_private\n")
            source.write_bytes(b"4\n[! ]")
            result = self.invoke("--input", source, "--out-file", parent / "out.json")
            self.assertEqual(result.stderr, "error:invalid_complete_json_frame\n")
            self.assertFalse((parent / "out.json").exists())
            bad = game()
            bad["roomId"] = "PRIVATE_ROOM_TOKEN"
            source.write_bytes(frame([change(bad)]))
            result = self.invoke("--input", source, "--out-file", parent / "out.json")
            self.assertEqual(result.stderr, "error:unsupported_game_schema\n")
            self.assertFalse((parent / "out.json").exists())
            result = self.invoke("--viewer-id", "PRIVATE_BAD_ARGUMENT")
            self.assertEqual(result.stderr, "error:invalid_arguments\n")


if __name__ == "__main__":
    unittest.main()
