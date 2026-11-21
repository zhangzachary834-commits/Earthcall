"""Run the native interaction audit against a local macOS WebGPU build."""

from pathlib import Path
import shlex
import subprocess
from tempfile import TemporaryDirectory


root = Path(__file__).resolve().parents[2]
build = root / "build"
target = build / "CMakeFiles/earthcall_webgpu.dir"
flags = {}
for line in (target / "flags.make").read_text().splitlines():
    if " = " in line:
        name, value = line.split(" = ", 1)
        flags[name] = shlex.split(value)
compile_flags = flags["CXX_DEFINES"] + flags["CXX_INCLUDES"] + flags["CXX_FLAGS"]
link = shlex.split((target / "link.txt").read_text())

# Replace these built objects with the current checkout's audited sources.
# Other support objects and libraries come from the configured app build.
sources = {
    "src/entry.cpp": "scratch/probes/native_interaction_audit_probe.cpp",
    "src/Singularity/Input/Interaction/InteractionChannel.cpp": None,
    "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp": None,
    "src/ConstructedBeing/Singular/Object/ObjectRaycast.cpp": None,
    "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp": None,
}
with TemporaryDirectory(prefix="earthcall-interaction-audit-") as temp:
    stage = Path(temp)
    for original, replacement in sources.items():
        source = root / (replacement or original)
        obj = stage / (source.stem + ".o")
        subprocess.run(
            [link[0], *compile_flags, "-O0", "-Wno-unused-parameter", "-c", str(source), "-o", str(obj)],
            cwd=build,
            check=True,
        )
        index = next(i for i, arg in enumerate(link) if arg.endswith(f"/{original}.o"))
        link[index] = str(obj)
    binary = stage / "native-interaction-audit"
    link[link.index("-o") + 1] = str(binary)
    subprocess.run(link, cwd=build, check=True)
    subprocess.run([str(binary)], cwd=stage, check=True)
