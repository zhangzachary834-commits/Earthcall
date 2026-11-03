"""§6dd: Source-Pinned Continuation on `claude/ray-domain-transport-rung0` (`Earthcall-ray-transport`).
Implements the exact next steps specified by Sixth-One Sun (Codex / GPT-6.1 Sol) in §6dc:

1. `parallel-soa` & `parallel-diag`:
   - Exports all 32,784 parallel packets & headers (`parallel_packet_census_inputs`) and all 32,784 normal/forced
     output rows BEFORE any strict bit-parity gate throws, locating every endpoint/output divergence across the
     entire 32,784-ray capture and evaluating all 12 dual-resolution (65,536 & 32,768) holdout accuracy gates
     plus all-ray AST agreement.
   - `parallel-soa` dispatches `memberRootsMain` in 2D Member-Major Structure-of-Arrays (SoA) order:
     `@compute @workgroup_size(64, 1, 1)` dispatched as `((n + 63u) / 64u, 4u, 1u)` with
     `rayId = gid.x`, `member = gid.y`, `pairId = member * cfg.rayCount + rayId`.
     This keeps `curtains[member]` 100% uniform across every 64-thread workgroup (matching `producerMain`'s
     uniform `curtains[member]` load and eliminating 4-way intra-warp curtain branch divergence).

2. `coarse-monotone-bowskip` (32,768 capacity) & `coarse-monotone-bowskip-wide` (65,536 capacity):
   - Strictly bit-exact single-producer continuation using ONLY the exact existing level-specific predicates
     for `c in {-m.limits.w, 0.0, +m.limits.w}` and keeping all original `20/32/96/48` private array capacities
     (zero new mathematical approximations, zero amplitude/linear-z culls, zero cross-level scale bounds):
     a) Evaluates the exact existing level-specific depth-0 bow check for each of the 3 levels across the 8
        coarse bins (`activeMask0`, `activeMask1`, `activeMask2`).
     b) When `(activeMask0 | activeMask1 | activeMask2) == 0u` and `abs(phi_knots[0]) > m.limits.w + m.curv.y`
        (and all 8 bins have `h0 > 0.0`), charges the exact 24 depth-0 work units (`if (work + 24u > cfg.budget)
        { reason |= 1u; break; } work += 24u; continue;`), skipping all 3 `roots()` calls, affine knot divisions,
        insertion sort, and `evalPhi` piece-midpoint evaluations on the 53.30% of box-hit members that have no
        roots and lie outside the slab.
     c) Inside `roots()`, skips depth-0 bow-rejected bins via the exact per-level `activeMask` and resolves
        depth-0 `minSlope > 0.0` monotone root isolation directly in scalar registers before touching `stack[0]`.
"""
import argparse, ast, copy, hashlib, importlib.util, json, math, os, re, shlex, statistics, subprocess
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def make_parallel_soa_wgsl() -> str:
    return r'''
struct CurtainSpec {
    origin: vec4<f32>,
    half: vec4<f32>,
    limits: vec4<f32>,
    curv: vec4<f32>,
    amps: vec4<f32>,
    freqs: vec4<f32>,
    phases: vec4<f32>,
};

struct Ray {
    ro_clipA: vec4<f32>,
    rd_clipB: vec4<f32>,
    counts: vec4<u32>,
};

struct Subpanel {
    span: vec2<f32>,
    info: vec2<u32>,
};

struct Overlap {
    span: vec2<f32>,
    info: vec2<u32>,
};

struct Config {
    rayCount: u32,
    maxSubPerRay: u32,
    maxOverPerRay: u32,
    budget: u32,
};

const FALLBACK: u32 = 0x80000000u;
const MAX_PIECES_PER_MEMBER: u32 = 48u;

@group(0) @binding(0) var<uniform> cfg: Config;
@group(0) @binding(1) var<storage, read> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage, read_write> rays: array<Ray>;
@group(0) @binding(3) var<storage, read_write> subpanels: array<Subpanel>;
@group(0) @binding(4) var<storage, read_write> overlaps: array<Overlap>;
@group(0) @binding(5) var<storage, read_write> memberPieces: array<vec2<f32>>;
@group(0) @binding(6) var<storage, read_write> memberMeta: array<vec4<u32>>;

fn evalWave(m: CurtainSpec, x: f32) -> f32 {
    return m.amps.x * sin(m.freqs.x * x + m.phases.x)
         + m.amps.y * cos(m.freqs.y * x + m.phases.y)
         + m.amps.z * sin(m.freqs.z * x + m.phases.z);
}

fn evalPhi(m: CurtainSpec, o: vec3f, d: vec3f, s: f32) -> f32 {
    return o.z + s * d.z + evalWave(m, o.x + s * d.x);
}

fn putBand(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>,
           bands: ptr<function, array<vec2f, 32>>, nr: ptr<function, u32>, reason: ptr<function, u32>) {
    if (*nr > 0u) {
        let last = (*bands)[*nr - 1u];
        if (lo <= last.y && hi >= last.x) {
            let merged = vec2f(min(lo, last.x), max(hi, last.y));
            (*bands)[*nr - 1u] = merged;
            (*bounds)[*nb - 2u] = merged.x;
            (*bounds)[*nb - 1u] = merged.y;
            return;
        }
    }
    if (*nb + 2u > 92u || *nr >= 32u) { *reason |= 2u; return; }
    (*bounds)[*nb] = lo; (*bounds)[*nb + 1u] = hi; *nb += 2u;
    (*bands)[*nr] = vec2f(lo, hi); *nr += 1u;
}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
             previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
    if (protectedBoundary || previous.y != current.x) { return false; }
    let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
    return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>,
                 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
                 nr: ptr<function, u32>, reason: ptr<function, u32>,
                 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
    putBand(lo, hi, bounds, nb, bands, nr, reason);
    if (*reason == 0u && isZeroLevel && *nr > 0u) { *zeroBands |= 1u << (*nr - 1u); }
}

fn getDepth(bits: vec3u, idx: u32) -> u32 {
    return (bits[idx >> 3u] >> ((idx & 7u) << 2u)) & 15u;
}

fn setDepth(bits: ptr<function, vec3u>, idx: u32, d: u32) {
    let w = idx >> 3u; let s = (idx & 7u) << 2u;
    (*bits)[w] = ((*bits)[w] & ~(15u << s)) | ((d & 15u) << s);
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
         bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
         nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
         phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool) {
    let M = m.curv.x * d.x * d.x * 1.00002;
    let err = m.curv.y;
    var stack: array<vec4f, 20>; var depthBits = vec3u(0u); var top = 0u;
    for (var bin = 0u; bin < 8u; bin++) {
        var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
        if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
        let phi_a0 = (*phi_knots)[bin]; let phi_b0 = (*phi_knots)[bin + 1u];
        stack[0] = vec4f(a, b, phi_a0, phi_b0); setDepth(&depthBits, 0u, 0u); top = 1u;
        loop {
            if (top == 0u || *reason != 0u) { break; }
            top--; let cell = stack[top]; a = cell.x; b = cell.y; let h = b - a;
            let depth = getDepth(depthBits, top);
            if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
            if (!(h > 0.0)) { *reason |= 4u; break; }
            let va = cell.z - c; let vb = cell.w - c;
            let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
            let arithmetic = 0.000001 * scale;
            let error = err + arithmetic;
            let bow = 0.125 * M * h * h + error;
            if (min(va, vb) > bow || max(va, vb) < -bow) { continue; }
            let slope = (vb - va) / h; let radius = 0.5 * M * h + 2.0 * error / h;
            let minSlope = abs(slope) - radius;
            if (minSlope > 0.0) {
                if (abs(va) <= error) { let w = 2.0 * error / minSlope; putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
                if (abs(vb) <= error) { let w = 2.0 * error / minSlope; putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
                if ((va > error && vb < -error) || (va < -error && vb > error)) {
                    var l = a; var r = b; var left = va;
                    for (var iter = 0u; iter < 32u; iter++) {
                        if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
                        let mid = l + 0.5 * (r - l);
                        if (mid <= l || mid >= r || r - l <= 0.0001) { break; }
                        let v = evalPhi(m, o, d, mid) - c;
                        if (abs(v) <= error) { let w = 2.0 * error / minSlope; l = max(l, mid - w); r = min(r, mid + w); break; }
                        if ((left < 0.0 && v > 0.0) || (left > 0.0 && v < 0.0)) { r = mid; } else { l = mid; left = v; }
                    }
                    putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                }
                continue;
            }
            if (0.125 * M * h * h <= error) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
            if (f32(depth) >= 14.0 || top + 2u > 20u) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
            let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
            let phi_mid = evalPhi(m, o, d, mid);
            stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
            stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
            top += 2u;
        }
        if (*reason != 0u) { break; }
    }
}

// Pass 1 (Member-Major SoA 2D Dispatch): gid.x = rayId (0..rayCount-1), gid.y = member (0..3).
// Every 64-thread workgroup shares a uniform `member = gid.y`, eliminating intra-warp curtain divergence.
@compute @workgroup_size(64, 1, 1)
fn memberRootsMain(@builtin(global_invocation_id) gid: vec3u) {
    let rayId = gid.x;
    let member = gid.y;
    if (rayId >= cfg.rayCount || member >= 4u) { return; }
    let pairId = member * cfg.rayCount + rayId;
    if (cfg.budget == 0u) {
        memberMeta[pairId] = vec4u(0u, 0u, 1u, 0u);
        return;
    }
    let input = rays[rayId];
    let ro = input.ro_clipA.xyz;
    let d = input.rd_clipB.xyz;
    let m = curtains[member];
    let o = ro - m.origin.xyz;
    var a = input.ro_clipA.w;
    var b = input.rd_clipB.w;
    var hit = true;
    for (var axis = 0u; axis < 3u; axis++) {
        var half = m.half[axis];
        if (axis == 0u) { half = min(half, m.limits.x); }
        if (axis == 1u) { half = min(half, m.limits.y); }
        if (d[axis] == 0.0) {
            if (abs(o[axis]) > half + m.curv.y) { hit = false; break; }
        } else {
            let q0 = (-half - m.curv.y - o[axis]) / d[axis];
            let q1 = (half + m.curv.y - o[axis]) / d[axis];
            a = max(a, min(q0, q1));
            b = min(b, max(q0, q1));
        }
    }
    if (!hit || !(b > a)) {
        memberMeta[pairId] = vec4u(0u, 0u, 0u, 0u);
        return;
    }
    var reason = 0u;
    var work = 0u;
    var knots: array<f32, 96>;
    var nb = 1u;
    knots[0] = a;
    var bands: array<vec2f, 32>;
    var nr = 0u;
    var zeroBands = 0u;
    var phi_knots: array<f32, 9>;
    for (var k = 0u; k < 9u; k++) {
        var s = a + (b - a) * f32(k) / 8.0;
        if (k == 0u) { s = a; }
        if (k == 8u) { s = b; }
        phi_knots[k] = evalPhi(m, o, d, s);
    }
    roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);
    if (reason == 0u) {
        roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true);
    }
    if (reason == 0u) {
        roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);
    }
    if (reason != 0u) {
        memberMeta[pairId] = vec4u(0u, work, reason, 0u);
        return;
    }
    if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
    if (d.y != 0.0) {
        let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
        let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
    }
    knots[nb] = b; nb++;
    for (var i = 1u; i < nb; i++) {
        let v = knots[i]; var j = i;
        loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
        knots[j] = v;
    }
    let pieceBase = pairId * MAX_PIECES_PER_MEMBER;
    var count = 0u;
    var lastPiece = vec2f(0.0);
    var previousShortBand = false;
    var previousPositive = false;
    for (var i = 1u; i < nb; i++) {
        let l = knots[i - 1u]; let r = knots[i];
        if (!(r > l)) { continue; }
        let mid = l + 0.5 * (r - l);
        var uncertain = false;
        var protectedBoundary = false;
        if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
        if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
        for (var k = 0u; k < nr; k++) {
            uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
            if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
        }
        if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
            if (count > 0u && mayAbsorb(lastPiece, vec2f(l, r),
                 previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                lastPiece.y = r;
                memberPieces[pieceBase + count - 1u] = lastPiece;
                previousShortBand = false;
                previousPositive = true;
            } else {
                if (count >= MAX_PIECES_PER_MEMBER) { reason |= 8u; break; }
                lastPiece = vec2f(l, r);
                memberPieces[pieceBase + count] = lastPiece;
                count++;
                previousShortBand = uncertain && r - l <= 0.02;
                previousPositive = !uncertain;
            }
        }
    }
    memberMeta[pairId] = vec4u(count, work, reason, 0u);
}

@compute @workgroup_size(64)
fn mergeMain(@builtin(global_invocation_id) gid: vec3u) {
    let id = gid.x;
    if (id >= cfg.rayCount) { return; }
    let subBase = id * cfg.maxSubPerRay;
    let overBase = id * cfg.maxOverPerRay;
    if (cfg.budget == 0u) {
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | 1u);
        return;
    }
    var reason = 0u;
    var totalWork = 0u;
    var totalPieces = 0u;
    var memCount = vec4u(0u);
    var pieces: array<vec2f, 48>;
    var memStart = vec4u(0u);
    for (var m = 0u; m < 4u; m++) {
        let pairId = m * cfg.rayCount + id;
        let memberInfo = memberMeta[pairId];
        let mCount = memberInfo.x;
        let mWork = memberInfo.y;
        let mReason = memberInfo.z;
        if (mReason != 0u) {
            reason |= mReason;
            break;
        }
        if (totalWork > cfg.budget || cfg.budget - totalWork < mWork) {
            reason |= 1u;
            break;
        }
        totalWork += mWork;
        if (totalPieces + mCount > 48u) {
            reason |= 8u;
            break;
        }
        memStart[m] = totalPieces;
        memCount[m] = mCount;
        let pieceBase = pairId * MAX_PIECES_PER_MEMBER;
        for (var k = 0u; k < mCount; k++) {
            pieces[totalPieces + k] = memberPieces[pieceBase + k];
        }
        totalPieces += mCount;
    }
    if (reason != 0u) {
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
        return;
    }
    var knots: array<f32, 96>;
    var ne = 0u;
    var head = vec4u(0u);
    let totalKnots = totalPieces * 2u;
    loop {
        if (ne >= totalKnots) { break; }
        var bestM = 4u;
        var bestVal = 0.0;
        for (var m = 0u; m < 4u; m++) {
            let h = head[m];
            if (h < memCount[m] * 2u) {
                let pIdx = memStart[m] + (h >> 1u);
                let isY = (h & 1u) != 0u;
                let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
                if (bestM == 4u || val < bestVal) {
                    bestVal = val;
                    bestM = m;
                }
            }
        }
        knots[ne] = bestVal;
        ne++;
        head[bestM]++;
    }
    var singles = 0u;
    var overs = 0u;
    var pIdx = vec4u(0u);
    for (var i = 1u; i < ne && reason == 0u; i++) {
        let a = knots[i - 1u];
        let b = knots[i];
        if (!(b > a)) { continue; }
        let mid = a + 0.5 * (b - a);
        var mask = 0u;
        for (var m = 0u; m < 4u; m++) {
            let start = memStart[m];
            let numPieces = memCount[m];
            var cur = pIdx[m];
            loop {
                if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
                cur++;
            }
            pIdx[m] = cur;
            if (cur < numPieces) {
                let p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) {
                    mask |= (1u << m);
                }
            }
        }
        if (mask == 0u) { continue; }
        if (countOneBits(mask) == 1u) {
            let n = max(1u, u32(ceil((b - a) / 4.0)));
            if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
            let member = firstTrailingBit(mask);
            for (var k = 0u; k < n; k++) {
                let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
                let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
                subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0u));
                singles++;
            }
        } else {
            if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
            overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0u));
            overs++;
        }
    }
    if (reason != 0u) {
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
    } else {
        rays[id].counts = vec4u(subBase, singles, overBase, overs);
    }
}
'''


def make_exact_monotone_bowskip_wgsl(disjoint_fastpath=False, tile_cert=False, scratch_trimmed=False, stream_merge=False) -> str:
    """Uses only the existing level-specific depth-0 bow and monotone predicates.
    The untrimmed form retains the original20/32/96/48 capacities. No new slack constants.
    In the trimmed form, bounds was an ALIAS of the caller's knots, not a second array.
    Delaying reconstruction removes that alias from the roots call graph and narrows
    source scope; it does not prove physical stack savings (compiler spills grew in6dl).
    Capacity proof: nr<=32; member knots need1+2*nr+3+1<=69 entries, so70 is sufficient.
    The old nb=1+2*nr makes its >92 guard redundant under the32-band bound. Merge knots
    need2*count<=96 under the unchanged48-piece guard. Both refusal contracts survive.
    Scoped WGSL declarations do not guarantee conditional physical allocation or reuse.
    Streaming merge (§6dm) eliminates mergeKnots[96] entirely, streaming four-way merge
    directly into adjacent-interval emission using scalar prevKnot/bestVal and head cursors.
    """
    if stream_merge:
        scratch_trimmed = True
    s = r'''
struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Overlap {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<storage,read> curtains:array<CurtainSpec,4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;
const FALLBACK:u32=0x80000000u;

fn evalWave(m:CurtainSpec,x:f32)->f32 {
 return m.amps.x*sin(m.freqs.x*x+m.phases.x)+m.amps.y*cos(m.freqs.y*x+m.phases.y)+m.amps.z*sin(m.freqs.z*x+m.phases.z);
}
fn evalPhi(m:CurtainSpec,o:vec3f,d:vec3f,s:f32)->f32{return o.z+s*d.z+evalWave(m,o.x+s*d.x);}

fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,32>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>92u || *nr>=32u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
 previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
 if (protectedBoundary || previous.y != current.x) { return false; }
 let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
 return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>,
 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
 putBand(lo, hi, bounds, nb, bands, nr, reason);
 if (*reason == 0u && isZeroLevel && *nr > 0u) { *zeroBands |= 1u << (*nr - 1u); }
}

fn getDepth(bits: vec3u, idx: u32) -> u32 {
 return (bits[idx >> 3u] >> ((idx & 7u) << 2u)) & 15u;
}
fn setDepth(bits: ptr<function, vec3u>, idx: u32, d: u32) {
 let w = idx >> 3u; let s = (idx & 7u) << 2u;
 (*bits)[w] = ((*bits)[w] & ~(15u << s)) | ((d & 15u) << s);
}

fn isolateMonotone(m: CurtainSpec, o: vec3f, d: vec3f, a: f32, b: f32, c: f32,
 va: f32, vb: f32, error: f32, minSlope: f32,
 bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>,
 bands: ptr<function, array<vec2f, 32>>, nr: ptr<function, u32>,
 reason: ptr<function, u32>, work: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
 if (abs(va) <= error) { let w = 2.0 * error / minSlope; putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if (abs(vb) <= error) { let w = 2.0 * error / minSlope; putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if ((va > error && vb < -error) || (va < -error && vb > error)) {
  var l = a; var r = b; var left = va;
  for (var iter = 0u; iter < 32u; iter++) {
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   let mid = l + 0.5 * (r - l);
   if (mid <= l || mid >= r || r - l <= 0.0001) { break; }
   let v = evalPhi(m, o, d, mid) - c;
   if (abs(v) <= error) { let w = 2.0 * error / minSlope; l = max(l, mid - w); r = min(r, mid + w); break; }
   if ((left < 0.0 && v > 0.0) || (left > 0.0 && v < 0.0)) { r = mid; } else { l = mid; left = v; }
  }
  putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
 }
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
 phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool,
 levelActiveMask: u32) {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var stack: array<vec4f, 20>; var depthBits = vec3u(0u); var top = 0u;
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  if (*work >= cfg.budget) { *reason |= 1u; break; }
  let coarseH = b - a;
  if (!(coarseH > 0.0)) { *work += 1u; *reason |= 4u; break; }
  if ((levelActiveMask & (1u << bin)) == 0u) { *work += 1u; continue; }
  *work += 1u;
  let phi_a0 = (*phi_knots)[bin]; let phi_b0 = (*phi_knots)[bin + 1u];
  let coarseVa = phi_a0 - c; let coarseVb = phi_b0 - c;
  let coarseScale = 1.0 + abs(coarseVa) + abs(coarseVb) + abs(a) + abs(b);
  let coarseError = err + 0.000001 * coarseScale;
  let coarseSlope = (coarseVb - coarseVa) / coarseH;
  let coarseRadius = 0.5 * M * coarseH + 2.0 * coarseError / coarseH;
  let coarseMinSlope = abs(coarseSlope) - coarseRadius;
  if (coarseMinSlope > 0.0) {
   isolateMonotone(m, o, d, a, b, c, coarseVa, coarseVb, coarseError, coarseMinSlope,
                   bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
   if (*reason != 0u) { break; }
   continue;
  }
  if (0.125 * M * coarseH * coarseH <= coarseError) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { break; }
   continue;
  }
  let mid0 = a + 0.5 * coarseH;
  if (mid0 <= a || mid0 >= b) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { break; }
   continue;
  }
  let phi_mid0 = evalPhi(m, o, d, mid0);
  stack[0] = vec4f(mid0, b, phi_mid0, phi_b0); setDepth(&depthBits, 0u, 1u);
  stack[1] = vec4f(a, mid0, phi_a0, phi_mid0); setDepth(&depthBits, 1u, 1u);
  top = 2u;
  loop {
   if (top == 0u || *reason != 0u) { break; }
   top--; let cell = stack[top]; a = cell.x; b = cell.y; let h = b - a;
   let depth = getDepth(depthBits, top);
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   if (!(h > 0.0)) { *reason |= 4u; break; }
   let va = cell.z - c; let vb = cell.w - c;
   let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
   let arithmetic = 0.000001 * scale;
   let error = err + arithmetic;
   let bow = 0.125 * M * h * h + error;
   if (min(va, vb) > bow || max(va, vb) < -bow) { continue; }
   let slope = (vb - va) / h; let radius = 0.5 * M * h + 2.0 * error / h;
   let minSlope = abs(slope) - radius;
   if (minSlope > 0.0) {
    isolateMonotone(m, o, d, a, b, c, va, vb, error, minSlope,
                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
    continue;
   }
   if (0.125 * M * h * h <= error) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   if (f32(depth) >= 14.0 || top + 2u > 20u) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let phi_mid = evalPhi(m, o, d, mid);
   stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
   stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
   top += 2u;
  }
  if (*reason != 0u) { break; }
 }
}

fn getMember(pm: vec3u, k: u32) -> u32 {
 return (pm[k >> 4u] >> ((k & 15u) << 1u)) & 3u;
}
fn setMember(pm: ptr<function, vec3u>, k: u32, m: u32) {
 let w = k >> 4u; let s = (k & 15u) << 1u;
 (*pm)[w] = ((*pm)[w] & ~(3u << s)) | ((m & 3u) << s);
}

fn exactLevelActiveMask(m: CurtainSpec, d: vec3f, lo: f32, hi: f32, c: f32,
                        phi_knots: ptr<function, array<f32, 9>>, allPositiveH: ptr<function, bool>) -> u32 {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var mask = 0u;
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  let h = b - a;
  if (!(h > 0.0)) { *allPositiveH = false; }
  let va = (*phi_knots)[bin] - c; let vb = (*phi_knots)[bin + 1u] - c;
  let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
  let error = err + 0.000001 * scale;
  let bow = 0.125 * M * h * h + error;
  if (!(min(va, vb) > bow || max(va, vb) < -bow)) {
   mask |= (1u << bin);
  }
 }
 return mask;
}

@compute @workgroup_size(64)
fn producerFastMain(@builtin(global_invocation_id) gid: vec3u) {
 let id = gid.x; if (id >= cfg.rayCount) { return; }
 let input = rays[id]; let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
 let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;
 var reason = 0u; var work = 0u; var count = 0u;
 var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
 var knots: array<f32, 96>;
 var memStart = vec4u(0u); var memCount = vec4u(0u);
 if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }
 for (var member = 0u; member < 4u; member++) {
  memStart[member] = count;
  let m = curtains[member]; let o = ro - m.origin.xyz;
  var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
  for (var axis = 0u; axis < 3u; axis++) {
   var half = m.half[axis];
   if (axis == 0u) { half = min(half, m.limits.x); } if (axis == 1u) { half = min(half, m.limits.y); }
   if (d[axis] == 0.0) { if (abs(o[axis]) > half + m.curv.y) { hit = false; break; } }
   else {
    let q0 = (-half - m.curv.y - o[axis]) / d[axis]; let q1 = (half + m.curv.y - o[axis]) / d[axis];
    a = max(a, min(q0, q1)); b = min(b, max(q0, q1));
   }
  }
  if (!hit || !(b > a)) { continue; }
  var phi_knots: array<f32, 9>;
  for (var k = 0u; k < 9u; k++) {
   var s = a + (b - a) * f32(k) / 8.0;
   if (k == 0u) { s = a; } if (k == 8u) { s = b; }
   phi_knots[k] = evalPhi(m, o, d, s);
  }
  var allPositiveH = true;
  let mask0 = exactLevelActiveMask(m, d, a, b, -m.limits.w, &phi_knots, &allPositiveH);
  let mask1 = exactLevelActiveMask(m, d, a, b, 0.0, &phi_knots, &allPositiveH);
  let mask2 = exactLevelActiveMask(m, d, a, b, m.limits.w, &phi_knots, &allPositiveH);
  if (allPositiveH && (mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
   if (work + 24u > cfg.budget) { reason |= 1u; break; }
   work += 24u;
   memCount[member] = 0u;
   continue;
  }
  var nb = 1u; knots[0] = a;
  var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
  roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
  if (reason != 0u) { break; }
  if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
  if (d.y != 0.0) {
   let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
   let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
  }
  knots[nb] = b; nb++;
  for (var i = 1u; i < nb; i++) {
   let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
   knots[j] = v;
  }
  let memberStart = count; var previousShortBand = false; var previousPositive = false;
  for (var i = 1u; i < nb; i++) {
   let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
   let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
   if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
   if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
   for (var k = 0u; k < nr; k++) {
    uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
    if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
   }
   if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
    if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
         previousShortBand, previousPositive, uncertain, protectedBoundary)) {
     pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
    } else {
     if (count >= 48u) { reason |= 8u; break; }
     pieces[count] = vec2f(l, r);
     setMember(&pieceMembers, count, member);
     count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    }
   }
  }
  memCount[member] = count - memStart[member];
  if (reason != 0u) { break; }
 }
 if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }
 var ne = 0u;
 var head = vec4u(0u);
 let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
 loop {
  if (ne >= totalKnots) { break; }
  var bestM = 4u;
  var bestVal = 0.0;
  for (var m = 0u; m < 4u; m++) {
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let pIdx = memStart[m] + (h >> 1u);
    let isY = (h & 1u) != 0u;
    let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  knots[ne] = bestVal;
  ne++;
  head[bestM]++;
 }
 var singles = 0u; var overs = 0u;
 var pIdx = vec4u(0u);
 for (var i = 1u; i < ne && reason == 0u; i++) {
  let a = knots[i - 1u]; let b = knots[i]; if (!(b > a)) { continue; }
  let mid = a + 0.5 * (b - a);
  var mask = 0u;
  for (var m = 0u; m < 4u; m++) {
   let start = memStart[m];
   let numPieces = memCount[m];
   var cur = pIdx[m];
   loop {
    if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
    cur++;
   }
   pIdx[m] = cur;
   if (cur < numPieces) {
    let p = pieces[start + cur];
    if (mid >= p.x && mid <= p.y) {
     mask |= (1u << m);
    }
   }
  }
  if (mask == 0u) { continue; }
  if (countOneBits(mask) == 1u) {
   let n = max(1u, u32(ceil((b - a) / 4.0)));
   if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
   let member = firstTrailingBit(mask);
   for (var k = 0u; k < n; k++) {
    let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
    let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
    subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0)); singles++;
   }
  } else {
   if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
   overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0)); overs++;
  }
 }
 if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); }
 else { rays[id].counts = vec4u(subBase, singles, overBase, overs); }
}
'''
    if scratch_trimmed:
        # 1. putBand signature and body
        old_pb = '''fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,32>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>92u || *nr>=32u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}'''
        new_pb = '''fn putBand(lo:f32,hi:f32,
 bands:ptr<function,array<vec2f,32>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   return;
  }
 }
 if(*nr>=32u){*reason|=2u;return;}
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}'''
        if s.count(old_pb) != 1: raise ValueError('scratch_trimmed old_pb hook')
        s = s.replace(old_pb, new_pb, 1)

        old_pbt = '''fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>,
 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {'''
        new_pbt = '''fn putBandTagged(lo: f32, hi: f32,
 bands: ptr<function, array<vec2f, 32>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {'''
        if s.count(old_pbt) != 1: raise ValueError('scratch_trimmed old_pbt hook')
        s = s.replace(old_pbt, new_pbt, 1)

        old_iso = '''fn isolateMonotone(m: CurtainSpec, o: vec3f, d: vec3f, a: f32, b: f32, c: f32,
 va: f32, vb: f32, error: f32, minSlope: f32,
 bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>,
 bands: ptr<function, array<vec2f, 32>>, nr: ptr<function, u32>,'''
        new_iso = '''fn isolateMonotone(m: CurtainSpec, o: vec3f, d: vec3f, a: f32, b: f32, c: f32,
 va: f32, vb: f32, error: f32, minSlope: f32,
 bands: ptr<function, array<vec2f, 32>>, nr: ptr<function, u32>,'''
        if s.count(old_iso) != 1: raise ValueError('scratch_trimmed old_iso hook')
        s = s.replace(old_iso, new_iso, 1)

        old_rts = '''fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,'''
        new_rts = '''fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bands: ptr<function, array<vec2f, 32>>,'''
        if s.count(old_rts) != 1: raise ValueError('scratch_trimmed old_rts hook')
        s = s.replace(old_rts, new_rts, 1)

        s = re.sub(r'bounds,\s*nb,\s*bands', 'bands', s)

        old_top = ''' var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
 var knots: array<f32, 96>;
 var memStart = vec4u(0u); var memCount = vec4u(0u);'''
        new_top = ''' var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
 var memStart = vec4u(0u); var memCount = vec4u(0u);'''
        if s.count(old_top) != 1: raise ValueError('scratch_trimmed old_top hook')
        s = s.replace(old_top, new_top, 1)

        old_member_roots = '''  var nb = 1u; knots[0] = a;
  var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
  roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
  if (reason != 0u) { break; }
  if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
  if (d.y != 0.0) {
   let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
   let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
  }
  knots[nb] = b; nb++;'''
        new_member_roots = '''  var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
  roots(m, o, d, a, b, -m.limits.w, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, 0.0, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, m.limits.w, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
  if (reason != 0u) { break; }
  var knots: array<f32, 70>;
  var nb = 1u; knots[0] = a;
  for (var k = 0u; k < nr; k++) {
   knots[nb] = bands[k].x; nb++;
   knots[nb] = bands[k].y; nb++;
  }
  if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
  if (d.y != 0.0) {
   let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
   let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
  }
  knots[nb] = b; nb++;'''
        if s.count(old_member_roots) != 1: raise ValueError('scratch_trimmed old_member_roots hook')
        s = s.replace(old_member_roots, new_member_roots, 1)

        if stream_merge:
            old_two_loops = ''' var ne = 0u;
 var head = vec4u(0u);
 let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
 loop {
  if (ne >= totalKnots) { break; }
  var bestM = 4u;
  var bestVal = 0.0;
  for (var m = 0u; m < 4u; m++) {
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let pIdx = memStart[m] + (h >> 1u);
    let isY = (h & 1u) != 0u;
    let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  knots[ne] = bestVal;
  ne++;
  head[bestM]++;
 }
 var singles = 0u; var overs = 0u;
 var pIdx = vec4u(0u);
 for (var i = 1u; i < ne && reason == 0u; i++) {
  let a = knots[i - 1u]; let b = knots[i]; if (!(b > a)) { continue; }
  let mid = a + 0.5 * (b - a);
  var mask = 0u;
  for (var m = 0u; m < 4u; m++) {
   let start = memStart[m];
   let numPieces = memCount[m];
   var cur = pIdx[m];
   loop {
    if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
    cur++;
   }
   pIdx[m] = cur;
   if (cur < numPieces) {
    let p = pieces[start + cur];
    if (mid >= p.x && mid <= p.y) {
     mask |= (1u << m);
    }
   }
  }
  if (mask == 0u) { continue; }
  if (countOneBits(mask) == 1u) {
   let n = max(1u, u32(ceil((b - a) / 4.0)));
   if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
   let member = firstTrailingBit(mask);
   for (var k = 0u; k < n; k++) {
    let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
    let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
    subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0)); singles++;
   }
  } else {
   if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
   overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0)); overs++;
  }
 }'''
            streaming_merge_body = ''' var singles = 0u; var overs = 0u;
 var head = vec4u(0u);
 var pIdx = vec4u(0u);
 let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
 var stepCount = 0u;
 var prevKnot = 0.0;
 loop {
  if (stepCount >= totalKnots || reason != 0u) { break; }
  var bestM = 4u;
  var bestVal = 0.0;
  for (var m = 0u; m < 4u; m++) {
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let curP = memStart[m] + (h >> 1u);
    let isY = (h & 1u) != 0u;
    let val = select(pieces[curP].x, pieces[curP].y, isY);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  head[bestM]++;
  if (stepCount > 0u) {
   let a = prevKnot; let b = bestVal;
   if (b > a) {
    let mid = a + 0.5 * (b - a);
    var mask = 0u;
    for (var m = 0u; m < 4u; m++) {
     let start = memStart[m];
     let numPieces = memCount[m];
     var cur = pIdx[m];
     loop {
      if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
      cur++;
     }
     pIdx[m] = cur;
     if (cur < numPieces) {
      let p = pieces[start + cur];
      if (mid >= p.x && mid <= p.y) {
       mask |= (1u << m);
      }
     }
    }
    if (mask != 0u) {
     if (countOneBits(mask) == 1u) {
      let n = max(1u, u32(ceil((b - a) / 4.0)));
      if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
      let member = firstTrailingBit(mask);
      for (var k = 0u; k < n; k++) {
       let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
       let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
       subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0)); singles++;
      }
     } else {
      if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
      overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0)); overs++;
     }
    }
   }
  }
  prevKnot = bestVal;
  stepCount++;
 }'''
            if s.count(old_two_loops) != 1: raise ValueError('stream_merge old_two_loops hook')
            s = s.replace(old_two_loops, streaming_merge_body, 1)
        else:
            old_merge = ''' var ne = 0u;
 var head = vec4u(0u);
 let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
 loop {
  if (ne >= totalKnots) { break; }
  var bestM = 4u;
  var bestVal = 0.0;
  for (var m = 0u; m < 4u; m++) {
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let pIdx = memStart[m] + (h >> 1u);
    let isY = (h & 1u) != 0u;
    let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  knots[ne] = bestVal;
  ne++;
  head[bestM]++;
 }
 var singles = 0u; var overs = 0u;
 var pIdx = vec4u(0u);
 for (var i = 1u; i < ne && reason == 0u; i++) {
  let a = knots[i - 1u]; let b = knots[i]; if (!(b > a)) { continue; }'''
            new_merge = ''' var mergeKnots: array<f32, 96>;
 var ne = 0u;
 var head = vec4u(0u);
 let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
 loop {
  if (ne >= totalKnots) { break; }
  var bestM = 4u;
  var bestVal = 0.0;
  for (var m = 0u; m < 4u; m++) {
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let pIdx = memStart[m] + (h >> 1u);
    let isY = (h & 1u) != 0u;
    let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  mergeKnots[ne] = bestVal;
  ne++;
  head[bestM]++;
 }
 var singles = 0u; var overs = 0u;
 var pIdx = vec4u(0u);
 for (var i = 1u; i < ne && reason == 0u; i++) {
  let a = mergeKnots[i - 1u]; let b = mergeKnots[i]; if (!(b > a)) { continue; }'''
            if s.count(old_merge) != 1: raise ValueError('scratch_trimmed old_merge hook')
            s = s.replace(old_merge, new_merge, 1)

    if tile_cert:
        needle_bind = '@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;'
        insertion_bind = '@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;\n@group(0) @binding(5) var<storage,read> rayMasks:array<u32>;'
        if s.count(needle_bind) != 1:
            raise ValueError('tile_cert needle_bind mismatch')
        s = s.replace(needle_bind, insertion_bind, 1)

        needle_cert = '  if (!hit || !(b > a)) { continue; }\n  var phi_knots: array<f32, 9>;'
        insertion_cert = '''  if (!hit || !(b > a)) { continue; }
  let tileMask = rayMasks[id];
  if ((tileMask & (1u << member)) == 0u) {
   if (work + 24u > cfg.budget) { reason |= 1u; break; }
   work += 24u;
   memCount[member] = 0u;
   continue;
  }
  var phi_knots: array<f32, 9>;'''
        if s.count(needle_cert) != 1:
            raise ValueError('tile_cert needle hook mismatch')
        s = s.replace(needle_cert, insertion_cert, 1)
    if disjoint_fastpath:
        if stream_merge:
            needle = 'if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }\n var singles = 0u; var overs = 0u;\n var head = vec4u(0u);'
            tail = 'var singles = 0u; var overs = 0u;\n var head = vec4u(0u);'
        elif scratch_trimmed:
            needle = 'if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }\n var mergeKnots: array<f32, 96>;\n var ne = 0u;'
            tail = 'var mergeKnots: array<f32, 96>;\n var ne = 0u;'
        else:
            needle = 'if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }\n var ne = 0u;'
            tail = 'var ne = 0u;'
        insertion = '''if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }
 var strictlyAscendingDisjoint = true;
 for (var p = 1u; p < count; p++) {
  if (!(pieces[p].x > pieces[p - 1u].y)) {
   strictlyAscendingDisjoint = false;
   break;
  }
 }
 if (strictlyAscendingDisjoint) {
  var singles = 0u;
  for (var m = 0u; m < 4u && reason == 0u; m++) {
   let start = memStart[m];
   let numP = memCount[m];
   for (var k = 0u; k < numP; k++) {
    let piece = pieces[start + k];
    let a = piece.x; let b = piece.y;
    if (!(b > a)) { continue; }
    let n = max(1u, u32(ceil((b - a) / 4.0)));
    if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
    for (var s = 0u; s < n; s++) {
     let l = select(a + f32(s) * (b - a) / f32(n), a, s == 0u);
     let r = select(a + f32(s + 1u) * (b - a) / f32(n), b, s + 1u == n);
     subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(m, 0));
     singles++;
    }
   }
  }
  if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); }
  else { rays[id].counts = vec4u(subBase, singles, overBase, 0u); }
  return;
 }
 ''' + tail
        if s.count(needle) != 1:
            raise ValueError('strictlyAscendingDisjoint needle hook mismatch')
        s = s.replace(needle, insertion, 1)
    return s


def parallel_soa_source(base, root):
    cr = load(root, 'ray_transport_gpu_continuation_review')
    s = cr.parallel_source(base, root)
    # Replace kParallelRootsShader with the 2D Member-Major SoA shader
    old_shader = re.search(r'static const char\* kParallelRootsShader = R"WGSL\((.*?)\)WGSL";', s, re.S)[0]
    new_shader = 'static const char* kParallelRootsShader = R"WGSL(\n' + make_parallel_soa_wgsl().strip() + '\n)WGSL";'
    if s.count(old_shader) != 1:
        raise ValueError('kParallelRootsShader hook')
    s = s.replace(old_shader, new_shader, 1)

    # Update dispatch to 2D ((n+63)/64, 4, 1) for cpMemberRoots in both tile execute and full packet census
    old_tile_disp = 'dispatch(cpMemberRoots,forced?tile.forcedMemberRoots:tile.memberRoots,tile.n*4u);'
    new_tile_disp = r'''{
                        auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
                        wgpuComputePassEncoderSetPipeline(pass,cpMemberRoots);
                        wgpuComputePassEncoderSetBindGroup(pass,0,forced?tile.forcedMemberRoots:tile.memberRoots,0,nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(pass,(tile.n+63u)/64u,4u,1u);
                        wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
                    }'''
    if s.count(old_tile_disp) != 1:
        raise ValueError('tile memberRoots dispatch hook')
    s = s.replace(old_tile_disp, new_tile_disp, 1)

    old_full_disp = 'wgpuComputePassEncoderDispatchWorkgroups(pass1, (count*4u+63u)/64u, 1, 1);'
    new_full_disp = 'wgpuComputePassEncoderDispatchWorkgroups(pass1, (count+63u)/64u, 4, 1);'
    if s.count(old_full_disp) != 1:
        raise ValueError('full memberRoots dispatch hook')
    s = s.replace(old_full_disp, new_full_disp, 1)

    # Replace the correctness & packet-census section so that:
    # 1) Full 32,784-ray packet census runs BEFORE correctness(3, false) and exports all packets + exact ULP diff summary.
    # 2) correctness(3, false) exports all 32,784 rows and records strict ULP diffs + counterexample witness while
    #    requiring forced fallback and Mode 2 to be 100% bit-exact and Mode 3 normal outputs to satisfy <= 1e-5f AST/holdout gate.
    old_corr_to_timing = re.search(
        r'        auto correctness=\[&\].*?        for\(int k=0;k<2;\+\+k\)\{execute\(0,1,false\);execute\(1,1,false\);execute\(2,1,false\);execute\(3,1,false\);\}',
        s,
        re.S,
    )[0]
    new_corr_to_timing = r'''        // 1. Run full 32,784-ray parallel-root packet census FIRST (before any output gate)
        {
            const uint64_t fullMPBytes = uint64_t(count)*4u*48u*sizeof(glm::vec2);
            const uint64_t fullMMBytes = uint64_t(count)*4u*sizeof(glm::uvec4);
            auto fullMP = makeBuffer(WGPUBufferUsage_Storage, fullMPBytes, nullptr);
            auto fullMM = makeBuffer(WGPUBufferUsage_Storage, fullMMBytes, nullptr);
            auto parSub = makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc, subpanelBytes, nullptr);
            auto parOver = makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc, overlapBytes, nullptr);
            auto parRay = makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst, rayBytes, gpuRays.data());
            auto bgMR = group(cpMemberRoots, {
                entry(0,cfgBuf,0,sizeof(cfg)), entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,parRay,0,rayBytes),
                entry(5,fullMP,0,fullMPBytes), entry(6,fullMM,0,fullMMBytes)});
            auto bgMg = group(cpMerge, {
                entry(0,cfgBuf,0,sizeof(cfg)),
                entry(2,parRay,0,rayBytes), entry(3,parSub,0,subpanelBytes), entry(4,parOver,0,overlapBytes),
                entry(5,fullMP,0,fullMPBytes), entry(6,fullMM,0,fullMMBytes)});
            auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass1, cpMemberRoots);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgMR, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (count+63u)/64u, 4, 1);
            wgpuComputePassEncoderEnd(pass1); wgpuComputePassEncoderRelease(pass1);
            auto pass2 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass2, cpMerge);
            wgpuComputePassEncoderSetBindGroup(pass2, 0, bgMg, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass2, (count+63u)/64u, 1, 1);
            wgpuComputePassEncoderEnd(pass2); wgpuComputePassEncoderRelease(pass2);
            auto cmd = wgpuCommandEncoderFinish(enc, nullptr); wgpuCommandEncoderRelease(enc);
            wgpuQueueSubmit(gpu.queue, 1, &cmd); wgpuCommandBufferRelease(cmd);

            auto pRayB = readBackBuffer(parRay, rayBytes);
            auto pSubB = readBackBuffer(parSub, subpanelBytes);
            auto pOverB = readBackBuffer(parOver, overlapBytes);
            const auto* pRays = reinterpret_cast<const GpuRayHeader*>(pRayB.data());
            const auto* pSubs = reinterpret_cast<const GpuSubpanel*>(pSubB.data());
            const auto* pOvers = reinterpret_cast<const GpuOverlap*>(pOverB.data());
            J parallelPackets = J::array();
            size_t pktCountDiffRays = 0, pktBitDiffRays = 0;
            uint32_t maxSubUlp = 0, maxOverUlp = 0;
            float maxSubAbs = 0.0f, maxOverAbs = 0.0f;
            J firstPktWitness = J::object();
            for(size_t i=0;i<count;++i){
                J singles=J::array(),overs=J::array();
                for(uint32_t k=0;k<pRays[i].subpanelCount;++k){const auto& a=pSubs[pRays[i].subpanelOffset+k];singles.push_back({a.s0,a.s1,a.mediumIndex});}
                for(uint32_t k=0;k<pRays[i].overlapCount;++k){const auto& a=pOvers[pRays[i].overlapOffset+k];overs.push_back({a.s0,a.s1,a.activeMask});}
                parallelPackets.push_back({R[i][0],R[i][1],pRays[i].overlapCount,singles,overs});
                if(pRays[i].subpanelCount!=gpuRays[i].subpanelCount || pRays[i].overlapCount!=gpuRays[i].overlapCount){
                    pktCountDiffRays++;
                    if(firstPktWitness.empty()) firstPktWitness = {{"ray_index",i},{"kind","count_mismatch"},{"actual_counts",{pRays[i].subpanelCount,pRays[i].overlapCount}},{"expected_counts",{gpuRays[i].subpanelCount,gpuRays[i].overlapCount}}};
                    continue;
                }
                bool rayDiff = false;
                for(uint32_t k=0;k<pRays[i].subpanelCount;++k){
                    const auto& a = pSubs[pRays[i].subpanelOffset+k];
                    const auto& b = gpuSubpanels[gpuRays[i].subpanelOffset+k];
                    if(a.mediumIndex!=b.mediumIndex) throw std::runtime_error("parallel-root mediumIndex mismatch at ray " + std::to_string(i));
                    uint32_t u0 = std::abs(int64_t(std::bit_cast<uint32_t>(a.s0)) - int64_t(std::bit_cast<uint32_t>(b.s0)));
                    uint32_t u1 = std::abs(int64_t(std::bit_cast<uint32_t>(a.s1)) - int64_t(std::bit_cast<uint32_t>(b.s1)));
                    if(u0 || u1){
                        rayDiff = true;
                        maxSubUlp = std::max(maxSubUlp, std::max(u0, u1));
                        maxSubAbs = std::max(maxSubAbs, std::max(std::abs(a.s0 - b.s0), std::abs(a.s1 - b.s1)));
                        if(firstPktWitness.empty()) firstPktWitness = {{"ray_index",i},{"subpanel_index",k},{"actual_span",{a.s0,a.s1}},{"expected_span",{b.s0,b.s1}},{"ulp_diff",{u0,u1}}};
                    }
                }
                for(uint32_t k=0;k<pRays[i].overlapCount;++k){
                    const auto& a = pOvers[pRays[i].overlapOffset+k];
                    const auto& b = gpuOverlaps[gpuRays[i].overlapOffset+k];
                    if(a.activeMask!=b.activeMask) throw std::runtime_error("parallel-root activeMask mismatch at ray " + std::to_string(i));
                    uint32_t u0 = std::abs(int64_t(std::bit_cast<uint32_t>(a.s0)) - int64_t(std::bit_cast<uint32_t>(b.s0)));
                    uint32_t u1 = std::abs(int64_t(std::bit_cast<uint32_t>(a.s1)) - int64_t(std::bit_cast<uint32_t>(b.s1)));
                    if(u0 || u1){
                        rayDiff = true;
                        maxOverUlp = std::max(maxOverUlp, std::max(u0, u1));
                        maxOverAbs = std::max(maxOverAbs, std::max(std::abs(a.s0 - b.s0), std::abs(a.s1 - b.s1)));
                    }
                }
                if(rayDiff) pktBitDiffRays++;
            }
            report["parallel_packet_census_inputs"] = parallelPackets;
            report["parallel_packet_diff_summary"] = {
                {"packet_count_differing_rays", pktCountDiffRays},
                {"packet_bit_differing_rays", pktBitDiffRays},
                {"max_subpanel_ulp_diff", maxSubUlp},
                {"max_subpanel_abs_diff", maxSubAbs},
                {"max_overlap_ulp_diff", maxOverUlp},
                {"max_overlap_abs_diff", maxOverAbs},
                {"first_packet_witness", firstPktWitness}
            };
            wgpuBindGroupRelease(bgMR); wgpuBindGroupRelease(bgMg);
            wgpuBufferRelease(fullMP); wgpuBufferRelease(fullMM);
            wgpuBufferRelease(parSub); wgpuBufferRelease(parOver); wgpuBufferRelease(parRay);
        }

        // 2. Verify Mode 2 (strict bit-exact) and Mode 3 (export all rows + record both strict ULP & numerical gates)
        size_t mode3StrictBitDiffRays = 0;
        uint32_t mode3MaxRgbUlp = 0, mode3MaxTUlp = 0;
        float mode3MaxRgbAbs = 0.0f, mode3MaxTAbs = 0.0f;
        J firstOutWitness = J::object();
        auto correctness=[&](int testMode, bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(testMode,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i){
                bool rayBitDiff = false;
                for(int k=0;k<4;++k){
                    if(!std::isfinite(result[i].rgb_T[k])) throw std::runtime_error("nonfinite output at ray " + std::to_string(i));
                    uint32_t uActual = std::bit_cast<uint32_t>(result[i].rgb_T[k]);
                    uint32_t uExpect = std::bit_cast<uint32_t>(expected[i].rgb_T[k]);
                    if(uActual != uExpect){
                        if(testMode != 3 || forced){
                            throw std::runtime_error("stream mode " + std::to_string(testMode) + " forced=" + std::to_string(forced) + " bit mismatch at ray " + std::to_string(i));
                        }
                        rayBitDiff = true;
                        uint32_t ulp = std::abs(int64_t(uActual) - int64_t(uExpect));
                        float absDiff = std::abs(result[i].rgb_T[k] - expected[i].rgb_T[k]);
                        if(k < 3){ mode3MaxRgbUlp = std::max(mode3MaxRgbUlp, ulp); mode3MaxRgbAbs = std::max(mode3MaxRgbAbs, absDiff); }
                        else { mode3MaxTUlp = std::max(mode3MaxTUlp, ulp); mode3MaxTAbs = std::max(mode3MaxTAbs, absDiff); }
                        if(absDiff > 1e-5f) throw std::runtime_error("mode 3 numerical gate (>1e-5) exceeded at ray " + std::to_string(i));
                    }
                }
                if(rayBitDiff){
                    mode3StrictBitDiffRays++;
                    if(firstOutWitness.empty()){
                        auto hb=readBackBuffer(rBuf,rayBytes);const auto* hs=reinterpret_cast<const GpuRayHeader*>(hb.data());
                        J actual=J::array(),expectedWords=J::array(),actualWords=J::array();
                        for(int k=0;k<4;++k){actual.push_back(result[i].rgb_T[k]);expectedWords.push_back(std::bit_cast<uint32_t>(expected[i].rgb_T[k]));actualWords.push_back(std::bit_cast<uint32_t>(result[i].rgb_T[k]));}
                        firstOutWitness={{"ray_index",i},{"ray",R[i]},{"mode",testMode},{"forced",forced},{"actual",actual},{"expected_words",expectedWords},{"actual_words",actualWords},{"actual_header",{hs[i].subpanelCount,hs[i].overlapCount}},{"baseline_header",{gpuRays[i].subpanelCount,gpuRays[i].overlapCount}}};
                        std::ofstream(outputPath+"-counterexample.json")<<firstOutWitness.dump(2);
                    }
                }
            }
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        auto mode2NormalRows = correctness(2, false);
        auto mode2NormalHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode2NormalHdrs = reinterpret_cast<const GpuRayHeader*>(mode2NormalHdrBytes.data());
        for(size_t i=0;i<count;++i)if(mode2NormalHdrs[i].overlapCount&0x80000000u)throw std::runtime_error("mode2 unexpected normal fallback");
        auto mode2ForcedRows = correctness(2, true);
        auto mode2ForcedHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode2ForcedHdrs = reinterpret_cast<const GpuRayHeader*>(mode2ForcedHdrBytes.data());
        for(size_t i=0;i<count;++i)if(!(mode2ForcedHdrs[i].overlapCount&0x80000000u))throw std::runtime_error("mode2 did not force fallback");

        auto normalRows = correctness(3, false);
        auto mode3NormalHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode3NormalHdrs = reinterpret_cast<const GpuRayHeader*>(mode3NormalHdrBytes.data());
        size_t mode3Singles = 0, mode3Overs = 0;
        for(size_t i=0;i<count;++i){
            if(mode3NormalHdrs[i].overlapCount&0x80000000u)throw std::runtime_error("mode3 unexpected normal fallback");
            mode3Singles += mode3NormalHdrs[i].subpanelCount;
            mode3Overs += mode3NormalHdrs[i].overlapCount;
        }
        auto forcedRows = correctness(3, true);
        auto mode3ForcedHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode3ForcedHdrs = reinterpret_cast<const GpuRayHeader*>(mode3ForcedHdrBytes.data());
        unsigned forcedCount = 0;
        for(size_t i=0;i<count;++i)if(mode3ForcedHdrs[i].overlapCount&0x80000000u)++forcedCount;
        if(forcedCount!=count)throw std::runtime_error("mode3 stream did not force every fallback");

        report["parallel_output_diff_summary"] = {
            {"strict_bit_differing_rays", mode3StrictBitDiffRays},
            {"strict_bit_exact_pass", mode3StrictBitDiffRays == 0},
            {"max_rgb_ulp_diff", mode3MaxRgbUlp},
            {"max_t_ulp_diff", mode3MaxTUlp},
            {"max_rgb_abs_diff", mode3MaxRgbAbs},
            {"max_t_abs_diff", mode3MaxTAbs},
            {"first_output_witness", firstOutWitness}
        };

        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}'''
    s = s.replace(old_corr_to_timing, new_corr_to_timing, 1)
    return s


def exact_monotone_bowskip_source(base, root, wide=False):
    cr = load(root, 'ray_transport_gpu_continuation_review')
    s = cr.coarse_peel_source(base, root, wide=wide)
    # Replace kFastPathCompactShader with our exact-predicate depth-0 monotone + whole-member bow-skip shader
    old_compact = re.search(r'static const char\* kFastPathCompactShader = R"WGSL\((.*?)\)WGSL";', s, re.S)[0]
    new_compact = 'static const char* kFastPathCompactShader = R"WGSL(\n' + make_exact_monotone_bowskip_wgsl().strip() + '\n)WGSL";'
    if s.count(old_compact) != 1:
        raise ValueError('kFastPathCompactShader hook')
    s = s.replace(old_compact, new_compact, 1)
    s = s.replace(
        '"Exact level-specific coarse bow reject before stack; original20/32/96/48 capacities"',
        '"Exact level-specific 3-level bow mask + whole-member bow skip + depth-0 monotone register peel; original 20/32/96/48 capacities"',
        1,
    )
    return s


def disjoint_fastpath_source(base, root, wide=True, scratch_trimmed=False, stream_merge=False):
    cr = load(root, 'ray_transport_gpu_continuation_review')
    s = cr.coarse_peel_source(base, root, wide=wide)
    old_compact = re.search(r'static const char\* kFastPathCompactShader = R"WGSL\((.*?)\)WGSL";', s, re.S)[0]
    if stream_merge:
        scratch_trimmed = True
    new_compact = 'static const char* kFastPathCompactShader = R"WGSL(\n' + make_exact_monotone_bowskip_wgsl(disjoint_fastpath=True, scratch_trimmed=scratch_trimmed, stream_merge=stream_merge).strip() + '\n)WGSL";'
    if s.count(old_compact) != 1:
        raise ValueError('kFastPathCompactShader hook')
    s = s.replace(old_compact, new_compact, 1)
    s = s.replace(
        '"Exact level-specific coarse bow reject before stack; original20/32/96/48 capacities"',
        '"Exact level-specific 3-level bow mask + whole-member bow skip + depth-0 monotone register peel + strictlyAscendingDisjoint fastpath' + (' + stream-merge' if stream_merge else (' + scratch-trimmed' if scratch_trimmed else '')) + '; 65536 capacity"',
        1,
    )
    return s


def ux_tile_cert_disjoint_source(base, root, wide=True, scratch_trimmed=False, stream_merge=False):
    cr = load(root, 'ray_transport_gpu_continuation_review')
    s = cr.coarse_peel_source(base, root, wide=wide)
    old_compact = re.search(r'static const char\* kFastPathCompactShader = R"WGSL\((.*?)\)WGSL";', s, re.S)[0]
    if stream_merge:
        scratch_trimmed = True
    new_compact = 'static const char* kFastPathCompactShader = R"WGSL(\n' + make_exact_monotone_bowskip_wgsl(disjoint_fastpath=True, tile_cert=True, scratch_trimmed=scratch_trimmed, stream_merge=stream_merge).strip() + '\n)WGSL";'
    if s.count(old_compact) != 1:
        raise ValueError('kFastPathCompactShader hook')
    s = s.replace(old_compact, new_compact, 1)
    s = s.replace(
        '"Exact level-specific coarse bow reject before stack; original20/32/96/48 capacities"',
        '"Exact level-specific 3-level bow mask + whole-member bow skip + depth-0 monotone register peel + ux-tile-cert + strictlyAscendingDisjoint fastpath' + (' + stream-merge' if stream_merge else (' + scratch-trimmed' if scratch_trimmed else '')) + '; 65536 capacity"',
        1,
    )
    needle_spec = '    if (!extractCurtainSpecs(zone, curtainSpecs)) {\n        throw std::runtime_error("Failed to dynamically extract curtain specs from zone");\n    }'
    inject_tile_cert = needle_spec + '''
    std::vector<uint32_t> rayTileMasks(count, 0u);
    {
        std::map<uint32_t, std::vector<uint32_t>> tileToRays;
        for(size_t i=0;i<count;++i){
            uint32_t gx = uint32_t(R[i][0]) / 10u;
            uint32_t gy = uint32_t(R[i][1]) / 10u;
            uint32_t tid = (gy / 4u) * 64u + (gx / 4u);
            tileToRays[tid].push_back(uint32_t(i));
        }
        for(const auto& [tid, rIndices] : tileToRays){
            uint32_t mask = 0u;
            for(uint32_t m=0; m<4; ++m){
                const auto& cur = curtainSpecs[m];
                float uxMin = 1e9f, uxMax = -1e9f;
                struct HR { uint32_t rIdx; float a, b; };
                std::vector<HR> hitRays;
                for(uint32_t rIdx : rIndices){
                    const auto& row = R[rIdx];
                    glm::vec3 d(row[2], row[3], row[4]);
                    glm::vec3 oRel = glm::vec3(ro) - glm::vec3(cur.origin[0], cur.origin[1], cur.origin[2]);
                    float a = row[5], b = row[6];
                    bool hit = true;
                    for(int axis=0; axis<3; ++axis){
                        float half = cur.half[axis];
                        if(axis == 0) half = std::min(half, cur.limits[0]);
                        if(axis == 1) half = std::min(half, cur.limits[1]);
                        float dx = d[axis], ox = oRel[axis];
                        if(dx == 0.0f){
                            if(std::abs(ox) > half + cur.curv[1]){ hit = false; break; }
                        } else {
                            float q0 = (-half - cur.curv[1] - ox) / dx;
                            float q1 = (half + cur.curv[1] - ox) / dx;
                            a = std::max(a, std::min(q0, q1));
                            b = std::min(b, std::max(q0, q1));
                        }
                    }
                    if(hit && b > a){
                        float u0 = oRel.x + a * d.x;
                        float u1 = oRel.x + b * d.x;
                        uxMin = std::min({uxMin, u0, u1});
                        uxMax = std::max({uxMax, u0, u1});
                        hitRays.push_back({rIdx, a, b});
                    }
                }
                if(!hitRays.empty() && uxMax > uxMin){
                    std::array<float, 9> wKnots;
                    for(int k=0; k<9; ++k){
                        float ux = uxMin + float(k)/8.0f * (uxMax - uxMin);
                        wKnots[k] = cur.amps[0]*std::sin(cur.freqs[0]*ux + cur.phases[0])
                                  + cur.amps[1]*std::cos(cur.freqs[1]*ux + cur.phases[1])
                                  + cur.amps[2]*std::sin(cur.freqs[2]*ux + cur.phases[2]);
                    }
                    float h_ux = (uxMax - uxMin) / 8.0f;
                    float bow = 0.125f * cur.curv[0] * h_ux * h_ux;
                    float thresh = bow + cur.limits[3] + 0.001f;
                    bool curActive = false;
                    for(size_t i=0; i<hitRays.size(); ++i){
                        float a = hitRays[i].a, b = hitRays[i].b;
                        uint32_t rIdx = hitRays[i].rIdx;
                        const auto& row = R[rIdx];
                        glm::vec3 d(row[2], row[3], row[4]);
                        glm::vec3 oRel = glm::vec3(ro) - glm::vec3(cur.origin[0], cur.origin[1], cur.origin[2]);
                        if(std::abs(d.x) < 1e-6f) { curActive = true; break; }
                        for(int bin=0; bin<8; ++bin){
                            float u0 = uxMin + float(bin)/8.0f * (uxMax - uxMin);
                            float u1 = uxMin + float(bin+1)/8.0f * (uxMax - uxMin);
                            float su0 = (u0 - oRel.x) / d.x;
                            float su1 = (u1 - oRel.x) / d.x;
                            float phi0 = oRel.z + su0 * d.z + wKnots[bin];
                            float phi1 = oRel.z + su1 * d.z + wKnots[bin+1];
                            float sMin = std::min(su0, su1), sMax = std::max(su0, su1);
                            float cLo = std::max(a, sMin), cHi = std::min(b, sMax);
                            if(cHi > cLo){
                                float tA = (cLo - su0) / (su1 - su0);
                                float tB = (cHi - su0) / (su1 - su0);
                                float pA = phi0 + tA * (phi1 - phi0);
                                float pB = phi0 + tB * (phi1 - phi0);
                                if(!(std::min(pA, pB) > thresh || std::max(pA, pB) < -thresh)){
                                    curActive = true;
                                    break;
                                }
                            }
                        }
                        if(curActive) break;
                    }
                    if(curActive) mask |= (1u << m);
                }
            }
            for(uint32_t rIdx : rIndices){
                rayTileMasks[rIdx] = mask;
            }
        }
    }
    auto maskBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, count * sizeof(uint32_t), rayTileMasks.data());'''
    if s.count(needle_spec) != 1:
        raise ValueError('curtainSpecs needle hook mismatch')
    s = s.replace(needle_spec, inject_tile_cert, 1)

    needle_prod = '''            auto prodGroup=[&](WGPUComputePipeline cp, WGPUBuffer configuration){return group(cp,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),entry(4,tileOverlaps,0,tileOverlapBytes)});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),
                prodGroup(cpFastFull,cb),prodGroup(cpFastFull,fb),
                prodGroup(cpFastCompact,cb),prodGroup(cpFastCompact,fb)});'''
    inject_prod = '''            auto prodGroup=[&](WGPUComputePipeline cp, WGPUBuffer configuration){return group(cp,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),entry(4,tileOverlaps,0,tileOverlapBytes)});};
            auto prodCompactGroup=[&](WGPUBuffer configuration){return group(cpFastCompact,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),entry(4,tileOverlaps,0,tileOverlapBytes),
                entry(5,maskBuf,rayOffset/sizeof(GpuRayHeader)*sizeof(uint32_t),uint64_t(n)*sizeof(uint32_t))});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),
                prodGroup(cpFastFull,cb),prodGroup(cpFastFull,fb),
                prodCompactGroup(cb),prodCompactGroup(fb)});'''
    if s.count(needle_prod) != 1:
        raise ValueError('prodGroup needle hook mismatch')
    s = s.replace(needle_prod, inject_prod, 1)

    needle_full = '''            auto bg=group(cpFastCompact,{entry(0,cfgBuf,0,sizeof(cfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,0,rayBytes),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});'''
    inject_full = '''            auto bg=group(cpFastCompact,{entry(0,cfgBuf,0,sizeof(cfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,0,rayBytes),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes),
                entry(5,maskBuf,0,count*sizeof(uint32_t))});'''
    if s.count(needle_full) != 1:
        raise ValueError('full dispatch needle hook mismatch')
    s = s.replace(needle_full, inject_full, 1)

    needle_budget = '''        for(int variant:{0,1})for(uint32_t budget:{23u,24u,25u}){
            auto u=cfg;u.rayCount=16;u.pad=budget;auto cb=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(u),&u);auto cp=variant?cpFastCompact:cpFastFull;
            wgpuQueueWriteBuffer(gpu.queue,rayB,0,synthetic.data(),16*sizeof(GpuRayHeader));
            auto bg=group(cp,{entry(0,cb,0,sizeof(u)),entry(1,specB,0,specs.size()*sizeof(GpuCurtainSpec)),entry(2,rayB,0,16*sizeof(GpuRayHeader)),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});'''
    inject_budget = '''        std::vector<uint32_t> synMasks(16, 0xFFFFFFFFu);
        auto synMaskBuf=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,16*sizeof(uint32_t),synMasks.data());
        for(int variant:{0,1})for(uint32_t budget:{23u,24u,25u}){
            auto u=cfg;u.rayCount=16;u.pad=budget;auto cb=makeBuffer(WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,sizeof(u),&u);auto cp=variant?cpFastCompact:cpFastFull;
            wgpuQueueWriteBuffer(gpu.queue,rayB,0,synthetic.data(),16*sizeof(GpuRayHeader));
            auto bg=variant?group(cpFastCompact,{entry(0,cb,0,sizeof(u)),entry(1,specB,0,specs.size()*sizeof(GpuCurtainSpec)),entry(2,rayB,0,16*sizeof(GpuRayHeader)),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes),entry(5,synMaskBuf,0,16*sizeof(uint32_t))}):group(cpFastFull,{entry(0,cb,0,sizeof(u)),entry(1,specB,0,specs.size()*sizeof(GpuCurtainSpec)),entry(2,rayB,0,16*sizeof(GpuRayHeader)),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});'''
    if s.count(needle_budget) != 1:
        raise ValueError('budget needle hook mismatch')
    s = s.replace(needle_budget, inject_budget, 1)

    return s


def build_variant(root: Path, build_root: Path, work: Path, variant: str):
    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    if variant == 'parallel-soa':
        cpp = parallel_soa_source(v['CPP_SOURCE'], root)
    elif variant == 'coarse-monotone-bowskip':
        cpp = exact_monotone_bowskip_source(v['CPP_SOURCE'], root, wide=False)
    elif variant == 'coarse-monotone-bowskip-wide':
        cpp = exact_monotone_bowskip_source(v['CPP_SOURCE'], root, wide=True)
    elif variant == 'disjoint-fastpath-wide':
        cpp = disjoint_fastpath_source(v['CPP_SOURCE'], root, wide=True)
    elif variant == 'disjoint-scratch-trimmed-wide':
        cpp = disjoint_fastpath_source(v['CPP_SOURCE'], root, wide=True, scratch_trimmed=True)
    elif variant == 'disjoint-stream-merge-wide':
        cpp = disjoint_fastpath_source(v['CPP_SOURCE'], root, wide=True, scratch_trimmed=True, stream_merge=True)
    elif variant == 'ux-tile-cert-disjoint-wide':
        cpp = ux_tile_cert_disjoint_source(v['CPP_SOURCE'], root, wide=True)
    elif variant == 'ux-tile-cert-scratch-trimmed-wide':
        cpp = ux_tile_cert_disjoint_source(v['CPP_SOURCE'], root, wide=True, scratch_trimmed=True)
    elif variant == 'ux-tile-cert-stream-merge-wide':
        cpp = ux_tile_cert_disjoint_source(v['CPP_SOURCE'], root, wide=True, scratch_trimmed=True, stream_merge=True)
    else:
        raise ValueError('unsupported variant: ' + variant)

    work.mkdir(parents=True, exist_ok=False)
    (work / 'probe.cpp').write_text(cpp)
    (work / 'source.json').write_text(json.dumps({
        'variant': variant,
        'cpp_sha256': sha(work / 'probe.cpp'),
        'zone_sha256': v['ZONE_HASH'],
        'build_root': str(build_root),
    }, indent=2) + '\n')

    b = build_root / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(work / 'probe.cpp')
    cmd[cmd.index('-o') + 1] = str(work / 'probe.o')
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    link_cmd[link_cmd.index('-o') + 1] = str(work / 'probe')
    subprocess.run(link_cmd, cwd=b, check=True)
    return v['ZONE_HASH']


def validate_6dd(root, report, rows, baseline, rays, ref):
    cr = load(root, 'ray_transport_gpu_continuation_review')
    p = load(root, 'ray_transport_sliver_review')
    p.clocks = cr.clocks
    n = len(rows)
    if 'coarse_peel_experiment' in report:
        res = cr.validate(root, report, rows, baseline, rays, ref)
        res['affine'] = p.affine_endpoints(
            report['schedule_census_inputs'],
            report['coarse_packet_census_inputs'],
            rays,
            report['guarded_sliver_policy']['curtain_bindings'],
        )
        if res['affine']['missing']:
            raise ValueError('coarse-monotone-bowskip affine endpoint missing')
        if 'routed-residual' in report['coarse_peel_experiment']['scope']:
            census = report.get('routed_residual', {})
            keys = ['s1_tiles', 's2_tiles', 'fallback_tiles']
            if set(census) != set(keys) or any(type(census[k]) is not int or census[k] < 0 for k in keys) or sum(census.values()) != 2304:
                raise ValueError('routed residual census coverage')
            res['routed_residual_scope'] = {
                'candidate_timing': 'mode3; GPU reset/classification/indirect producers/canonical consumer/Current fallback',
                'host_preparation_initial_upload_and_compile_included': False,
                'legacy_budget_controls': 'bulk cpFastFull/cpFastCompact, not routed kernels',
                'routed_capacity_and_forced_current_controls': True,
                'changed_pose_and_ownership_mutations_in_this_report': False,
                'timing_series': 'one four-mode32sample series per repeat; eight observations per mode',
                'physical_register_spill_occupancy_profile': False,
                'shipping_performance_qualified': False,
            }
        elif 'stream-merge' in report['coarse_peel_experiment']['scope']:
            res['stream_merge_scope'] = {
                'qualification_lane': 'strict_bit_exact_to_guarded_schedule_and_transport',
                'candidate_timing': 'streamed_schedule.cases[].series[].samples mode3',
                'legacy_batch_timing': 'unchanged cpProducer; not the streaming merge candidate',
                'host_certificate_creation_and_initial_upload_in_mode3_timing': False,
                'member_knots_capacity': 70, 'member_knots_required_upper_bound': 69,
                'merge_knots_eliminated': True,
                'physical_bytes_saved_established': False,
                'shipping_performance_qualified': False,
            }
        elif 'scratch-trimmed' in report['coarse_peel_experiment']['scope']:
            # These labels describe actual callers, not the audit's desired conclusion.
            # BATCH uses unchanged cpProducer; mode3 uses trimmed cpFastCompact.
            res['scratch_trimmed_scope'] = {
                'qualification_lane': 'strict_bit_exact_to_guarded_schedule_and_transport',
                'candidate_timing': 'streamed_schedule.cases[].series[].samples mode3',
                'legacy_batch_timing': 'unchanged cpProducer; not the trimmed candidate',
                'host_certificate_creation_and_initial_upload_in_mode3_timing': False,
                'member_knots_capacity': 70, 'member_knots_required_upper_bound': 69,
                'merge_knots_capacity': 96, 'merge_knots_required_upper_bound': 96,
                'physical_bytes_saved_established': False,
                'shipping_performance_qualified': False,
            }
        return res
    result = p.check(root, report, rows, baseline, rays, ref['rows'], ref['ray_indices'])
    for c in report['streamed_schedule']['cases']:
        if len(c['mode2_normal_rows']) != n or len(c['mode2_forced_rows']) != n:
            raise ValueError('mode2 output coverage')
        if any(a[:2] != b[:2] or p.bits(a[2:6]) != p.bits(b[2:6]) for a, b in zip(c['mode2_forced_rows'], baseline)):
            raise ValueError('mode2 forced parity')
    result['affine'] = p.affine_endpoints(
        report['schedule_census_inputs'],
        report['parallel_packet_census_inputs'],
        rays,
        report['guarded_sliver_policy']['curtain_bindings'],
    )
    if result['affine']['missing']:
        raise ValueError('parallel-soa affine endpoint missing')
    result['parallel_packet_diff_summary'] = report['parallel_packet_diff_summary']
    result['parallel_output_diff_summary'] = report['parallel_output_diff_summary']
    result['qualification_lane'] = (
        'strict_bit_exact'
        if (report['parallel_packet_diff_summary']['packet_bit_differing_rays'] == 0
            and report['parallel_output_diff_summary']['strict_bit_differing_rays'] == 0)
        else 'predeclared_numerical_accuracy'
    )
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, default=Path('/private/tmp/earthcall-rt-pure-build'))
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--variant', choices=[
        'parallel-soa',
        'coarse-monotone-bowskip',
        'coarse-monotone-bowskip-wide',
        'disjoint-fastpath-wide',
        'disjoint-scratch-trimmed-wide',
        'disjoint-stream-merge-wide',
        'ux-tile-cert-disjoint-wide',
        'ux-tile-cert-scratch-trimmed-wide',
        'ux-tile-cert-stream-merge-wide',
    ], required=True)
    p.add_argument('--build-only', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    case = args.case.resolve()
    wide = load(root, 'ray_transport_gpu_wide_stream')

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    z = sha(zone)
    meta, rays, refs = wide.verify_case(root, case, z)
    case_sha = sha(case)
    expected = build_variant(root, args.build_root.resolve(), work, args.variant)
    if z != expected or sha(zone) != z or sha(case) != case_sha:
        raise ValueError('pinned source/case drift')
    if args.build_only:
        print('PASS §6dd build-only:', args.variant, work / 'probe', 'cpp_sha256:', sha(work / 'probe.cpp'))
        return

    output = work / 'result.json'
    env = dict(os.environ,
               EC_POSE_RAYS=meta['rays'],
               EC_POSE_REFERENCE=meta['reference'],
               EC_POSE_CLOCK=str(rays['time']),
               EC_MATCH_REPEAT='1',
               EC_GPU_BUDGET='1024',
               EC_GPU_SUB_CAP='64',
               EC_GPU_OVER_CAP='16',
               EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None)
    with (work / 'native.log').open('x') as f:
        subprocess.run([str(work / 'probe'), str(output)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)
    if sha(zone) != z or sha(case) != case_sha:
        raise ValueError('source/case changed during execution')
    wide.verify_case(root, case, z)
    report = json.loads(output.read_text())
    rows = json.loads(Path(str(output) + '-rows.json').read_text())
    baseline = json.loads(Path(str(output) + '-baseline-rows.json').read_text())
    ref = json.loads(Path(meta['reference']).read_text())
    checked = validate_6dd(root, report, rows, baseline, rays, ref)
    (work / 'independent.json').write_text(json.dumps(checked, indent=2) + '\n')
    print('PASS §6dd native', args.variant, 'qualification:', work / 'independent.json')


if __name__ == '__main__':
    main()
