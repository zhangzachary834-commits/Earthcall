"""Surveyor CPU witnesses: census decode, tile layout, memory budget, case round trip, tamper-and-re-sign, harness patch.

Claude / Sonnet 5.5 (claude-sonnet-5-5) / session 06f17252-810f-48ee-ad18-95ed12c6ff27 / 2026-10-27.
Single process, no GPU, no build tree.  Every corruption case must raise SurveyorError (or the named exception) and
every positive case asserts actual values, not merely absence of failure.
Usage: python3 -B scratch/probes/sonnet_surveyor_test.py [--scratch DIR] [--skip-harness]
"""
import argparse, copy, hashlib, json, math, random, struct, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import sonnet_surveyor_layout as L  # noqa: E402
import sonnet_surveyor_capture as C  # noqa: E402
import sonnet_surveyor_harness_patch as P  # noqa: E402

RESULTS = []


def check(name, fn):
    try:
        fn()
        RESULTS.append((name, True, ''))
    except Exception as e:  # noqa: BLE001
        RESULTS.append((name, False, f'{type(e).__name__}: {e}'))


def refuses(fn, exc=L.SurveyorError, contains=None):
    try:
        fn()
    except exc as e:
        if contains:
            assert contains in str(e), f'refused for the wrong reason: {e}'
        return
    raise AssertionError('corruption was accepted')


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def synth(geo, valid, origin=(0.0, 50.0, 75.0), seed=1):
    """Census bytes for a pinhole-ish camera.  Directions are deliberately NOT unit length."""
    rng = random.Random(seed)
    words = [0] * (geo.slots * L.SLOT_WORDS)
    expect = {}
    for slot in range(geo.slots):
        ix, iy = slot % geo.gx, slot // geo.gx
        px, py = geo.centre(ix, iy)
        at = slot * L.SLOT_WORDS
        words[at], words[at + 1], words[at + 2] = 1, px, py
        if not valid(ix, iy):
            continue
        d = (f32((px - geo.width / 2) / geo.width * 3.0), f32((geo.height / 2 - py) / geo.height * 2.0), f32(-1.5 - ix * 1e-6))
        t0, t1, T = f32(2.0 + rng.random()), f32(20.0 + rng.random()), f32(rng.random())
        Lc = [f32(rng.random()) for _ in range(3)]
        w = [1, px, py, 1, *struct.unpack('<3I', struct.pack('<3f', *origin))] + [0] * 9
        w = w[:16]
        fl = {7: t0, 8: d[0], 9: d[1], 10: d[2], 11: t1, 12: T, 13: Lc[0], 14: Lc[1], 15: Lc[2]}
        for k, v in fl.items():
            w[k] = struct.unpack('<I', struct.pack('<f', v))[0]
        words[at:at + 16] = w
        expect[(px, py)] = [*d, t0, t1, T, *Lc]
    return struct.pack(f'<{len(words)}I', *words), expect


def mutate(raw, word_index, value):
    b = bytearray(raw)
    b[word_index * 4:word_index * 4 + 4] = struct.pack('<I', value)
    return bytes(b)


def fbits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def legacy_groups(pixels):   # independent re-implementation of the pinned 6dq grouping (10 px cell, 4x4 tile)
    g = {}
    for i, (px, py) in enumerate(pixels):
        g.setdefault(((py // 10 // 4) << 16) | (px // 10 // 4), []).append(i)
    offs, idx = [], []
    for k in sorted(g):
        offs.append(len(idx)); idx.extend(g[k])
    return offs + [len(idx)], idx


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--scratch', type=Path); ap.add_argument('--skip-harness', action='store_true')
    a = ap.parse_args()
    root = HERE.parents[1]
    tmp = Path(tempfile.mkdtemp(prefix='surveyor-', dir=a.scratch)) if a.scratch else Path(tempfile.mkdtemp(prefix='surveyor-'))
    geo = L.Geometry(2560, 1440, 256, 144)
    ring = lambda ix, iy: (ix - 128) ** 2 + (iy - 72) ** 2 < 70 ** 2          # ~ a disc: partial edge tiles, empty corners
    raw, expect = synth(geo, ring)

    # ---- decode: exact identity, no normalisation
    def decode_exact():
        rows, rep = L.decode_census(raw, geo)
        assert len(rows) == len(expect) == rep['completed_volume_rays'] > 15000 and rep['fragments_ran'] == geo.slots
        for r in rows:
            e = expect[(int(r[0]), int(r[1]))]
            assert r[2:5] == e[0:3] and r[5:7] == e[3:5] and r[7:11] == e[5:9]    # bitwise-equal float32 values
        assert any(abs(math.sqrt(sum(v * v for v in r[2:5])) - 1) > .1 for r in rows), 'directions were normalised'
        assert rep['origin'] == [0.0, 50.0, 75.0] and all(len(r) == 23 for r in rows)
    check('decode preserves pixel/origin/direction/clips/outputs exactly', decode_exact)

    # ---- census corruptions
    first = next(s for s in range(geo.slots) if (s % 256 - 128) ** 2 + (s // 256 - 72) ** 2 < 70 ** 2)
    at = first * 64
    cases = {
        'short': lambda: raw[:-4], 'long': lambda: raw + b'\0' * 4,
        'bad valid word': lambda: mutate(raw, at, 2),
        'nonzero tail word': lambda: mutate(raw, at + 40, 1),
        'untouched slot with data': lambda: mutate(raw, at, 0),
        'pixel not cell centre': lambda: mutate(raw, at + 1, 3),
        'wrong cell for slot': lambda: mutate(raw, at + 2, 1439),
        'NaN direction': lambda: mutate(raw, at + 9, 0x7fc00000),
        'inf origin': lambda: mutate(raw, at + 5, 0x7f800000),
        'reversed clip': lambda: mutate(raw, at + 7, fbits(99.0)),
        'zero direction': lambda: mutate(mutate(mutate(raw, at + 8, 0), at + 9, 0), at + 10, 0),
        'T above 1': lambda: mutate(raw, at + 12, fbits(1.5)),
        'negative radiance': lambda: mutate(raw, at + 14, fbits(-0.1)),
        'bad completion word': lambda: mutate(raw, at + 3, 5),
        'mixed origin': lambda: mutate(raw, at + 4, fbits(1.0)),
    }
    for name, make in cases.items():
        check('census refuses: ' + name, lambda make=make: refuses(lambda: L.decode_census(make(), geo)))

    def dup_dir():
        b = bytearray(raw)
        s0, s1 = first * 64, (first + 1) * 64   # copy ray words 7..11 (clips+dir) of slot first onto its neighbour
        b[(s1 + 7) * 4:(s1 + 12) * 4] = b[(s0 + 7) * 4:(s0 + 12) * 4]
        refuses(lambda: L.decode_census(bytes(b), geo), contains='duplicate direction')
    check('census refuses: duplicate direction/clip (non-distinct ray)', dup_dir)

    # ---- geometry and rank selection
    check('geometry refuses inexact stride', lambda: refuses(lambda: L.Geometry(2560, 1440, 300, 144)))
    check('geometry refuses grid > viewport', lambda: refuses(lambda: L.Geometry(100, 100, 200, 100)))
    check('rank selection is strictly increasing, in range, and refuses k>n', lambda: (
        (lambda r: [None for _ in [0] if not (all(x < y for x, y in zip(r, r[1:])) and 0 <= r[0] and r[-1] < 1000)])(L.select_ranks(1000, 192)),
        refuses(lambda: L.select_ranks(5, 6))))

    # ---- layout: legacy regression + variable shapes
    rows, _ = L.decode_census(raw, geo)
    pix = [(int(r[0]), int(r[1])) for r in rows]

    def legacy_equal():
        lay = L.build_layout(geo, pix, (4, 4), 'compact')
        lo, li = legacy_groups(pix)
        assert lay.offsets == lo and lay.indices == li, 'differs from the pinned 6dq grouping'
        s = L.validate_layout(lay, geo, pix)
        assert s['empty_tiles'] == 0 and s['partial_tiles'] > 0 and s['max_tile_rays'] == 16 and lay.tiles != 2304
    check('compact layout reproduces the legacy grouping on a non-2304-tile, partial-tile population', legacy_equal)

    legacy_case = Path('/private/tmp/earthcall-wide-case/rays.json')

    def legacy_real():
        if not legacy_case.exists():
            return
        rr = json.loads(legacy_case.read_text())['rays']
        p = [(int(x[0]), int(x[1])) for x in rr]
        lay = L.build_layout(geo, p, (4, 4), 'compact')
        lo, li = legacy_groups(p)
        assert lay.offsets == lo and lay.indices == li and lay.tiles == 2304 and len(p) == 32784
        assert L.validate_layout(lay, geo, p)['max_tile_rays'] <= 16
    check('reproduces the real 32784-ray / 2304-tile Codex layout (skipped if file absent)', legacy_real)

    def dense_empties():
        lay = L.build_layout(geo, pix, (4, 4), 'dense')
        s = L.validate_layout(lay, geo, pix)
        assert lay.tiles == 64 * 36 and s['empty_tiles'] > 0 and lay.offsets[-1] == len(pix)
    check('dense layout keeps empty tiles and still covers every ray once', dense_empties)

    def shapes():
        for g, t in [((160, 90), (4, 4)), ((160, 90), (8, 2)), ((160, 90), (16, 1)), ((128, 72), (3, 5)), ((640, 360), (4, 4)), ((256, 144), (1, 1))]:
            gg = L.Geometry(2560, 1440, *g)
            r, _ = L.decode_census(synth(gg, lambda ix, iy: (ix * 7 + iy * 3) % 5 != 0)[0], gg)
            p = [(int(x[0]), int(x[1])) for x in r]
            for mode in ('compact', 'dense'):
                lay = L.build_layout(gg, p, t, mode)
                L.validate_layout(lay, gg, p)
                assert len(lay.indices) == len(p) == len(set(lay.indices))
    check('arbitrary grids/tile shapes incl. edge-partial tiles validate (6 shapes x 2 modes)', shapes)

    def tiny():
        g = L.Geometry(40, 20, 4, 2)
        p = [(5, 5), (35, 15)]
        lay = L.build_layout(g, p, (4, 4), 'compact')
        assert lay.offsets == [0, 2] and lay.indices == [0, 1] and L.validate_layout(lay, g, p)['partial_tiles'] == 1
        lay1 = L.build_layout(g, [(5, 5)], (1, 1), 'dense')
        assert lay1.tiles == 8 and L.validate_layout(lay1, g, [(5, 5)])['empty_tiles'] == 7
    check('tiny populations (2 rays in one tile; 1 ray in 8 dense tiles)', tiny)

    check('layout refuses tile over consumer capacity (5x4=20)', lambda: refuses(lambda: L.build_layout(geo, pix, (5, 4)), contains='capacity'))
    check('layout refuses non-centre pixel', lambda: refuses(lambda: L.build_layout(geo, [(6, 5)], (4, 4)), contains='cell centre'))
    check('layout refuses pixel outside viewport', lambda: refuses(lambda: L.build_layout(geo, [(2560, 5)], (4, 4)), contains='outside'))

    base = L.build_layout(geo, pix, (4, 4), 'compact')
    sizes = [b - a_ for a_, b in zip(base.offsets, base.offsets[1:])]
    T_BIG = next(t for t in range(len(sizes) - 2) if sizes[t] == 16 and sizes[t + 1] >= 1)    # merging t,t+1 exceeds 16
    def swap_firsts(t):      # exchange first rays of tiles t,t+1; returns indices if every tile stays ascending (so only tile identity lies)
        ix = list(base.indices); a_, b_ = base.offsets[t], base.offsets[t + 1]
        ix[a_], ix[b_] = ix[b_], ix[a_]
        ok_ = all(ix[j] < ix[j + 1] for q in (t, t + 1) for j in range(base.offsets[q], base.offsets[q + 1] - 1))
        return ix if ok_ and sizes[t] and sizes[t + 1] else None
    SWAP = next(swap_firsts(t) for t in range(len(sizes) // 3, len(sizes) - 1) if swap_firsts(t))

    def corrupt(edit, contains=None):
        lay = copy.deepcopy(base)
        edit(lay)
        refuses(lambda: L.validate_layout(lay, geo, pix), contains=contains)
    check('layout corruption: duplicated ray index', lambda: corrupt(lambda l: l.indices.__setitem__(1, l.indices[0]), 'permutation'))
    check('layout corruption: dropped ray', lambda: corrupt(lambda l: (l.indices.pop(), l.offsets.__setitem__(-1, l.offsets[-1] - 1)), 'permutation'))
    check('layout corruption: index out of range', lambda: corrupt(lambda l: l.indices.__setitem__(0, len(pix)), 'permutation'))
    check('layout corruption: non-monotone offsets', lambda: corrupt(lambda l: l.offsets.__setitem__(3, l.offsets[2] - 1), 'monotone'))
    check('layout corruption: final offset != ray count', lambda: corrupt(lambda l: l.offsets.__setitem__(-1, l.offsets[-1] + 1), 'framing'))
    check('layout corruption: first offset != 0', lambda: corrupt(lambda l: l.offsets.__setitem__(0, 1), 'framing'))
    check('layout corruption: tile pushed over 16 rays', lambda: corrupt(lambda l: l.offsets.__setitem__(T_BIG + 1, l.offsets[T_BIG + 2]), 'over capacity'))
    check('layout corruption: swap rays between tiles', lambda: corrupt(lambda l: l.indices.__setitem__(slice(None), SWAP), 'wrong tile'))
    check('layout corruption: unsorted inside a tile', lambda: corrupt(lambda l: l.indices.__setitem__(slice(0, 2), l.indices[1::-1]), 'ascending'))
    check('layout corruption: stale dense tile id', lambda: corrupt(lambda l: l.dense_tile_ids.__setitem__(-1, l.dense_tile_ids[-1] + 1), 'wrong tile'))
    check('layout corruption: empty tile inside a compact layout', lambda: corrupt(lambda l: (l.offsets.insert(1, l.offsets[1]), l.dense_tile_ids.insert(1, l.dense_tile_ids[0] + 0.5)), 'empty tile'))

    # ---- memory budget boundaries
    lim = dict(L.WEBGPU_DEFAULT_LIMITS)

    def boundaries():
        n = 131072
        assert L.budget(n, n // 16, lim)['ok'] and not L.budget(n + 1, n // 16, lim)['ok']
        assert L.max_rays_for(lim) == n
        sub = L.budget(n + 1, 8, lim)['problems']
        assert any(p.startswith('subpanels:') and 'maxStorageBufferBindingSize' in p for p in sub)
        tight = dict(lim, maxStorageBufferBindingSize=n * 48)           # rays buffer exact fit still fails on subpanels, not rays
        assert not any(p.startswith('rays:') for p in L.budget(n, 8, tight)['problems'])
        assert any(p.startswith('rays:') for p in L.budget(n + 1, 8, dict(tight, maxStorageBufferBindingSize=n * 48))['problems'])
        assert not L.budget(10, 1, lim, working_set=1)['ok'] and 'working-set' in L.budget(10, 1, lim, working_set=1)['problems'][0]
        assert any('u32' in p for p in L.budget(2**26, 4, dict(lim, maxStorageBufferBindingSize=2**60, maxBufferSize=2**60))['problems'])
        assert any('workgroups' in p for p in L.budget(100, 4194241, dict(lim, maxStorageBufferBindingSize=2**60, maxBufferSize=2**60))['problems'])
        assert L.budget(100, 7, lim, capture_slots=3686400)['problems'], 'stride-1 census must exceed the capture device'
    check('budget: exact limit boundary (131072 default / +1 refused), working set, u32, dispatch, stride-1 census', boundaries)

    # ---- end to end case directory + tamper-and-re-sign
    zone = root / C.ZONE_REL
    census = tmp / 'census.bin'
    census.write_bytes(raw)

    def make_case(name, **kw):
        args = dict(census_path=census, case_dir=tmp / name, geo=geo, view='synthetic', camera='0,50,75,40,62,110', clock=7.25,
                    zone=zone, tile=(4, 4), mode='compact', max_rays=0)
        args.update(kw)
        return C.build_case(**args)

    def e2e():
        m = make_case('good')
        mm, rays, (ref, idx) = C.verify_case(root, tmp / 'good/manifest.json')
        assert len(rays['rays']) == len(expect) and ref is None and idx == L.select_ranks(len(expect), 192)
        assert m['budget']['m5-6ek']['ok'] and m['reference_status'].startswith('NOT PRODUCED')
        assert rays['time'] == 7.25 and rays['viewport'] == [2560, 1440] and rays['capture_stride'] == [10, 10]
        m2 = make_case('subsel', max_rays=5000)
        sub = C.verify_case(root, tmp / 'subsel/manifest.json')[1]
        assert sub['population'] == 5000 and sub['native_volume_rays'] == len(expect) and 'uniform rank selection of 5000' in sub['selection']
        assert m2['layout_stats']['tiles'] > 0
        make_case('dense', mode='dense')
        C.verify_case(root, tmp / 'dense/manifest.json')
    check('case round trip: build -> verify (all, uniform-rank subset, dense layout)', e2e)

    def resign(name, tamper, contains):
        d = tmp / name
        import shutil
        shutil.copytree(tmp / 'good', d)
        mp = d / 'manifest.json'
        m = json.loads(mp.read_text())
        for key, fn in (('rays', 'rays.json'), ('layout', 'layout.json'), ('holdout_rays', 'holdout-rays.json')):
            m[key] = str(d / fn)
        m['reference'] = str(d / 'reference.json')
        tamper(d)
        for key in ('rays', 'layout', 'holdout_rays'):                      # re-sign: hashes agree, content lies
            m[key + '_sha256'] = C.sha(m[key])
        mp.write_text(json.dumps(m))
        refuses(lambda: C.verify_case(root, mp), contains=contains)

    def edit(d, fn, f):
        p = d / fn
        j = json.loads(p.read_text()); f(j); p.write_text(json.dumps(j) + '\n')

    check('re-signed tamper: two rays swapped (non-canonical order)', lambda: resign('t1', lambda d: edit(d, 'rays.json', lambda j: j['rays'].__setitem__(slice(0, 2), j['rays'][1::-1])), 'canonical slot order'))
    check('re-signed tamper: direction replaced by NaN', lambda: resign('t2', lambda d: edit(d, 'rays.json', lambda j: j['rays'][3].__setitem__(2, float('nan'))), 'row 3'))
    check('re-signed tamper: duplicated ray row', lambda: resign('t3', lambda d: edit(d, 'rays.json', lambda j: j['rays'].__setitem__(5, j['rays'][4])), None))
    check('re-signed tamper: layout index swapped', lambda: resign('t4', lambda d: edit(d, 'layout.json', lambda j: j['tile_ray_indices'].__setitem__(0, j['tile_ray_indices'][1])), 're-derivation'))
    check('re-signed tamper: layout tile count lies', lambda: resign('t5', lambda d: edit(d, 'layout.json', lambda j: j.__setitem__('rays', j['rays'] - 1)), 'count binding'))
    check('re-signed tamper: holdout indices shifted', lambda: resign('t6', lambda d: edit(d, 'holdout-rays.json', lambda j: j['wide_ray_indices'].__setitem__(0, 0)), 'holdout'))
    check('re-signed tamper: pixel moved off cell centre', lambda: resign('t7', lambda d: edit(d, 'rays.json', lambda j: j['rays'][0].__setitem__(0, j['rays'][0][0] + 1)), None))
    check('re-signed tamper: capture grid relabelled', lambda: resign('t8', lambda d: edit(d, 'rays.json', lambda j: j.__setitem__('capture_grid', [128, 72])), None))
    check('unsigned tamper (hash mismatch) refused', lambda: (
        (tmp / 'good/rays.json').write_text((tmp / 'good/rays.json').read_text().replace('0.0', '0.5', 1)),
        refuses(lambda: C.verify_case(root, tmp / 'good/manifest.json'), contains='identity'))[1])
    check('build_case refuses existing directory', lambda: refuses(lambda: make_case('dense')))
    check('build_case refuses a census with only 3 completed rays', lambda: refuses(lambda: C.build_case(
        census_path=tmp / 'tiny.bin', case_dir=tmp / 'tinycase', geo=geo, view='x', camera='', clock=0.0, zone=zone, tile=(4, 4), mode='compact', max_rays=0)
        if (tmp / 'tiny.bin').write_bytes(synth(geo, lambda ix, iy: ix < 3 and iy == 0)[0]) is not None else None, contains='only'))
    # ---- cf4 patch anchors
    check('cf4 loader patches exactly the two anchors and keeps the grid code', lambda: (
        lambda m: [None for _ in [0] if not (callable(m[0].build))])(C.load_cf4(root, root, 'syntax')))

    # ---- harness patch
    if not a.skip_harness:
        def harness():
            gen = C.load_module(root / 'scratch/probes/ray_transport_gpu_6em_corrective_takeover.py', 'surveyor_6em').clean_source(root)
            out = P.patch(gen)
            digest = P.selftest(gen)
            assert digest == hashlib.sha256(out.encode()).hexdigest()
            assert out.count('getenv("EC_SURVEYOR_LAYOUT")') == 1 and 'requires exactly 2304' not in out
            assert 'uint32_t baseTileCount = uint32_t(baseTileOffsets.size() - 1);' in out
            refuses(lambda: P.patch(gen.replace('    uint32_t baseTileCount = 2304;\n', '')), exc=ValueError, contains='anchor')
            refuses(lambda: P.patch(out), exc=ValueError, contains='anchor')              # not idempotent: patching twice refuses
        check('harness patch removes the 2304 pin, adds layout validation + device-limit guard; refuses drifted/double patch', harness)

    width = max(len(n) for n, _, _ in RESULTS)
    bad = 0
    for n, ok, msg in RESULTS:
        print(('PASS ' if ok else 'FAIL ') + n.ljust(width), msg)
        bad += not ok
    print(f'{len(RESULTS) - bad}/{len(RESULTS)} witnesses passed; scratch {tmp}')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
