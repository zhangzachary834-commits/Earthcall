"""Build and run the saved-Cathedral compiler census on CPU only.

Codex / GPT-6 / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-03.
The linked C++ probe's optional --render entry point requires a separate launch.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--build", type=Path, default=ROOT / "build_release")
    args = parser.parse_args()
    work, build = args.work.resolve(), args.build.resolve()
    work.mkdir(parents=True, exist_ok=False)
    zone = ROOT / "saves/zones/Cathedral of the Living Logos/zone.ecform"
    source = ROOT / "scratch/probes/cathedral_surface_probe.cpp"
    tracked = [zone, source, Path(__file__).resolve(),
               ROOT / "src/Singularity/Screen/WebGPU/SdfWgsl.cpp",
               ROOT / "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp",
               ROOT / "src/Singularity/OntoMath/ScalarForm.cpp"]
    before = {str(path.relative_to(ROOT)): digest(path) for path in tracked}
    cache = (build / "CMakeCache.txt").read_text()
    assert "CMAKE_BUILD_TYPE:STRING=Release\n" in cache
    assert "CMAKE_HOME_DIRECTORY:INTERNAL=" + str(ROOT) + "\n" in cache
    entries = json.loads((build / "compile_commands.json").read_text())
    entry = next(row for row in entries if row["file"].endswith("/entry.cpp")
                 and "earthcall_webgpu.dir" in row["command"])
    compile_command = shlex.split(entry["command"])
    compile_command[compile_command.index("-c") + 1] = str(source)
    compile_command[compile_command.index("-o") + 1] = str(work / "probe.o")
    link = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    link = [str(work / "probe.o") if item.endswith("/src/entry.cpp.o") else item for item in link]
    link[link.index("-o") + 1] = str(work / "probe")
    with (work / "build.log").open("w") as log:
        subprocess.run(compile_command, cwd=entry["directory"], stdout=log, stderr=log, check=True)
        subprocess.run(link, cwd=build, stdout=log, stderr=log, check=True)
    command = [str(work / "probe"), str(zone), str(work / "cpu.json")]
    with (work / "cpu.log").open("w") as log:
        subprocess.run(command, cwd=ROOT, stdout=log, stderr=log, check=True, timeout=120)
    report = json.loads((work / "cpu.json").read_text())
    assert report["gpu_executed"] is False
    assert report["parameter_mismatches"] == 0
    after = {str(path.relative_to(ROOT)): digest(path) for path in tracked}
    assert before == after, "inputs changed during the census"
    receipt = {
        "at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "by": "Codex/GPT-6/01a148a7-bd85-7550-a416-0e859a729c1a",
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "before": before, "after": after,
        "compile_command": compile_command, "link_command": link, "run_command": command,
        "binary_sha256": digest(work / "probe"), "report_sha256": digest(work / "cpu.json"),
        "sdf_object_sha256": digest(build / "CMakeFiles/earthcall_webgpu.dir/src/Singularity/Screen/WebGPU/SdfWgsl.cpp.o"),
        "gpu_executed": False,
    }
    (work / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print((work / "cpu.log").read_text().strip())


if __name__ == "__main__":
    main()
