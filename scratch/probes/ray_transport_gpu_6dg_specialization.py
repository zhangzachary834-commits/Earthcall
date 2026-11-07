"""§6dg: Query-Family Specialization, A Priori Tile Routing, and Single-Pass Residual Pipelines.
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-24.
Commissioned by 🌌✨ The Firmament (Gemini 4 Argon) & 🌈☀️ Original Prism Sun (GPT-5.6 Sol).

Compiles and benchmarks physically separate Metal compute pipelines:
- Pass 0: classifyTilesMain (2,304 threads = 36 workgroups, 0 LDS, 0 barriers)
- Pipeline Alpha (Shape S1: Curtain 0 only, NO_OVERLAP, register-streamed)
- Pipeline Beta (Shape S2: Curtains 0 & 1 sequential, NO_OVERLAP, register-streamed)
- Pipeline Fallback (Current Marcher on remaining 29.98% rays)
- Baseline Reference (Frozen Current Marcher over all 32,784 rays)

Measures paired ABBA timings across 1, 2, and 4 repeats for baseline, standalone kernels,
warm routed total (stationary camera), and cold routed total (moving camera).
Verifies parity against reference, and exports complete machine record to:
docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.json.
"""
import argparse, json, os, shlex, subprocess, sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Compile and run 6dg specialization benchmark")
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--build-root", type=Path, default=Path("/private/tmp/earthcall-rt-pure-build"))
    parser.add_argument("--work", type=Path, default=Path("/private/tmp/earthcall-6dg-specialization"))
    parser.add_argument("--rays", type=Path, default=Path("/private/tmp/earthcall-wide-case/rays.json"))
    parser.add_argument("--zone", type=Path, default=Path("saves/zones/Northern Veil/zone.ecform"))
    parser.add_argument("--output", type=Path, default=Path("docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.json"))
    args = parser.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)

    cpp_src = root / "scratch/probes/ray_transport_gpu_6dg_specialization.cpp"
    obj_path = work / "specialization.o"
    bin_path = work / "specialization"
    out_json = args.output.resolve()

    b = args.build_root.resolve() / "build"
    db = json.loads((b / "compile_commands.json").read_text())
    entry = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    cmd = shlex.split(entry["command"])
    cmd[cmd.index("-c") + 1] = str(cpp_src)
    cmd[cmd.index("-o") + 1] = str(obj_path)
    cmd += ["-std=c++20", "-O3"]
    print("Compiling 6dg specialization C++ probe...")
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith("/src/entry.cpp.o"))] = str(obj_path)
    link_cmd[link_cmd.index("-o") + 1] = str(bin_path)
    print("Linking 6dg specialization binary...")
    subprocess.run(link_cmd, cwd=b, check=True)

    zone_file = args.zone.resolve()
    rays_file = args.rays.resolve()
    print(f"Running specialization benchmark against {zone_file} and {rays_file} -> {out_json}...")
    subprocess.run([str(bin_path), str(zone_file), str(rays_file), str(out_json)], check=True)

    print(f"Finished! Output written to {out_json}")


if __name__ == "__main__":
    main()
