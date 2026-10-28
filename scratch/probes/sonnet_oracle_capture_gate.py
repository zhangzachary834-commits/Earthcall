"""Oracle Keeper wrapper: identity-bound, variable-size distinct native ray capture -> full-workload gate.
Claude Code / Claude Sonnet 5.5 (claude-sonnet-5-5) / session 37985354-1fde-47c7-9707-50ceed335c11 / 2026-10-27.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) on Zach's approval; follows the 6em takeover.
CPU only. Wraps, and does not modify, ray_transport_full_workload_accuracy.evaluate (the shared checker) and
veil_projected_reference.cpp (the original-AST converged reference). Tolerances are never widened here:
the shared six gates (RGB/T mean, p99, max at BOTH resolutions) are evaluated on ALL rows; the extra
guards below only REFUSE (identity, coverage, order, finiteness, convergence), they never admit.
"""
import argparse, gzip, hashlib, importlib.util, json, math, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
SCHEMA = 'sonnet-oracle-capture-1'
# Reference convergence ceiling per ray (|finest - half| in T and each of RGB). Observed full-workload
# agreement on wide-inside was RGB 6.5e-8 / T 1.1e-7; Current's best-case error scale is ~1e-4.
# A ceiling is a REFUSAL bound (stricter is always allowed; looser needs Sun/Zach decision, see audit).
CONVERGENCE_CEILING = 1e-6
RAY_COLUMNS = 23  # px,py,dx,dy,dz,a,b + 16 output columns: the unchanged probe indexes r[7..22] for its own printout, so narrower rows read past the array

def need(ok, msg):
    if not ok: raise ValueError(msg)

def shared_gate():
    spec = importlib.util.spec_from_file_location('full_gate', HERE / 'ray_transport_full_workload_accuracy.py')
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); return m

def sha256_file(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def sha256_bytes(b): return hashlib.sha256(b).hexdigest()

def read_json(p):
    p = Path(p); b = p.read_bytes(); return json.loads(gzip.decompress(b) if p.suffix == '.gz' else b)

def validate_capture(cap, *, zone_sha256, expected_clock=None, expected_count=None, expected_origin=None):
    """Refuse anything that is not a complete, finite, ordered, identity-bound distinct capture.
    Variable size: any n >= 1 accepted; n must equal the declared ray_count (no silent truncation)."""
    need(cap.get('schema') == SCHEMA, 'capture schema mismatch')
    need(cap.get('zone_sha256') == zone_sha256, 'stale fixture identity: zone_sha256 differs from live fixture')
    rays = cap.get('rays'); need(isinstance(rays, list) and rays, 'empty capture')
    n = cap.get('ray_count'); need(isinstance(n, int) and n == len(rays), 'ray_count differs from ray rows (truncated/extended capture)')
    if expected_count is not None: need(n == expected_count, 'ray count differs from frozen capture plan')
    # clock identity: single scalar (the unchanged probe applies one clock to every medium).
    mc = cap.get('medium_clocks')
    need(isinstance(mc, dict) and mc, 'medium_clocks map required (medium id -> clock)')
    clocks = set(mc.values())
    need(all(isinstance(c, (int, float)) and math.isfinite(c) for c in clocks), 'nonfinite medium clock')
    need(len(clocks) == 1, 'medium-specific clocks differ: unchanged reference applies one clock to all media (unresolved decision D1)')
    clock = next(iter(clocks)); need(cap.get('time') == clock, 'capture time differs from medium clocks')
    if expected_clock is not None: need(clock == expected_clock, 'stale clock identity')
    o = cap.get('origin'); need(isinstance(o, list) and len(o) == 3 and all(math.isfinite(x) for x in o), 'origin')
    if expected_origin is not None: need(list(o) == list(expected_origin), 'stale pose identity: origin differs')
    seen = set()
    for i, r in enumerate(rays):
        need(isinstance(r, list) and len(r) >= RAY_COLUMNS, f'ray {i}: width')
        need(all(isinstance(x, (int, float)) and math.isfinite(x) for x in r), f'ray {i}: nonfinite/non-numeric value')
        need(all(not isinstance(v, bool) and int(v) == v for v in r[:2]), f'ray {i}: pixel not integer')
        key = (r[0], r[1]); need(key not in seen, f'ray {i}: duplicate pixel (not distinct)'); seen.add(key)
        need(any(v != 0 for v in r[2:5]), f'ray {i}: zero direction (recorded direction is preserved, never normalized)')
        need(r[6] > r[5] >= 0, f'ray {i}: empty/inverted clip interval')
    return clock

def reference_to_checker_form(ref_cols_rows, cap, rays_sha256, *, reference_identity=None, reference_sha256=None):
    """The probe writes columns T,Lr,Lg,Lb,T_half,... without pixels; bind each row to its ray's pixel by position
    and the shared checker's schema (px,py first). Row count must equal ray count exactly."""
    # A raw reference has no ray identifiers. Never label arbitrary old rows with
    # today's capture hash. Require a receipt retained with the reference job.
    need(isinstance(reference_identity, dict), 'reference generation receipt required')
    expected = dict(rays_sha256=rays_sha256, reference_sha256=reference_sha256,
                    zone_sha256=cap['zone_sha256'], time=cap['time'], origin=cap['origin'],
                    medium_clocks=cap['medium_clocks'])
    need(reference_sha256 is not None, 'reference byte identity required')
    for key, value in expected.items():
        need(reference_identity.get(key) == value, 'reference receipt identity mismatch: ' + key)
    need(reference_identity.get('generator_sha256') == sha256_file(HERE / 'veil_projected_reference.cpp'),
         'reference generator identity mismatch')
    rows = ref_cols_rows['rows']; rays = cap['rays']
    need(len(rows) == len(rays), 'reference row count differs from capture (missing rays)')
    need(ref_cols_rows['columns'] == ['T', 'Lr', 'Lg', 'Lb', 'T_half', 'Lr_half', 'Lg_half', 'Lb_half'], 'probe column schema')
    out = [[r[0], r[1], *row] for r, row in zip(rays, rows)]
    return {'rays_sha256': rays_sha256, 'columns': ['px', 'py', 'T', 'Lr', 'Lg', 'Lb', 'T_half', 'Lr_half', 'Lg_half', 'Lb_half'], 'rows': out}

def check_convergence(reference, ceiling=CONVERGENCE_CEILING):
    need(ceiling <= CONVERGENCE_CEILING, 'convergence ceiling may only be tightened, never widened')
    bad = [i for i, r in enumerate(reference['rows'])
           if abs(r[2] - r[6]) > ceiling or any(abs(r[3 + k] - r[7 + k]) > ceiling for k in range(3))]
    need(not bad, f'reference failed convergence on {len(bad)} rays (first {bad[:5]}); finer grid or refusal required')
    return {'ceiling': ceiling, 'rays_failing': 0}

def bind_outputs(rows, cap, label):
    """Candidate/Current rows must be [px,py,Lr,Lg,Lb,T] (the shared checker's native_rows layout: the
    checker compares a[2..4] to reference Lr..Lb and a[5] to reference T) in EXACT capture order, one per ray."""
    need(len(rows) == len(cap['rays']), f'{label}: coverage {len(rows)} != {len(cap["rays"])}')
    for i, (row, r) in enumerate(zip(rows, cap['rays'])):
        need(len(row) == 6, f'{label}: row width')
        need(row[0] == r[0] and row[1] == r[1], f'{label}: reordered/misaligned ray at index {i}')
    return rows

def evaluate_capture(cap, candidate, current, reference, *, rays_sha256, zone_sha256, **identity):
    clock = validate_capture(cap, zone_sha256=zone_sha256, **identity)
    bind_outputs(candidate, cap, 'candidate'); bind_outputs(current, cap, 'current')
    need(reference.get('rays_sha256') == rays_sha256, 'reference/ray identity mismatch')
    conv = check_convergence(reference)
    result = shared_gate().evaluate(candidate, current, reference, rays_sha256)
    result['capture_identity'] = {'rays_sha256': rays_sha256, 'zone_sha256': zone_sha256, 'clock': clock, 'ray_count': len(cap['rays'])}
    result['reference_convergence_guard'] = conv
    return result

def surveyor_capture(manifest_path, medium_clocks, zone_sha256):
    """Lossless adapter: ray bytes/hash remain the original verified Surveyor file.
    Clock bindings are required explicitly; never infer independent medium clocks.
    """
    spec=importlib.util.spec_from_file_location('surveyor_capture', HERE/'sonnet_surveyor_capture.py')
    mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
    manifest, raw, _ = mod.verify_case(HERE.parents[1], manifest_path, zone_sha256)
    cap=dict(raw, schema=SCHEMA, zone_sha256=zone_sha256,
             medium_clocks=medium_clocks, ray_count=len(raw['rays']))
    validate_capture(cap, zone_sha256=zone_sha256)
    return cap, sha256_file(manifest['rays'])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument('--capture', type=Path)
    group.add_argument('--manifest', type=Path, help='verified Surveyor manifest; requires explicit --medium-clocks')
    p.add_argument('--medium-clocks', type=Path, help='JSON map of actual per-medium clock bindings')
    for n in ['reference', 'reference-receipt', 'candidate', 'current', 'zone', 'output']: p.add_argument('--' + n, type=Path, required=True)
    p.add_argument('--expected-count', type=int); p.add_argument('--expected-clock', type=float)
    a = p.parse_args()
    zone_sha = sha256_file(a.zone)
    if a.manifest:
        need(a.medium_clocks is not None, 'explicit actual medium clocks required for Surveyor capture')
        cap, sha = surveyor_capture(a.manifest, read_json(a.medium_clocks), zone_sha)
    else:
        cap = read_json(a.capture); sha = sha256_file(a.capture)
    ref = reference_to_checker_form(read_json(a.reference), cap, sha,
          reference_identity=read_json(a.reference_receipt), reference_sha256=sha256_file(a.reference))
    d = evaluate_capture(cap, read_json(a.candidate), read_json(a.current), ref, rays_sha256=sha, zone_sha256=zone_sha,
                         expected_clock=a.expected_clock, expected_count=a.expected_count)
    a.output.write_text(json.dumps(d, indent=2) + '\n')
    print('ORACLE', d['rays'], 'PASS' if d['both_resolutions_pass'] else 'FAIL', 'beyond Current max', d['finest']['rays_beyond_current_max'])
    raise SystemExit(0 if d['both_resolutions_pass'] else 2)
if __name__ == '__main__': main()
