"""Replay app-scoped recovered Metal observations; never claim clean economics.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29.
"""
import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
import copy
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import statistics as S
import sys
import xml.etree.ElementTree as X
import zipfile


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        while b := f.read(8 << 20):
            h.update(b)
    return h.hexdigest()


def read(p):
    return json.loads(p.read_text())


def xml_rows(path):
    root = X.parse(path).getroot()
    ids = {e.get('id'): e for e in root.iter() if e.get('id')}

    def q(e):
        while e.get('ref'):
            e = ids[e.get('ref')]
        return e

    def own(row):
        for e in row:
            if e.tag == 'process':
                p = q(e)
                if p.find('pid') is not None and q(p.find('pid')).text == '70203':
                    return True
        return False

    return root, q, own


def normalize(work):
    root, q, own = xml_rows(work / 'selected-tables.xml')
    present, submissions, gpu = [], [], []
    for node in root:
        rows = node.findall('row')
        if not rows:
            continue
        typ = rows[0][1].tag
        if typ == 'metal-command-buffer-id':
            present = [dict(time=int(q(r[0]).text), cb=int(q(r[1]).text)) for r in rows if own(r)]
        elif rows[0][2].tag == 'metal-event-name':
            for r in rows:
                if not own(r):
                    continue
                label = q(r[10]).get('fmt')
                match = re.fullmatch(r'Committed "(.*?)" (?:with (\d+) encoders|without encoders)', label)
                need(match is not None, 'known submission narrative format')
                submissions.append(dict(cb=int(q(r[14]).text), start=int(q(r[0]).text),
                                        duration=int(q(r[1]).text), label=match[1], encoders=int(match[2] or 0)))
        elif rows[0][2].tag == 'gpu-channel-name':
            for r in rows:
                if own(r):
                    gpu.append(dict(start=int(q(r[0]).text), duration=int(q(r[1]).text),
                                    channel=q(r[2]).text, latency=int(q(r[4]).text),
                                    depth=int(q(r[5]).text), label=q(r[6]).get('fmt'),
                                    state=q(r[7]).text, cb=int(q(r[15]).text),
                                    encoder=int(q(r[16]).text)))
    root, q, own = xml_rows(work / 'cpu-symbolicated.xml')
    cpu = []
    for r in root.findall('.//row'):
        if own(r):
            frames = [q(e).get('name') for e in q(r[-1]) if e.tag == 'frame']
            cpu.append(dict(time=int(q(r[0]).text), weight=int(q(r[5]).text), frames=frames))
    return dict(schema='6fa-normalized-app-only-1', pid=70203, presents=present,
                submissions=submissions, gpu=gpu, cpu=cpu, returned_sample_signposts=0,
                native=read(work / 'session/result.json'))


def union(spans):
    total, hi = 0, -1
    for lo, end in sorted(spans):
        total += max(0, end - max(lo, hi))
        hi = max(hi, end)
    return total


def check(j):
    from ray_transport_6ey_replay import path
    native = j['native']
    need(len(native['samples']) == 4 and [s['on'] for s in native['samples']] == [False, True, True, False], 'native ABBA')
    modes = []
    for sample in native['samples']:
        path(sample['frames'], sample['on'])
        need(len(sample['activation']) == 2, 'paid activation')
        modes.extend([sample['on']] * 66)
    need(len(native['warmup']) == 128 and all(v['focused'] for v in native['warmup']), 'focused warmup')
    need(len(j['presents']) == len(modes) == 264, 'complete native present ownership')
    boundaries = [p['cb'] for p in j['presents']]
    need(boundaries == sorted(set(boundaries)), 'strictly ordered present command buffers')
    cbs = {c['cb']: c for c in j['submissions']}
    need(len(cbs) == len(j['submissions']), 'unique app command-buffer submissions')
    groups = [[] for _ in boundaries]
    trailing = []
    for c in cbs.values():
        f = bisect_left(boundaries, c['cb'])
        (groups[f] if f < 264 else trailing).append(c)
    frames = []
    for f, group in enumerate(groups):
        need(cbs[boundaries[f]]['label'] == '(wgpu internal) Present', 'recorded present linkage')
        n = Counter(c['label'] for c in group)
        need(len(group) == (497 if modes[f] else 17), 'source-bound complete Metal buffer count')
        need(n['(wgpu internal) Pre Pass'] == (245 if modes[f] else 5), 'one extra prepass per routed compute pass')
        need(all(c['encoders'] == 0 for c in group if c['label'] == '(wgpu internal) Pre Pass'), 'prepass has no encoders')
        frames.append(dict(index=f, on=modes[f], submissions=len(group), prepasses=n['(wgpu internal) Pre Pass'], gpu=[]))
    for g in j['gpu']:
        need(all(type(g[k]) is int and g[k] >= 0 for k in ['start', 'duration', 'latency']), 'finite integral GPU times')
        need(g['cb'] in cbs, 'GPU command belongs to recorded app submission')
        c = cbs[g['cb']]
        need(c['encoders'] > 0 and g['state'] == 'Active', 'actual encoder activity')
        need(g['start'] - g['latency'] == c['start'] + c['duration'], 'GPU latency matches exact CPU commit endpoint')
        f = bisect_left(boundaries, g['cb'])
        if f < 264:
            frames[f]['gpu'].append(g)
    stage_names = ['setup', 'reset', 'classify', 'args', 'S1', 'S2', 'fallback', 'consumer']
    for f in frames:
        compute = defaultdict(list)
        spans = [(g['start'], g['start'] + g['duration']) for g in f['gpu']]
        need(spans, 'GPU activity for every native frame')
        for g in f['gpu']:
            if 'Compute Command' in g['label']:
                compute[g['cb']].append((g['start'], g['start'] + g['duration']))
        need(len(compute) == (241 if f['on'] else 1), 'prep plus exact 30x8 routed dispatch encoders')
        f['active_union_ms'] = union(spans) / 1e6
        f['active_span_ms'] = (max(b for a, b in spans) - min(a for a, b in spans)) / 1e6
        f['gap_ms'] = f['active_span_ms'] - f['active_union_ms']
        f['channels_ms'] = {channel: union([(g['start'], g['start'] + g['duration']) for g in f['gpu'] if g['channel'] == channel]) / 1e6
                            for channel in ['Compute', 'Vertex', 'Fragment']}
        stages = dict.fromkeys(stage_names, 0.)
        if f['on']:
            ordered = sorted(compute.items())
            for i, (_, spans) in enumerate(ordered[1:]):
                stages[stage_names[i % 8]] += union(spans) / 1e6
        f['source_order_stage_ms'] = stages
        del f['gpu']
    samples = []
    for s, native_sample in enumerate(native['samples']):
        cohort = frames[s * 66 + 2:s * 66 + 66]
        stages = {k: S.mean(f['source_order_stage_ms'][k] for f in cohort) for k in stage_names}
        samples.append(dict(order=s, on=native_sample['on'], frames=64,
                            instrumented_complete_ms_per_frame=native_sample['instrumented_complete_ms'] / 64,
                            app_active_union_mean_ms=S.mean(f['active_union_ms'] for f in cohort),
                            app_gap_mean_ms=S.mean(f['gap_ms'] for f in cohort),
                            source_order_stage_mean_ms=stages))
    bymode = {name: dict(app_active_union_ms=S.mean(f['active_union_ms'] for i, f in enumerate(frames) if f['on'] == on and i % 66 >= 2),
                         traced_endFrame_mean_ms=S.mean(row['submit_ms'] for sample in native['samples'] if sample['on'] == on for row in sample['frames']))
              for name, on in [('Current', False), ('routed', True)]}
    return dict(samples=samples, frames=frames, by_mode=bymode,
                presents=264, submissions=len(cbs), empty_prepass_count=sum(c['label'] == '(wgpu internal) Pre Pass' for c in cbs.values()),
                gpu_rows=len(j['gpu']), cpu_samples=len(j['cpu']), sample_signposts=j['returned_sample_signposts'],
                source_order_stage_inference=True, kernel_name_labels_observed=False,
                sampled_CPU_functions_symbolicated=False, clean_economics=False,
                scope='Recovered instrumented native trace. App active-interval union, not device-wide busy/utilization or scanout. Frame ownership inferred from recorded present CB boundaries with exact full-count/dispatch/latency checks; stage names inferred from unchanged source order.')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--replay', type=Path)
    a = p.parse_args()
    root, work = a.root.resolve(), a.work.resolve()
    sys.path.insert(0, str(root / 'scratch/probes'))
    work.mkdir(exist_ok=True)
    if a.replay:
        with zipfile.ZipFile(a.replay) as z:
            manifest = json.loads(z.read('manifest.json'))
            for name, digest in manifest.items():
                need(hashlib.sha256(z.read(name)).hexdigest() == digest, 'archive payload hash')
            j = json.loads(z.read('normalized.json'))
            expected = json.loads(z.read('analysis.json'))
        need(check(j) == expected['analysis'], 'independent retained trace replay')
        print('PASS retained app-only trace replay; no GPU run or clean speed claim')
        return
    source = Path('/private/tmp/earthcall-6ez-trace-notify')
    j = normalize(source)
    analysis = check(j)
    negatives = []
    for label, mutate in [
        ('missing-present', lambda j: j['presents'].pop()),
        ('forged-mode', lambda j: j['native']['samples'][1].__setitem__('on', False)),
        ('missing-prepass', lambda j: j['submissions'].pop(next(i for i, c in enumerate(j['submissions']) if c['label'] == '(wgpu internal) Pre Pass'))),
        ('unowned-GPU-command', lambda j: j['gpu'][0].__setitem__('cb', 1)),
        ('invalid-GPU-time', lambda j: j['gpu'][0].__setitem__('duration', float('nan'))),
        ('wrong-commit-latency', lambda j: j['gpu'][0].__setitem__('latency', j['gpu'][0]['latency'] + 1)),
    ]:
        broken = copy.deepcopy(j)
        mutate(broken)
        try:
            check(broken)
        except ValueError as error:
            negatives.append(dict(control=label, refusal=str(error)))
        else:
            raise ValueError('failed control: ' + label)
    (work / 'normalized.json').write_text(json.dumps(j, separators=(',', ':')) + '\n')
    artifacts = {name: sha(source / name) for name in ['selected-tables.xml', 'gpu-schema.xml', 'frame-schema.xml', 'cpu-symbolicated.xml', 'raw-recovery-toc.xml', 'metal.trace/Trace1.run/Attachments/trace-data.atrc']}
    receipts = [read(Path(d) / 'session/receipt.json') for d in ['/private/tmp/earthcall-6ez-trace-final', str(source)]]
    from ray_transport_6ey_engine import R
    for directory, receipt in zip([Path('/private/tmp/earthcall-6ez-trace-final'), source], receipts):
        meta = read(directory / 'build.json')
        need(sha(directory / 'build.json') == receipt['build_sha256'], 'native build receipt binding')
        need(sha(directory / 'engine') == meta['binary_sha256'], 'actual executed binary binding')
        need(sha(directory / 'executed-runner.py') == meta['script_sha256'], 'preserved executed runner binding')
        need(sha(directory / 'engine-trace.cpp') == meta['derived_cpp_sha256'], 'preserved derived main binding')
        need(receipt['before'] == receipt['after'] == meta['after'] == R.snapshot(), 'unchanged production source/fixture')
    need(receipts[1]['native_exit_code'] == 0 and receipts[1]['profiler_exit_code'] == -9, 'preserved failed-save receipt, successful native process')
    need(sum(r['elapsed_seconds'] for r in receipts) < 300, 'declared cumulative native enclosing cap')
    result = dict(schema='6fa-recovered-native-Metal-attribution-1', analysis=analysis,
                  negatives=negatives, raw_artifact_sha256=artifacts, attempt_receipts=receipts,
                  signature='Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a',
                  timestamp=dt.datetime.now().astimezone().isoformat())
    (work / 'analysis.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in analysis.items() if k != 'frames'}, indent=2))
    print('PASS six deliberate controls; full per-frame normalized observations retained')


if __name__ == '__main__':
    main()
