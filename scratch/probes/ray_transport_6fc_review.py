"""Independent CPU replay of Sonnet §6fb native observations; no new GPU run.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import statistics as S
import sys
import zipfile


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def check(parity, timing, images):
    import ray_transport_6ey_replay as V
    need(parity['foreground_required'] and timing['foreground_required'], 'native foreground protocol')
    need(len(parity['captures']) == 18, 'complete captures')
    comparisons = []
    for index in [0, 32]:
        caps = {c['name']: c for c in parity['captures'] if c['index'] == index}
        need(len(caps) == 9, 'nine variants per pose')
        for name, c in caps.items():
            need(c['frame']['focused'], 'focused capture')
            if c['routed']:
                need(c['encoded_dispatches'] == 240 and c['encoded_passes'] == (1 if c['coalesced'] else 240), 'actual dispatch/pass count')
                need(c['publication']['nonfinite'] == 0, 'finite accepted publication')
                if c['work_limit'] == 0:
                    need(c['publication']['refused'] == 2560 * 1440 and c['publication']['accepted'] == 0, 'whole-ray forced refusal')
                else:
                    need(c['publication']['refused'] == 0 and c['publication']['nonempty'] > 100, 'actual normal contribution')
            if c['poisoned']:
                need(c['poison_nan_words'] == 2560 * 1440 * 4, 'observed publication poison')
        for a, b in [('orig', 'orig2'), ('orig', 'coal'), ('orig', 'origP'), ('orig', 'coalP'),
                     ('current', 'origR'), ('current', 'coalR'), ('current', 'coalRP')]:
            need(images[caps[a]['snapshot']] == images[caps[b]['snapshot']], 'independent decoded pixel equality')
            comparisons.append(dict(pose=index, a=a, b=b, compared_RGBA8_bytes=2560 * 1440 * 4))
        need(images[caps['orig']['snapshot']] != images[caps['current']['snapshot']], 'routed affects image')
        for name in ['orig2', 'coal', 'origP', 'coalP']:
            need(caps[name]['publication_fnv'] == caps['orig']['publication_fnv'], 'retained full-publication digest agreement')
        for name in ['coalR', 'coalRP']:
            need(caps[name]['publication_fnv'] == caps['origR']['publication_fnv'], 'refusal publication digest agreement')
        need(caps['orig']['dispatch_trace'] == caps['coal']['dispatch_trace'], 'same-process dispatch digest')
    samples = timing['samples']
    need(len(samples) == 24, 'complete 24-sample screen')
    need(len(timing['warmup']) == 192, 'three paid warm paths')
    orders = [[0, 1, 2], [1, 2, 0], [2, 0, 1], [2, 1, 0]]
    result = {}
    for serial in [False, True]:
        blocks, costs, submits = [], {i: [] for i in range(3)}, {i: [] for i in range(3)}
        for block in range(4):
            ss = sorted((s for s in samples if s['serial'] == serial and s['block'] == block), key=lambda s: s['position'])
            need([s['lane'] for s in ss] == orders[block], 'recorded rotated protocol (not ABBA)')
            by_lane = {}
            for s in ss:
                lane = s['lane']
                V.path(s['frames'], lane != 0)
                need(len(s['mode_activation_frames']) == 2, 'paid activation retained')
                if lane:
                    need(all(f['encoded_passes'] == (1 if lane == 2 else 240) and f['encoded_dispatches'] == 240 for f in s['frames']), 'measured lane execution')
                need(s['total_ms'] >= s['terminal_drain_ms'] >= 0, 'completion included')
                value = s['total_ms'] / 64
                costs[lane].append(value)
                submits[lane].extend(f['submit_ms'] for f in s['frames'])
                by_lane[lane] = value
            blocks.append(dict(block=block, coalesced_over_original=by_lane[2] / by_lane[1], coalesced_over_Current=by_lane[2] / by_lane[0]))
        result['fenced' if serial else 'queued'] = dict(mean_ms={str(i): S.mean(costs[i]) for i in range(3)},
                                                      endFrame_ms={str(i): S.mean(submits[i]) for i in range(3)}, blocks=blocks)
    return dict(decoded_pixel_comparisons=comparisons, decoded_compared_bytes=sum(c['compared_RGBA8_bytes'] for c in comparisons),
                measured_frames=1536, timing=result,
                publication_scope='All publication bytes were read and FNV64-hashed in the native process; raw publication arrays were not retained. This replay verifies recorded digests, not independent raw-publication equality.',
                accuracy_scope='No new authored reference on the complete 45-degree route; parity is not authored accuracy.')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--replay', type=Path)
    a = p.parse_args()
    root, work = a.root.resolve(), a.work.resolve()
    sys.path.insert(0, str(root / 'scratch/probes'))
    import ray_transport_6ey_replay as V
    work.mkdir(exist_ok=True)
    source = Path('/private/tmp/earthcall-6fb-engine')
    if a.replay:
        extracted = work / 'extracted'
        need(not extracted.exists(), 'fresh archive extraction')
        with zipfile.ZipFile(a.replay) as z:
            m = json.loads(z.read('manifest.json'))
            for name, digest in m['files'].items():
                path = Path(name)
                need(not path.is_absolute() and '..' not in path.parts, 'safe archive path')
                raw = z.read('blobs/' + digest)
                need(hashlib.sha256(raw).hexdigest() == digest, 'archive payload identity')
                out = extracted / name
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_bytes(raw)
        source = extracted / 'sonnet'
    meta = json.loads((source / 'build.json').read_text())
    if not a.replay:
        need(sha(source / 'engine') == meta['binary_sha256'], 'actual executed binary')
    for mode in ['parity', 'packaging']:
        receipt = json.loads((source / mode / 'receipt.json').read_text())
        need(receipt['exit_code'] == 0 and receipt['build_sha256'] == sha(source / 'build.json'), 'successful identity-bound native receipt')
        need(receipt['binary_sha256'] == meta['binary_sha256'], 'receipt binary binding')
        need(receipt['before'] == receipt['after'] == meta['after'], 'frozen execution sources')
        for name, digest in receipt['files_sha256'].items():
            need(sha(source / mode / name) == digest, 'retained native output/log integrity')
    parity = json.loads((source / 'parity/result.json').read_text())
    timing = json.loads((source / 'packaging/result.json').read_text())
    images = {c['snapshot']: V.png(source / 'parity' / c['snapshot']) for c in parity['captures']}
    result = check(parity, timing, images)
    negatives = []
    for label, mutate in [('missing-sample', lambda t: t['samples'].pop()),
                          ('forged-lane', lambda t: t['samples'][0].__setitem__('lane', 2)),
                          ('lost-focus', lambda t: t['samples'][0]['frames'][0].__setitem__('focused', False))]:
        broken = copy.deepcopy(timing)
        mutate(broken)
        try:
            check(parity, broken, images)
        except ValueError as e:
            negatives.append(dict(control=label, refusal=str(e)))
        else:
            raise ValueError('missed corruption: ' + label)
    result.update(schema='6fc-independent-6fb-evidence-review-1', negatives=negatives,
                  build_sha256=sha(source / 'build.json'), binary_sha256=meta['binary_sha256'])
    if a.replay:
        need(result == json.loads((extracted / 'review/result.json').read_text()), 'same complete retained replay')
    (work / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'decoded_pixel_comparisons'}, indent=2))


if __name__ == '__main__':
    main()
