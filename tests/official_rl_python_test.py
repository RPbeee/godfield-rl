import random
import sys
import time
import unittest
from pathlib import Path
from types import SimpleNamespace
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_match import Command, Config, Match, MatchError, InvalidMatchError, UnsupportedMatchError
from official_state import OfficialState
from official_rl import enumerate_commands, pending_kind, _enumerate_commands_python
LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else None

KINDS = ("play", "defend", "discard", "pray", "exchange", "sell", "buy", "accept", "decline", "sacrifice")
def key(command):
    return (KINDS.index(command.kind), command.items, -1 if command.target is None else command.target,
            command.allocation or (-1, -1, -1))

class EnumerateCommandsTest(unittest.TestCase):
    def audit(self, match, limit=2, exchange=0):
        before = [match.observe(), match.observe(match.status.actor)]
        commands = match.legal_commands(limit, exchange)
        self.assertEqual(set(commands), set(_enumerate_commands_python(match, limit, exchange)))
        self.assertEqual(commands, sorted(set(commands), key=key))
        self.assertEqual(commands, match.legal_commands(limit, exchange))
        self.assertEqual(commands, enumerate_commands(match, limit, exchange))
        self.assertTrue(all(match.legal(command) for command in commands))
        self.assertEqual(before, [match.observe(), match.observe(match.status.actor)])
        return commands

    def scene(self, match, own, enemy=(), **extra):
        raw = {"gf": 2, "players": [
            {"id": 0, "hp": 40, "mp": 10, "cp": 20,
             "items": [{"id": i, "modelId": model} for i, model in enumerate(own)]},
            {"id": 77, "hp": 40, "mp": 10, "cp": 90,
             "items": [{"id": i, "modelId": model} for i, model in enumerate(enemy)]}]}
        raw.update(extra)
        with OfficialState(raw, library=LIBRARY) as state:
            match.load_scene(state, 0, 42)

    def test_random_oracle_and_samples(self):
        picker = random.Random(14)
        phases = set()
        checked = sampled_count = triples = 0
        for external in (False, True):
            for fate in (False, True):
                with Match(config=Config(external_grants=external, include_fate=fate), library=LIBRARY) as match:
                    for seed in range(8):
                        match.reset(seed)
                        policy, chance = seed + 1, seed + 2
                        for index in range(15):
                            if match.status.done:
                                break
                            if pending_kind(match):
                                match_status, policy, chance = match.rollout_step(policy, chance)
                                continue
                            phases.add(match.status.phase)
                            commands = self.audit(match)
                            checked += 1
                            if index == 0 and seed < 2:
                                self.audit(match, 3)
                                triples += 1
                            sampled, policy = match.sample_action(policy)
                            if len(sampled.items) <= 2 and sampled.kind != "exchange":
                                sampled_count += 1
                                self.assertIn(sampled, commands)
                            match.step(picker.choice(commands) if picker.random() < .5 else sampled)
        self.assertGreater(checked, 300)
        self.assertGreater(sampled_count, 250)
        self.assertTrue({"main", "response"} <= phases)
        print(f"Python random oracle: scenes={checked}, max_items3={triples}, sample membership={sampled_count}, phases={sorted(phases)}")

    def test_trade_purchase_response_sacrifice_and_exchange(self):
        with Match(config=Config(include_fate=True), library=LIBRARY) as match:
            self.scene(match, [3, 4, 5, 199, 207], [199])
            for limit in (1, 2, 3):
                commands = self.audit(match, limit, 10)
                self.assertTrue(any(command.kind == "exchange" for command in commands))
                self.assertEqual(any(command.kind == "sell" for command in commands), limit >= 2)
            self.audit(match, 1, 2**31 - 1)
            self.assertNotIn("exchange", {command.kind for command in match.legal_commands()})
            # Buy a commodity from self takes no response; foreign buy does.
            match.step(Command(0, "buy", (2,), 77))
            self.assertEqual(match.status.phase, "response")
            self.audit(match, 3)
            match.step(Command(77, "defend"))
            self.assertEqual(match.status.phase, "purchase")
            self.assertEqual({command.kind for command in self.audit(match)}, {"accept", "decline"})
            self.scene(match, [199, 200, 235, 207], tiebreakGF=0)
            commands = self.audit(match, 3)
            self.assertTrue(any(command.kind == "sacrifice" and len(command.items) == 3 for command in commands))

    def test_invalid_and_rejected_phases(self):
        with Match(config=Config(max_decisions=1), library=LIBRARY) as match:
            for limit, step in ((0, 0), (5, 0), (-1, 0), (2, -1), (2, 2**31), (True, 0), (2, 1.5)):
                with self.assertRaises(InvalidMatchError):
                    match.legal_commands(limit, step)
            command, _ = match.sample_action(1)
            match.step(command)
            for fn in (match.legal_commands, lambda: enumerate_commands(match)):
                with self.assertRaises(InvalidMatchError): fn()
        with Match(config=Config(external_artifacts=True), library=LIBRARY) as match:
            self.scene(match, [199])
            match.step(Command(0, "pray"))
            self.assertEqual(pending_kind(match), "grant")
            for fn in (match.legal_commands, lambda: enumerate_commands(match), lambda: _enumerate_commands_python(match)):
                with self.assertRaises(UnsupportedMatchError): fn()
        with Match(library=LIBRARY) as match:
            raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "confusionCount": 3}, {"id": 77, "hp": 40}]}
            with OfficialState(raw, library=LIBRARY) as state: match.load_scene(state, 0)
            self.assertEqual(pending_kind(match), "confusion")
            for fn in (match.legal_commands, lambda: enumerate_commands(match), lambda: _enumerate_commands_python(match)):
                with self.assertRaises(UnsupportedMatchError): fn()

    def test_fallback_only_for_missing_symbol(self):
        with Match(library=LIBRARY) as match:
            class OldMatch:
                _lib = SimpleNamespace()
                status = match.status
                observe = match.observe
                legal = match.legal
            self.assertEqual(enumerate_commands(OldMatch()), match.legal_commands())
            # Existing native errors must propagate, never trigger fallback.
            with self.assertRaises(InvalidMatchError): enumerate_commands(match, 0)


def benchmark():
    for limit in (1, 2, 3):
        native = brute = total = maximum = 0
        with Match(library=LIBRARY) as match:
            for seed in range(200):
                match.reset(seed)
                start = time.perf_counter()
                commands = match.legal_commands(limit)
                native += time.perf_counter() - start
                start = time.perf_counter()
                oracle = _enumerate_commands_python(match, limit)
                brute += time.perf_counter() - start
                assert set(commands) == set(oracle)
                total += len(commands)
                maximum = max(maximum, len(commands))
        print(f"max_items={limit} scenes=200 mean={total/200:.3f} max={maximum} native_python_us={native*1e6/200:.3f} brute_us={brute*1e6/200:.3f} speedup={brute/native:.3f}", flush=True)

if __name__ == "__main__":
    if "--benchmark" in sys.argv:
        benchmark()
    else:
        unittest.main()
