"""End-to-end native trainer checkpoint/restart/evaluation contract.

Runs only the executable (including sanitizer builds); no ctypes or ML packages.
Usage: python3 tests/learning_cli_test.py path/to/godfield_train
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

EXECUTABLE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None


class LearningCLI(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if EXECUTABLE is None or not EXECUTABLE.is_file():
            raise RuntimeError("expected godfield_train executable argument")

    def run_cli(self, *args, ok=True):
        result = subprocess.run([str(EXECUTABLE), *map(str, args)], text=True,
                                capture_output=True, timeout=90, env=os.environ.copy())
        if ok:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        fields = {}
        for line in result.stdout.splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                fields[key.strip()] = value.strip()
        return fields, result

    def test_resume_matches_uninterrupted_and_evaluation_does_not_mutate(self):
        with tempfile.TemporaryDirectory(prefix="godfield-learning-cli-") as tmp:
            tmp = Path(tmp)
            direct, partial, resumed = (tmp / name for name in ("direct.gflearn", "partial.gflearn", "resumed.gflearn"))
            config = ["--seed", 123, "--candidates", 6, "--batch", 8,
                      "--snapshot-interval", 16, "--opponent", "snapshot"]
            evaluation = ["--eval-games", 8, "--eval-seed", 900]
            full, _ = self.run_cli("--episodes", 96, *config, "--out", direct, *evaluation)
            first, _ = self.run_cli("--episodes", 37, *config, "--out", partial, *evaluation)
            checkpoint_before = partial.read_bytes()
            continued, _ = self.run_cli("--resume", partial, "--episodes", 59,
                                        "--out", resumed, *evaluation)
            self.assertEqual(direct.read_bytes(), resumed.read_bytes(), "restart must include pending gradients, optimizer moments, RNG and frozen opponent")
            self.assertEqual(partial.read_bytes(), checkpoint_before, "resuming into a new output must preserve source checkpoint")
            self.assertEqual(full["mode"], "train")
            self.assertEqual(continued["mode"], "train")
            self.assertEqual(int(full["stats.episodes"]), 96)
            self.assertEqual(int(first["stats.episodes"]), 37)
            self.assertEqual(int(first["stats.pending_episodes"]), 5)
            self.assertEqual(int(continued["stats.episodes"]), 96)
            self.assertEqual(full["stats.checksum"], continued["stats.checksum"])
            self.assertEqual(full["final.checksum"], continued["final.checksum"])
            immutable = direct.read_bytes()
            for greedy in (False, True):
                args = ["--evaluate", direct, "--eval-games", 11, "--eval-seed", 901]
                if greedy:
                    args.append("--greedy")
                one, _ = self.run_cli(*args)
                two, _ = self.run_cli(*args)
                self.assertEqual(one, two, "fixed evaluation seed and policy must produce identical records")
                self.assertEqual(one["mode"], "evaluate")
                self.assertEqual(int(one["evaluation.games"]), 11)
                self.assertEqual(sum(int(one[f"evaluation.{name}"]) for name in ("wins", "losses", "draws", "truncations")), 11)
                self.assertEqual(direct.read_bytes(), immutable)
            for override in (["--seed", 123], ["--candidates", 6], ["--batch", 8],
                             ["--snapshot-interval", 16], ["--opponent", "snapshot"]):
                destination = tmp / "must-not-exist.gflearn"
                self.run_cli("--resume", direct, "--episodes", 1, "--out", destination,
                             *override, ok=False)
                self.assertFalse(destination.exists(), "config overrides must fail before checkpoint output")
                self.assertEqual(direct.read_bytes(), immutable)
            self.run_cli("--evaluate", direct, "--episodes", 1, ok=False)
            self.run_cli("--evaluate", direct, "--resume", partial, ok=False)
            self.run_cli("--evaluate", direct, "--out", tmp / "bad.gflearn", ok=False)
            self.run_cli("--evaluate", direct, "--seed", 123, ok=False)
            for malformed in (b"", immutable[:len(immutable)//2], immutable + b"\0",
                              immutable[:8] + (999).to_bytes(8, "little") + immutable[16:]):
                path = tmp / "corrupt.gflearn"
                path.write_bytes(malformed)
                self.run_cli("--evaluate", path, "--eval-games", 1, ok=False)
                self.run_cli("--resume", path, "--episodes", 1, "--out", tmp / "bad.gflearn", ok=False)
            self.assertEqual(direct.read_bytes(), immutable)

    def test_random_opponent_seed_zero_and_invalid_arguments(self):
        with tempfile.TemporaryDirectory(prefix="godfield-learning-options-") as tmp:
            path = Path(tmp) / "random.gflearn"
            fields, _ = self.run_cli("--episodes", 5, "--seed", 0, "--candidates", 4,
                                     "--batch", 3, "--snapshot-interval", 2,
                                     "--opponent", "random", "--out", path,
                                     "--eval-games", 3, "--eval-seed", 0, "--greedy")
            self.assertEqual(fields["config.opponent"], "random")
            self.assertEqual(int(fields["stats.episodes"]), 5)
            self.assertTrue(path.is_file())
            invalid = (["--episodes", 0], ["--episodes", -1], ["--episodes", "1oops"],
                       ["--seed", "18446744073709551616"], ["--candidates", 0],
                       ["--candidates", 33], ["--batch", 0], ["--snapshot-interval", 0],
                       ["--opponent", "unknown"], ["--eval-games", 0],
                       ["--eval-seed", -1], ["--unknown"], ["--seed"])
            for args in invalid:
                self.run_cli(*args, ok=False)


if __name__ == "__main__":
    unittest.main()
