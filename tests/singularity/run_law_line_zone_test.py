#!/usr/bin/env python3
"""Layer the current authored seed over frozen fixtures, never the app home."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

REPO = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="earthcall-law-line-seed-") as temp:
    root = Path(temp)
    for directory in ("zones/LawLine", "zones/World", "laws"):
        shutil.copytree(REPO / "saves" / directory, root / "saves" / directory)
    # Existing examples are source programs, not writable fixture data.
    shutil.copytree(REPO / "examples", root / "examples")
    env = dict(os.environ, EARTHCALL_SEED_ROOT=str(root),
               EARTHCALL_LAW_LINE_TEST_SAVES=str(root / "saves"),
               EARTHCALL_TERMINAL_HISTORY=str(root / "history.txt"))
    subprocess.run([sys.executable, str(REPO / "scripts/seed_law_line.py")],
                   env=env, check=True, stdout=subprocess.DEVNULL)
    result = subprocess.run(sys.argv[1:], cwd=REPO, env=env)
    sys.exit(result.returncode)
