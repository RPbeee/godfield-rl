"""Verify archived official-v1 inputs without replacing the current catalog."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, str(root / "data/official-v1/generate.py"), "--check", "--archive-only"], check=True)
subprocess.run([sys.executable, str(root / "scripts/check_official_vectors.py"), "--catalog-dir", str(root / "data/official-v1")], check=True)
print("Archived official-v1 catalog, crosswalk and all three vector corpora verified.")
