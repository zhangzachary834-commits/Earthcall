#!/usr/bin/env python3
"""Build a scratch probe against the actual configured core and run in isolation.

OpenCode (GPT-6 Astra), session language-depth-20260910-115436.
2026-09-10 11:59 PDT. Uses CMake's existing compile/link recipes, without
registering a scratch test or changing the project. Requires Makefile generator.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--scratch-root", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    build = repo / "build"
    source = repo / "scratch/probes/language_meaning_probe.cpp"
    database = json.loads((build / "compile_commands.json").read_text())
    recipe = next(entry for entry in database
                  if entry["file"].endswith("/tests/singularity/logos_modality_test.cpp"))
    compile_args = shlex.split(recipe["command"])
    link_args = shlex.split((build / "CMakeFiles/logos_modality_test.dir/link.txt").read_text())
    with tempfile.TemporaryDirectory(prefix="earthcall-language-", dir=args.scratch_root) as tmp:
        root = Path(tmp)
        obj = root / "language_meaning_probe.o"
        exe = root / "language_meaning_probe"
        compile_args[compile_args.index("-o") + 1] = str(obj)
        compile_args[compile_args.index("-c") + 1] = str(source)
        subprocess.run(compile_args, cwd=recipe["directory"], check=True)
        link_args = [str(obj) if arg.endswith("/logos_modality_test.cpp.o") else arg
                     for arg in link_args]
        link_args[link_args.index("-o") + 1] = str(exe)
        subprocess.run(link_args, cwd=build, check=True)
        # No Engine::initLogic, SaveSystem loads, or real save-tree guards are
        # needed: these tests use only in-memory fixtures; relative log output
        # and any process-side artifacts stay inside this temporary directory.
        for executable in (build / "logos_modality_test",
                           build / "relation_retry_lexeme_test", exe):
            print(f"\nRUN {executable.name} (isolated working directory)", flush=True)
            subprocess.run([str(executable)], cwd=root, check=True)


if __name__ == "__main__":
    main()
