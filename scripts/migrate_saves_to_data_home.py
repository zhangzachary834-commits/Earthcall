#!/usr/bin/env python3
"""Copy the checkout's saves/ into the Person's data home, once, verified.

Zach's decision (2026-10-12): Earthcall keeps its data like an installed app,
outside the repository — macOS ~/Library/Application Support/Earthcall — and
that data home is its own git repository so the history of Persons, Homes,
Joys, Relations, Laws, Zones and Objects continues there. backups/ (8.6 GB of
automatic snapshots) stays in the checkout.

Save files are sacred (AGENTS.md, FIRST_MOVER_AUTHORING.md §7 rule 8), so:
  * the checkout's saves/ is only read — nothing there is moved or deleted;
  * everything is copied into a staging directory beside the home, every file
    is compared byte-for-byte (size + SHA-256) against its source, and only
    then is staging renamed onto the home in one atomic step;
  * the data-home.json marker is written last inside staging; the engine
    trusts a home only when it carries that marker (SaveSystem::chooseDataRoot),
    so a half-finished copy can never be mistaken for the Person's world.

The home location comes from scripts/earthcall_data_home.py, which mirrors
SaveSystem::platformDataHome — change both together.

Usage:
  python3 scripts/migrate_saves_to_data_home.py --dry-run
  python3 scripts/migrate_saves_to_data_home.py

Written by Claude Code / Claude Opus 5.5, 2026-10-12, at Zach's request.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from earthcall_data_home import CHECKOUT_SAVES as SOURCE, MARKER, REPO, platform_data_home  # noqa: E402
EXCLUDED_TOP = {"backups"}

# Mirrors the checkout's saves/ rules in .gitignore so the same set stays
# under version control after the move.
DATA_HOME_GITIGNORE = """\
# Earthcall data home. Mirrors the saves/ rules from the checkout's .gitignore.
backups/
games/
logs/
earthcall-io.log
tests/*_final.json
identity/*.staged
.DS_Store
"""


def source_files() -> list[Path]:
    out = []
    for root, dirs, files in os.walk(SOURCE):
        rel_root = Path(root).relative_to(SOURCE)
        if rel_root == Path("."):
            dirs[:] = [d for d in dirs if d not in EXCLUDED_TOP]
        for name in files:
            out.append((Path(root) / name).relative_to(SOURCE))
    return sorted(out)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def git(cwd: Path, *args: str) -> str:
    return subprocess.run(["git", "-C", str(cwd), *args], check=True,
                          capture_output=True, text=True).stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--dry-run", action="store_true", help="report, copy nothing")
    args = ap.parse_args()

    home = platform_data_home()
    if home is None:
        print("refused: no home directory known; set EARTHCALL_DATA", file=sys.stderr)
        return 2
    if not SOURCE.is_dir():
        print(f"refused: no checkout saves/ at {SOURCE}", file=sys.stderr)
        return 2
    if (home / MARKER).is_file():
        print(f"already migrated: {home} carries {MARKER}; nothing to do")
        return 0
    if home.exists() and any(home.iterdir()):
        print(f"refused: {home} exists, is not empty, and has no {MARKER}. "
              "Inspect it by hand; this script never merges into or overwrites a home.",
              file=sys.stderr)
        return 2

    files = source_files()
    total = sum((SOURCE / f).stat().st_size for f in files if not (SOURCE / f).is_symlink())
    print(f"source : {SOURCE}")
    print(f"home   : {home}")
    print(f"files  : {len(files)}  ({total / 1e9:.2f} GB), excluding {sorted(EXCLUDED_TOP)}")
    if args.dry_run:
        return 0

    free = shutil.disk_usage(home.parent if home.parent.exists() else Path.home()).free
    if free < total * 1.5:
        print(f"refused: {free / 1e9:.1f} GB free, need ~{total * 1.5 / 1e9:.1f} GB", file=sys.stderr)
        return 2

    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    staging = home.parent / f".{home.name}.staging-{stamp}"
    home.parent.mkdir(parents=True, exist_ok=True)
    print(f"staging: {staging}")

    for rel in files:
        src, dst = SOURCE / rel, staging / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        if src.is_symlink():
            os.symlink(os.readlink(src), dst)
        else:
            shutil.copy2(src, dst)

    mismatches = []
    for rel in files:
        src, dst = SOURCE / rel, staging / rel
        if src.is_symlink():
            if not dst.is_symlink() or os.readlink(src) != os.readlink(dst):
                mismatches.append(rel)
        elif src.stat().st_size != dst.stat().st_size or sha256(src) != sha256(dst):
            mismatches.append(rel)
    if mismatches:
        print(f"refused: {len(mismatches)} file(s) differ after copy (was Earthcall running?). "
              f"Staging kept for inspection at {staging}; the home is untouched.", file=sys.stderr)
        for rel in mismatches[:20]:
            print(f"  {rel}", file=sys.stderr)
        return 1
    print(f"verified: {len(files)} files byte-identical")

    try:
        commit = git(REPO, "rev-parse", "HEAD")
    except subprocess.CalledProcessError:
        commit = None
    (staging / ".gitignore").write_text(DATA_HOME_GITIGNORE)
    (staging / MARKER).write_text(json.dumps({
        "earthcallDataHome": 1,
        "created": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
        "migratedFrom": str(SOURCE),
        "checkoutCommit": commit,
        "files": len(files),
        "bytes": total,
        "excluded": sorted(EXCLUDED_TOP),
        "authorizedBy": "Zach (Person), 2026-10-12",
        "performedBy": "scripts/migrate_saves_to_data_home.py",
    }, indent=2) + "\n")

    git(staging, "init", "-q")
    git(staging, "add", "-A")
    git(staging, "commit", "-q", "-m",
        "Earthcall data home: migrated from checkout saves/\n\n"
        f"Copied byte-for-byte from {SOURCE} at checkout commit {commit}.\n"
        "backups/ stays in the checkout. The checkout copy is untouched.\n\n"
        "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>")

    if home.exists():
        home.rmdir()  # empty by the check above
    os.rename(staging, home)
    print(f"done: {home} (git: {git(home, 'rev-parse', '--short', 'HEAD')})")
    print("The checkout's saves/ was not modified. Earthcall now saves into the data home.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
