import subprocess
import json
import statistics

PROBE = "/private/tmp/cathedral-gpu-rung34/probe"
CATHEDRAL_ZONE = "saves/zones/Cathedral of the Living Logos/zone.ecform"
MERRY_ZONE = "saves/zones/The Merry Dancers/zone.json"

benchmarks = {
    "cathedral_field_subset": {
        "zone": CATHEDRAL_ZONE,
        "mode": "--render",
        "rgba": "/private/tmp/cathedral-gpu-rung34/cathedral_field.rgba",
        "json": "/private/tmp/cathedral-gpu-rung34/cathedral_field.json",
    },
    "cathedral_full_world": {
        "zone": CATHEDRAL_ZONE,
        "mode": "--render-world",
        "rgba": "/private/tmp/cathedral-gpu-rung34/cathedral_world.rgba",
        "json": "/private/tmp/cathedral-gpu-rung34/cathedral_world.json",
    },
    "merry_dancers_full_world": {
        "zone": MERRY_ZONE,
        "mode": "--render-world",
        "rgba": "/private/tmp/cathedral-gpu-rung34/merry.rgba",
        "json": "/private/tmp/cathedral-gpu-rung34/merry.json",
    }
}

results = {k: [] for k in benchmarks}

print("Running 4-cycle paired Metal ABBA benchmark for Rung 3 & 4...")

for cycle in range(4):
    print(f"\n--- Cycle {cycle + 1}/4 ---")
    for name, cfg in benchmarks.items():
        cmd = [PROBE, cfg["zone"], cfg["json"], cfg["mode"], cfg["rgba"]]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with open(cfg["json"]) as f:
            data = json.load(f)
        warm_frames = data["surface_subset_complete_ms"][-4:]
        results[name].extend(warm_frames)
        mean_warm = sum(warm_frames) / len(warm_frames)
        print(f"  {name}: {[round(x, 3) for x in warm_frames]} (mean: {mean_warm:.3f} ms)")

print("\n=== SUMMARY RESULTS (16 warm frames per target) ===")
summary = {}
for name, times in results.items():
    mean_val = float(statistics.mean(times))
    min_val = float(min(times))
    max_val = float(max(times))
    p50_val = float(statistics.median(times))
    summary[name] = {
        "mean_ms": round(mean_val, 3),
        "min_ms": round(min_val, 3),
        "max_ms": round(max_val, 3),
        "median_ms": round(p50_val, 3),
        "all_samples": [round(t, 3) for t in times]
    }
    print(f"{name}: Mean = {mean_val:.3f} ms | Range = [{min_val:.3f} - {max_val:.3f}] ms | Median = {p50_val:.3f} ms")

with open("/private/tmp/cathedral-gpu-rung34/benchmark_summary.json", "w") as f:
    json.dump(summary, f, indent=2)
print("Saved summary to /private/tmp/cathedral-gpu-rung34/benchmark_summary.json")
