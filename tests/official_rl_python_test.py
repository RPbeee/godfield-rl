import random
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_match import Match
from official_rl import enumerate_commands, pending_kind
LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else None


class EnumerateCommandsTest(unittest.TestCase):
    def test_sampled_commands_are_enumerated_and_all_enumerated_are_legal(self):
        picker = random.Random(0)
        checked = 0
        with Match(seed=1, library=LIBRARY) as match:
            policy_rng = 1
            for game in range(12):
                match.reset(game)
                for _ in range(300):
                    if match.status.done:
                        break
                    self.assertIsNone(pending_kind(match))
                    commands = enumerate_commands(match)
                    self.assertTrue(commands)
                    self.assertTrue(all(match.legal(c) for c in commands))
                    sampled, policy_rng = match.sample_action(policy_rng)
                    if len(sampled.items) <= 2 and sampled.kind != "exchange":
                        checked += 1
                        self.assertIn(sampled, commands)
                    match.step(picker.choice(commands) if picker.random() < 0.5 else sampled)
        self.assertGreater(checked, 100)

    def test_finished_match_has_no_commands(self):
        with Match(seed=3, library=LIBRARY) as match:
            status, _ = match.random_rollout(5)
            self.assertTrue(status.terminated or status.truncated)
            self.assertEqual(pending_kind(match), "finished")
            self.assertEqual(enumerate_commands(match), [])


if __name__ == "__main__":
    unittest.main()
