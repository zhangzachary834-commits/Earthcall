"""Identity-bound CPU reference pilot; never a whole-capture accuracy admission.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Commissioned by Zach after the five-Sonnet integration and section6en.
Uses Opus's unchanged original-AST generator and Oracle Keeper's convergence guard.
"""
import argparse
import datetime as dt
import gzip
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
ZONE = ROOT / 'saves/zones/Northern Veil/zone.ecform'
GENERATOR = HERE / 'veil_projected_reference.cpp'
IMPORTED_TEST = ROOT / 'tests/singularity/webgpu_line_quadrature_test.cpp'
OLD_REF = ROOT / 'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz'
OLD_SHA = 'cd403a013e75bfa748b6fc5c60ffe74a2df2fdae5bacce73605973d447dfa402'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def read(path):
    b = Path(path).read_bytes()
    return json.loads(gzip.decompress(b) if str(path).endswith('.gz') else b)


def write(path, data):
    Path(path).write_text(json.dumps(data, indent=2, allow_nan=False) + '\n')


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def uniform(n, count):
    need(1 < count <= n, 'invalid fixed uniform cohort')
    return [i * (n - 1) // (count - 1) for i in range(count)]


def prepare(manifest, old_rays_path, out):
    import sonnet_oracle_capture_gate as oracle
    need(not out.exists(), 'fresh output directory required')
    m = read(manifest)
    # The capture program applies this one clock to all four fixture members.
    # These are producer identifiers from the executed Bridge binding report;
    # differing clocks must refuse, never silently collapse to this scalar.
    bridge = read(ROOT / 'docs/audits/SONNET_BRIDGE_ROUTED_MODE2_TO_SCREEN_2026-10-27.json')
    ids = [x['producer'] for x in bridge['A_production_program']['member_evaluators']]
    clocks = dict.fromkeys(ids, 7.25)
    cap, original_sha = oracle.surveyor_capture(manifest, clocks, digest(ZONE))
    oracle.validate_capture(cap, zone_sha256=digest(ZONE), expected_clock=7.25,
                            expected_origin=[0, 50, 75], expected_count=230400)
    old = read(old_rays_path)
    ref = read(OLD_REF)
    need(digest(old_rays_path) == OLD_SHA == ref['rays_sha256'], 'old ray identity')
    need(len(old['rays']) == len(ref['rows']) == 32784, 'old coverage')
    need(old['origin'] == cap['origin'] and old['time'] == cap['time'], 'continuity pose/clock')
    for r, rr in zip(old['rays'], ref['rows']):
        need(r[:2] == rr[:2], 'old reference pixel order')
    continuity = sorted(set(uniform(len(old['rays']), 16) + [26392, 28559]))
    # Fixed BEFORE reference evaluation. Nine stride4 neighbours surround the
    # previously identified grazing failure; also include the nearest recorded
    # pixel to the previous healthy-bisection maximum-error ray.
    by_pixel = {tuple(r[:2]): i for i, r in enumerate(cap['rays'])}
    stress_pixels = [(2374 + dx, 1154 + dy) for dy in (-4, 0, 4) for dx in (-4, 0, 4)]
    previous_worst = old['rays'][28559][:2]
    nearest = min(by_pixel, key=lambda p: ((p[0]-previous_worst[0])**2 +
                                         (p[1]-previous_worst[1])**2, p))
    stress_pixels.append(nearest)
    stress = [by_pixel[p] for p in stress_pixels]
    pilot = sorted(set(uniform(len(cap['rays']), 64) + stress))
    cost = uniform(len(cap['rays']), 1024)
    out.mkdir(parents=True)
    jobs = []
    for name, source, source_sha, indices in [
            ('continuity', old, OLD_SHA, continuity),
            ('uniform-stress', cap, original_sha, pilot),
            ('cost-cohort', cap, original_sha, cost)]:
        selected = dict(source, schema=oracle.SCHEMA, zone_sha256=digest(ZONE),
                        medium_clocks=clocks, ray_count=len(indices),
                        rays=[source['rays'][i] for i in indices])
        # Headers from the parent describe the parent; retain them nested only.
        selected = {k: selected[k] for k in
                    ('schema','zone_sha256','time','origin','medium_clocks','ray_count','rays')}
        selected['selection'] = {'kind': 'bounded-pilot-not-acceptance',
                                 'parent_rays_sha256': source_sha, 'indices': indices}
        oracle.validate_capture(selected, zone_sha256=digest(ZONE))
        path = out / (name + '-rays.json')
        write(path, selected)
        jobs.append(dict(name=name, path=str(path), rays_sha256=digest(path),
                         parent_rays_sha256=source_sha, indices=indices, count=len(indices)))
    selection = dict(schema='6eo-pilot-selection-1', capture_manifest_sha256=digest(manifest),
                     parent_rays_sha256=original_sha, full_population=230400,
                     zone_sha256=digest(ZONE), generator_sha256=digest(GENERATOR),
                     old_reference_sha256=digest(OLD_REF), stress_pixels=stress_pixels,
                     cohort_policy='16 uniform old +2 known witnesses; 64 uniform new +10 stress; 1024 uniform cost',
                     log2N=16, threads=4, per_job_timeout_seconds=300, jobs=jobs)
    write(out / 'selection.json', selection)
    print(json.dumps({j['name']: j['count'] for j in jobs}))


def fingerprint(binary):
    metadata = binary.parent / 'source.json'
    build = read(metadata)
    need(build['cpp_sha256'] == digest(GENERATOR), 'build generator mismatch')
    src = git(ROOT, 'rev-parse', 'HEAD:src')
    need(src == build['engine_src_tree'] == git(build['build_root'], 'rev-parse', 'HEAD:src'), 'build engine mismatch')
    for root in (ROOT, Path(build['build_root'])):
        need(not git(root, 'status', '--porcelain', '--', 'src'), 'dirty engine source')
    optimized = []
    for item in build.get('optimized_engine_objects', []):
        need(digest(item['source']) == item['source_sha256'] and
             digest(item['object']) == item['object_sha256'], 'optimized evaluator identity mismatch')
        optimized.append(dict(source_sha256=item['source_sha256'], object_sha256=item['object_sha256']))
    return dict(binary_sha256=digest(binary), build_receipt_sha256=digest(metadata),
                generator_sha256=digest(GENERATOR), imported_test_sha256=digest(IMPORTED_TEST),
                zone_sha256=digest(ZONE), engine_src_tree=src,
                compile_database_sha256=digest(Path(build['build_root']) / 'build/compile_commands.json'),
                optimized_engine_objects=optimized)


def run(selection_path, binary, job_name):
    import sonnet_oracle_capture_gate as oracle
    selection = read(selection_path)
    job = next(j for j in selection['jobs'] if j['name'] == job_name)
    rays_path = Path(job['path'])
    need(digest(rays_path) == job['rays_sha256'], 'selection input changed')
    cap = read(rays_path)
    need(cap['selection']['indices'] == job['indices'] and
         cap['selection']['parent_rays_sha256'] == job['parent_rays_sha256'], 'selection provenance')
    oracle.validate_capture(cap, zone_sha256=digest(ZONE), expected_count=job['count'],
                            expected_clock=7.25, expected_origin=[0,50,75])
    need(selection['generator_sha256'] == digest(GENERATOR) and
         selection['zone_sha256'] == digest(ZONE), 'planned source/fixture changed')
    before = fingerprint(binary)
    base = selection_path.parent / job_name
    output = base.with_name(job_name + '-reference.json')
    receipt_path = base.with_name(job_name + '-receipt.json')
    need(not output.exists() and not receipt_path.exists(), 'no reference overwrite')
    argv = [str(binary), str(rays_path), '16', str(job['count']), '4', str(output)]
    started = dt.datetime.now(dt.timezone.utc).isoformat()
    t0 = time.monotonic()
    # Explicit whitelist: inherited EC_DUMP_PIXEL or other probe settings cannot
    # silently add diagnostic work or alter the selected job.
    env = {k:v for k,v in os.environ.items() if not k.startswith('EC_')}
    with base.with_name(job_name + '.log').open('w') as log:
        try:
            proc = subprocess.run(argv, cwd=ROOT, env=env, stdout=log,
                                  stderr=subprocess.STDOUT, timeout=300)
            code = proc.returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
    elapsed = time.monotonic() - t0
    after = fingerprint(binary)
    receipt = dict(schema='6eo-reference-execution-1', argv=argv, started_utc=started,
                   elapsed_seconds=elapsed, exit_code=code, threads=4, log2N=16,
                   input_sha256_before=job['rays_sha256'], input_sha256_after=digest(rays_path),
                   before=before, after=after, selection_sha256=digest(selection_path),
                   status='REFUSED', rays_sha256=digest(rays_path),
                   zone_sha256=digest(ZONE), time=cap['time'], origin=cap['origin'],
                   medium_clocks=cap['medium_clocks'], generator_sha256=digest(GENERATOR))
    # Always preserve failure/timeout evidence. An incomplete job has no accepted receipt.
    write(receipt_path, receipt)
    need(code == 0, f'reference job failed: {code}')
    need(before == after and receipt['input_sha256_before'] == receipt['input_sha256_after'], 'execution identity changed')
    raw = read(output)
    need(len(raw['rows']) == job['count'] and
         all(len(r)==8 and all(math.isfinite(x) for x in r) for r in raw['rows']), 'output coverage/finite')
    receipt['reference_sha256'] = digest(output)
    reference = oracle.reference_to_checker_form(raw, cap, digest(rays_path),
                    reference_identity=receipt, reference_sha256=digest(output))
    oracle.check_convergence(reference)
    receipt['convergence_max'] = {'T': max(abs(r[0]-r[4]) for r in raw['rows']),
                                'RGB': max(abs(r[c]-r[c+4]) for r in raw['rows'] for c in (1,2,3))}
    if job_name == 'continuity':
        need(digest(OLD_REF) == selection['old_reference_sha256'], 'old reference changed')
        previous = read(OLD_REF)
        differences = [abs(a-b) for row,i in zip(raw['rows'],job['indices'])
                       for a,b in zip(row,previous['rows'][i][2:])]
        receipt['continuity'] = dict(compared_values=len(differences), max_delta=max(differences),
                                     bit_identical_numeric_values=all(x == 0 for x in differences))
        need(max(differences) == 0, 'unchanged generator does not reproduce retained reference exactly')
    log_text = base.with_name(job_name + '.log').read_text()
    need('SOURCE authored radiance identically zero: yes' in log_text and 'MEDIA 4, clock 7.250' in log_text,
         'reference model precondition')
    match = re.search(r'RAYS (\d+), finest 2\^16 intervals, ([\d.]+) s on 4 threads, ([\de+.]+) field evaluations', log_text)
    need(match and int(match[1]) == job['count'], 'execution count/log')
    receipt['reported_field_evaluations_rounded'] = float(match[3])
    receipt['seconds_per_ray'] = elapsed / job['count']
    receipt['full_population_minutes_linear_estimate'] = elapsed / job['count'] * 230400 / 60
    receipt['status'] = 'PILOT_PASS_NOT_POPULATION_ACCEPTANCE'
    write(receipt_path, receipt)
    print(json.dumps(dict(job=job_name, rays=job['count'], seconds=elapsed,
                          convergence=receipt['convergence_max'], continuity=receipt.get('continuity'),
                          full_minutes_estimate=receipt['full_population_minutes_linear_estimate'])))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='command', required=True)
    a = sub.add_parser('prepare')
    a.add_argument('--manifest', type=Path, required=True)
    a.add_argument('--old-rays', type=Path, required=True)
    a.add_argument('--out', type=Path, required=True)
    a = sub.add_parser('run')
    a.add_argument('--selection', type=Path, required=True)
    a.add_argument('--binary', type=Path, required=True)
    a.add_argument('--job', choices=['continuity','uniform-stress','cost-cohort'], required=True)
    args = p.parse_args()
    if args.command == 'prepare':
        prepare(args.manifest.resolve(), args.old_rays.resolve(), args.out.resolve())
    else:
        run(args.selection.resolve(), args.binary.resolve(), args.job)


if __name__ == '__main__':
    main()
