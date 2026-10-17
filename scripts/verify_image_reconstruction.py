"""Native production-object witness; requires built earthcall_webgpu and desktop GPU.
Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
Usage: python3 scripts/verify_image_reconstruction.py SOURCE.png SEED.json OUTPUT_DIR
"""
from pathlib import Path
import shlex
import subprocess
import tempfile
import sys
root = Path(__file__).resolve().parents[1]
build = root / "build"
source, seed, out = (Path(p).resolve() for p in sys.argv[1:])
stage = Path(tempfile.mkdtemp(prefix="earthcall-image-witness-"))
(stage / "saves").mkdir()
(stage / "AGENTS.md").write_text("Isolated image witness fixture.\n")
flags = {}
for line in (build / "CMakeFiles/earthcall_webgpu.dir/flags.make").read_text().splitlines():
    if " = " in line:
        key, value = line.split(" = ", 1)
        flags[key] = shlex.split(value)
obj = stage / "witness.o"
subprocess.run(["/usr/bin/c++", *flags["CXX_DEFINES"], *flags["CXX_INCLUDES"],
    *flags["CXX_FLAGS"], "-c", str(root / "scripts/image_reconstruction_native_probe.cpp"),
    "-o", str(obj)], check=True)
command = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
entry = next(i for i, arg in enumerate(command) if arg.endswith("/src/entry.cpp.o"))
command[entry] = str(obj)
command[command.index("-o") + 1] = str(stage / "witness")
subprocess.run(command, cwd=build, check=True)
subprocess.run([str(stage / "witness"), str(source), str(seed), str(out)], cwd=stage, check=True)
