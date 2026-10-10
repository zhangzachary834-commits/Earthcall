import sys
import subprocess

def run_mode(adapter, direct):
    cmd = ["./build/slow_adapter_zone_perf_test", "saves/worlds/chess_app.json", f"--adapter={adapter}", f"--direct={direct}", "--frames=10"]
    try:
        result = subprocess.run(cmd, text=True, capture_output=True, timeout=120)
    except subprocess.TimeoutExpired:
        print(f"ADAPTER_PERF_TIMEOUT adapter={adapter} direct={direct} timeout_s=120")
        raise SystemExit(1)

    lines = [line for line in result.stdout.splitlines() if line.startswith("ADAPTER_PERF ")]
    for line in lines:
        print(line)
    if result.returncode != 0 or not lines:
        tail = (result.stdout + "\n" + result.stderr).splitlines()[-20:]
        print("\n".join(tail), file=sys.stderr)
        raise SystemExit(result.returncode or 1)

    for token in lines[-1].split():
        if token.startswith("frame_median_ms="):
            return float(token.split("=")[1])
    return 0.0

print("Testing floor")
off_ms = run_mode("off", "off")
print("Testing pre-direct")
pre_direct_ms = run_mode("on", "off")
print("Testing direct")
direct_ms = run_mode("on", "on")

print(f"off={off_ms} pre_direct={pre_direct_ms} direct={direct_ms}")
