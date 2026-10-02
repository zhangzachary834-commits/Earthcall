"""Link a capture witness against already-built production WebGPU app objects.

Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
Run after cmake --build build --target earthcall_webgpu.
The isolated first-seed root deliberately contains no identity key material.
"""
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
build = root / "build"
stage = Path(tempfile.mkdtemp(prefix="earthcall-engine-capture-"))
(stage / "saves").mkdir()
(stage / "AGENTS.md").write_text("Isolated Earthcall capture test fixture.\n")
(stage / "src").symlink_to(root / "src", target_is_directory=True)
(stage / "imgui").symlink_to(root / "imgui", target_is_directory=True)
flags = {}
for line in (build / "CMakeFiles/earthcall_webgpu.dir/flags.make").read_text().splitlines():
    if " = " in line:
        key, value = line.split(" = ", 1)
        flags[key] = shlex.split(value)
obj = stage / "capture_probe.o"
subprocess.run(["/usr/bin/c++", *flags["CXX_DEFINES"], *flags["CXX_INCLUDES"],
    *flags["CXX_FLAGS"], "-c", str(root / "scratch/probes/screen_recorder_engine_probe.cpp"),
    "-o", str(obj)], check=True)
command = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
entry = next(i for i, arg in enumerate(command) if arg.endswith("/src/entry.cpp.o"))
command[entry] = str(obj)
command[command.index("-o") + 1] = str(stage / "capture_probe")
subprocess.run(command, cwd=build, check=True)
print(f"Isolated fixture: {stage}", flush=True)
subprocess.run([str(stage / "capture_probe"), str(root / "saves/worlds/chess_app.json")],
    cwd=stage, check=True)
