"""Native visibility witness using production WebGPU app objects, no inhabited writes.
Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-05.
Build earthcall_webgpu first; the executable creates only an isolated test save root.
"""
from pathlib import Path
import shlex
import subprocess
import tempfile
import sys
import os

root = Path(__file__).resolve().parents[2]
build = root / "build"
stage = Path(tempfile.mkdtemp(prefix="earthcall-law-visibility-build-"))
flags = {}
for line in (build / "CMakeFiles/earthcall_webgpu.dir/flags.make").read_text().splitlines():
    if " = " in line:
        key, value = line.split(" = ", 1)
        flags[key] = shlex.split(value)
obj = stage / "visibility.o"
subprocess.run(["/usr/bin/c++", *flags["CXX_DEFINES"], *flags["CXX_INCLUDES"],
    *flags["CXX_FLAGS"], "-c", str(root / "scratch/probes/law_line_visibility_probe.cpp"),
    "-o", str(obj)], check=True)
command = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
command[next(i for i, arg in enumerate(command) if arg.endswith("/src/entry.cpp.o"))] = str(obj)
command[command.index("-o") + 1] = str(stage / "visibility")
subprocess.run(command, cwd=build, check=True)
if "--engine" in sys.argv or "--stairway" in sys.argv:
    (stage / "AGENTS.md").write_text("Isolated native Engine visibility fixture. No inhabited saves or keys.\n")
    for name in ("src", "imgui"):
        (stage / name).symlink_to(root / name, target_is_directory=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith("EARTHCALL_")}
    mode = "--stairway" if "--stairway" in sys.argv else "--engine"
    subprocess.run([str(stage / "visibility"), mode, str(root)], cwd=stage, env=env, check=True)
else:
    subprocess.run([str(stage / "visibility")], cwd=root, check=True)
