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
    def test_dying_player_ids_exact_roster_array(self):
        for dying in ([], [1], [2, 1]):
            for lenient in (False, True):
                value = game()
                value['dyingPlayerIds'] = dying
                decoded = parser.parse_prefix(frame([change(value)]), 1,
                    lenient_events=lenient)['games'][0]['game']
                self.assertEqual(decoded['dyingPlayerIds'], dying)

    def test_dying_player_ids_reject_invalid_values_even_when_lenient(self):
        for dying in (None, 1, 'player1', {}, [True], [1.0], ['1'],
                      [-1], [3], [1, 1], [[1]], [{'id': 1}]):
            for lenient in (False, True):
                value = game()
                value['dyingPlayerIds'] = dying
                with self.subTest(dying=dying, lenient=lenient), self.assertRaises(parser.ParseError) as caught:
                    parser.parse_prefix(frame([change(value)]), 1, lenient_events=lenient)
                self.assertEqual(str(caught.exception), 'unsupported_game_schema')
                self.assertEqual(caught.exception.detail['path'], 'game.dyingPlayerIds')

    def test_dying_player_ids_only_allowed_at_game_root(self):
        for location in ('players', 'attacks', 'events'):
            value = game()
            if location == 'players':
                value['players'][0]['dyingPlayerIds'] = []
            elif location == 'attacks':
                value['attacks'] = [{'dyingPlayerIds': []}]
            else:
                value['events'][0]['dyingPlayerIds'] = []
            with self.subTest(location=location), self.assertRaises(parser.ParseError):
                parser.parse_prefix(frame([change(value)]), 1)

    def test_lenient_events_are_replaced_individually_with_safe_metadata(self):
        events = [{"action": "removeItems", "items": []},
                  {"action": "dealDarkDamage", "damage": 7},
                  {"action": "hit", "index": 0},
                  {"action": "PRIVATE AUTH!", "name": "PRIVATE NAME"}, 42]
        value = game()
        for event in events:
            value["events"] = [{"action": "startGame"}, event]
            with self.assertRaises(parser.ParseError) as caught:
                parser.parse_prefix(frame([change(value)]), 1)
            if isinstance(event, dict) and event.get("action") == "removeItems":
                self.assertEqual(caught.exception.detail["action"], "removeItems")
            record = parser.parse_prefix(frame([change(value)]), 1, lenient_events=True)["games"][0]
            self.assertEqual(record["game"]["events"], [{"action": "startGame"}, {"action": "unknownEvent"}])
            metadata = record["replaced_events"][0]
            self.assertIn("event_keys", metadata)
            self.assertNotIn("PRIVATE", json.dumps(metadata))
            self.assertLessEqual(set(metadata), {"event_keys", "action", "event_key_other"})

    def test_lenient_action_must_be_an_event_not_another_symbol(self):
        value = game()
        value["events"] = [{"action": "cold"}]
        record = parser.parse_prefix(frame([change(value)]), 1, lenient_events=True)["games"][0]
        self.assertEqual(record["game"]["events"], [{"action": "unknownEvent"}])
        self.assertEqual(record["replaced_events"], [{"event_keys": ["action"], "action": "cold"}])

    def test_lenient_events_still_reject_authoritative_state(self):
        for location in ("players", "attacks", "top"):
            value = game()
            value["events"].append({"action": "dealDarkDamage", "damage": 7})
            if location == "players":
                value["players"][0]["unknownKey"] = 1
            elif location == "attacks":
                value["attacks"] = [{"unknownKey": 1}]
            else:
                value["unknownKey"] = 1
            with self.subTest(location=location), self.assertRaises(parser.ParseError):
                parser.parse_prefix(frame([change(value)]), 1, lenient_events=True)
        value = game()
        value["players"][0]["guardian"] = "unknownGuardian"
        with self.assertRaises(parser.ParseError):
            parser.parse_prefix(frame([change(value)]), 1, lenient_events=True)

    def test_sell_observed_shape_only(self):
        value = game()
        event = {"action": "sell", "item": {"id": 10, "modelId": 189}}
        value["events"] = [event]
        self.assertEqual(parser.parse_prefix(frame([change(value)]), 1)["games"][0]["game"]["events"], [event])
        for invalid in ({"action": "sell"},
                        {"action": "sell", "item": {"modelId": True}},
                        {"action": "sell", "item": {"modelId": 189}, "playerId": 1},
                        {"action": "unknown"}):
            value["events"] = [invalid]
            with self.assertRaises(parser.ParseError):
                parser.parse_prefix(frame([change(value)]), 1)

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

    def test_v91_danger_marker_is_preserved_and_unknown_shapes_fail_closed(self):
        # Offline public client SHA256 is pinned in the v91 witness:
        # case126 danger -> case128 A(2000) -> event loop; no payload read.
        g = game()
        g["events"] = [{"action": "danger"}, {"action": "dealDamage", "damage": 26}]
        self.assertEqual(parser.parse_prefix(frame([change(g)]))["games"][0]["game"]["events"], g["events"])
        for event in ({"action": "danger", "playerId": 2},
                      {"action": "danger", "damage": 26},
                      {"action": "danger", "items": []},
                      {"action": "dangerUnknown"},
                      {"action": "attractDanger"},
                      {"action": "danger", "secret": "PRIVATE_AUTH"}):
            with self.subTest(event=event):
                g["events"] = [event]
                with self.assertRaisesRegex(parser.ParseError, "^unsupported_game_schema$"):
                    parser.parse_prefix(frame([change(g)]))

    def test_v91_public_wire_roundtrip_and_witness_consistency(self):
        root = SCRIPT.parents[1] / "data" / "official-v2"
        wire = json.loads((root / "wire-game-updates-v91.json").read_text())
        witness = json.loads((root / "live-transition-witnesses-v91.json").read_text())
        self.assertEqual(wire["roles"], {"viewer_player_id": 1, "cpu_player_id": 2})
        for update in wire["updates"]:
            g = update["game"]
            self.assertEqual(parser.parse_prefix(frame([change(g)]), 1)["games"][0]["game"], g)
            self.assertFalse(any(r["modelId"] == 209 for p in g["players"] for r in p["items"]))
        before, after = [u["game"] for u in wire["updates"]]
        self.assertEqual([before["updateCount"], after["updateCount"]], [1, 2])
        self.assertEqual([before["gf"], after["gf"]], [1, 3])
        self.assertEqual(after["events"], witness["cases"][0]["received_ordered_effects"])
        self.assertEqual(after["events"][1], {"action": "danger"})
        self.assertEqual(after["events"][4], {"damage": 26, "action": "dealDamage"})
        self.assertEqual([[p[k] for k in ("hp", "mp", "cp")] for p in after["players"]],
                         [[40, 5, 20], [30, 14, 0]])
        self.assertEqual(wire["source_bodies"], witness["source_body_provenance"])
        for update, source in zip(wire["updates"], wire["source_bodies"]):
            self.assertEqual(source["selected_frame_indices_zero_based"], [update["source_frame_index"]])
            self.assertFalse(source["whole_http_response_claimed"])
            self.assertFalse(source["http_response_completed"])
        encoded = json.dumps(wire) + json.dumps(witness)
        for secret in ('"userId":', '"updatedAt":', '"documentChange":', '/tmp/', 'session-D'):
            self.assertNotIn(secret, encoded)

    def test_miss_exact_known_shape_only(self):
        g = game()
        g["events"] = [{"action": "miss", "playerId": 2}]
        self.assertEqual(parser.parse_prefix(frame([change(g)]))["games"][0]["game"]["events"], g["events"])
        for event in ({"action": "miss"}, {"action": "miss", "playerId": True},
                      {"action": "miss", "playerId": 2.0}, {"action": "miss", "playerId": "2"},
                      {"action": "miss", "playerId": None}, {"action": "miss", "playerId": -1},
                      {"action": "miss", "playerId": 99}, {"action": "miss", "playerId": []},
                      {"action": "miss", "playerId": 2, "damage": 0},
                      {"action": "miss", "playerId": 2, "targetPlayerId": 1},
                      {"action": "miss", "playerId": 2, "secret": "PRIVATE_AUTH"},
                      {"action": "missUnknown", "playerId": 2}):
            with self.subTest(event=event):
                g["events"] = [event]
                with self.assertRaisesRegex(parser.ParseError, "^unsupported_game_schema$"):
                    parser.parse_prefix(frame([change(g)]))

    def test_hit_and_removals_strict_shapes(self):
        for action in ('hit', 'removeGuardian', 'removeItems', 'removeUsedMiracles'):
            item = dict(id=10, modelId=223, fakeModelId=0, used=True)
            event = dict(action=action, playerId=2)
            if action in ('removeItems', 'removeUsedMiracles'):
                event['items'] = [item]
            value = game(); value['events'] = [event]
            self.assertEqual(parser.sanitize_game(value)['events'], [event])
            invalid = [dict(event, playerId=x) for x in (True, 2.0, '2', None, -1, 99, [], {})]
            invalid += [dict(event, damage=0), dict(event, userId='PRIVATE_AUTH')]
            invalid += [{k: v for k, v in event.items() if k != key} for key in event]
            if action in ('removeItems', 'removeUsedMiracles'):
                invalid += [dict(event, items=x) for x in (None, {}, 'private', [None])]
                for key in item:
                    invalid.append(dict(event, items=[{k: v for k, v in item.items() if k != key}]))
                for key, bad in [('id', True), ('id', -1), ('modelId', 0), ('modelId', 297),
                                 ('modelId', True), ('fakeModelId', None), ('fakeModelId', 297),
                                 ('used', 0), ('used', 'true')]:
                    invalid.append(dict(event, items=[dict(item, **{key: bad})]))
                invalid.append(dict(event, items=[dict(item, damage=0)]))
            for bad in invalid:
                with self.subTest(event=bad), self.assertRaises(parser.ParseError) as caught:
                    parser.sanitize_game(dict(value, events=[bad]))
                self.assertEqual(caught.exception.detail['event_keys'], sorted(k for k in bad if k != 'userId'))

    def test_purchase_model_integer_range(self):
        for model in (1, 296):
            value = game(); value['attacks'] = [dict(buyingItemModelId=model)]
            self.assertEqual(parser.sanitize_game(value)['attacks'][0]['buyingItemModelId'], model)
        for model in (0, 297, True, 1.0, '1', None):
            value = game(); value['attacks'] = [dict(buyingItemModelId=model)]
            with self.assertRaises(parser.ParseError):
                parser.sanitize_game(value)

    def test_v93_v94_wire_miss_and_record_only_witnesses(self):
        root = SCRIPT.parents[1] / "data" / "official-v2"
        for version, model, viewer, cpu, hp, gift in ((93, 111, 1, 2, 40, 180), (94, 108, 2, 1, 33, 212)):
            with self.subTest(version=version):
                wire = json.loads((root / f"wire-game-updates-v{version}.json").read_text())
                witness = json.loads((root / f"live-transition-witnesses-v{version}.json").read_text())
                self.assertEqual(wire["roles"], {"viewer_player_id": viewer, "cpu_player_id": cpu})
                for update in wire["updates"]:
                    g = update["game"]
                    self.assertEqual(parser.parse_prefix(frame([change(g)]), viewer)["games"][0]["game"], g)
                    player = next(p for p in g["players"] if p["id"] == viewer)
                    self.assertEqual([player[k] for k in ("hp", "mp", "cp")], [hp, 10, 20])
                    self.assertEqual(len(player["items"]), 9)
                before, after = [u["game"] for u in wire["updates"]]
                self.assertEqual(after["gf"], 3)
                self.assertEqual(after["events"][0]["items"][0]["modelId"], model)
                self.assertEqual(len(after["events"][0]["items"]), 1)
                self.assertEqual(after["events"][1], {"action": "miss", "playerId": cpu})
                self.assertEqual(after["events"][2]["item"]["modelId"], gift)
                self.assertEqual(after["events"], witness["cases"][0]["received_ordered_effects"])
                self.assertEqual(wire["source_bodies"], witness["source_body_provenance"])
                self.assertFalse(witness["cases"][0]["native_projection_covered"])
                self.assertFalse(witness["cases"][0]["confirmed_choice"]["self_target_confirmed"])
                self.assertEqual(witness["new_wire_checkpoint_comparisons"], 0)
                self.assertEqual(witness["cumulative_wire_checkpoint_comparisons"], 712)
                encoded = json.dumps(wire) + json.dumps(witness)
                for secret in ('"userId":', '"updatedAt":', '"documentChange":', '/tmp/', 'goodfield-E'):
                    self.assertNotIn(secret, encoded)

    def test_pinned_bounce_receiver_enum_without_claiming_live_observation(self):
        # Finite decoder input only,not a captured game. Pinned main.dart.js
        # SHA256 5ece06c224b6a7a5d877eb45d22ea18b82a03aaf1b7161cbbdf6a1e23ceab2cd:
        # case132 dispatches "bounce" to case138,which reads event.playerId.
        g = game()
        g["events"] = [{"action": "bounce", "playerId": 1}]
        result = parser.parse_prefix(frame([change(g)]))
        self.assertEqual(result["games"][0]["game"]["events"], g["events"])
        g["events"][0]["action"] = "unverifiedReaction"
        with self.assertRaisesRegex(parser.ParseError, "^unsupported_game_schema$"):
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



class DetailTests(unittest.TestCase):
    def error(self, value):
        with self.assertRaises(parser.ParseError) as caught:
            parser.parse_prefix(frame([change(value)]), 1)
        self.assertEqual(caught.exception.args, (str(caught.exception),))
        return caught.exception.detail

    def test_invalid_purchase_model_without_values(self):
        value = game()
        value['attacks'] = [{'itemModelIds': [5], 'buyingItemModelId': 987654321}]
        detail = self.error(value)
        self.assertEqual(detail, dict(path='game.attacks[].buyingItemModelId',
                                      value_type='int'))
        self.assertNotIn('987654321', json.dumps(detail))

    def test_enum_values_only_and_normalized_list_paths(self):
        for key in ('action', 'curse', 'curses', 'guardian'):
            value = game()
            value['events'] = [{key: ['newEnum'] if key == 'curses' else 'newEnum'}]
            detail = self.error(value)
            self.assertEqual(detail['value'], 'newEnum')
            self.assertEqual(detail['event_keys'], [key])
            self.assertEqual(detail['path'], 'game.events[].'+key+('[]' if key == 'curses' else ''))
        for key, text in [('action', 'Synthetic Name / PRIVATE_AUTH'), ('hp', 'SyntheticName'),
                          ('name', 'SyntheticOpponent'), ('action', 'x'*33)]:
            value = game(); value['events'] = [{key: text}]
            detail = self.error(value)
            self.assertNotIn('value', detail)
            self.assertEqual(detail['length'], len(text))
            self.assertEqual(detail['value_type'], 'str')
            self.assertNotIn(text, json.dumps(detail))

    def test_unknown_keys_are_bounded_sorted_and_masked(self):
        value = game()
        value['attacks'] = [dict.fromkeys(['z%02d' % i for i in range(20)] +
                                        ['PRIVATE / NAME', '1bad', 'x'*65], 987654321)]
        detail = self.error(value)
        self.assertEqual(detail['unknown_keys'], ['z%02d' % i for i in range(16)])
        self.assertEqual(detail['unknown_key_other'], 3)
        self.assertNotIn('987654321', json.dumps(detail))
        value['attacks'] = [{'PRIVATE / NAME': 987654321}]
        self.assertEqual(self.error(value), dict(path='game.attacks[].?',
                                               unknown_keys=[], unknown_key_other=1))

    def test_names_and_sensitive_subtrees_never_contribute_values_or_keys(self):
        value = game()
        value['players'][0]['name'] = 'SyntheticName'
        value['players'][1]['name'] = 'SyntheticOpponent'
        value['events'] = [{'name': {'SyntheticName': 'PRIVATE_AUTH'}}]
        detail = self.error(value)
        self.assertEqual(detail['path'], 'game.events[].?.?')
        self.assertEqual(detail['unknown_keys'], [])
        for secret in ('SyntheticName', 'SyntheticOpponent', 'PRIVATE_AUTH'):
            self.assertNotIn(secret, json.dumps(detail))
        value['events'] = [{'action': 'newAction'}]
        value['players'][0]['userId'] = {'SyntheticName': 'PRIVATE_AUTH'}
        self.assertEqual(self.error(value)['value'], 'newAction')
        value['players'][0]['name'] = 987654321
        detail = self.error(value)
        self.assertEqual(detail, dict(path='game.players[].?', value_type='int'))

    def test_unknown_event_keys_are_safe_and_bounded(self):
        value = game()
        value['events'] = [dict(action='newAction', playerId=2, items=[],
                                 name='SyntheticName', **{'PRIVATE / NAME': 987654321})]
        detail = self.error(value)
        self.assertEqual(detail['event_keys'], ['action', 'items', 'playerId'])
        self.assertEqual(detail['event_key_other'], 2)
        for secret in ('SyntheticName', 'PRIVATE / NAME', '987654321'):
            self.assertNotIn(secret, json.dumps(detail))
        value['events'] = [dict(action='newAction', **{'z%02d' % i: 0 for i in range(20)})]
        self.assertEqual(len(self.error(value)['event_keys']), 16)

    def test_type_failure_contains_no_numeric_value(self):
        value = game(); value['gf'] = 987654321.5
        self.assertEqual(self.error(value), dict(path='game.gf', value_type='float'))


class FrequentEventTests(unittest.TestCase):
    def test_observed_exact_shapes(self):
        from copy import deepcopy
        item = dict(id=7, modelId=6, fakeModelId=0, used=False)
        events = [dict(action='pray'),
                  *[dict(action=a, cp=-7) for a in ('takeCP','boostCP','boostCPOfEverybody')],
                  dict(action='upgradeDisease', curse='fever', playerId=1),
                  dict(action='upgradeDisease', curse='hell', playerId=2, natural=False),
                  dict(action='sell', item=item, overflowItem=item)]
        for event in events:
            with self.subTest(action=event['action']):
                g = game();g['events']=[event]
                self.assertEqual(parser.sanitize_game(g,1)['events'],[event])
                for key,value in [('extra',0),('userId','private'),('name','private')]:
                    bad=deepcopy(g);bad['events'][0][key]=value
                    with self.assertRaises(parser.ParseError): parser.sanitize_game(bad,1)
                    replaced=[]
                    sanitized=parser.sanitize_game(bad,1,lenient_events=True,replaced_events=replaced)
                    self.assertEqual(sanitized['events'],[dict(action='unknownEvent')])
                    self.assertEqual(len(replaced),1)

    def test_new_payload_types_are_strict(self):
        from copy import deepcopy
        item = dict(id=7, modelId=6, fakeModelId=0, used=False)
        bad = [dict(action=a,cp=v) for a in ('takeCP','boostCP','boostCPOfEverybody')
               for v in (True,1.5,'7',None)]
        bad += [dict(action='upgradeDisease',curse=c,playerId=1) for c in ('fog','none','unknown')]
        bad += [dict(action='upgradeDisease',curse='fever',playerId=p) for p in (True,99,'1')]
        bad += [dict(action='upgradeDisease',curse='fever',playerId=1,natural=n) for n in (1,None,'true')]
        for field,value in [('id',True),('modelId',0),('fakeModelId',297),('used',1)]:
            overflow=deepcopy(item);overflow[field]=value
            bad.append(dict(action='sell',item=item,overflowItem=overflow))
        for event in bad:
            with self.subTest(event=event):
                g=game();g['events']=[event]
                with self.assertRaises(parser.ParseError): parser.sanitize_game(g,1)


if __name__ == "__main__":
    unittest.main()
