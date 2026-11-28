import subprocess
import json
import statistics
import os
from pathlib import Path

PROBE = "/private/tmp/cathedral-rung16-18/probe"
WORK_DIR = "/private/tmp/cathedral-rung16-18"
CATHEDRAL_ZONE = "saves/zones/Cathedral of the Living Logos/zone.ecform"
MERRY_ZONE = "saves/zones/The Merry Dancers/zone.json"

benchmarks = {
    "cathedral_field_subset": {
        "zone": CATHEDRAL_ZONE,
        "mode": "--render",
        "rgba": f"{WORK_DIR}/cathedral_field.rgba",
        "json": f"{WORK_DIR}/cathedral_field.json",
    },
    "cathedral_full_world": {
        "zone": CATHEDRAL_ZONE,
        "mode": "--render-world",
        "rgba": f"{WORK_DIR}/cathedral_world.rgba",
        "json": f"{WORK_DIR}/cathedral_world.json",
    },
    "merry_dancers_full_world": {
        "zone": MERRY_ZONE,
        "mode": "--render-world",
        "rgba": f"{WORK_DIR}/merry.rgba",
        "json": f"{WORK_DIR}/merry.json",
    }
}

results = {
    k: {
        "wall_ms": [],
        "encode_ms": [],
        "flush_ms": [],
        "completion_wait_ms": [],
        "mesh_draw_calls": []
    }
    for k in benchmarks
}

print("Running 4-cycle paired Metal ABBA benchmark for Rungs 16, 17 & 18...")

for cycle in range(4):
    print(f"\n--- Cycle {cycle + 1}/4 ---")
    for name, cfg in benchmarks.items():
        cmd = [PROBE, cfg["zone"], cfg["json"], cfg["mode"], cfg["rgba"]]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with open(cfg["json"]) as f:
            data = json.load(f)
        warm_wall = data["surface_subset_complete_ms"][-4:]
        warm_encode = data["encode_ms"][-4:]
        warm_flush = data["flush_ms"][-4:]
        warm_wait = data["completion_wait_ms"][-4:]
        draw_calls = data.get("mesh_draw_calls", 0)

        results[name]["wall_ms"].extend(warm_wall)
        results[name]["encode_ms"].extend(warm_encode)
        results[name]["flush_ms"].extend(warm_flush)
        results[name]["completion_wait_ms"].extend(warm_wait)
        results[name]["mesh_draw_calls"].append(draw_calls)

        mean_wall = sum(warm_wall) / len(warm_wall)
        mean_encode = sum(warm_encode) / len(warm_encode)
        mean_flush = sum(warm_flush) / len(warm_flush)
        mean_wait = sum(warm_wait) / len(warm_wait)
        print(f"  {name}: wall={mean_wall:.3f} ms (encode={mean_encode:.3f} ms, flush={mean_flush:.3f} ms, wait={mean_wait:.3f} ms, mesh_draws={draw_calls})")

print("\n=== SUMMARY RESULTS (16 warm frames per target) ===")
summary = {}
for name, metrics in results.items():
    wall = metrics["wall_ms"]
    enc = metrics["encode_ms"]
    fl = metrics["flush_ms"]
    wait = metrics["completion_wait_ms"]
    draws = metrics["mesh_draw_calls"][-1]

    summary[name] = {
        "wall_mean_ms": round(float(statistics.mean(wall)), 3),
        "wall_median_ms": round(float(statistics.median(wall)), 3),
        "wall_min_ms": round(float(min(wall)), 3),
        "wall_max_ms": round(float(max(wall)), 3),
        "encode_mean_ms": round(float(statistics.mean(enc)), 3),
        "flush_mean_ms": round(float(statistics.mean(fl)), 3),
        "completion_wait_mean_ms": round(float(statistics.mean(wait)), 3),
        "mesh_draw_calls": draws,
        "all_wall_samples": [round(t, 3) for t in wall]
    }
    s = summary[name]
    print(f"{name}:")
    print(f"  Mesh draws: {s['mesh_draw_calls']}")
    print(f"  Wall ms: Mean={s['wall_mean_ms']} | Median={s['wall_median_ms']} | Range=[{s['wall_min_ms']} - {s['wall_max_ms']}]")
    print(f"  Stage breakdown: Encode={s['encode_mean_ms']} ms | Flush={s['flush_mean_ms']} ms | Completion Wait={s['completion_wait_mean_ms']} ms")

# Check pixel parity against Rung 13-15 and Rung 11-12
for prev_name, prev_dir in [("Rung 13-15", Path("/private/tmp/cathedral-rung13-15")), ("Rung 11-12", Path("/private/tmp/cathedral-rung11-12"))]:
    if prev_dir.exists():
        print(f"\n=== PIXEL PARITY COMPARISON vs {prev_name} ===")
        for name, cfg in benchmarks.items():
            curr_rgba = Path(cfg["rgba"])
            prev_rgba = prev_dir / curr_rgba.name
            if curr_rgba.exists() and prev_rgba.exists():
                curr_bytes = curr_rgba.read_bytes()
                prev_bytes = prev_rgba.read_bytes()
                total = len(curr_bytes)
                diff = sum(1 for a, b in zip(curr_bytes, prev_bytes) if a != b)
                pct = 100.0 * (total - diff) / total
                print(f"  {name}: {total - diff} / {total} bytes match ({pct:.3f}% parity, {diff} diff bytes)")

with open(f"{WORK_DIR}/rung1618_summary.json", "w") as f:
    json.dump(summary, f, indent=2)
print(f"\nWrote {WORK_DIR}/rung1618_summary.json")
