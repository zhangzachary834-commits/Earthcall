"""Surveyor: bounded capture-and-layout route for a denser population of DISTINCT recorded native camera rays.

Claude / Sonnet 5.5 (claude-sonnet-5-5) / session 06f17252-810f-48ee-ad18-95ed12c6ff27 / 2026-10-27.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) on Zach's behalf; builds on the fixed 256x144 route in
ray_transport_wide_capture.py (Codex, 2026-10-22) and the pose capture of ray_transport_gpu_pose_capture.py.

Subcommands (nothing here touches a GPU unless `capture --execute-gpu` is passed, and that is the Sun's to schedule):
  plan           feasible strides / ray ceilings and the exact proposed invocation
  prepare        generate the capture probe for an arbitrary divisor grid and syntax-check it (default) or
                 `--full-build` it; writes only to a fresh external --output directory
  capture        run the prepared probe once (REQUIRES --execute-gpu) and convert its census
  convert        census.bin -> case directory (rays.json, layout.json, holdout-rays.json, manifest.json)
  verify         re-derive every property of a case directory from its files (variable N, any grid)
  patch-harness  emit the 6em clean harness with the tile/ray pin removed (see sonnet_surveyor_harness_patch.py)

Provenance: every row is one real pixel centre's recorded origin/direction/depth clips/native outputs.  Nothing is
jittered, normalised, replicated or upsampled.  The per-ray *medium clock* is not stored in the census; it is fixed by
(zone sha256, clock argument), both recorded in the manifest and bound again at verify time.
"""
import argparse, hashlib, importlib.util, json, math, os, shlex, subprocess, sys, types
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import sonnet_surveyor_layout as L  # noqa: E402
import sonnet_surveyor_harness_patch as P  # noqa: E402

ZONE_REL = 'saves/zones/Northern Veil/zone.ecform'
HOLDOUT = 192
REFERENCE_CMD = '<reference-bin> {holdout} 16 {n} 8 {out}'   # same arguments as ray_transport_wide_capture.py
GPU_FLAGS = ('--execute-gpu',)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def git_src_tree(repo):
    st = subprocess.check_output(['git', '-c', 'core.fsmonitor=false', 'status', '--porcelain', '--', 'src'], cwd=repo, text=True)
    L.need(not st.strip(), f'{repo}: dirty engine source')
    return subprocess.check_output(['git', 'rev-parse', 'HEAD:src'], cwd=repo, text=True).strip()


# ---------------------------------------------------------------------------------------------- prepare
class Runner:
    """Stands in for cf4's `subprocess`: strips dependency-file flags (so the shared build tree is never written),
    and in syntax mode turns each compile into -fsyntax-only and skips linking."""
    CalledProcessError = subprocess.CalledProcessError

    def __init__(self, mode):
        self.mode, self.calls = mode, []

    def run(self, args, **kw):
        args = list(args)
        compiling = '-c' in args
        if compiling:
            out = []
            skip = 0
            for i, a in enumerate(args):
                if skip:
                    skip -= 1
                elif a in ('-MD', '-MMD'):
                    pass
                elif a in ('-MT', '-MF'):
                    skip = 1
                else:
                    out.append(a)
            args = out
            if self.mode == 'syntax':
                i = args.index('-o')
                args = args[:i] + args[i + 2:] + ['-fsyntax-only']
        self.calls.append(shlex.join(args))
        if self.mode == 'syntax' and not compiling:
            return subprocess.CompletedProcess(args, 0)
        return subprocess.run(args, **kw)


def load_cf4(root, build_root, mode):
    """cf4_trial.build() restricted to its 3 fixed grids; this loads an exact-text-patched in-memory copy that
    (a) accepts any validated grid and (b) reads the compile database from `build_root`.  The file on disk is untouched."""
    text = (root / 'scratch/probes/ray_transport_cf4_trial.py').read_text()
    old_assert = "    assert capture_grid in ((64,36),(128,72),(256,144)), 'bounded native capture grids only'\n"
    old_build = "    b=root/'build';out.mkdir(parents=True,exist_ok=False)\n"
    for s in (old_assert, old_build):
        L.need(text.count(s) == 1, 'cf4_trial.py changed: patch anchor not found exactly once')
    text = text.replace(old_assert, "    assert len(capture_grid)==2 and all(type(v) is int and v>0 for v in capture_grid)\n")
    text = text.replace(old_build, "    b=_SURVEYOR_BUILD_ROOT/'build';out.mkdir(parents=True,exist_ok=False)\n")
    mod = types.ModuleType('surveyor_cf4')
    mod.__file__ = str(root / 'scratch/probes/ray_transport_cf4_trial.py')
    mod._SURVEYOR_BUILD_ROOT = build_root
    exec(compile(text, mod.__file__, 'exec'), mod.__dict__)
    mod.subprocess = runner = Runner(mode)
    return mod, runner


def check_generated(out, geo):
    """The generated renderer must carry the requested grid and no residue of the 64x36 default."""
    src = (out / 'cf4-renderer.cpp').read_text()
    gx, gy = geo.gx, geo.gy
    must = [f'{gx}u * {gy}u * 64u * sizeof(uint32_t)', f'vec2<f32>({gx}.0,{gy}.0)', f'vec2<u32>({gx}u,{gy}u)', f'dc.x+{gx}u*dc.y']
    for m in must:
        L.need(m in src, f'generated renderer lacks {m!r}')
    if (gx, gy) != (64, 36):
        for m in ('64u * 36u * 64u', '(64.0,36.0)', '(64u,36u)', 'dc.x+64u*dc.y'):
            L.need(m not in src, f'generated renderer still carries default-grid residue {m!r}')
    return dict(census_bytes=geo.census_bytes, renderer_sha256=hashlib.sha256(src.encode()).hexdigest())


def prepare(root, build_root, out, geo, full_build):
    L.need(out.resolve() != root and root not in out.resolve().parents, 'output must be outside the repository')
    t_root, t_build = git_src_tree(root), git_src_tree(build_root)
    L.need(t_root == t_build, f'engine src trees differ: {t_root} vs {t_build}')
    L.need(geo.census_bytes <= L.WEBGPU_DEFAULT_LIMITS['maxStorageBufferBindingSize'],
           f'census {geo.census_bytes} B exceeds the capture device binding limit '
           f'{L.WEBGPU_DEFAULT_LIMITS["maxStorageBufferBindingSize"]} (the probe requests no limits)')
    mod, runner = load_cf4(root, build_root, 'build' if full_build else 'syntax')
    mod.build(root, out, capture=True, capture_grid=(geo.gx, geo.gy))
    info = check_generated(out, geo)
    info.update(mode='full-build' if full_build else 'syntax-check-only', engine_src_tree=t_root,
                build_root=str(build_root), commands=runner.calls, grid=[geo.gx, geo.gy],
                capture_binary=str(out / 'cf4') if full_build else None)
    (out / 'surveyor-prepare.json').write_text(json.dumps(info, indent=2) + '\n')
    return info


def syntax_check_harness(source, build_root, root):
    """-fsyntax-only of a generated harness TU with the engine's own entry.cpp flags (no object, no dep file, no link)."""
    L.need(git_src_tree(root) == git_src_tree(build_root), 'engine src trees differ')
    db = json.loads((build_root / 'build/compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    args = Runner('syntax')
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(source)
    cmd = ['-std=c++20' if x == '-std=gnu++17' else x for x in cmd]   # the harness uses std::bit_cast, as ray_transport_probe_build.py does
    args.run(cmd, cwd=build_root / 'build', check=True)
    return args.calls[0]


# ---------------------------------------------------------------------------------------------- convert
def build_case(census_path, case_dir, geo, view, camera, clock, zone, tile, mode, max_rays, holdout=HOLDOUT):
    L.need(not case_dir.exists(), f'refusing existing case directory {case_dir}')
    raw = Path(census_path).read_bytes()
    rows, rep = L.decode_census(raw, geo)
    n_all = len(rows)
    L.need(n_all >= 5, f'only {n_all} completed volume rays')
    if max_rays and max_rays < n_all:
        keep = L.select_ranks(n_all, max_rays)
        rows, selection = [rows[i] for i in keep], f'uniform rank selection of {max_rays} of {n_all} real rows (before any evaluation)'
    else:
        selection = f'all {n_all} completed volume rays; no subsampling'
    pixels = [(int(r[0]), int(r[1])) for r in rows]
    layout = L.build_layout(geo, pixels, tile, mode)
    stats = L.validate_layout(layout, geo, pixels)
    h = min(holdout, len(rows))
    L.need(h >= 5, 'holdout too small')
    hold_idx = L.select_ranks(len(rows), h)
    head = {'scope': 'Distinct recorded native pixel-centre transport inputs from the fragment census; not frame timing, not full-viewport evidence',
            'viewport': [geo.width, geo.height], 'time': clock, 'origin': rep['origin'], 'camera': camera,
            'capture_grid': [geo.gx, geo.gy], 'capture_stride': list(geo.stride), 'census_slots': geo.slots,
            'native_volume_rays': n_all, 'population': len(rows), 'selection': selection,
            'census_sha256': hashlib.sha256(raw).hexdigest(),
            'column_groups': ['pixel x,y', 'direction x,y,z (unnormalised, as recorded)', 'clip start,end', 'native current T,Lr,Lg,Lb', '12 unused legacy placeholders']}
    case_dir.mkdir(parents=True)
    rays = dict(head, view=view, rays=rows)
    (case_dir / 'rays.json').write_text(json.dumps(rays) + '\n')
    hold = dict(head, view=view + '-holdout', rays=[rows[i] for i in hold_idx], wide_ray_indices=hold_idx)
    (case_dir / 'holdout-rays.json').write_text(json.dumps(hold) + '\n')
    lj = L.layout_json(layout)
    lj.update(viewport=[geo.width, geo.height], capture_grid=[geo.gx, geo.gy], rays=len(rows), layout_stats=stats)
    (case_dir / 'layout.json').write_text(json.dumps(lj) + '\n')
    limits = {'webgpu-default': L.WEBGPU_DEFAULT_LIMITS, 'm5-6ek': L.OBSERVED_M5_LIMITS}
    ws = L.HOST_MEMORY_BYTES // 2
    manifest = {'case': view, 'zone_sha256': sha(zone), 'zone': str(zone), 'rays': str(case_dir / 'rays.json'),
                'rays_sha256': sha(case_dir / 'rays.json'), 'layout': str(case_dir / 'layout.json'),
                'layout_sha256': sha(case_dir / 'layout.json'), 'holdout_rays': str(case_dir / 'holdout-rays.json'),
                'holdout_rays_sha256': sha(case_dir / 'holdout-rays.json'),
                'reference': str(case_dir / 'reference.json'),
                'reference_status': 'NOT PRODUCED: CPU reference is a heavy job the Sun has not scheduled',
                'reference_command': REFERENCE_CMD.format(holdout=case_dir / 'holdout-rays.json', n=len(hold_idx), out=case_dir / 'reference-raw.json'),
                'census_report': rep, 'layout_stats': stats, 'tile': list(tile), 'layout_mode': mode,
                'budget_assumptions': {'working_set_bytes': ws, 'basis': 'half of hw.memsize, an assumption not a measured Metal limit'},
                'budget': {k: L.budget(len(rows), layout.tiles, v, working_set=ws, capture_slots=geo.slots) for k, v in limits.items()},
                'harness_env': {'EC_POSE_RAYS': str(case_dir / 'rays.json'), 'EC_SURVEYOR_LAYOUT': str(case_dir / 'layout.json'),
                                'EC_POSE_CLOCK': str(clock), 'EC_GPU_SCALE': '1', 'EC_MATCH_REPEAT': '1'}}
    stub = case_dir / 'reference-STUB-NOT-A-REFERENCE.json'      # the harness opens EC_POSE_REFERENCE unconditionally and reads only rows
    stub.write_text(json.dumps({'stub': True, 'rows': [], 'note': 'placeholder so timing/parity runs can start; carries no accuracy information'}) + '\n')
    manifest['harness_env']['EC_POSE_REFERENCE'] = str(stub)
    manifest['harness_env_note'] = 'Replace EC_POSE_REFERENCE with the real reference.json before any accuracy claim.'
    (case_dir / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def verify_case(root, manifest_path, zone_hash=None):
    """Variable-N replacement for ray_transport_gpu_wide_stream.verify_case (which pins 32784 rays / 256x144).
    Returns (manifest, rays, (reference_rows, reference_indices)) like the original; reference may be absent."""
    mpath = Path(manifest_path)
    m = json.loads(mpath.read_text())
    zone = Path(m['zone'])
    L.need(zone_hash is None or m['zone_sha256'] == zone_hash, 'zone identity')
    L.need(sha(zone) == m['zone_sha256'], 'zone file changed since capture')
    for key in ('rays', 'layout', 'holdout_rays'):
        L.need(Path(m[key]).is_absolute() and sha(m[key]) == m[key + '_sha256'], f'{key} identity')
    rays = json.loads(Path(m['rays']).read_text())
    lay = json.loads(Path(m['layout']).read_text())
    hold = json.loads(Path(m['holdout_rays']).read_text())
    w, h = rays['viewport']
    gx, gy = rays['capture_grid']
    geo = L.Geometry(w, h, gx, gy)
    rows = rays['rays']
    L.need(len(rows) == rays['population'] == lay['rays'] and len(rows) >= 5, 'count binding')
    L.need(len(rays['origin']) == 3 and all(math.isfinite(v) for v in rays['origin']) and math.isfinite(rays['time']), 'bindings')
    for i, r in enumerate(rows):
        L.need(len(r) == L.ROW_COLUMNS + L.ROW_PAD and all(math.isfinite(x) for x in r) and r[5] <= r[6]
               and any(r[2:5]) and 0 <= r[0] < w and 0 <= r[1] < h, f'row {i}')
    L.need(len({tuple(r[:2]) for r in rows}) == len(rows) and len({tuple(r[2:7]) for r in rows}) == len(rows), 'distinctness')
    pixels = [(int(r[0]), int(r[1])) for r in rows]
    # canonical slot order: strictly ascending row-major cell index
    cells = [geo.cell_of_pixel(*p) for p in pixels]
    L.need(all(geo.centre(*c) == p for c, p in zip(cells, pixels)), 'pixel is not a capture-cell centre')
    L.need(all(a[1] * geo.gx + a[0] < b[1] * geo.gx + b[0] for a, b in zip(cells, cells[1:])), 'rows not in canonical slot order')
    expect = L.build_layout(geo, pixels, tuple(lay['tile']), lay['mode'])
    L.need(expect.offsets == lay['tile_offsets'] and expect.indices == lay['tile_ray_indices']
           and expect.dense_tile_ids == lay['dense_tile_ids'], 'layout does not match independent re-derivation')
    L.validate_layout(expect, geo, pixels)
    hi = hold['wide_ray_indices']
    L.need(hi == L.select_ranks(len(rows), len(hi)) and hold['rays'] == [rows[i] for i in hi], 'holdout selection')
    ref_pair = (None, hi)
    refp = Path(m['reference'])
    if refp.exists():
        ref = json.loads(refp.read_text())
        L.need(ref['ray_indices'] == hi and len(ref['rows']) == len(hi) and ref['finest_intervals'] == 65536 and ref['half_intervals'] == 32768, 'reference binding')
        for i, v in zip(hi, ref['rows']):
            L.need(len(v) == 10 and v[:2] == rows[i][:2] and all(math.isfinite(x) for x in v), 'reference alignment')
        ref_pair = (ref['rows'], hi)
    return m, rays, ref_pair


# ---------------------------------------------------------------------------------------------- CLI
def make_geo(a):
    w, h = map(int, a.viewport.split('x'))
    L.need(a.stride > 0 and w % a.stride == 0 and h % a.stride == 0, f'stride {a.stride} must divide {w}x{h} exactly')
    return L.Geometry(w, h, w // a.stride, h // a.stride)


def cases(root):
    return load_module(root / 'scratch/probes/ray_transport_gpu_pose_capture.py', 'surveyor_pose_cases').CASES


def run_capture(a, geo, root):
    L.need(a.execute_gpu, 'GPU capture needs --execute-gpu (scheduling belongs to the Sun)')
    cam, clock = cases(root)[a.case]
    work = a.work.resolve()
    L.need(not work.exists() and root != work and root not in work.parents, 'fresh external --work required')
    work.mkdir(parents=True)
    zone = root / ZONE_REL
    zh = sha(zone)
    census = work / 'census.bin'
    env = dict(os.environ, EC_CAM=cam, EC_VOLUME_SOURCE_MODE='authored', EC_CENSUS=str(census), EC_CF4_BASELINE='1')
    for k in ('EC_FULL_LATTICE', 'EC_REFERENCE_SAMPLES', 'EC_VOLUME_OUTPUT_ERROR_BUDGET'):
        env.pop(k, None)
    cmd = [str(a.capture_bin.resolve()), str(zone), '0123', str(geo.width), str(geo.height), str(clock), str(work / 'capture.rgba'), '1']
    with (work / 'capture.log').open('w') as log:
        subprocess.run(cmd, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    L.need(sha(zone) == zh, 'zone changed during capture')
    return convert(a, geo, root, census, work / 'case')


def convert(a, geo, root, census, case_dir):
    cam, clock = cases(root)[a.case]
    tile = tuple(int(v) for v in a.tile.split('x'))
    m = build_case(census, case_dir, geo, 'surveyor-' + a.case, cam, clock, root / ZONE_REL, tile, a.layout_mode, a.max_rays)
    verify_case(root, case_dir / 'manifest.json')
    print('CASE', case_dir, 'rays', m['census_report']['completed_volume_rays'], 'tiles', m['layout_stats']['tiles'], flush=True)
    return m


def plan(a, geo, root):
    ws = L.HOST_MEMORY_BYTES // 2
    print(json.dumps(L.feasibility_table(geo.width, geo.height, [L.WEBGPU_DEFAULT_LIMITS, L.OBSERVED_M5_LIMITS], ws), indent=1))
    me, prep, work = Path(__file__).resolve(), '<scratch>/surveyor-build', '<scratch>/surveyor-case'
    print('\nPROPOSED (stride %d => grid %dx%d, <=%d distinct rays):' % (a.stride, geo.gx, geo.gy, geo.slots))
    print(f'  python3 -B {me} prepare --root <worktree> --build-root <checkout-with-build/> --output {prep} --stride {a.stride}            # syntax check')
    print(f'  python3 -B {me} prepare ... --full-build                                                                       # real binary {prep}/cf4')
    print(f'  python3 -B {me} capture --root <worktree> --capture-bin {prep}/cf4 --work {work} --stride {a.stride} --case inside --execute-gpu   # GPU: Sun only')


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('cmd', choices=['plan', 'prepare', 'capture', 'convert', 'verify', 'patch-harness'])
    p.add_argument('--root', type=Path, default=HERE.parents[1])
    p.add_argument('--build-root', type=Path, help='checkout holding build/compile_commands.json with the same src tree')
    p.add_argument('--output', type=Path); p.add_argument('--work', type=Path); p.add_argument('--capture-bin', type=Path)
    p.add_argument('--census', type=Path); p.add_argument('--case-dir', type=Path); p.add_argument('--manifest', type=Path)
    p.add_argument('--viewport', default='2560x1440'); p.add_argument('--stride', type=int, default=4)
    p.add_argument('--case', default='inside'); p.add_argument('--tile', default='4x4')
    p.add_argument('--layout-mode', choices=['compact', 'dense'], default='compact')
    p.add_argument('--max-rays', type=int, default=0, help='0 = keep every completed ray; else uniform rank selection of real rows')
    p.add_argument('--syntax-check', action='store_true', help='patch-harness: also -fsyntax-only the result (needs --build-root)')
    p.add_argument('--full-build', action='store_true'); p.add_argument('--execute-gpu', action='store_true')
    a = p.parse_args()
    root = a.root.resolve()
    geo = make_geo(a)
    if a.cmd == 'plan':
        plan(a, geo, root)
    elif a.cmd == 'prepare':
        L.need(a.output and a.build_root, '--output and --build-root required')
        info = prepare(root, a.build_root.resolve(), a.output.resolve(), geo, a.full_build)
        print('PREPARED', info['mode'], 'grid', info['grid'], 'census_bytes', info['census_bytes'])
    elif a.cmd == 'capture':
        L.need(a.capture_bin and a.work, '--capture-bin and --work required')
        run_capture(a, geo, root)
    elif a.cmd == 'convert':
        L.need(a.census and a.case_dir, '--census and --case-dir required')
        convert(a, geo, root, a.census, a.case_dir.resolve())
    elif a.cmd == 'verify':
        m, rays, _ = verify_case(root, a.manifest.resolve())
        print('VERIFIED', m['case'], 'rays', len(rays['rays']), 'tiles', m['layout_stats']['tiles'])
    else:
        L.need(a.output, '--output required')
        gen = load_module(root / 'scratch/probes/ray_transport_gpu_6em_corrective_takeover.py', 'surveyor_6em').clean_source(root)
        patched = P.patch(gen)
        a.output.write_text(patched)
        print('PATCHED', a.output, hashlib.sha256(patched.encode()).hexdigest())
        if a.syntax_check:
            L.need(a.build_root, '--build-root required for --syntax-check')
            syntax_check_harness(a.output.resolve(), a.build_root.resolve(), root)
            print('SYNTAX OK (-fsyntax-only, engine entry.cpp flags)')


if __name__ == '__main__':
    main()
