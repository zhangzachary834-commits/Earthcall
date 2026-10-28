"""Surveyor: census decoding, deterministic tile layout and memory budget for DISTINCT native rays.

Claude / Sonnet 5.5 (claude-sonnet-5-5) / session 06f17252-810f-48ee-ad18-95ed12c6ff27 / 2026-10-27.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol), who relayed Zach's goal of faithful authored
rendering with a useful Northern Veil checkpoint.  Pure CPU; never touches a GPU.

What this owns (and the existing 6dq/6em route hard-codes):
  * the census record contract (256 B slot, 16 live words) written by the fragment-census shader in
    scratch/probes/ray_transport_cf4_trial.py `captureShader`;
  * pixel -> capture cell -> tile mapping for any grid whose cell stride divides the viewport,
    instead of `px/10`, `/4`, and `groups4x4.size()==2304`;
  * validated layout arrays (tileOffsets / tileRayIndices) with partial and (optionally) empty tiles;
  * a buffer-by-buffer memory budget checked against adapter limits.

Nothing is jittered, normalised, replicated or relabelled: a row is exactly what the shader recorded
for one real pixel.  Subsampling (`select_ranks`) only chooses which real rows to keep.
"""
import argparse, json, math, struct, sys
from dataclasses import dataclass
from pathlib import Path

SLOT_WORDS = 64                 # u32 words per census slot (`captureSlot=(dc.x+gx*dc.y)*64u`)
SLOT_BYTES = SLOT_WORDS * 4
LIVE_WORDS = 16                 # words 0..15 are written; 16..63 must remain zero
TILE_RAY_CAPACITY = 16          # consumer shaders loop `i < 16u` / `rayInTile = lid & 15u`
ROW_COLUMNS = 11                # px,py,dx,dy,dz,t0,t1,T,Lr,Lg,Lb  (+12 legacy zero placeholders)
ROW_PAD = 12
U32_MAX = 2**32 - 1

# GpuRayHeader 48, GpuSubpanel 16, GpuOverlap 16, GpuResult 16 (generated 6em harness structs).
BYTES_RAY, BYTES_SUB, BYTES_OVER, BYTES_RESULT = 48, 16, 16, 16
STREAM_CAPACITY = 65536         # fixed `for(capacity:{65536u})` streaming buffers in the harness
WEBGPU_DEFAULT_LIMITS = dict(name='webgpu-default (what volume_demand_probe gpu.init() gets: no requiredLimits)',
                             maxStorageBufferBindingSize=134217728, maxBufferSize=268435456,
                             maxComputeWorkgroupsPerDimension=65535)
# Observed by Antigravity in 6ek on Apple M5 when the harness requests adapter limits
# (docs/audits/GPU_RAY_TRANSPORT_6EK_INDIRECT_CONSUMER_AND_MULTI_SCALE_2026-10-27.md).  Re-read with the
# limits probe before relying on it.
OBSERVED_M5_LIMITS = dict(name='observed Apple M5 adapter limits as recorded in 6ek (unverified this session)',
                          maxStorageBufferBindingSize=4294967292, maxBufferSize=9534832640,
                          maxComputeWorkgroupsPerDimension=65535)
HOST_MEMORY_BYTES = 17179869184  # `sysctl hw.memsize` on this machine, unified memory


class SurveyorError(ValueError):
    pass


def need(cond, msg):
    if not cond:
        raise SurveyorError(msg)


@dataclass(frozen=True)
class Geometry:
    width: int
    height: int
    gx: int           # capture cells across
    gy: int           # capture cells down

    def __post_init__(self):
        need(self.width > 0 and self.height > 0 and 0 < self.gx <= self.width and 0 < self.gy <= self.height,
             'grid must satisfy 0 < grid <= viewport')
        need(self.width % self.gx == 0 and self.height % self.gy == 0,
             'grid must divide viewport exactly (shader centre is f32 floor((cell+.5)*W/g)); refusing inexact stride')

    @property
    def stride(self):
        return self.width // self.gx, self.height // self.gy

    @property
    def slots(self):
        return self.gx * self.gy

    @property
    def census_bytes(self):
        return self.slots * SLOT_BYTES

    def centre(self, ix, iy):
        sx, sy = self.stride
        return (2 * ix + 1) * sx // 2, (2 * iy + 1) * sy // 2   # == floor((i+.5)*stride)

    def cell_of_pixel(self, px, py):
        need(0 <= px < self.width and 0 <= py < self.height, f'pixel ({px},{py}) outside viewport')
        sx, sy = self.stride
        return px // sx, py // sy


# --------------------------------------------------------------------------------------------- census
def decode_census(raw, geo):
    """Decode a fragment-census dump.  Returns (rows, report).  rows are in slot (row-major cell) order."""
    need(len(raw) == geo.census_bytes, f'census size {len(raw)} != expected {geo.census_bytes}')
    words = memoryview(raw).cast('I')
    floats = memoryview(raw).cast('f')
    rows, origin, ran, completed = [], None, 0, 0
    for slot in range(geo.slots):
        at = slot * SLOT_WORDS
        if words[at] == 0:
            need(not any(words[at:at + SLOT_WORDS]), f'slot {slot}: untouched slot carries data')
            continue
        need(words[at] == 1, f'slot {slot}: bad valid word {words[at]}')
        ran += 1
        need(not any(words[at + LIVE_WORDS:at + SLOT_WORDS]), f'slot {slot}: nonzero words beyond the 16-word record')
        ix, iy = slot % geo.gx, slot // geo.gx
        need((words[at + 1], words[at + 2]) == geo.centre(ix, iy),
             f'slot {slot}: recorded pixel ({words[at+1]},{words[at+2]}) is not cell centre {geo.centre(ix, iy)}')
        if words[at + 3] != 1:       # fragment ran but never integrated (not a native volume ray)
            need(words[at + 3] == 0, f'slot {slot}: bad completion word {words[at+3]}')
            continue
        completed += 1
        o = tuple(words[at + 4:at + 7])               # compare origins bitwise
        if origin is None:
            origin = o
        need(o == origin, f'slot {slot}: mixed origins (single-origin harness)')
        d = floats[at + 8:at + 11]
        t0, t1 = floats[at + 7], floats[at + 11]
        T, L = floats[at + 12], floats[at + 13:at + 16]
        vals = [*d, t0, t1, T, *L, *floats[at + 4:at + 7]]
        need(all(math.isfinite(v) for v in vals), f'slot {slot}: nonfinite recorded value')
        need(any(v != 0.0 for v in d), f'slot {slot}: zero direction')
        need(t0 <= t1, f'slot {slot}: reversed depth clip {t0} > {t1}')
        need(0.0 <= T <= 1.0 and all(v >= 0.0 for v in L), f'slot {slot}: transmittance/radiance outside range')
        rows.append([float(words[at + 1]), float(words[at + 2]), *d, t0, t1, T, *L] + [0.0] * ROW_PAD)
    need(len(set((r[0], r[1]) for r in rows)) == len(rows), 'duplicate pixel identity')
    need(len(set(tuple(r[2:7]) for r in rows)) == len(rows), 'duplicate direction/clip (not distinct rays)')
    report = dict(slots=geo.slots, fragments_ran=ran, completed_volume_rays=completed,
                  origin=list(struct.unpack('<3f', struct.pack('<3I', *origin))) if origin else None)
    return rows, report


def select_ranks(n, k):
    """Uniform rank selection of k of n (same formula as the legacy capture); chooses, never alters."""
    need(0 < k <= n, f'cannot select {k} of {n}')
    return [min(n - 1, int((i + .5) * n / k)) for i in range(k)]


# --------------------------------------------------------------------------------------------- layout
@dataclass
class Layout:
    tile_w: int
    tile_h: int
    ntx: int
    nty: int
    mode: str
    offsets: list
    indices: list
    dense_tile_ids: list      # dense (row-major) id of each emitted tile

    @property
    def tiles(self):
        return len(self.offsets) - 1


def build_layout(geo, pixels, tile=(4, 4), mode='compact'):
    """pixels: list of (px,py) in canonical (slot) order; ray index == position in that list.

    compact: emit only non-empty tiles, ascending dense id (identical to the legacy std::map order).
    dense:   emit every tile of the ceil(gx/tw) x ceil(gy/th) grid, empties included.
    Within a tile rays keep ascending ray index (== row-major cell order)."""
    tw, th = tile
    need(tw > 0 and th > 0 and tw * th <= TILE_RAY_CAPACITY, f'tile {tw}x{th} exceeds the {TILE_RAY_CAPACITY}-ray consumer capacity')
    need(mode in ('compact', 'dense'), 'mode must be compact|dense')
    ntx, nty = -(-geo.gx // tw), -(-geo.gy // th)
    members = {}
    for i, (px, py) in enumerate(pixels):
        cx, cy = geo.cell_of_pixel(px, py)
        need(geo.centre(cx, cy) == (px, py), f'ray {i}: pixel ({px},{py}) is not a capture-cell centre')
        members.setdefault((cy // th) * ntx + cx // tw, []).append(i)
    ids = sorted(members) if mode == 'compact' else list(range(ntx * nty))
    offsets, indices = [], []
    for t in ids:
        offsets.append(len(indices))
        indices.extend(members.get(t, ()))
    offsets.append(len(indices))
    return Layout(tw, th, ntx, nty, mode, offsets, indices, ids)


def validate_layout(layout, geo, pixels):
    """Re-derive every property independently of how the layout was built; raises on any violation."""
    n, tiles = len(pixels), layout.tiles
    off, idx = layout.offsets, layout.indices
    need(tiles >= 1 and len(off) == tiles + 1 and off[0] == 0 and off[-1] == len(idx), 'offset framing')
    need(all(a <= b for a, b in zip(off, off[1:])), 'offsets not monotone')
    need(len(idx) == n and sorted(idx) == list(range(n)), 'indices are not an exact permutation (coverage/uniqueness)')
    need(max(idx, default=0) <= U32_MAX and tiles <= U32_MAX, 'index exceeds u32')
    need(len(layout.dense_tile_ids) == tiles and all(a < b for a, b in zip(layout.dense_tile_ids, layout.dense_tile_ids[1:])),
         'tile ids not strictly ascending')
    for t in range(tiles):
        lo, hi = off[t], off[t + 1]
        need(hi - lo <= TILE_RAY_CAPACITY and hi - lo <= layout.tile_w * layout.tile_h, f'tile {t} over capacity {hi-lo}')
        need(layout.mode == 'dense' or hi > lo, f'compact layout contains empty tile {t}')
        need(all(idx[j] < idx[j + 1] for j in range(lo, hi - 1)), f'tile {t} not in ascending ray order')
        for j in range(lo, hi):
            cx, cy = geo.cell_of_pixel(*pixels[idx[j]])
            need((cy // layout.tile_h) * layout.ntx + cx // layout.tile_w == layout.dense_tile_ids[t],
                 f'ray {idx[j]} sits in the wrong tile')
    return dict(tiles=tiles, empty_tiles=sum(1 for t in range(tiles) if off[t] == off[t + 1]),
                partial_tiles=sum(1 for t in range(tiles) if 0 < off[t + 1] - off[t] < layout.tile_w * layout.tile_h),
                max_tile_rays=max((off[t + 1] - off[t] for t in range(tiles)), default=0))


def layout_json(layout):
    return dict(tile=[layout.tile_w, layout.tile_h], tile_grid=[layout.ntx, layout.nty], mode=layout.mode,
                tile_count=layout.tiles, tile_offsets=layout.offsets, tile_ray_indices=layout.indices,
                dense_tile_ids=layout.dense_tile_ids)


# --------------------------------------------------------------------------------------------- budget
def budget(rays, tiles, limits, sub_cap=64, over_cap=16, k=1, capture_slots=0, working_set=None):
    """Buffer-by-buffer GPU/host budget of the 6em clean harness (K=1 distinct rays; k replicates a workload)."""
    n = rays * k
    nt = tiles * k
    buf = {  # name -> (bytes, is_storage_binding)
        'rays': (n * BYTES_RAY, True), 'subpanels': (n * sub_cap * BYTES_SUB, True),
        'overlaps': (n * over_cap * BYTES_OVER, True), 'outputs': (n * BYTES_RESULT, True),
        'tile_offsets': ((nt + 1) * 4, True), 'tile_ray_indices': (n * 4, True),
        'tile_lists_s1_s2_fb': (3 * nt * 4, True),
        'stream_singles': (STREAM_CAPACITY * sub_cap * BYTES_SUB, True),
        'stream_overlaps': (STREAM_CAPACITY * over_cap * BYTES_OVER, True)}
    readback = {'output_readback': n * BYTES_RESULT, 'diagnostic_readback_sub_over_ray':
                n * sub_cap * BYTES_SUB + n * over_cap * BYTES_OVER + n * BYTES_RAY}
    if capture_slots:
        buf['capture_census'] = (capture_slots * SLOT_BYTES, True)
        readback['capture_census_readback'] = capture_slots * SLOT_BYTES
    problems = []
    for name, (b, storage) in buf.items():
        if storage and b > limits['maxStorageBufferBindingSize']:
            problems.append(f'{name}: {b} B exceeds maxStorageBufferBindingSize {limits["maxStorageBufferBindingSize"]}')
        if b > limits['maxBufferSize']:
            problems.append(f'{name}: {b} B exceeds maxBufferSize {limits["maxBufferSize"]}')
    for name, b in readback.items():
        if b > limits['maxBufferSize']:
            problems.append(f'{name}: {b} B exceeds maxBufferSize {limits["maxBufferSize"]}')
    if n * max(sub_cap, over_cap) > U32_MAX:
        problems.append('WGSL u32 index id*maxSubPerRay overflows')
    if (nt + 63) // 64 > limits['maxComputeWorkgroupsPerDimension']:
        problems.append(f'classify dispatch {(nt + 63) // 64} workgroups exceeds per-dimension limit')
    gpu = sum(b for b, _ in buf.values()) + sum(readback.values())
    host = n * (24 * 16 + 96 + BYTES_RAY)   # estimate: nlohmann json row of 24 values + vector header + GpuRayHeader
    if working_set is not None and gpu > working_set:
        problems.append(f'GPU total {gpu} B exceeds working-set budget {working_set}')
    return dict(rays=n, tiles=nt, limits=limits['name'], buffers={k_: v[0] for k_, v in buf.items()},
                readbacks=readback, gpu_total_bytes=gpu, host_estimate_bytes=host,
                host_memory_bytes=HOST_MEMORY_BYTES, problems=problems, ok=not problems)


def max_rays_for(limits, sub_cap=64, over_cap=16, working_set=None):
    """Largest K=1 ray count the budget admits (tile count bounded by rays; compact tiles <= rays)."""
    lo, hi = 1, 1 << 26
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if budget(mid, mid, limits, sub_cap, over_cap, 1, working_set=working_set)['ok']:
            lo = mid
        else:
            hi = mid - 1
    return lo


def feasibility_table(width, height, limits_list, working_set):
    """Capture runs on the probe's default-limit device (WgpuDevice.hpp requests no limits); the consumer
    harness requests adapter limits, so the two sides are judged against different profiles."""
    cap = WEBGPU_DEFAULT_LIMITS
    out = []
    for s in (20, 16, 10, 8, 5, 4, 2, 1):
        if width % s or height % s:
            continue
        geo = Geometry(width, height, width // s, height // s)
        row = dict(stride=s, grid=[geo.gx, geo.gy], max_slots=geo.slots, census_MiB=round(geo.census_bytes / 2**20, 1),
                   capture_ok_default_device=geo.census_bytes <= min(cap['maxStorageBufferBindingSize'], cap['maxBufferSize']))
        for lim in limits_list:
            row['consumer_ok_' + lim['name'].split(' ')[0]] = budget(geo.slots, -(-geo.slots // 16), lim, working_set=working_set)['ok']
        out.append(row)
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    b = sub.add_parser('budget', help='print the memory budget for a ray/tile count')
    b.add_argument('--rays', type=int, required=True); b.add_argument('--tiles', type=int, required=True)
    b.add_argument('--limits', choices=['webgpu-default', 'm5-6ek'], default='m5-6ek')
    b.add_argument('--limits-json', type=Path, help='output of the limits probe; overrides --limits')
    b.add_argument('--working-set-bytes', type=int, default=int(HOST_MEMORY_BYTES * 0.5))
    f = sub.add_parser('feasible', help='feasible capture strides and distinct-ray ceilings')
    f.add_argument('--viewport', default='2560x1440'); f.add_argument('--working-set-bytes', type=int, default=int(HOST_MEMORY_BYTES * 0.5))
    a = p.parse_args()
    if a.cmd == 'budget':
        lim = json.loads(a.limits_json.read_text()) if a.limits_json else (WEBGPU_DEFAULT_LIMITS if a.limits == 'webgpu-default' else OBSERVED_M5_LIMITS)
        lim.setdefault('name', str(a.limits_json))
        print(json.dumps(budget(a.rays, a.tiles, lim, working_set=a.working_set_bytes), indent=2))
    else:
        w, h = map(int, a.viewport.split('x'))
        print(json.dumps(feasibility_table(w, h, [WEBGPU_DEFAULT_LIMITS, OBSERVED_M5_LIMITS], a.working_set_bytes), indent=2))
        for lim in (WEBGPU_DEFAULT_LIMITS, OBSERVED_M5_LIMITS):
            print(lim['name'], '-> max K=1 consumer rays:', max_rays_for(lim, working_set=a.working_set_bytes))


if __name__ == '__main__':
    main()
