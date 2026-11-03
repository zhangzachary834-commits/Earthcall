"""Link a direct Screen witness against already-built production WebGPU app objects.

Codex / GPT-6.1 Sol / 01a10a2b-a247-7c11-9d5f-7a8b89df6cfc / 2026-10-04 PDT.
Run after cmake --build build --target earthcall_webgpu.
The isolated first-seed root deliberately contains no identity key material.
"""
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
build = root / "build"
stage = Path(tempfile.mkdtemp(prefix="earthcall-direct-screen-"))
(stage / "saves").mkdir()
(stage / "AGENTS.md").write_text("Isolated Earthcall capture test fixture.\n")
(stage / "src").symlink_to(root / "src", target_is_directory=True)
(stage / "imgui").symlink_to(root / "imgui", target_is_directory=True)
flags = {}
for line in (build / "CMakeFiles/earthcall_webgpu.dir/flags.make").read_text().splitlines():
    if " = " in line:
        key, value = line.split(" = ", 1)
        flags[key] = shlex.split(value)
obj = stage / "direct_screen_probe.o"
subprocess.run(["/usr/bin/c++", *flags["CXX_DEFINES"], *flags["CXX_INCLUDES"],
    *flags["CXX_FLAGS"], "-c", str(root / "scratch/probes/direct_screen_form_probe.cpp"),
    "-o", str(obj)], check=True)
command = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
entry = next(i for i, arg in enumerate(command) if arg.endswith("/src/entry.cpp.o"))
command[entry] = str(obj)
command[command.index("-o") + 1] = str(stage / "direct_screen_probe")
subprocess.run(command, cwd=build, check=True)
print(f"Isolated fixture: {stage}", flush=True)
subprocess.run([str(stage / "direct_screen_probe")],
    cwd=stage, check=True)

subprocess.run([str(stage / "direct_screen_probe"), "--engine"], cwd=stage, check=True)
print(f"Evidence directory: {stage}", flush=True)
