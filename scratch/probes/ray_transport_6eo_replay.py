"""Replay section6eo CPU pilot identity, exact continuity and convergence.
Replays stored execution evidence; does not rerun the CPU integrator or establish
whole-population accuracy, formal error bounds, GPU cost, or frame economics.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28, commissioned by Zach's next-pass request.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

import sonnet_oracle_capture_gate as G
from ray_transport_6eo_reference_pilot import ROOT, OLD_REF, digest, need, read


def sha(b):
    return hashlib.sha256(b).hexdigest()


def same_numbers(a, b):
    return len(a) == len(b) and all(len(x) == len(y) and
        all(struct.pack('<d', u) == struct.pack('<d', v) for u,v in zip(x,y))
        for x,y in zip(a,b))


def check_job(cap, raw, receipt, job, source_sha, *, ray_bytes, reference_bytes,
              expected_rows, expected_origin, expected_clock, old_reference=None):
    need(receipt['status'] == 'PILOT_PASS_NOT_POPULATION_ACCEPTANCE' and
         receipt['exit_code'] == 0, 'uncompleted execution receipt')
    need(receipt['before'] == receipt['after'] and
         receipt['input_sha256_before'] == receipt['input_sha256_after'], 'execution changed')
    need(sha(ray_bytes) == job['rays_sha256'] == receipt['rays_sha256'] ==
         receipt['input_sha256_before'], 'input byte identity')
    need(sha(reference_bytes) == receipt['reference_sha256'], 'reference byte identity')
    need(receipt['log2N'] == 16 and receipt['threads'] == 4 and
         0 < receipt['elapsed_seconds'] <= 300, 'bounded work contract')
    need(receipt['generator_sha256'] == receipt['before']['generator_sha256'] == source_sha,
         'generator identity')
    need(cap['selection']['indices'] == job['indices'] and
         cap['selection']['parent_rays_sha256'] == job['parent_rays_sha256'], 'selection identity')
    need(cap['rays'] == expected_rows and cap['origin'] == expected_origin and
         cap['time'] == expected_clock, 'selected native rows/bindings differ from parent capture')
    G.validate_capture(cap, zone_sha256=receipt['before']['zone_sha256'],
                       expected_count=job['count'], expected_origin=expected_origin,
                       expected_clock=expected_clock)
    need(len(raw['rows']) == job['count'] and
         all(len(r)==8 and all(math.isfinite(x) for x in r) for r in raw['rows']),
         'reference coverage/finite')
    ref = G.reference_to_checker_form(raw, cap, job['rays_sha256'],
                reference_identity=receipt, reference_sha256=sha(reference_bytes))
    G.check_convergence(ref)
    conv = dict(T=max(abs(r[0]-r[4]) for r in raw['rows']),
                RGB=max(abs(r[c]-r[c+4]) for r in raw['rows'] for c in (1,2,3)))
    need(conv == receipt['convergence_max'], 'stored convergence differs')
    if old_reference is not None:
        need(all(cap['rays'][k][:2] == old_reference['rows'][i][:2]
                 for k,i in enumerate(job['indices'])) and
             same_numbers(raw['rows'], [old_reference['rows'][i][2:] for i in job['indices']]),
             'original reference continuity')
    return dict(rays=job['count'], seconds=receipt['elapsed_seconds'], convergence_max=conv)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--record', type=Path, required=True)
    p.add_argument('--archive', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    rec = read(a.record)
    need(digest(a.archive) == rec['archive_sha256'], 'archive identity')
    source_sha = digest(G.HERE / 'veil_projected_reference.cpp')
    need(source_sha == rec['generator_sha256'], 'current original generator changed')
    need(digest(OLD_REF) == rec['old_reference_sha256'], 'old reference identity')
    old_reference = read(OLD_REF)
    parent_record = read(ROOT / rec['parent_record'])
    parent_path = ROOT / rec['parent_archive']
    need(digest(parent_path) == rec['parent_archive_sha256'] == parent_record['archive_sha256'],
         'parent capture archive identity')
    with zipfile.ZipFile(parent_path) as z:
        old_bytes = z.read('native/original-rays.json')
        old = json.loads(old_bytes)
        live_bytes = z.read('capture/census-live16.bin')
        meta = json.loads(z.read('capture/metadata.json'))
        for name, b in [('native/original-rays.json',old_bytes),('capture/census-live16.bin',live_bytes)]:
            need(sha(b) == parent_record['members_sha256'][name], 'parent member identity')
    def parent_rows(job):
        if job['name'] == 'continuity':
            need(job['parent_rays_sha256'] == sha(old_bytes), 'old parent rays')
            return [old['rays'][i] for i in job['indices']], old['origin'], old['time']
        need(job['parent_rays_sha256'] == meta['manifest']['rays_sha256'], 'new parent rays')
        rows = []
        for i in job['indices']:
            w = struct.unpack_from('<16I',live_bytes,i*64)
            f = struct.unpack_from('<16f',live_bytes,i*64)
            need(w[0] == w[3] == 1, 'selected native record incomplete')
            rows.append([float(w[1]),float(w[2]),*f[8:11],f[7],f[11],*f[12:16]]+[0.]*12)
        return rows, meta['ray_header']['origin'], meta['ray_header']['time']

    results, cached, distinct = {}, {}, set()
    with zipfile.ZipFile(a.archive) as z:
        for name,h in rec['members_sha256'].items():
            need(sha(z.read(name)) == h, 'member identity '+name)
        for entry in rec['executions']:
            prefix = entry['prefix']
            cap_b, ref_b = z.read(entry['rays']), z.read(prefix+'-reference.json')
            cap, raw = json.loads(cap_b), json.loads(ref_b)
            receipt = json.loads(z.read(prefix+'-receipt.json'))
            selection_b = z.read(entry['selection'])
            need(sha(selection_b) == receipt['selection_sha256'], 'frozen selection byte identity')
            selection = json.loads(selection_b)
            job = next(j for j in selection['jobs'] if j['name'] == entry['job'])
            expected, origin, clock = parent_rows(job)
            kwargs = dict(ray_bytes=cap_b,reference_bytes=ref_b,expected_rows=expected,
                          expected_origin=origin,expected_clock=clock,
                          old_reference=old_reference if entry['job']=='continuity' else None)
            results[prefix] = check_job(cap, raw, receipt, job, source_sha, **kwargs)
            build = z.read(entry['build_receipt'])
            need(sha(build) == receipt['before']['build_receipt_sha256'], 'build receipt identity')
            need(json.loads(build)['cpp_sha256'] == source_sha and
                 json.loads(build)['engine_src_tree'] == receipt['before']['engine_src_tree'], 'build source identity')
            if entry['job'] != 'continuity':
                distinct.update(tuple(r[:2]) for r in cap['rays'])
            cached[prefix] = (cap, raw, receipt, job, kwargs)
        # Both optimization controls must preserve all eight values per selected
        # ray, including both nested resolutions, without a tolerance allowance.
        for name in ('continuity','uniform-stress'):
            baseline = cached['base/'+name][1]
            need(cached['optimized/'+name][1]['columns'] == baseline['columns'] and
                 cached['optimized-first/'+name][1]['columns'] == baseline['columns'] and
                 same_numbers(cached['optimized/'+name][1]['rows'], baseline['rows']) and
                 same_numbers(cached['optimized-first/'+name][1]['rows'], baseline['rows']),
                 'optimized pilot numerical parity')
    controls = []
    def refuse(label, change, message):
        cap, raw, receipt, job, kw = copy.deepcopy(cached['optimized-first/uniform-stress'])
        change(cap,raw,receipt,job,kw)
        try:
            check_job(cap,raw,receipt,job,source_sha,**kw)
        except ValueError as e:
            need(message in str(e), 'wrong refusal for '+label+': '+str(e))
            controls.append(label)
            return
        raise ValueError('mutation admitted: '+label)
    refuse('changed input during execution', lambda c,r,s,j,k:s.update(input_sha256_after='changed'), 'execution changed')
    refuse('timeout cannot be accepted', lambda c,r,s,j,k:s.update(exit_code='timeout'), 'uncompleted')
    refuse('stale generator', lambda c,r,s,j,k:s.update(generator_sha256='changed'), 'generator identity')
    def altered_ray(c,r,s,j,k):
        c['rays'][0][2] += .01
        k['ray_bytes'] = (json.dumps(c)+'\n').encode()
        h = sha(k['ray_bytes'])
        j['rays_sha256'] = h
        s.update(rays_sha256=h,input_sha256_before=h,input_sha256_after=h)
    refuse('re-signed altered native ray', altered_ray, 'selected native rows/bindings')
    def truncate(c,r,s,j,k):
        r['rows'].pop()
        k['reference_bytes'] = (json.dumps(r)+'\n').encode()
        s['reference_sha256'] = sha(k['reference_bytes'])
    refuse('re-signed truncated reference', truncate, 'reference coverage')
    def divergent(c,r,s,j,k):
        r['rows'][0][4] = r['rows'][0][0] + 1e-4
        k['reference_bytes'] = (json.dumps(r)+'\n').encode()
        s['reference_sha256'] = sha(k['reference_bytes'])
    refuse('re-signed nonconverged reference', divergent, 'failed convergence')
    def nan(c,r,s,j,k):
        r['rows'][0][0] = float('nan')
        k['reference_bytes'] = json.dumps(r).encode()
        s['reference_sha256'] = sha(k['reference_bytes'])
    refuse('re-signed nonfinite reference', nan, 'reference coverage/finite')
    report = dict(scope=__doc__.splitlines()[0], executions=results, negative_controls=controls,
                  distinct_new_capture_rays=len(distinct), full_population=230400,
                  population_acceptance=False, gpu_execution=False)
    need(results == rec['results'] and len(distinct) == rec['distinct_new_capture_rays'], 'record result mismatch')
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(f"PASS: {len(results)} stored CPU executions, {len(distinct)} distinct new rays, {len(controls)} targeted corruptions; no population/GPU acceptance")


if __name__ == '__main__':
    main()
