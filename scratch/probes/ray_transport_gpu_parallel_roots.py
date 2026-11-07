"""§6da Part 2: Per-Member Parallel Root-Production Experiment on Apple Silicon M5 Metal.
Completes Sixth-One Sun (Codex / GPT-6.1 Sol) §6da experimental design + Aetherion / The Firmament execution.

Tests splitting root production across 4 * rayCount parallel invocations (1 thread per (ray, member) pair
in `memberRootsMain`), writing ordered per-member pieces and work counts, followed by `mergeMain`
(per-ray 4-way merge + linear active-mask sweep emitting subpanels and overlaps) and the unchanged
`cpUnfused` consumer and `fallbackCP` native marcher fallback.

Benchmarks 4 modes in balanced paired order at primary 32,768 capacity (1, 2, 4 repeats):
- Mode 0: Current Marcher
- Mode 1: Guarded Bulk Connected (Sequential Producer + Unfused Consumer + Fallback)
- Mode 2: Guarded Bounded Stream 32,768 (Sequential Producer + Unfused Consumer + Fallback)
- Mode 3: Parallel-Root Bounded Stream 32,768 (memberRootsMain [4*N] + mergeMain [N] + Unfused Consumer + Fallback)
"""
import argparse, ast, gzip, hashlib, importlib.util, json, math, os, shlex, statistics, subprocess, sys
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


PARALLEL_ROOTS_WGSL = r'''
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
const MAX_PIECES_PER_MEMBER: u32 = 16u;

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

// Pass 1: 4 * rayCount parallel invocations (1 thread per (ray, member) pair).
// Computes member-local pieces and work count without cross-member serial dependency.
@compute @workgroup_size(64)
fn memberRootsMain(@builtin(global_invocation_id) gid: vec3u) {
    let pairId = gid.x;
    if (pairId >= cfg.rayCount * 4u) { return; }
    let rayId = pairId >> 2u;
    let member = pairId & 3u;
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

// Pass 2: Per-ray 4-way merge and linear active-mask sweep over the 4 parallel member streams.
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
        let pairId = id * 4u + m;
        let meta = memberMeta[pairId];
        let mCount = meta.x;
        let mWork = meta.y;
        let mReason = meta.z;
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


def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    ms = load(root, 'ray_transport_gpu_producer_merge_sweep')
    s = ms.source(base, root)

    # 1. Add kParallelRootsShader and build cpMemberRoots and cpMerge
    shader_decl = 'static const char* kParallelRootsShader = R"WGSL(\n' + PARALLEL_ROOTS_WGSL.strip() + '\n)WGSL";\n\n'
    s = q.once(
        s,
        '    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");',
        shader_decl
        + '    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");\n'
        + '    auto cpMemberRoots = buildCompute(kParallelRootsShader, "memberRootsMain");\n'
        + '    auto cpMerge = buildCompute(kParallelRootsShader, "mergeMain");',
    )

    # 2. Add readBackBuffer helper
    readback_helper = r'''
    auto readBackBuffer = [&](WGPUBuffer src, uint64_t size) -> std::vector<uint8_t> {
        auto rb = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, size, nullptr);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, rb, 0, size);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        MapContext mc;
        WGPUBufferMapCallbackInfo cbInfo = {};
        cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        cbInfo.callback = onMapped;
        cbInfo.userdata1 = &mc;
        wgpuBufferMapAsync(rb, WGPUMapMode_Read, 0, size, cbInfo);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (!mc.ok) throw std::runtime_error("buffer mapping failed");

        const uint8_t* ptr = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(rb, 0, size));
        std::vector<uint8_t> data(ptr, ptr + size);
        wgpuBufferUnmap(rb);
        wgpuBufferRelease(rb);
        return data;
    };
'''
    s = q.once(s, '    auto readResults = [&]() -> std::vector<GpuResult> {',
               readback_helper + '\n    auto readResults = [&]() -> std::vector<GpuResult> {')

    # 3. Restrict capacity loop to primary 32768u and allocate tileMemberPieces + tileMemberMeta
    s = q.once(s, 'for(uint32_t capacity:{8192u,16384u,32768u})', 'for(uint32_t capacity:{32768u})')

    old_tile_buffers = r'''        const uint64_t tileSingleBytes=uint64_t(capacity)*maxSubPerRay*sizeof(GpuSubpanel);
        const uint64_t tileOverlapBytes=uint64_t(capacity)*maxOverPerRay*sizeof(GpuOverlap);
        auto tileSingles=makeBuffer(WGPUBufferUsage_Storage,tileSingleBytes,nullptr);
        auto tileOverlaps=makeBuffer(WGPUBufferUsage_Storage,tileOverlapBytes,nullptr);
        struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback;};'''

    new_tile_buffers = r'''        const uint64_t tileSingleBytes=uint64_t(capacity)*maxSubPerRay*sizeof(GpuSubpanel);
        const uint64_t tileOverlapBytes=uint64_t(capacity)*maxOverPerRay*sizeof(GpuOverlap);
        const uint64_t tileMemberPiecesBytes=uint64_t(capacity)*4u*16u*sizeof(glm::vec2);
        const uint64_t tileMemberMetaBytes=uint64_t(capacity)*4u*sizeof(glm::uvec4);
        auto tileSingles=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,tileSingleBytes,nullptr);
        auto tileOverlaps=makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,tileOverlapBytes,nullptr);
        auto tileMemberPieces=makeBuffer(WGPUBufferUsage_Storage,tileMemberPiecesBytes,nullptr);
        auto tileMemberMeta=makeBuffer(WGPUBufferUsage_Storage,tileMemberMetaBytes,nullptr);
        struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback,memberRoots,forcedMemberRoots,merge,forcedMerge;};'''
    s = q.once(s, old_tile_buffers, new_tile_buffers)

    old_tile_push = r'''            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP)});'''
    new_tile_push = r'''            auto parallelGroup=[&](WGPUComputePipeline cp, WGPUBuffer configuration){return group(cp,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),
                entry(4,tileOverlaps,0,tileOverlapBytes),entry(5,tileMemberPieces,0,tileMemberPiecesBytes),
                entry(6,tileMemberMeta,0,tileMemberMetaBytes)});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),
                parallelGroup(cpMemberRoots,cb),parallelGroup(cpMemberRoots,fb),
                parallelGroup(cpMerge,cb),parallelGroup(cpMerge,fb)});'''
    s = q.once(s, old_tile_push, new_tile_push)

    # 4. Update execute lambda to support 4 modes:
    # Mode 0: Current Marcher
    # Mode 1: Guarded Bulk Connected (Sequential Producer + Consumer + Fallback)
    # Mode 2: Guarded Bounded Stream 32,768 (Sequential Producer + Consumer + Fallback)
    # Mode 3: Parallel-Root Bounded Stream 32,768 (memberRootsMain [4*N] + mergeMain [N] + Consumer + Fallback)
    old_exec = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){
                    dispatch(cpProducer,forced?tile.forced:tile.producer,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    new_exec = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else if(mode==2)for(const auto& tile:tiles){
                    dispatch(cpProducer,forced?tile.forced:tile.producer,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpMemberRoots,forced?tile.forcedMemberRoots:tile.memberRoots,tile.n*4u);
                    dispatch(cpMerge,forced?tile.forcedMerge:tile.merge,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    s = q.once(s, old_exec, new_exec)

    # 5. Update correctness and packet/header readback for both Mode 2 and Mode 3
    old_corr_section = r'''        // Poison checks and raw output are outside timed batches.
        auto correctness=[&](bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(2,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
                if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k]))throw std::runtime_error("stream changed or missed a result");
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        auto normalRows=correctness(false);auto forcedRows=correctness(true);
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,rayBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,rBuf,0,rb,0,rayBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,rayBytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("stream header readback refused");
        const auto* headers=static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(rb,0,rayBytes));
        unsigned forcedCount=0;for(size_t i=0;i<count;++i)if(headers[i].overlapCount&0x80000000u)++forcedCount;
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);
        if(forcedCount!=count)throw std::runtime_error("stream did not force every fallback");'''

    new_corr_section = r'''        auto correctness=[&](int testMode, bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(testMode,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
                if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k]))
                    throw std::runtime_error("stream mode " + std::to_string(testMode) + " changed or missed a result at ray " + std::to_string(i));
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        // Verify Mode 2 (sequential stream) normal & forced
        auto mode2NormalRows = correctness(2, false);
        auto mode2NormalHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode2NormalHdrs = reinterpret_cast<const GpuRayHeader*>(mode2NormalHdrBytes.data());
        for(size_t i=0;i<count;++i)if(mode2NormalHdrs[i].overlapCount&0x80000000u)throw std::runtime_error("mode2 unexpected normal fallback");
        auto mode2ForcedRows = correctness(2, true);
        auto mode2ForcedHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode2ForcedHdrs = reinterpret_cast<const GpuRayHeader*>(mode2ForcedHdrBytes.data());
        for(size_t i=0;i<count;++i)if(!(mode2ForcedHdrs[i].overlapCount&0x80000000u))throw std::runtime_error("mode2 did not force fallback");

        // Verify Mode 3 (parallel-root stream) normal & read headers before forced fallback overwrites rBuf
        auto normalRows = correctness(3, false);
        auto mode3NormalHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode3NormalHdrs = reinterpret_cast<const GpuRayHeader*>(mode3NormalHdrBytes.data());
        size_t mode3Singles = 0, mode3Overs = 0;
        for(size_t i=0;i<count;++i){
            if(mode3NormalHdrs[i].overlapCount&0x80000000u)throw std::runtime_error("mode3 unexpected normal fallback");
            if(mode3NormalHdrs[i].subpanelCount!=gpuRays[i].subpanelCount || mode3NormalHdrs[i].overlapCount!=gpuRays[i].overlapCount)
                throw std::runtime_error("mode3 header counts differ from guarded producer");
            mode3Singles += mode3NormalHdrs[i].subpanelCount;
            mode3Overs += mode3NormalHdrs[i].overlapCount;
        }
        auto forcedRows = correctness(3, true);
        auto mode3ForcedHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode3ForcedHdrs = reinterpret_cast<const GpuRayHeader*>(mode3ForcedHdrBytes.data());
        unsigned forcedCount = 0;
        for(size_t i=0;i<count;++i)if(mode3ForcedHdrs[i].overlapCount&0x80000000u)++forcedCount;
        if(forcedCount!=count)throw std::runtime_error("mode3 stream did not force every fallback");

        // Also run a full 32,784-ray parallel-root packet census against gpuSubpanels / gpuOverlaps
        {
            const uint64_t fullMPBytes = uint64_t(count)*4u*16u*sizeof(glm::vec2);
            const uint64_t fullMMBytes = uint64_t(count)*4u*sizeof(glm::uvec4);
            auto fullMP = makeBuffer(WGPUBufferUsage_Storage, fullMPBytes, nullptr);
            auto fullMM = makeBuffer(WGPUBufferUsage_Storage, fullMMBytes, nullptr);
            auto parSub = makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc, subpanelBytes, nullptr);
            auto parOver = makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc, overlapBytes, nullptr);
            auto parRay = makeBuffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst, rayBytes, gpuRays.data());
            auto bgMR = group(cpMemberRoots, {
                entry(0,cfgBuf,0,sizeof(cfg)), entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,parRay,0,rayBytes), entry(3,parSub,0,subpanelBytes), entry(4,parOver,0,overlapBytes),
                entry(5,fullMP,0,fullMPBytes), entry(6,fullMM,0,fullMMBytes)});
            auto bgMg = group(cpMerge, {
                entry(0,cfgBuf,0,sizeof(cfg)), entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,parRay,0,rayBytes), entry(3,parSub,0,subpanelBytes), entry(4,parOver,0,overlapBytes),
                entry(5,fullMP,0,fullMPBytes), entry(6,fullMM,0,fullMMBytes)});
            auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass1, cpMemberRoots);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgMR, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (count*4u+63u)/64u, 1, 1);
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
            for(size_t i=0;i<count;++i){
                if(pRays[i].subpanelCount!=gpuRays[i].subpanelCount || pRays[i].overlapCount!=gpuRays[i].overlapCount)
                    throw std::runtime_error("parallel-root packet count mismatch at ray " + std::to_string(i));
                for(uint32_t k=0;k<pRays[i].subpanelCount;++k){
                    const auto& a = pSubs[pRays[i].subpanelOffset+k];
                    const auto& b = gpuSubpanels[gpuRays[i].subpanelOffset+k];
                    if(std::bit_cast<uint32_t>(a.s0)!=std::bit_cast<uint32_t>(b.s0) ||
                       std::bit_cast<uint32_t>(a.s1)!=std::bit_cast<uint32_t>(b.s1) ||
                       a.mediumIndex!=b.mediumIndex)
                        throw std::runtime_error("parallel-root subpanel bit mismatch at ray " + std::to_string(i));
                }
                for(uint32_t k=0;k<pRays[i].overlapCount;++k){
                    const auto& a = pOvers[pRays[i].overlapOffset+k];
                    const auto& b = gpuOverlaps[gpuRays[i].overlapOffset+k];
                    if(std::bit_cast<uint32_t>(a.s0)!=std::bit_cast<uint32_t>(b.s0) ||
                       std::bit_cast<uint32_t>(a.s1)!=std::bit_cast<uint32_t>(b.s1) ||
                       a.activeMask!=b.activeMask)
                        throw std::runtime_error("parallel-root overlap bit mismatch at ray " + std::to_string(i));
                }
            }
            wgpuBindGroupRelease(bgMR); wgpuBindGroupRelease(bgMg);
            wgpuBufferRelease(fullMP); wgpuBufferRelease(fullMM);
            wgpuBufferRelease(parSub); wgpuBufferRelease(parOver); wgpuBufferRelease(parRay);
        }'''
    s = q.once(s, old_corr_section, new_corr_section)

    # 6. Update timing loop to 4 modes in balanced order
    old_timing = r'''        // Separate warm correctness from measured ABBA/BAAB complete costs.
        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order=block%2?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0};
                for(int mode:order){std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;auto t=execute(mode,repeats,false);samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});}
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }'''
    new_timing = r'''        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order=block%2?std::vector<int>{3,2,1,0,0,1,2,3}:std::vector<int>{0,1,2,3,3,2,1,0};
                for(int mode:order){std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;auto t=execute(mode,repeats,false);samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});}
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }'''
    s = q.once(s, old_timing, new_timing)

    # 7. Release extra bind groups and buffers
    old_release = r'''        for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback})wgpuBindGroupRelease(bg);
        for(auto b:tileUniforms)wgpuBufferRelease(b);wgpuBufferRelease(tileSingles);wgpuBufferRelease(tileOverlaps);'''
    new_release = r'''        for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback,tile.memberRoots,tile.forcedMemberRoots,tile.merge,tile.forcedMerge})wgpuBindGroupRelease(bg);
        for(auto b:tileUniforms)wgpuBufferRelease(b);wgpuBufferRelease(tileSingles);wgpuBufferRelease(tileOverlaps);wgpuBufferRelease(tileMemberPieces);wgpuBufferRelease(tileMemberMeta);'''
    s = q.once(s, old_release, new_release)

    # 8. Export mode2NormalRows and mode2ForcedRows in streamingCases.push_back
    old_push = r'''        streamingCases.push_back({{"capacity",capacity},{"tiles",tiles.size()},{"tail_rays",tiles.back().n},
            {"schedule_storage_bytes",tileSingleBytes+tileOverlapBytes},{"uniform_buffer_bytes",tiles.size()*(sizeof(udata)+2*sizeof(cfg)+sizeof(currentU))},
            {"binding_buffer_setup_ms",setupMs},{"normal_rows",normalRows},{"forced_rows",forcedRows},
            {"forced_fallback_rays",forcedCount},{"bit_identical_to_bulk_candidate",true},{"forced_bit_identical_to_native",true},{"series",series}});'''
    new_push = r'''        streamingCases.push_back({{"capacity",capacity},{"tiles",tiles.size()},{"tail_rays",tiles.back().n},
            {"schedule_storage_bytes",tileSingleBytes+tileOverlapBytes},
            {"tile_member_pieces_bytes",tileMemberPiecesBytes+tileMemberMetaBytes},
            {"uniform_buffer_bytes",tiles.size()*(sizeof(udata)+2*sizeof(cfg)+sizeof(currentU))},
            {"binding_buffer_setup_ms",setupMs},{"normal_rows",normalRows},{"forced_rows",forcedRows},
            {"mode2_normal_rows",mode2NormalRows},{"mode2_forced_rows",mode2ForcedRows},
            {"mode3_singles",mode3Singles},{"mode3_overs",mode3Overs},
            {"forced_fallback_rays",forcedCount},{"bit_identical_to_bulk_candidate",true},{"forced_bit_identical_to_native",true},{"series",series}});'''
    s = q.once(s, old_push, new_push)

    # 9. Add metadata and avoid stdout dump if large
    meta = r'''
    report["parallel_roots_experiment"] = {
        {"protocol", "§6da: Per-member parallel root production (4 * rayCount threads) + per-ray 4-way merge & sweep + unfused consumer + native fallback"},
        {"bit_exact_packets_verified_in_cpp", true},
        {"bit_exact_outputs_verified_in_cpp", true},
        {"modes", {{"0", "Current Marcher"}, {"1", "Guarded Bulk Connected"}, {"2", "Guarded Sequential Stream 32768"}, {"3", "Parallel-Root Stream 32768"}}}
    };
    wgpuComputePipelineRelease(cpMemberRoots);
    wgpuComputePipelineRelease(cpMerge);
'''
    anchor = '    std::cout << report.dump(2) << std::endl;'
    s = q.once(s, anchor, meta + '\n' + anchor)
    return s


def build_and_run(root, work, case_path, label, build_only=False):
    wide = load(root, 'ray_transport_gpu_wide_stream')
    fly_rev = load(root, 'ray_transport_gpu_on_the_fly_review')
    sliver = load(root, 'ray_transport_sliver_review')

    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    if sha(zone) != v['ZONE_HASH']:
        raise SystemExit('pinned authored source changed')
    case_meta, rays, (ref_rows, ref_indices) = wide.verify_case(root, case_path, v['ZONE_HASH'])
    mhash = sha(case_path)

    cpp = source(v['CPP_SOURCE'], root)
    digest = hashlib.sha256(cpp.encode()).hexdigest()

    work.mkdir(parents=True, exist_ok=False)
    probe_cpp = work / 'probe.cpp'
    probe_o = work / 'probe.o'
    probe_bin = work / 'probe'
    probe_cpp.write_text(cpp)
    (work / 'source.json').write_text(json.dumps({'cpp_sha256': digest, 'zone_sha256': v['ZONE_HASH']}, indent=2) + '\n')

    b = root / 'build'
    tmp_inc = Path('/private/tmp/earthcall-d4963d062-src')
    db = json.loads((b / 'compile_commands.json').read_text())
    c = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(c['command'])
    if tmp_inc.exists():
        inc_idx = next(i for i, x in enumerate(cmd) if x.startswith('-I'))
        cmd.insert(inc_idx, f'-I{tmp_inc}/src')
    cmd[cmd.index('-c') + 1] = str(probe_cpp)
    cmd[cmd.index('-o') + 1] = str(probe_o)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(probe_o)
    if (tmp_inc / 'ScalarForm.o').exists():
        link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('ScalarForm.cpp.o'))] = str(tmp_inc / 'ScalarForm.o')
        link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('SdfWgsl.cpp.o'))] = str(tmp_inc / 'SdfWgsl.o')
        for extra_o in ['LineRestriction.o', 'VolumeRayTransport.o', 'Integral.o', 'EllipticIntegrals.o']:
            link_cmd.insert(link_cmd.index('-o'), str(tmp_inc / extra_o))
    link_cmd[link_cmd.index('-o') + 1] = str(probe_bin)
    subprocess.run(link_cmd, cwd=b, check=True)
    if build_only:
        return

    output_json = work / (label + '.json')
    env = dict(os.environ,
               EC_POSE_RAYS=case_meta['rays'],
               EC_POSE_REFERENCE=case_meta['reference'],
               EC_POSE_CLOCK=str(rays['time']),
               EC_MATCH_REPEAT='1',
               EC_GPU_BUDGET='1024',
               EC_GPU_SUB_CAP='64',
               EC_GPU_OVER_CAP='16',
               EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None)
    wide.verify_case(root, case_path, v['ZONE_HASH'])
    if sha(case_path) != mhash or sha(zone) != v['ZONE_HASH']:
        raise SystemExit('fixture changed before execution')

    with output_json.open('w') as out_f:
        subprocess.run([str(probe_bin), str(output_json)], cwd=root, env=env, stdout=out_f, check=True)

    # Parse stdout where STREAM lines precede JSON
    raw_text = output_json.read_text()
    json_start = raw_text.find('{\n')
    report = json.loads(raw_text[json_start:])
    output_json.write_text(json.dumps(report, indent=2) + '\n')

    rows = json.loads((work / (label + '.json-rows.json')).read_text())
    baseline = json.loads((work / (label + '.json-baseline-rows.json')).read_text())
    sliver.clocks = fly_rev.analyze_clocks_4mode
    check = sliver.check(root, report, rows, baseline, rays, ref_rows, ref_indices)
    case0 = report['streamed_schedule']['cases'][0]
    if any(sliver.bits(a[2:6]) != sliver.bits(b[2:6]) for a, b in zip(case0['mode2_normal_rows'], rows)):
        raise ValueError('mode2 normal rows mismatch')
    if any(sliver.bits(a[2:6]) != sliver.bits(b[2:6]) for a, b in zip(case0['mode2_forced_rows'], baseline)):
        raise ValueError('mode2 forced rows mismatch')

    summary = {
        'protocol': '§6da Part 2: Per-member parallel root-production experiment',
        'cpp_sha256': digest,
        'zone_sha256': v['ZONE_HASH'],
        'all_32784_packets_bit_exact': True,
        'all_32784_outputs_bit_exact': True,
        'mode3_singles': case0['mode3_singles'],
        'mode3_overs': case0['mode3_overs'],
        'forced_fallback_rays': case0['forced_fallback_rays'],
        'reference_accuracy': check['accuracy'],
        'contemporary_accuracy': check['contemporary_accuracy'],
        'clocks_4mode': check['clocks'],
    }
    (work / (label + '-summary.json')).write_text(json.dumps(summary, indent=2) + '\n')
    print('PASS §6da parallel-root experiment:', work / (label + '-summary.json'))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--label', default='parallel_roots')
    p.add_argument('--build-only', action='store_true')
    args = p.parse_args()
    build_and_run(args.root.resolve(), args.work.resolve(), args.case.resolve(), args.label, args.build_only)


if __name__ == '__main__':
    main()
