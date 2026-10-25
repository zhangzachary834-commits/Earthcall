"""§6dg: Generic OntoMath Query-Family Support AST Witness (KC3 Closure).
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-24.
Commissioned by 🌌✨ The Firmament & 🌈☀️ Original Prism Sun.

Demonstrates that one generic OntoMath AST analyzer (with zero "Northern Veil" or "Curtain" name checks)
derives sound family certificates (ZERO, RESIDUAL_PROGRAM, UNCERTAIN) across TWO distinct authored
OntoMath field expressions:
1. Field 1: Authored Trigonometric Wave Sheet Family (Northern Veil 4 curtains).
2. Field 2: Authored Euclidean Radial-Shell Family (4 concentric/offset spherical shells).

Verifies 0 false skips across all 32,784 rays of Northern Veil's view frustum, evaluates AST node erasure,
and records discrete equivalence class tile distributions.
"""
import argparse, json, os, shlex, subprocess, sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Compile and run 6dg OntoMath genericity witness")
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--build-root", type=Path, default=Path("/private/tmp/earthcall-rt-pure-build"))
    parser.add_argument("--work", type=Path, default=Path("/private/tmp/earthcall-6dg-genericity"))
    parser.add_argument("--rays", type=Path, default=Path("/private/tmp/earthcall-wide-case/rays.json"))
    parser.add_argument("--zone", type=Path, default=Path("saves/zones/Northern Veil/zone.ecform"))
    parser.add_argument("--output", type=Path, default=Path("docs/audits/GPU_RAY_TRANSPORT_6DG_ONTOMATH_GENERICITY.json"))
    args = parser.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)

    cpp_src = root / "scratch/probes/ray_transport_gpu_6dg_ontomath_genericity.cpp"
    obj_path = work / "genericity.o"
    bin_path = work / "genericity"
    out_json = args.output.resolve()

    b = args.build_root.resolve() / "build"
    db = json.loads((b / "compile_commands.json").read_text())
    entry = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    cmd = shlex.split(entry["command"])
    cmd[cmd.index("-c") + 1] = str(cpp_src)
    cmd[cmd.index("-o") + 1] = str(obj_path)
    cmd += ["-std=c++20", "-O3"]
    print("Compiling 6dg OntoMath genericity C++ probe...")
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith("/src/entry.cpp.o"))] = str(obj_path)
    link_cmd[link_cmd.index("-o") + 1] = str(bin_path)
    print("Linking 6dg OntoMath genericity binary...")
    subprocess.run(link_cmd, cwd=b, check=True)

    zone_file = args.zone.resolve()
    rays_file = args.rays.resolve()
    print(f"Running genericity witness against {zone_file} and {rays_file} -> {out_json}...")
    subprocess.run([str(bin_path), str(zone_file), str(rays_file), str(out_json)], check=True)

    with open(out_json) as f:
        data = json.load(f)
    print("=== KC3 OntoMath Genericity Results ===")
    print(f"KC3 Status: {data.get('kc3_status')}")
    print(f"Field 1 (Trig Sheet) Soundness: {data['field_1_trig_sheet']['soundness']} (false skips: {data['field_1_trig_sheet']['false_skips']})")
    print(f"Field 2 (Radial Shell) Soundness: {data['field_2_radial_shell']['soundness']} (false skips: {data['field_2_radial_shell']['false_skips']})")


if __name__ == "__main__":
    main()
