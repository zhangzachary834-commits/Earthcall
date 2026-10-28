"""Oracle Keeper: compute/memory estimate and ready (NOT executed) invocation for a distinct native ray capture.
Claude Code / Claude Sonnet 5.5 (claude-sonnet-5-5) / session 37985354-1fde-47c7-9707-50ceed335c11 / 2026-10-27.
Prints a plan only. It never launches the reference: the Sun/Surveyor freeze the capture and cost cap first.
Rates are taken from recorded evidence (not re-measured here):
  * NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21: 3.3e8 field evaluations in 109 s on 4 threads -> ~3.03e6 evals/s total.
  * NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27: 4.73e9 evaluations for 32,784 rays at 2^16 intervals (~1.44e5 evals/ray).
"""
import argparse, json
EVALS_PER_RAY_AT_2_16 = 4.73e9 / 32784        # recorded; scales linearly with (2^k + 1) nodes
RATE_4_THREADS = 3.3e8 / 109.0                # recorded evals/s on 4 threads (scales ~linearly with threads, unverified)
def plan(rays, log2n=16, threads=4, rate=RATE_4_THREADS):
    nodes = (1 << log2n) + 1
    evals = EVALS_PER_RAY_AT_2_16 * rays * nodes / ((1 << 16) + 1)
    seconds = evals / (rate * threads / 4.0)
    # probe holds, per worker thread, sig[N+1] (double) + src[N+1] (dvec3 = 24 B) for one ray at a time; plus 8 doubles out per ray.
    per_thread = nodes * (8 + 24)
    out_bytes = rays * 8 * 8
    ray_json_bytes = rays * 200   # nlohmann DOM for 7..23 numbers/ray; wide-case rays.json is ~23 columns
    return {'rays': rays, 'log2N': log2n, 'threads': threads, 'field_evaluations': evals, 'est_wall_seconds': seconds,
            'est_wall_minutes': seconds / 60, 'peak_bytes_per_worker': per_thread,
            'peak_bytes_workers': per_thread * threads, 'output_bytes': out_bytes, 'ray_dom_bytes_est': ray_json_bytes,
            'basis': 'extrapolated from recorded rates; NOT measured by this session'}
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rays', type=int, default=32784); p.add_argument('--log2n', type=int, default=16); p.add_argument('--threads', type=int, default=4)
    p.add_argument('--capture', default='<CAPTURE.json>'); p.add_argument('--zone', default='saves/zones/Northern Veil/zone.ecform')
    a = p.parse_args(); d = plan(a.rays, a.log2n, a.threads)
    half = plan(a.rays, a.log2n - 1, a.threads)
    d['note'] = 'one finest grid (2^k) yields both resolutions: the probe reads stride-2 nodes for the half level; no second run'
    d['proposed_invocation'] = [
        'cd <worktree root>   # probe reads saves/zones/Northern Veil/zone.ecform relative to cwd',
        f'build veilref as in WIDE_REFERENCE audit (Release flags/link line of webgpu_line_quadrature_test) from scratch/probes/veil_projected_reference.cpp',
        f'veilref {a.capture} {a.log2n} 1000000 {a.threads} <REF.json>   # probe args: rays log2N limit threads out',
        f'python3 -B scratch/probes/sonnet_oracle_capture_gate.py --capture {a.capture} --reference <REF.json> --candidate <CAND.json> --current <CURRENT.json> --zone "{a.zone}" --expected-count {a.rays} --output <GATE.json>',
    ]
    d['half_level_only_cost_fraction'] = half['field_evaluations'] / d['field_evaluations']
    print(json.dumps(d, indent=2))
if __name__ == '__main__': main()
