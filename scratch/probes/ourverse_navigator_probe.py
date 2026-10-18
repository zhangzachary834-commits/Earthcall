#!/usr/bin/env python3
"""Build and run the Personal Ourverse Navigator Prototyping Probe.
Compiles ourverse_navigator_probe.cpp against the WebGPU production objects and runs it.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import sys

def main():
    root = Path(__file__).resolve().parents[2]
    build = root / "build"
    stage = Path(tempfile.mkdtemp(prefix="earthcall-navigator-probe-build-"))

    flags = {}
    for line in (build / "CMakeFiles/earthcall_webgpu.dir/flags.make").read_text().splitlines():
        if " = " in line:
            key, value = line.split(" = ", 1)
            flags[key] = shlex.split(value)

    obj = stage / "navigator_probe.o"
    print("Compiling ourverse_navigator_probe.cpp...", flush=True)
    subprocess.run([
        "/usr/bin/c++",
        *flags["CXX_DEFINES"],
        *flags["CXX_INCLUDES"],
        *flags["CXX_FLAGS"],
        "-c",
        str(root / "scratch/probes/ourverse_navigator_probe.cpp"),
        "-o",
        str(obj)
    ], check=True)

    print("Linking ourverse_navigator_probe...", flush=True)
    command = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    entry_idx = next(i for i, arg in enumerate(command) if arg.endswith("/src/entry.cpp.o"))
    command[entry_idx] = str(obj)
    command[command.index("-o") + 1] = str(stage / "ourverse_navigator_probe")
    subprocess.run(command, cwd=build, check=True)

    (stage / "AGENTS.md").write_text("Isolated Personal Ourverse Navigator probe fixture.\n")
    for name in ("src", "imgui"):
        (stage / name).symlink_to(root / name, target_is_directory=True)

    env = {k: v for k, v in os.environ.items() if not k.startswith("EARTHCALL_")}

    print(f"Executing probe in {stage}...", flush=True)
    proc = subprocess.run(
        [str(stage / "ourverse_navigator_probe"), str(root)],
        cwd=stage,
        env=env,
        stdin=subprocess.DEVNULL
    )

    if proc.returncode != 0:
        print(f"Probe exited with code {proc.returncode}")
        sys.exit(proc.returncode)
    else:
        print("Probe completed successfully!")

if __name__ == "__main__":
    main()
