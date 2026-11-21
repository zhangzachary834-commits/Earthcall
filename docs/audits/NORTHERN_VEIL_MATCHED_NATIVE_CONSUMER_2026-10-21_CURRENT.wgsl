
fn mod289(x: vec4<f32>) -> vec4<f32> {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
fn mod289_3(x: vec3<f32>) -> vec3<f32> {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
fn permute4(x: vec4<f32>) -> vec4<f32> {
    return mod289(((x * 34.0) + 1.0) * x);
}
fn taylorInvSqrt(r: vec4<f32>) -> vec4<f32> {
    return 1.79284291400159 - 0.85373472095314 * r;
}
fn cnoise3(P: vec3<f32>) -> f32 {
    let Pi0 = floor(P);
    let Pi1 = Pi0 + vec3<f32>(1.0);
    let Pi0_mod = mod289_3(Pi0);
    let Pi1_mod = mod289_3(Pi1);
    let Pf0 = fract(P);
    let Pf1 = Pf0 - vec3<f32>(1.0);
    let ix = vec4<f32>(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    let iy = vec4<f32>(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    let iz0 = vec4<f32>(Pi0_mod.z);
    let iz1 = vec4<f32>(Pi1_mod.z);

    let ixy = permute4(permute4(ix) + iy);
    let ixy0 = permute4(ixy + iz0);
    let ixy1 = permute4(ixy + iz1);

    var gx0 = ixy0 / 7.0;
    var gy0 = fract(floor(gx0) / 7.0) - 0.5;
    gx0 = fract(gx0);
    var gz0 = vec4<f32>(0.5) - abs(gx0) - abs(gy0);
    let sz0 = step(gz0, vec4<f32>(0.0));
    gx0 = gx0 - sz0 * (step(vec4<f32>(0.0), gx0) - 0.5);
    gy0 = gy0 - sz0 * (step(vec4<f32>(0.0), gy0) - 0.5);

    var gx1 = ixy1 / 7.0;
    var gy1 = fract(floor(gx1) / 7.0) - 0.5;
    gx1 = fract(gx1);
    var gz1 = vec4<f32>(0.5) - abs(gx1) - abs(gy1);
    let sz1 = step(gz1, vec4<f32>(0.0));
    gx1 = gx1 - sz1 * (step(vec4<f32>(0.0), gx1) - 0.5);
    gy1 = gy1 - sz1 * (step(vec4<f32>(0.0), gy1) - 0.5);

    var g000 = vec3<f32>(gx0.x,gy0.x,gz0.x);
    var g100 = vec3<f32>(gx0.y,gy0.y,gz0.y);
    var g010 = vec3<f32>(gx0.z,gy0.z,gz0.z);
    var g110 = vec3<f32>(gx0.w,gy0.w,gz0.w);
    var g001 = vec3<f32>(gx1.x,gy1.x,gz1.x);
    var g101 = vec3<f32>(gx1.y,gy1.y,gz1.y);
    var g011 = vec3<f32>(gx1.z,gy1.z,gz1.z);
    var g111 = vec3<f32>(gx1.w,gy1.w,gz1.w);

    let norm0 = taylorInvSqrt(vec4<f32>(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 = g000 * norm0.x;
    g010 = g010 * norm0.y;
    g100 = g100 * norm0.z;
    g110 = g110 * norm0.w;
    let norm1 = taylorInvSqrt(vec4<f32>(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 = g001 * norm1.x;
    g011 = g011 * norm1.y;
    g101 = g101 * norm1.z;
    g111 = g111 * norm1.w;

    let n000 = dot(g000, Pf0);
    let n100 = dot(g100, vec3<f32>(Pf1.x, Pf0.y, Pf0.z));
    let n010 = dot(g010, vec3<f32>(Pf0.x, Pf1.y, Pf0.z));
    let n110 = dot(g110, vec3<f32>(Pf1.x, Pf1.y, Pf0.z));
    let n001 = dot(g001, vec3<f32>(Pf0.x, Pf0.y, Pf1.z));
    let n101 = dot(g101, vec3<f32>(Pf1.x, Pf0.y, Pf1.z));
    let n011 = dot(g011, vec3<f32>(Pf0.x, Pf1.y, Pf1.z));
    let n111 = dot(g111, Pf1);

    let fade_xyz = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0 - 15.0) + 10.0);
    let n_z = mix(vec4<f32>(n000, n100, n010, n110), vec4<f32>(n001, n101, n011, n111), fade_xyz.z);
    let n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    let n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
    return 2.2 * n_xyz;
}

fn ontoRawPow(base: f32, exp: f32) -> f32 {
    let baseBits = bitcast<u32>(base);
    let baseAbsBits = baseBits & 0x7fffffffu;
    let expBits = bitcast<u32>(exp);
    let expAbsBits = expBits & 0x7fffffffu;
    if (expAbsBits == 0u || baseBits == 0x3f800000u) { return 1.0; }
    if (expAbsBits > 0x7f800000u) { return bitcast<f32>(0x7fc00000u); }
    if (expAbsBits == 0x7f800000u) {
        if (baseAbsBits == 0x3f800000u) { return 1.0; }
        if (baseAbsBits > 0x7f800000u) { return bitcast<f32>(0x7fc00000u); }
        let grows = (baseAbsBits > 0x3f800000u) == ((expBits & 0x80000000u) == 0u);
        return bitcast<f32>(select(0u, 0x7f800000u, grows));
    }
    if (baseAbsBits > 0x7f800000u) { return bitcast<f32>(0x7fc00000u); }
    let integral = floor(exp) == exp;
    let odd = integral && floor(exp * 0.5) != exp * 0.5;
    let sign = select(0u, 0x80000000u, (baseBits & 0x80000000u) != 0u && odd);
    if (baseAbsBits == 0u || baseAbsBits == 0x7f800000u) {
        let infinite = (baseAbsBits == 0u) == (exp < 0.0);
        return bitcast<f32>(select(0u, 0x7f800000u, infinite) | sign);
    }
    if ((baseBits & 0x80000000u) != 0u && !integral) { return bitcast<f32>(0x7fc00000u); }
    let magnitude = pow(abs(base), exp);
    return bitcast<f32>(bitcast<u32>(magnitude) | sign);
}
fn ontoPow(base: f32, exp: f32) -> f32 {
    // Matches CPU MathNode::Op::Pow, including its exact integrality predicate.
    if (base < 0.0 && floor(exp) != exp) { return 0.0; }
    return ontoRawPow(base, exp);
}
fn ontoPow3(base: vec3<f32>, exp: f32) -> vec3<f32> {
    return vec3<f32>(ontoPow(base.x, exp), ontoPow(base.y, exp), ontoPow(base.z, exp));
}

fn dot2(v: vec2<f32>) -> f32 { return dot(v, v); }


fn sdSphere(p: vec3<f32>, r: f32) -> f32 { return length(p) - r; }

fn sdBox(p: vec3<f32>, b: vec3<f32>) -> f32 {
    let q = abs(p) - b;
    return length(max(q, vec3<f32>(0.0))) + min(max(q.x, max(q.y, q.z)), 0.0);
}

fn sdRoundBox(p: vec3<f32>, b: vec3<f32>, r: f32) -> f32 {
    return sdBox(p, max(b - vec3<f32>(r), vec3<f32>(0.0))) - r;
}

fn sdEllipsoid(p: vec3<f32>, r: vec3<f32>) -> f32 {
    let rr = max(r, vec3<f32>(1e-4));
    let k0 = length(p / rr);
    let k1 = length(p / (rr * rr));
    if (k1 < 1e-8) { return -min(rr.x, min(rr.y, rr.z)); }
    return k0 * (k0 - 1.0) / k1;
}

fn sdCylinder(p: vec3<f32>, r: f32, h: f32) -> f32 {
    let d = abs(vec2<f32>(length(p.xy), p.z)) - vec2<f32>(r, h);
    return min(max(d.x, d.y), 0.0) + length(max(d, vec2<f32>(0.0)));
}

fn sdCone(p: vec3<f32>, r1: f32, h: f32) -> f32 {
    let r2 = 0.0;
    let q = vec2<f32>(length(p.xy), p.z);
    let k1 = vec2<f32>(r2, h);
    let k2 = vec2<f32>(r2 - r1, 2.0 * h);
    let ca = vec2<f32>(q.x - min(q.x, select(r2, r1, q.y < 0.0)), abs(q.y) - h);
    let cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot2(k2), 0.0, 1.0);
    let s = select(1.0, -1.0, cb.x < 0.0 && ca.y < 0.0);
    return s * sqrt(min(dot2(ca), dot2(cb)));
}

fn sdTorus(p: vec3<f32>, R: f32, r: f32) -> f32 {
    let q = vec2<f32>(length(p.xy) - R, p.z);
    return length(q) - r;
}

fn sminK(a: f32, b: f32, k: f32) -> f32 {
    if (k <= 1e-5) { return min(a, b); }
    let h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

struct VolumeGlobals {
    viewProj: mat4x4<f32>,
    invViewProj: mat4x4<f32>,
    eyePos: vec4<f32>,
    viewport: vec4<f32>,
    // xyz = the one admitted direct source position; w=1 iff such a source exists.
    incidentSource: vec4<f32>,
    // xyz = source color / diffuse radiance; w = source intensity / multiplier.
    incidentColor: vec4<f32>,
    // xy = the admitted source's own relative Timeline coordinate/delta.
    // Source t must never borrow a participating medium's instance time.
    sourceTime: vec4<f32>,
    // x = max shadow steps, y = local volumetric visibility enabled,
    // z deliberately unused: it once carried a hardcoded HG phase g (removed in
    // b1d16c41; sdf_wgsl_parameter_refresh_test forbids reading it),
    // w = samples per medium chord (ScreenChannel volumeSamplesPerChord: the
    // quadrature resolution a medium gets along a ray; a runtime value, so
    // changing it never recompiles).
    volumeControl: vec4<f32>,
    // x = final-output budget, y = opt-in (also for subnormal budgets).
    // Positive mode uses the entire accepted lattice when no certified tail
    // bound exists. The opacity heuristic cannot certify late bright emission.
    volumeOutputError: vec4<f32>,
};

struct VolumeInstanceData {
    origin: vec4<f32>,
    halfExtent: vec4<f32>,
    time: vec4<f32>,
    paramOffset: u32,
    // Zero-density proof (Rendering::VolumeZeroProof): absolute offset of its
    // bit words in P (then six support bounds, then four bounds per column),
    // cell dims packed 10 bits per axis, and presence/column-axis bits.
    // All are Kernel-derived proof data, never authored parameter slots.
    zeroProofOffset: u32,
    zeroProofDims: u32,
    zeroProofPresent: u32,
    // Support certificates (Rung 2f-native, VolumeSupportKillerProgram):
    // absolute offset of the header block in P and the enabled candidate
    // count; 0 = none. Kernel-derived from authored density × box.
    supportCertOffset: u32,
    supportCertCount: u32,
    supportClosedOffset: u32,
    supportClosedCount: u32,
};

struct Params { v: array<f32> };

@group(0) @binding(0) var<uniform> u: VolumeGlobals;
@group(0) @binding(1) var<storage, read> P: Params;

@group(0) @binding(2) var<storage, read> instances: array<VolumeInstanceData>;

var<private> g_instIdx: u32;

struct VolumeVSOut {
    @builtin(position) position: vec4<f32>,
    @location(0) @interpolate(flat) instIdx: u32,
};

@vertex
fn vs(@location(0) pos: vec3<f32>, @builtin(instance_index) instIdx: u32) -> VolumeVSOut {
    let inst = instances[instIdx];
    let world = inst.origin.xyz + pos * inst.halfExtent.xyz;
    var out: VolumeVSOut;
    out.position = u.viewProj * vec4<f32>(world, 1.0);
    if (out.position.w > 0.0) {
        out.position.z = min(out.position.z, out.position.w * 0.999999);
    }
    out.instIdx = instIdx;
    return out;
}

fn rayAabbWorld(ro: vec3<f32>, rd: vec3<f32>,
                bmin: vec3<f32>, bmax: vec3<f32>) -> vec2<f32> {
    let safeRd = select(rd, vec3<f32>(1e-8), abs(rd) < vec3<f32>(1e-8));
    let a = (bmin - ro) / safeRd;
    let b = (bmax - ro) / safeRd;
    let lo = min(a, b);
    let hi = max(a, b);
    return vec2<f32>(
        max(max(lo.x, lo.y), lo.z),
        min(min(hi.x, hi.y), hi.z));
}

fn worldAtDepth(pixel: vec2<f32>, depth: f32) -> vec3<f32> {
    let ndc = vec4<f32>(
        (pixel.x / u.viewport.x) * 2.0 - 1.0,
        (1.0 - pixel.y / u.viewport.y) * 2.0 - 1.0,
        depth,
        1.0);
    let h = u.invViewProj * ndc;
    return h.xyz / h.w;
}

// True only where the CPU proved this medium's authored density <= 0 for the
// whole cell holding local point p, so evaluating D there cannot add anything.
// Absent proof, a point outside the box, or a clear bit: evaluate exactly.
// xyz = the proof cell holding local point p; w = 1 iff that cell is proven.
fn volumeZeroProofCell(inst: VolumeInstanceData, p: vec3<f32>) -> vec4<u32> {
    if (inst.zeroProofPresent == 0u) { return vec4<u32>(0u); }
    let dims = vec3<u32>(inst.zeroProofDims & 1023u,
                         (inst.zeroProofDims >> 10u) & 1023u,
                         (inst.zeroProofDims >> 20u) & 1023u);
    let local = (p + inst.halfExtent.xyz) / (2.0 * inst.halfExtent.xyz);
    if (any(local < vec3<f32>(0.0)) || any(local >= vec3<f32>(1.0))) { return vec4<u32>(0u); }
    let cell = min(vec3<u32>(local * vec3<f32>(dims)), dims - vec3<u32>(1u));
    let index = cell.x + dims.x * (cell.y + dims.y * cell.z);
    let word = u32(P.v[inst.zeroProofOffset + index / 24u]);
    return vec4<u32>(cell, (word >> (index % 24u)) & 1u);
}

fn volumeZeroProven(inst: VolumeInstanceData, p: vec3<f32>) -> bool {
    return volumeZeroProofCell(inst, p).w == 1u;
}


// The envelope contains every unproven cell. It may remove whole empty rays,
// prefixes and tails, but never changes authored box events or chord spacing.
// Near-parallel directions retain the old ray interval: rayAabbWorld's safe
// direction substitution cannot certify a tighter support intersection.
fn volumeSupportInterval(inst: VolumeInstanceData, ro: vec3<f32>, rd: vec3<f32>) -> vec2<f32> {
    if (inst.zeroProofPresent == 0u || any(abs(rd) < vec3<f32>(1e-8))) {
        return rayAabbWorld(ro, rd, inst.origin.xyz - inst.halfExtent.xyz,
                            inst.origin.xyz + inst.halfExtent.xyz);
    }
    let dims = vec3<u32>(inst.zeroProofDims & 1023u,
                         (inst.zeroProofDims >> 10u) & 1023u,
                         (inst.zeroProofDims >> 20u) & 1023u);
    let at = inst.zeroProofOffset + (dims.x * dims.y * dims.z + 23u) / 24u;
    let lo = vec3<f32>(P.v[at], P.v[at + 1u], P.v[at + 2u]);
    let hi = vec3<f32>(P.v[at + 3u], P.v[at + 4u], P.v[at + 5u]);
    if (any(lo > hi)) { return vec2<f32>(1.0, -1.0); }
    // Enlarge again for world/local arithmetic, including translated media.
    let pad = (abs(inst.origin.xyz) + abs(ro) + inst.halfExtent.xyz) * 1e-6;
    return rayAabbWorld(ro, rd, inst.origin.xyz + lo - pad, inst.origin.xyz + hi + pad);
}


// Grid walk: for a sample in a proven-empty cell, the ray parameter where the
// ray leaves that cell (every sample before it lies in the same cell, so it
// contributes exactly nothing); -1 when the cell is not proven. The proof was
// made over each cell enlarged by 1%, which absorbs f32 rounding at the exit.
fn volumeZeroCellExit(inst: VolumeInstanceData, p: vec3<f32>,
                      ro: vec3<f32>, rd: vec3<f32>) -> f32 {
    let cell = volumeZeroProofCell(inst, p);
    if (cell.w != 1u) { return -1.0; }
    let dims = vec3<u32>(inst.zeroProofDims & 1023u,
                         (inst.zeroProofDims >> 10u) & 1023u,
                         (inst.zeroProofDims >> 20u) & 1023u);
    let cellSize = 2.0 * inst.halfExtent.xyz / vec3<f32>(dims);
    let cellMin = inst.origin.xyz - inst.halfExtent.xyz + vec3<f32>(cell.xyz) * cellSize;

    return rayAabbWorld(ro, rd, cellMin, cellMin + cellSize).y;
}

fn volumeDensityEval_0(p: vec3<f32>) -> f32 {
    let densityFactor0 = (P.v[instances[g_instIdx].paramOffset + 38]);
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = g_volumeShared_0_G0;
    if (densityFactor1 == 0.0) { return 0.0; }
    return (densityFactor0 * densityFactor1);
}

var<private> g_volumeShared_0_G0: f32;

fn volumeShared_0_G0(p: vec3<f32>) -> f32 {
    let densityFactor0 = clamp((((P.v[instances[g_instIdx].paramOffset + 0]) - select(0.0, ((abs((p).x)) / ((P.v[instances[g_instIdx].paramOffset + 1]))), abs((P.v[instances[g_instIdx].paramOffset + 1])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 2])), ((P.v[instances[g_instIdx].paramOffset + 3])));
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = clamp((((P.v[instances[g_instIdx].paramOffset + 4]) * ((p).y + (P.v[instances[g_instIdx].paramOffset + 5])))), ((P.v[instances[g_instIdx].paramOffset + 6])), ((P.v[instances[g_instIdx].paramOffset + 7])));
    if (densityFactor1 == 0.0) { return 0.0; }
    let densityFactor2 = clamp((((P.v[instances[g_instIdx].paramOffset + 8]) - select(0.0, ((((p).y + (P.v[instances[g_instIdx].paramOffset + 10]))) / ((P.v[instances[g_instIdx].paramOffset + 9]))), abs((P.v[instances[g_instIdx].paramOffset + 9])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 11])), ((P.v[instances[g_instIdx].paramOffset + 12])));
    if (densityFactor2 == 0.0) { return 0.0; }
    let densityFactor3 = clamp((((P.v[instances[g_instIdx].paramOffset + 13]) - select(0.0, ((abs(((((p).z + (P.v[instances[g_instIdx].paramOffset + 15] * sin((P.v[instances[g_instIdx].paramOffset + 16] * (p).x + P.v[instances[g_instIdx].paramOffset + 17])))) + (P.v[instances[g_instIdx].paramOffset + 18] * cos((P.v[instances[g_instIdx].paramOffset + 19] * (p).x + P.v[instances[g_instIdx].paramOffset + 20])))) + (P.v[instances[g_instIdx].paramOffset + 21] * sin((P.v[instances[g_instIdx].paramOffset + 22] * (p).x + P.v[instances[g_instIdx].paramOffset + 23])))))) / ((P.v[instances[g_instIdx].paramOffset + 14]))), abs((P.v[instances[g_instIdx].paramOffset + 14])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 24])), ((P.v[instances[g_instIdx].paramOffset + 25])));
    if (densityFactor3 == 0.0) { return 0.0; }
    let densityFactor4 = ((P.v[instances[g_instIdx].paramOffset + 26]) + ((P.v[instances[g_instIdx].paramOffset + 27] * cos((P.v[instances[g_instIdx].paramOffset + 28] * (p).x + P.v[instances[g_instIdx].paramOffset + 29]))) + (P.v[instances[g_instIdx].paramOffset + 30] * cos((P.v[instances[g_instIdx].paramOffset + 31] * (p).x + P.v[instances[g_instIdx].paramOffset + 32])))));
    if (densityFactor4 == 0.0) { return 0.0; }
    let densityFactor5 = clamp((((P.v[instances[g_instIdx].paramOffset + 33]) + ((P.v[instances[g_instIdx].paramOffset + 34]) * cnoise3(((P.v[instances[g_instIdx].paramOffset + 35]) * (p)))))), ((P.v[instances[g_instIdx].paramOffset + 36])), ((P.v[instances[g_instIdx].paramOffset + 37])));
    return ((densityFactor0 * ((densityFactor1 * densityFactor2) * densityFactor3)) * (densityFactor4 * densityFactor5));
}

fn volumeShared_0_Eval(p: vec3<f32>) {
    g_volumeShared_0_G0 = volumeShared_0_G0(p);
}

fn volumeExtinctionEval_0(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 39]) * g_volumeShared_0_G0);
}

fn volumeScatteringEval_0(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 40]) * g_volumeShared_0_G0);
}

fn volumeChromaEval_0(p: vec3<f32>) -> vec3<f32> {
    return vec3<f32>((P.v[instances[g_instIdx].paramOffset + 41]), (P.v[instances[g_instIdx].paramOffset + 42]), (P.v[instances[g_instIdx].paramOffset + 43]));
}

fn volumePhaseEval_0(p: vec3<f32>, wi: vec3<f32>, wo: vec3<f32>) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 44]) + ((P.v[instances[g_instIdx].paramOffset + 45]) * ((wi.x * wo.x) + ((wi.y * wo.y) + (wi.z * wo.z)))));
}

const HAS_AUTHORED_VOLUME_PHASE_0: bool = true;
const VOLUME_PHASE_READS_WI_0: bool = true;

fn volumeEmissionEval_0(p: vec3<f32>, omega: vec3<f32>) -> vec3<f32> {
    return (g_volumeShared_0_G0 * vec3<f32>(((P.v[instances[g_instIdx].paramOffset + 46]) * ((P.v[instances[g_instIdx].paramOffset + 47]) + P.v[instances[g_instIdx].paramOffset + 51])), ((P.v[instances[g_instIdx].paramOffset + 52]) * ((P.v[instances[g_instIdx].paramOffset + 53]) + P.v[instances[g_instIdx].paramOffset + 57])), ((P.v[instances[g_instIdx].paramOffset + 58]) * ((P.v[instances[g_instIdx].paramOffset + 59]) + P.v[instances[g_instIdx].paramOffset + 63]))));
}

const HAS_AUTHORED_VOLUME_EMISSION_0: bool = true;
const VOLUME_EMISSION_READS_OMEGA_0: bool = false;

fn lightRadianceEval_0(p: vec3<f32>) -> f32 {
    return (P.v[instances[g_instIdx].paramOffset + 64]);
}
const HAS_AUTHORED_LIGHT_RADIANCE_0: bool = true;

fn lightChromaEval_0(p: vec3<f32>) -> vec3<f32> {
    return u.incidentColor.xyz;
}
const HAS_AUTHORED_LIGHT_CHROMA_0: bool = false;

fn lightAngularEval_0(p: vec3<f32>, omega: vec3<f32>) -> f32 {
    return 1.0;
}
const HAS_AUTHORED_LIGHT_ANGULAR_0: bool = false;

fn volumeSdfEval_0(p: vec3<f32>) -> f32 { return 1e9; }
fn volumeSourceVisibility_0(worldP: vec3<f32>, sourceWorld: vec3<f32>) -> f32 { return 1.0; }
const HAS_OCCLUDER_SDF_0: bool = false;

fn volumeDensityEval_1(p: vec3<f32>) -> f32 {
    let densityFactor0 = (P.v[instances[g_instIdx].paramOffset + 38]);
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = g_volumeShared_1_G0;
    if (densityFactor1 == 0.0) { return 0.0; }
    return (densityFactor0 * densityFactor1);
}

var<private> g_volumeShared_1_G0: f32;

fn volumeShared_1_G0(p: vec3<f32>) -> f32 {
    let densityFactor0 = clamp((((P.v[instances[g_instIdx].paramOffset + 0]) - select(0.0, ((abs((p).x)) / ((P.v[instances[g_instIdx].paramOffset + 1]))), abs((P.v[instances[g_instIdx].paramOffset + 1])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 2])), ((P.v[instances[g_instIdx].paramOffset + 3])));
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = clamp((((P.v[instances[g_instIdx].paramOffset + 4]) * ((p).y + (P.v[instances[g_instIdx].paramOffset + 5])))), ((P.v[instances[g_instIdx].paramOffset + 6])), ((P.v[instances[g_instIdx].paramOffset + 7])));
    if (densityFactor1 == 0.0) { return 0.0; }
    let densityFactor2 = clamp((((P.v[instances[g_instIdx].paramOffset + 8]) - select(0.0, ((((p).y + (P.v[instances[g_instIdx].paramOffset + 10]))) / ((P.v[instances[g_instIdx].paramOffset + 9]))), abs((P.v[instances[g_instIdx].paramOffset + 9])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 11])), ((P.v[instances[g_instIdx].paramOffset + 12])));
    if (densityFactor2 == 0.0) { return 0.0; }
    let densityFactor3 = clamp((((P.v[instances[g_instIdx].paramOffset + 13]) - select(0.0, ((abs(((((p).z + (P.v[instances[g_instIdx].paramOffset + 15] * sin((P.v[instances[g_instIdx].paramOffset + 16] * (p).x + P.v[instances[g_instIdx].paramOffset + 17])))) + (P.v[instances[g_instIdx].paramOffset + 18] * cos((P.v[instances[g_instIdx].paramOffset + 19] * (p).x + P.v[instances[g_instIdx].paramOffset + 20])))) + (P.v[instances[g_instIdx].paramOffset + 21] * sin((P.v[instances[g_instIdx].paramOffset + 22] * (p).x + P.v[instances[g_instIdx].paramOffset + 23])))))) / ((P.v[instances[g_instIdx].paramOffset + 14]))), abs((P.v[instances[g_instIdx].paramOffset + 14])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 24])), ((P.v[instances[g_instIdx].paramOffset + 25])));
    if (densityFactor3 == 0.0) { return 0.0; }
    let densityFactor4 = ((P.v[instances[g_instIdx].paramOffset + 26]) + ((P.v[instances[g_instIdx].paramOffset + 27] * cos((P.v[instances[g_instIdx].paramOffset + 28] * (p).x + P.v[instances[g_instIdx].paramOffset + 29]))) + (P.v[instances[g_instIdx].paramOffset + 30] * cos((P.v[instances[g_instIdx].paramOffset + 31] * (p).x + P.v[instances[g_instIdx].paramOffset + 32])))));
    if (densityFactor4 == 0.0) { return 0.0; }
    let densityFactor5 = clamp((((P.v[instances[g_instIdx].paramOffset + 33]) + ((P.v[instances[g_instIdx].paramOffset + 34]) * cnoise3(((P.v[instances[g_instIdx].paramOffset + 35]) * (p)))))), ((P.v[instances[g_instIdx].paramOffset + 36])), ((P.v[instances[g_instIdx].paramOffset + 37])));
    return ((densityFactor0 * ((densityFactor1 * densityFactor2) * densityFactor3)) * (densityFactor4 * densityFactor5));
}

fn volumeShared_1_Eval(p: vec3<f32>) {
    g_volumeShared_1_G0 = volumeShared_1_G0(p);
}

fn volumeExtinctionEval_1(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 39]) * g_volumeShared_1_G0);
}

fn volumeScatteringEval_1(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 40]) * g_volumeShared_1_G0);
}

fn volumeChromaEval_1(p: vec3<f32>) -> vec3<f32> {
    return vec3<f32>((P.v[instances[g_instIdx].paramOffset + 41]), (P.v[instances[g_instIdx].paramOffset + 42]), (P.v[instances[g_instIdx].paramOffset + 43]));
}

fn volumePhaseEval_1(p: vec3<f32>, wi: vec3<f32>, wo: vec3<f32>) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 44]) + ((P.v[instances[g_instIdx].paramOffset + 45]) * ((wi.x * wo.x) + ((wi.y * wo.y) + (wi.z * wo.z)))));
}

const HAS_AUTHORED_VOLUME_PHASE_1: bool = true;
const VOLUME_PHASE_READS_WI_1: bool = true;

fn volumeEmissionEval_1(p: vec3<f32>, omega: vec3<f32>) -> vec3<f32> {
    return (g_volumeShared_1_G0 * vec3<f32>(((P.v[instances[g_instIdx].paramOffset + 46]) * ((P.v[instances[g_instIdx].paramOffset + 47]) + P.v[instances[g_instIdx].paramOffset + 51])), ((P.v[instances[g_instIdx].paramOffset + 52]) * ((P.v[instances[g_instIdx].paramOffset + 53]) + P.v[instances[g_instIdx].paramOffset + 57])), ((P.v[instances[g_instIdx].paramOffset + 58]) * ((P.v[instances[g_instIdx].paramOffset + 59]) + P.v[instances[g_instIdx].paramOffset + 63]))));
}

const HAS_AUTHORED_VOLUME_EMISSION_1: bool = true;
const VOLUME_EMISSION_READS_OMEGA_1: bool = false;

fn lightRadianceEval_1(p: vec3<f32>) -> f32 {
    return (P.v[instances[g_instIdx].paramOffset + 64]);
}
const HAS_AUTHORED_LIGHT_RADIANCE_1: bool = true;

fn lightChromaEval_1(p: vec3<f32>) -> vec3<f32> {
    return u.incidentColor.xyz;
}
const HAS_AUTHORED_LIGHT_CHROMA_1: bool = false;

fn lightAngularEval_1(p: vec3<f32>, omega: vec3<f32>) -> f32 {
    return 1.0;
}
const HAS_AUTHORED_LIGHT_ANGULAR_1: bool = false;

fn volumeSdfEval_1(p: vec3<f32>) -> f32 { return 1e9; }
fn volumeSourceVisibility_1(worldP: vec3<f32>, sourceWorld: vec3<f32>) -> f32 { return 1.0; }
const HAS_OCCLUDER_SDF_1: bool = false;

fn volumeDensityEval_2(p: vec3<f32>) -> f32 {
    let densityFactor0 = (P.v[instances[g_instIdx].paramOffset + 38]);
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = g_volumeShared_2_G0;
    if (densityFactor1 == 0.0) { return 0.0; }
    return (densityFactor0 * densityFactor1);
}

var<private> g_volumeShared_2_G0: f32;

fn volumeShared_2_G0(p: vec3<f32>) -> f32 {
    let densityFactor0 = clamp((((P.v[instances[g_instIdx].paramOffset + 0]) - select(0.0, ((abs((p).x)) / ((P.v[instances[g_instIdx].paramOffset + 1]))), abs((P.v[instances[g_instIdx].paramOffset + 1])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 2])), ((P.v[instances[g_instIdx].paramOffset + 3])));
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = clamp((((P.v[instances[g_instIdx].paramOffset + 4]) * ((p).y + (P.v[instances[g_instIdx].paramOffset + 5])))), ((P.v[instances[g_instIdx].paramOffset + 6])), ((P.v[instances[g_instIdx].paramOffset + 7])));
    if (densityFactor1 == 0.0) { return 0.0; }
    let densityFactor2 = clamp((((P.v[instances[g_instIdx].paramOffset + 8]) - select(0.0, ((((p).y + (P.v[instances[g_instIdx].paramOffset + 10]))) / ((P.v[instances[g_instIdx].paramOffset + 9]))), abs((P.v[instances[g_instIdx].paramOffset + 9])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 11])), ((P.v[instances[g_instIdx].paramOffset + 12])));
    if (densityFactor2 == 0.0) { return 0.0; }
    let densityFactor3 = clamp((((P.v[instances[g_instIdx].paramOffset + 13]) - select(0.0, ((abs(((((p).z + (P.v[instances[g_instIdx].paramOffset + 15] * sin((P.v[instances[g_instIdx].paramOffset + 16] * (p).x + P.v[instances[g_instIdx].paramOffset + 17])))) + (P.v[instances[g_instIdx].paramOffset + 18] * cos((P.v[instances[g_instIdx].paramOffset + 19] * (p).x + P.v[instances[g_instIdx].paramOffset + 20])))) + (P.v[instances[g_instIdx].paramOffset + 21] * sin((P.v[instances[g_instIdx].paramOffset + 22] * (p).x + P.v[instances[g_instIdx].paramOffset + 23])))))) / ((P.v[instances[g_instIdx].paramOffset + 14]))), abs((P.v[instances[g_instIdx].paramOffset + 14])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 24])), ((P.v[instances[g_instIdx].paramOffset + 25])));
    if (densityFactor3 == 0.0) { return 0.0; }
    let densityFactor4 = ((P.v[instances[g_instIdx].paramOffset + 26]) + ((P.v[instances[g_instIdx].paramOffset + 27] * cos((P.v[instances[g_instIdx].paramOffset + 28] * (p).x + P.v[instances[g_instIdx].paramOffset + 29]))) + (P.v[instances[g_instIdx].paramOffset + 30] * cos((P.v[instances[g_instIdx].paramOffset + 31] * (p).x + P.v[instances[g_instIdx].paramOffset + 32])))));
    if (densityFactor4 == 0.0) { return 0.0; }
    let densityFactor5 = clamp((((P.v[instances[g_instIdx].paramOffset + 33]) + ((P.v[instances[g_instIdx].paramOffset + 34]) * cnoise3(((P.v[instances[g_instIdx].paramOffset + 35]) * (p)))))), ((P.v[instances[g_instIdx].paramOffset + 36])), ((P.v[instances[g_instIdx].paramOffset + 37])));
    return ((densityFactor0 * ((densityFactor1 * densityFactor2) * densityFactor3)) * (densityFactor4 * densityFactor5));
}

fn volumeShared_2_Eval(p: vec3<f32>) {
    g_volumeShared_2_G0 = volumeShared_2_G0(p);
}

fn volumeExtinctionEval_2(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 39]) * g_volumeShared_2_G0);
}

fn volumeScatteringEval_2(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 40]) * g_volumeShared_2_G0);
}

fn volumeChromaEval_2(p: vec3<f32>) -> vec3<f32> {
    return vec3<f32>((P.v[instances[g_instIdx].paramOffset + 41]), (P.v[instances[g_instIdx].paramOffset + 42]), (P.v[instances[g_instIdx].paramOffset + 43]));
}

fn volumePhaseEval_2(p: vec3<f32>, wi: vec3<f32>, wo: vec3<f32>) -> f32 {
    return (P.v[instances[g_instIdx].paramOffset + 44]);
}

const HAS_AUTHORED_VOLUME_PHASE_2: bool = true;
const VOLUME_PHASE_READS_WI_2: bool = false;

fn volumeEmissionEval_2(p: vec3<f32>, omega: vec3<f32>) -> vec3<f32> {
    return (g_volumeShared_2_G0 * vec3<f32>(((P.v[instances[g_instIdx].paramOffset + 45]) * ((P.v[instances[g_instIdx].paramOffset + 46]) + P.v[instances[g_instIdx].paramOffset + 50])), ((P.v[instances[g_instIdx].paramOffset + 51]) * ((P.v[instances[g_instIdx].paramOffset + 52]) + P.v[instances[g_instIdx].paramOffset + 56])), ((P.v[instances[g_instIdx].paramOffset + 57]) * ((P.v[instances[g_instIdx].paramOffset + 58]) + P.v[instances[g_instIdx].paramOffset + 62]))));
}

const HAS_AUTHORED_VOLUME_EMISSION_2: bool = true;
const VOLUME_EMISSION_READS_OMEGA_2: bool = false;

fn lightRadianceEval_2(p: vec3<f32>) -> f32 {
    return (P.v[instances[g_instIdx].paramOffset + 63]);
}
const HAS_AUTHORED_LIGHT_RADIANCE_2: bool = true;

fn lightChromaEval_2(p: vec3<f32>) -> vec3<f32> {
    return u.incidentColor.xyz;
}
const HAS_AUTHORED_LIGHT_CHROMA_2: bool = false;

fn lightAngularEval_2(p: vec3<f32>, omega: vec3<f32>) -> f32 {
    return 1.0;
}
const HAS_AUTHORED_LIGHT_ANGULAR_2: bool = false;

fn volumeSdfEval_2(p: vec3<f32>) -> f32 { return 1e9; }
fn volumeSourceVisibility_2(worldP: vec3<f32>, sourceWorld: vec3<f32>) -> f32 { return 1.0; }
const HAS_OCCLUDER_SDF_2: bool = false;

fn volumeDensityEval_3(p: vec3<f32>) -> f32 {
    let densityFactor0 = (P.v[instances[g_instIdx].paramOffset + 38]);
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = g_volumeShared_3_G0;
    if (densityFactor1 == 0.0) { return 0.0; }
    return (densityFactor0 * densityFactor1);
}

var<private> g_volumeShared_3_G0: f32;

fn volumeShared_3_G0(p: vec3<f32>) -> f32 {
    let densityFactor0 = clamp((((P.v[instances[g_instIdx].paramOffset + 0]) - select(0.0, ((abs((p).x)) / ((P.v[instances[g_instIdx].paramOffset + 1]))), abs((P.v[instances[g_instIdx].paramOffset + 1])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 2])), ((P.v[instances[g_instIdx].paramOffset + 3])));
    if (densityFactor0 == 0.0) { return 0.0; }
    let densityFactor1 = clamp((((P.v[instances[g_instIdx].paramOffset + 4]) * ((p).y + (P.v[instances[g_instIdx].paramOffset + 5])))), ((P.v[instances[g_instIdx].paramOffset + 6])), ((P.v[instances[g_instIdx].paramOffset + 7])));
    if (densityFactor1 == 0.0) { return 0.0; }
    let densityFactor2 = clamp((((P.v[instances[g_instIdx].paramOffset + 8]) - select(0.0, ((((p).y + (P.v[instances[g_instIdx].paramOffset + 10]))) / ((P.v[instances[g_instIdx].paramOffset + 9]))), abs((P.v[instances[g_instIdx].paramOffset + 9])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 11])), ((P.v[instances[g_instIdx].paramOffset + 12])));
    if (densityFactor2 == 0.0) { return 0.0; }
    let densityFactor3 = clamp((((P.v[instances[g_instIdx].paramOffset + 13]) - select(0.0, ((abs(((((p).z + (P.v[instances[g_instIdx].paramOffset + 15] * sin((P.v[instances[g_instIdx].paramOffset + 16] * (p).x + P.v[instances[g_instIdx].paramOffset + 17])))) + (P.v[instances[g_instIdx].paramOffset + 18] * cos((P.v[instances[g_instIdx].paramOffset + 19] * (p).x + P.v[instances[g_instIdx].paramOffset + 20])))) + (P.v[instances[g_instIdx].paramOffset + 21] * sin((P.v[instances[g_instIdx].paramOffset + 22] * (p).x + P.v[instances[g_instIdx].paramOffset + 23])))))) / ((P.v[instances[g_instIdx].paramOffset + 14]))), abs((P.v[instances[g_instIdx].paramOffset + 14])) >= 1e-06))), ((P.v[instances[g_instIdx].paramOffset + 24])), ((P.v[instances[g_instIdx].paramOffset + 25])));
    if (densityFactor3 == 0.0) { return 0.0; }
    let densityFactor4 = ((P.v[instances[g_instIdx].paramOffset + 26]) + ((P.v[instances[g_instIdx].paramOffset + 27] * cos((P.v[instances[g_instIdx].paramOffset + 28] * (p).x + P.v[instances[g_instIdx].paramOffset + 29]))) + (P.v[instances[g_instIdx].paramOffset + 30] * cos((P.v[instances[g_instIdx].paramOffset + 31] * (p).x + P.v[instances[g_instIdx].paramOffset + 32])))));
    if (densityFactor4 == 0.0) { return 0.0; }
    let densityFactor5 = clamp((((P.v[instances[g_instIdx].paramOffset + 33]) + ((P.v[instances[g_instIdx].paramOffset + 34]) * cnoise3(((P.v[instances[g_instIdx].paramOffset + 35]) * (p)))))), ((P.v[instances[g_instIdx].paramOffset + 36])), ((P.v[instances[g_instIdx].paramOffset + 37])));
    return ((densityFactor0 * ((densityFactor1 * densityFactor2) * densityFactor3)) * (densityFactor4 * densityFactor5));
}

fn volumeShared_3_Eval(p: vec3<f32>) {
    g_volumeShared_3_G0 = volumeShared_3_G0(p);
}

fn volumeExtinctionEval_3(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 39]) * g_volumeShared_3_G0);
}

fn volumeScatteringEval_3(p: vec3<f32>, compatibilityDensity: f32) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 40]) * g_volumeShared_3_G0);
}

fn volumeChromaEval_3(p: vec3<f32>) -> vec3<f32> {
    return vec3<f32>((P.v[instances[g_instIdx].paramOffset + 41]), (P.v[instances[g_instIdx].paramOffset + 42]), (P.v[instances[g_instIdx].paramOffset + 43]));
}

fn volumePhaseEval_3(p: vec3<f32>, wi: vec3<f32>, wo: vec3<f32>) -> f32 {
    return ((P.v[instances[g_instIdx].paramOffset + 44]) + ((P.v[instances[g_instIdx].paramOffset + 45]) * ((wi.x * wo.x) + ((wi.y * wo.y) + (wi.z * wo.z)))));
}

const HAS_AUTHORED_VOLUME_PHASE_3: bool = true;
const VOLUME_PHASE_READS_WI_3: bool = true;

fn volumeEmissionEval_3(p: vec3<f32>, omega: vec3<f32>) -> vec3<f32> {
    return (g_volumeShared_3_G0 * vec3<f32>(((P.v[instances[g_instIdx].paramOffset + 46]) * ((P.v[instances[g_instIdx].paramOffset + 47]) + P.v[instances[g_instIdx].paramOffset + 51])), ((P.v[instances[g_instIdx].paramOffset + 52]) * ((P.v[instances[g_instIdx].paramOffset + 53]) + P.v[instances[g_instIdx].paramOffset + 57])), ((P.v[instances[g_instIdx].paramOffset + 58]) * ((P.v[instances[g_instIdx].paramOffset + 59]) + P.v[instances[g_instIdx].paramOffset + 63]))));
}

const HAS_AUTHORED_VOLUME_EMISSION_3: bool = true;
const VOLUME_EMISSION_READS_OMEGA_3: bool = false;

fn lightRadianceEval_3(p: vec3<f32>) -> f32 {
    return (P.v[instances[g_instIdx].paramOffset + 64]);
}
const HAS_AUTHORED_LIGHT_RADIANCE_3: bool = true;

fn lightChromaEval_3(p: vec3<f32>) -> vec3<f32> {
    return u.incidentColor.xyz;
}
const HAS_AUTHORED_LIGHT_CHROMA_3: bool = false;

fn lightAngularEval_3(p: vec3<f32>, omega: vec3<f32>) -> f32 {
    return 1.0;
}
const HAS_AUTHORED_LIGHT_ANGULAR_3: bool = false;

fn volumeSdfEval_3(p: vec3<f32>) -> f32 { return 1e9; }
fn volumeSourceVisibility_3(worldP: vec3<f32>, sourceWorld: vec3<f32>) -> f32 { return 1.0; }
const HAS_OCCLUDER_SDF_3: bool = false;


struct CapturedRay { ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32> };
@group(0) @binding(3) var<storage,read> capturedRays:array<CapturedRay>;
@group(0) @binding(4) var<storage,read_write> capturedOutput:array<vec4<f32>>;

@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid:vec3<u32>) {
 let idx=gid.x;if(idx>=u32(u.viewport.z)){return;}
 let ray=capturedRays[idx%arrayLength(&capturedRays)];
 let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;
 let t0=ray.ro_clipA.w;let t1=ray.rd_clipB.w;g_instIdx=0u;
    var transmittance = 1.0;
    var integratedRadiance = vec3<f32>(0.0);
    let raySupport0 = select(vec2<f32>(-3.0e38, 3.0e38), volumeSupportInterval(instances[1u], ro, rd), instances[1u].zeroProofPresent != 0u && all(abs(rd) >= vec3<f32>(1e-8)));
    let raySupport1 = select(vec2<f32>(-3.0e38, 3.0e38), volumeSupportInterval(instances[2u], ro, rd), instances[2u].zeroProofPresent != 0u && all(abs(rd) >= vec3<f32>(1e-8)));
    let raySupport2 = select(vec2<f32>(-3.0e38, 3.0e38), volumeSupportInterval(instances[3u], ro, rd), instances[3u].zeroProofPresent != 0u && all(abs(rd) >= vec3<f32>(1e-8)));
    let raySupport3 = select(vec2<f32>(-3.0e38, 3.0e38), volumeSupportInterval(instances[4u], ro, rd), instances[4u].zeroProofPresent != 0u && all(abs(rd) >= vec3<f32>(1e-8)));
    var mediumEvents: array<f32, 8>;
    let samplesPerChord = max(i32(u.volumeControl.w), 1);
    var mediumChord: array<f32, 4>;
    {
        let eventInst0 = instances[1u];
        let eventBounds0 = rayAabbWorld(
            ro, rd,
            eventInst0.origin.xyz - eventInst0.halfExtent.xyz,
            eventInst0.origin.xyz + eventInst0.halfExtent.xyz);
        var eventEnter0 = max(eventBounds0.x, t0);
        var eventExit0 = min(eventBounds0.y, t1);
        if (eventExit0 <= eventEnter0) {
            eventEnter0 = t1;
            eventExit0 = t1;
        }
        mediumChord[0] = eventExit0 - eventEnter0;
        mediumEvents[0u] = eventEnter0;
        mediumEvents[1u] = eventExit0;
    }
    {
        let eventInst1 = instances[2u];
        let eventBounds1 = rayAabbWorld(
            ro, rd,
            eventInst1.origin.xyz - eventInst1.halfExtent.xyz,
            eventInst1.origin.xyz + eventInst1.halfExtent.xyz);
        var eventEnter1 = max(eventBounds1.x, t0);
        var eventExit1 = min(eventBounds1.y, t1);
        if (eventExit1 <= eventEnter1) {
            eventEnter1 = t1;
            eventExit1 = t1;
        }
        mediumChord[1] = eventExit1 - eventEnter1;
        mediumEvents[2u] = eventEnter1;
        mediumEvents[3u] = eventExit1;
    }
    {
        let eventInst2 = instances[3u];
        let eventBounds2 = rayAabbWorld(
            ro, rd,
            eventInst2.origin.xyz - eventInst2.halfExtent.xyz,
            eventInst2.origin.xyz + eventInst2.halfExtent.xyz);
        var eventEnter2 = max(eventBounds2.x, t0);
        var eventExit2 = min(eventBounds2.y, t1);
        if (eventExit2 <= eventEnter2) {
            eventEnter2 = t1;
            eventExit2 = t1;
        }
        mediumChord[2] = eventExit2 - eventEnter2;
        mediumEvents[4u] = eventEnter2;
        mediumEvents[5u] = eventExit2;
    }
    {
        let eventInst3 = instances[4u];
        let eventBounds3 = rayAabbWorld(
            ro, rd,
            eventInst3.origin.xyz - eventInst3.halfExtent.xyz,
            eventInst3.origin.xyz + eventInst3.halfExtent.xyz);
        var eventEnter3 = max(eventBounds3.x, t0);
        var eventExit3 = min(eventBounds3.y, t1);
        if (eventExit3 <= eventEnter3) {
            eventEnter3 = t1;
            eventExit3 = t1;
        }
        mediumChord[3] = eventExit3 - eventEnter3;
        mediumEvents[6u] = eventEnter3;
        mediumEvents[7u] = eventExit3;
    }
    for (var eventIdx = 1u; eventIdx < 8u; eventIdx = eventIdx + 1u) {
        let key = mediumEvents[eventIdx];
        var insertIdx = eventIdx;
        loop {
            if (insertIdx == 0u) { break; }
            let previousIdx = insertIdx - 1u;
            if (mediumEvents[previousIdx] <= key) { break; }
            mediumEvents[insertIdx] = mediumEvents[previousIdx];
            insertIdx = previousIdx;
        }
        mediumEvents[insertIdx] = key;
    }
    for (var segmentIdx = 0u; segmentIdx + 1u < 8u; segmentIdx = segmentIdx + 1u) {
        let segmentStart = max(mediumEvents[segmentIdx], t0);
        let segmentEnd = min(mediumEvents[segmentIdx + 1u], t1);
        if (segmentEnd <= segmentStart + 1e-6) { continue; }
        let segmentMidT = 0.5 * (segmentStart + segmentEnd);
        let segmentMidP = ro + rd * segmentMidT;
        var segmentOccupied = false;
        var segmentSpacing = 3.0e38;
        {
            let segmentInst0 = instances[1u];
            let segmentMin0 = segmentInst0.origin.xyz - segmentInst0.halfExtent.xyz;
            let segmentMax0 = segmentInst0.origin.xyz + segmentInst0.halfExtent.xyz;
            if (all(segmentMidP >= segmentMin0) && all(segmentMidP <= segmentMax0)) {
                segmentOccupied = true;
                segmentSpacing = min(segmentSpacing, mediumChord[0] / f32(samplesPerChord));
            }
        }
        {
            let segmentInst1 = instances[2u];
            let segmentMin1 = segmentInst1.origin.xyz - segmentInst1.halfExtent.xyz;
            let segmentMax1 = segmentInst1.origin.xyz + segmentInst1.halfExtent.xyz;
            if (all(segmentMidP >= segmentMin1) && all(segmentMidP <= segmentMax1)) {
                segmentOccupied = true;
                segmentSpacing = min(segmentSpacing, mediumChord[1] / f32(samplesPerChord));
            }
        }
        {
            let segmentInst2 = instances[3u];
            let segmentMin2 = segmentInst2.origin.xyz - segmentInst2.halfExtent.xyz;
            let segmentMax2 = segmentInst2.origin.xyz + segmentInst2.halfExtent.xyz;
            if (all(segmentMidP >= segmentMin2) && all(segmentMidP <= segmentMax2)) {
                segmentOccupied = true;
                segmentSpacing = min(segmentSpacing, mediumChord[2] / f32(samplesPerChord));
            }
        }
        {
            let segmentInst3 = instances[4u];
            let segmentMin3 = segmentInst3.origin.xyz - segmentInst3.halfExtent.xyz;
            let segmentMax3 = segmentInst3.origin.xyz + segmentInst3.halfExtent.xyz;
            if (all(segmentMidP >= segmentMin3) && all(segmentMidP <= segmentMax3)) {
                segmentOccupied = true;
                segmentSpacing = min(segmentSpacing, mediumChord[3] / f32(samplesPerChord));
            }
        }

        if (!segmentOccupied) { continue; }

        // Unified quadrature (Zach, 2026-10-09: "a mathematical unification of
        // the drawing functions wherever it overlaps"). The media form one
        // field with one transmittance/radiance integral; box entry/exit are
        // its genuine discontinuities, so segments still break there. Each
        // segment is sampled at the finest standalone resolution of the media
        // present in it (chord / samplesPerChord), so no medium is ever sampled
        // more coarsely than it would be alone -- but an overlap no longer
        // multiplies the count, and a sliver no longer receives a full chord's
        // worth of samples. Witness: webgpu_volume_unified_quadrature_test.
        let segmentSpan = segmentEnd - segmentStart;
        let segmentSamples = clamp(i32(ceil(segmentSpan / segmentSpacing)), 1, samplesPerChord);
        let stepLength = segmentSpan / f32(segmentSamples);
        if (stepLength <= 0.0) { continue; }


        for (var step = 0; step < segmentSamples; step = step + 1) {
            let sampleT = segmentStart + (f32(step) + 0.5) * stepLength;

            let worldP = ro + rd * sampleT;

            // Consume each medium's proof and contribution together. Unknown
            // leaves cannot justify a collective jump; known leaves contribute
            // only their exit. Original medium order, sample coordinates and
            // transport arithmetic are unchanged (Astra counsel, 2026-10-11).
            var walkAllProven = true;
            var walkHeld = false;
            var walkExit = segmentEnd;
            var totalExtinction = 0.0;
            var totalSource = vec3<f32>(0.0);
            {
                let mediumInst0 = instances[1u];
                if (all(worldP >= mediumInst0.origin.xyz - mediumInst0.halfExtent.xyz) && all(worldP <= mediumInst0.origin.xyz + mediumInst0.halfExtent.xyz)) {
                    walkHeld = true;
                    if (raySupport0.y < raySupport0.x || sampleT > raySupport0.y) {
                        // No future support in this authored segment.
                    } else if (sampleT < raySupport0.x) {
                        walkExit = min(walkExit, raySupport0.x);
                    } else {
                        let cellExit0 = volumeZeroCellExit(mediumInst0, worldP - mediumInst0.origin.xyz, ro, rd);
                        if (cellExit0 < 0.0) {
                            walkAllProven = false;
                            g_instIdx = 1u;
                            let p0 = worldP - mediumInst0.origin.xyz;
                            var density0 = 0.0;
                            volumeShared_0_Eval(p0);
                            density0 = max(volumeDensityEval_0(p0), 0.0);
                            if (density0 > 0.0) {
                                let extinction0 = max(volumeExtinctionEval_0(p0, density0), 1e-6);
                                totalExtinction += extinction0;
                                let wiDelta = worldP - u.incidentSource.xyz;
                                let woDelta = ro - worldP;
                                let wiLen = length(wiDelta);
                                let woLen = length(woDelta);
                                let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, 1e-8),
                                                u.incidentSource.w > 0.5 && wiLen > 1e-8);
                                let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8),
                                                woLen > 1e-8);
                                var incidentLi0 = vec3<f32>(1.0);
                                if (u.incidentSource.w > 0.5) {
                                    let sourceDelta = worldP - u.incidentSource.xyz;
                                    let sourceDist = length(sourceDelta);
                                    let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);
                                    var radialRad = 1.0;
                                    if (HAS_AUTHORED_LIGHT_RADIANCE_0) {
                                        radialRad = max(lightRadianceEval_0(sourceDelta), 0.0);
                                    }
                                    if (radialRad == 0.0) {
                                        incidentLi0 = vec3<f32>(0.0);
                                    } else {
                                        var chroma = vec3<f32>(1.0);
                                        if (HAS_AUTHORED_LIGHT_CHROMA_0) {
                                            chroma = max(lightChromaEval_0(sourceDelta), vec3<f32>(0.0));
                                        }
                                        var angular = 1.0;
                                        if (HAS_AUTHORED_LIGHT_ANGULAR_0) {
                                            angular = max(lightAngularEval_0(sourceDelta, lightDir), 0.0);
                                        }
                                        let vis = volumeSourceVisibility_0(worldP, u.incidentSource.xyz);
                                        incidentLi0 = chroma * (radialRad * angular * vis);
                                    }
                                }
                                var emitted0 = vec3<f32>(0.0);
                                if (HAS_AUTHORED_VOLUME_EMISSION_0) {
                                    if (!VOLUME_EMISSION_READS_OMEGA_0 || woLen > 1e-8) {
                                        emitted0 = max(volumeEmissionEval_0(p0, wo), vec3<f32>(0.0));
                                    }
                                }
                                if (any(incidentLi0 != vec3<f32>(0.0))) {
                                    let scattering0 = max(volumeScatteringEval_0(p0, density0), 0.0);
                                    let mediumChroma0 = volumeChromaEval_0(p0);
                                    var phase0 = 1.0;
                                    if (HAS_AUTHORED_VOLUME_PHASE_0) {
                                        phase0 = 0.0;
                                        if ((!VOLUME_PHASE_READS_WI_0 || (u.incidentSource.w > 0.5 && wiLen > 1e-8)) && woLen > 1e-8) {
                                            phase0 = max(volumePhaseEval_0(p0, wi, wo), 0.0);
                                        }
                                    }
                                    totalSource += mediumChroma0 * incidentLi0 * scattering0 * phase0 + emitted0;
                                } else {
                                    totalSource += emitted0;
                                }
                            }
                        } else { walkExit = min(walkExit, cellExit0); }
                    }
                }
            }
            {
                let mediumInst1 = instances[2u];
                if (all(worldP >= mediumInst1.origin.xyz - mediumInst1.halfExtent.xyz) && all(worldP <= mediumInst1.origin.xyz + mediumInst1.halfExtent.xyz)) {
                    walkHeld = true;
                    if (raySupport1.y < raySupport1.x || sampleT > raySupport1.y) {
                        // No future support in this authored segment.
                    } else if (sampleT < raySupport1.x) {
                        walkExit = min(walkExit, raySupport1.x);
                    } else {
                        let cellExit1 = volumeZeroCellExit(mediumInst1, worldP - mediumInst1.origin.xyz, ro, rd);
                        if (cellExit1 < 0.0) {
                            walkAllProven = false;
                            g_instIdx = 2u;
                            let p1 = worldP - mediumInst1.origin.xyz;
                            var density1 = 0.0;
                            volumeShared_1_Eval(p1);
                            density1 = max(volumeDensityEval_1(p1), 0.0);
                            if (density1 > 0.0) {
                                let extinction1 = max(volumeExtinctionEval_1(p1, density1), 1e-6);
                                totalExtinction += extinction1;
                                let wiDelta = worldP - u.incidentSource.xyz;
                                let woDelta = ro - worldP;
                                let wiLen = length(wiDelta);
                                let woLen = length(woDelta);
                                let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, 1e-8),
                                                u.incidentSource.w > 0.5 && wiLen > 1e-8);
                                let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8),
                                                woLen > 1e-8);
                                var incidentLi1 = vec3<f32>(1.0);
                                if (u.incidentSource.w > 0.5) {
                                    let sourceDelta = worldP - u.incidentSource.xyz;
                                    let sourceDist = length(sourceDelta);
                                    let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);
                                    var radialRad = 1.0;
                                    if (HAS_AUTHORED_LIGHT_RADIANCE_1) {
                                        radialRad = max(lightRadianceEval_1(sourceDelta), 0.0);
                                    }
                                    if (radialRad == 0.0) {
                                        incidentLi1 = vec3<f32>(0.0);
                                    } else {
                                        var chroma = vec3<f32>(1.0);
                                        if (HAS_AUTHORED_LIGHT_CHROMA_1) {
                                            chroma = max(lightChromaEval_1(sourceDelta), vec3<f32>(0.0));
                                        }
                                        var angular = 1.0;
                                        if (HAS_AUTHORED_LIGHT_ANGULAR_1) {
                                            angular = max(lightAngularEval_1(sourceDelta, lightDir), 0.0);
                                        }
                                        let vis = volumeSourceVisibility_1(worldP, u.incidentSource.xyz);
                                        incidentLi1 = chroma * (radialRad * angular * vis);
                                    }
                                }
                                var emitted1 = vec3<f32>(0.0);
                                if (HAS_AUTHORED_VOLUME_EMISSION_1) {
                                    if (!VOLUME_EMISSION_READS_OMEGA_1 || woLen > 1e-8) {
                                        emitted1 = max(volumeEmissionEval_1(p1, wo), vec3<f32>(0.0));
                                    }
                                }
                                if (any(incidentLi1 != vec3<f32>(0.0))) {
                                    let scattering1 = max(volumeScatteringEval_1(p1, density1), 0.0);
                                    let mediumChroma1 = volumeChromaEval_1(p1);
                                    var phase1 = 1.0;
                                    if (HAS_AUTHORED_VOLUME_PHASE_1) {
                                        phase1 = 0.0;
                                        if ((!VOLUME_PHASE_READS_WI_1 || (u.incidentSource.w > 0.5 && wiLen > 1e-8)) && woLen > 1e-8) {
                                            phase1 = max(volumePhaseEval_1(p1, wi, wo), 0.0);
                                        }
                                    }
                                    totalSource += mediumChroma1 * incidentLi1 * scattering1 * phase1 + emitted1;
                                } else {
                                    totalSource += emitted1;
                                }
                            }
                        } else { walkExit = min(walkExit, cellExit1); }
                    }
                }
            }
            {
                let mediumInst2 = instances[3u];
                if (all(worldP >= mediumInst2.origin.xyz - mediumInst2.halfExtent.xyz) && all(worldP <= mediumInst2.origin.xyz + mediumInst2.halfExtent.xyz)) {
                    walkHeld = true;
                    if (raySupport2.y < raySupport2.x || sampleT > raySupport2.y) {
                        // No future support in this authored segment.
                    } else if (sampleT < raySupport2.x) {
                        walkExit = min(walkExit, raySupport2.x);
                    } else {
                        let cellExit2 = volumeZeroCellExit(mediumInst2, worldP - mediumInst2.origin.xyz, ro, rd);
                        if (cellExit2 < 0.0) {
                            walkAllProven = false;
                            g_instIdx = 3u;
                            let p2 = worldP - mediumInst2.origin.xyz;
                            var density2 = 0.0;
                            volumeShared_2_Eval(p2);
                            density2 = max(volumeDensityEval_2(p2), 0.0);
                            if (density2 > 0.0) {
                                let extinction2 = max(volumeExtinctionEval_2(p2, density2), 1e-6);
                                totalExtinction += extinction2;
                                let wiDelta = worldP - u.incidentSource.xyz;
                                let woDelta = ro - worldP;
                                let wiLen = length(wiDelta);
                                let woLen = length(woDelta);
                                let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, 1e-8),
                                                u.incidentSource.w > 0.5 && wiLen > 1e-8);
                                let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8),
                                                woLen > 1e-8);
                                var incidentLi2 = vec3<f32>(1.0);
                                if (u.incidentSource.w > 0.5) {
                                    let sourceDelta = worldP - u.incidentSource.xyz;
                                    let sourceDist = length(sourceDelta);
                                    let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);
                                    var radialRad = 1.0;
                                    if (HAS_AUTHORED_LIGHT_RADIANCE_2) {
                                        radialRad = max(lightRadianceEval_2(sourceDelta), 0.0);
                                    }
                                    if (radialRad == 0.0) {
                                        incidentLi2 = vec3<f32>(0.0);
                                    } else {
                                        var chroma = vec3<f32>(1.0);
                                        if (HAS_AUTHORED_LIGHT_CHROMA_2) {
                                            chroma = max(lightChromaEval_2(sourceDelta), vec3<f32>(0.0));
                                        }
                                        var angular = 1.0;
                                        if (HAS_AUTHORED_LIGHT_ANGULAR_2) {
                                            angular = max(lightAngularEval_2(sourceDelta, lightDir), 0.0);
                                        }
                                        let vis = volumeSourceVisibility_2(worldP, u.incidentSource.xyz);
                                        incidentLi2 = chroma * (radialRad * angular * vis);
                                    }
                                }
                                var emitted2 = vec3<f32>(0.0);
                                if (HAS_AUTHORED_VOLUME_EMISSION_2) {
                                    if (!VOLUME_EMISSION_READS_OMEGA_2 || woLen > 1e-8) {
                                        emitted2 = max(volumeEmissionEval_2(p2, wo), vec3<f32>(0.0));
                                    }
                                }
                                if (any(incidentLi2 != vec3<f32>(0.0))) {
                                    let scattering2 = max(volumeScatteringEval_2(p2, density2), 0.0);
                                    let mediumChroma2 = volumeChromaEval_2(p2);
                                    var phase2 = 1.0;
                                    if (HAS_AUTHORED_VOLUME_PHASE_2) {
                                        phase2 = 0.0;
                                        if ((!VOLUME_PHASE_READS_WI_2 || (u.incidentSource.w > 0.5 && wiLen > 1e-8)) && woLen > 1e-8) {
                                            phase2 = max(volumePhaseEval_2(p2, wi, wo), 0.0);
                                        }
                                    }
                                    totalSource += mediumChroma2 * incidentLi2 * scattering2 * phase2 + emitted2;
                                } else {
                                    totalSource += emitted2;
                                }
                            }
                        } else { walkExit = min(walkExit, cellExit2); }
                    }
                }
            }
            {
                let mediumInst3 = instances[4u];
                if (all(worldP >= mediumInst3.origin.xyz - mediumInst3.halfExtent.xyz) && all(worldP <= mediumInst3.origin.xyz + mediumInst3.halfExtent.xyz)) {
                    walkHeld = true;
                    if (raySupport3.y < raySupport3.x || sampleT > raySupport3.y) {
                        // No future support in this authored segment.
                    } else if (sampleT < raySupport3.x) {
                        walkExit = min(walkExit, raySupport3.x);
                    } else {
                        let cellExit3 = volumeZeroCellExit(mediumInst3, worldP - mediumInst3.origin.xyz, ro, rd);
                        if (cellExit3 < 0.0) {
                            walkAllProven = false;
                            g_instIdx = 4u;
                            let p3 = worldP - mediumInst3.origin.xyz;
                            var density3 = 0.0;
                            volumeShared_3_Eval(p3);
                            density3 = max(volumeDensityEval_3(p3), 0.0);
                            if (density3 > 0.0) {
                                let extinction3 = max(volumeExtinctionEval_3(p3, density3), 1e-6);
                                totalExtinction += extinction3;
                                let wiDelta = worldP - u.incidentSource.xyz;
                                let woDelta = ro - worldP;
                                let wiLen = length(wiDelta);
                                let woLen = length(woDelta);
                                let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, 1e-8),
                                                u.incidentSource.w > 0.5 && wiLen > 1e-8);
                                let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8),
                                                woLen > 1e-8);
                                var incidentLi3 = vec3<f32>(1.0);
                                if (u.incidentSource.w > 0.5) {
                                    let sourceDelta = worldP - u.incidentSource.xyz;
                                    let sourceDist = length(sourceDelta);
                                    let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);
                                    var radialRad = 1.0;
                                    if (HAS_AUTHORED_LIGHT_RADIANCE_3) {
                                        radialRad = max(lightRadianceEval_3(sourceDelta), 0.0);
                                    }
                                    if (radialRad == 0.0) {
                                        incidentLi3 = vec3<f32>(0.0);
                                    } else {
                                        var chroma = vec3<f32>(1.0);
                                        if (HAS_AUTHORED_LIGHT_CHROMA_3) {
                                            chroma = max(lightChromaEval_3(sourceDelta), vec3<f32>(0.0));
                                        }
                                        var angular = 1.0;
                                        if (HAS_AUTHORED_LIGHT_ANGULAR_3) {
                                            angular = max(lightAngularEval_3(sourceDelta, lightDir), 0.0);
                                        }
                                        let vis = volumeSourceVisibility_3(worldP, u.incidentSource.xyz);
                                        incidentLi3 = chroma * (radialRad * angular * vis);
                                    }
                                }
                                var emitted3 = vec3<f32>(0.0);
                                if (HAS_AUTHORED_VOLUME_EMISSION_3) {
                                    if (!VOLUME_EMISSION_READS_OMEGA_3 || woLen > 1e-8) {
                                        emitted3 = max(volumeEmissionEval_3(p3, wo), vec3<f32>(0.0));
                                    }
                                }
                                if (any(incidentLi3 != vec3<f32>(0.0))) {
                                    let scattering3 = max(volumeScatteringEval_3(p3, density3), 0.0);
                                    let mediumChroma3 = volumeChromaEval_3(p3);
                                    var phase3 = 1.0;
                                    if (HAS_AUTHORED_VOLUME_PHASE_3) {
                                        phase3 = 0.0;
                                        if ((!VOLUME_PHASE_READS_WI_3 || (u.incidentSource.w > 0.5 && wiLen > 1e-8)) && woLen > 1e-8) {
                                            phase3 = max(volumePhaseEval_3(p3, wi, wo), 0.0);
                                        }
                                    }
                                    totalSource += mediumChroma3 * incidentLi3 * scattering3 * phase3 + emitted3;
                                } else {
                                    totalSource += emitted3;
                                }
                            }
                        } else { walkExit = min(walkExit, cellExit3); }
                    }
                }
            }

            if (!walkHeld) { walkExit = sampleT; }
            if (walkAllProven) {
                step = max(step, i32(ceil((walkExit - segmentStart) / stepLength - 0.5)) - 1);
                continue;
            }

        let oldT = transmittance;
        if (totalExtinction > 0.0) {
            transmittance *= exp(-totalExtinction * stepLength);
            // Shared participating-medium integral. For one active medium this
            // algebra reduces to the V4 scatter + E_v attenuation formulas.
            let intervalGain = (oldT - transmittance) / totalExtinction;
            integratedRadiance += totalSource * intervalGain;
        } else {
            // Continuous sigma_t -> 0 limit. Usually unreachable under the
            // current 1e-6 per-active-medium clamp, but it states the transport
            // law rather than relying on division by an implementation epsilon.
            integratedRadiance += oldT * totalSource * stepLength;
        }

            if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }
        }
        if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }
    }


 capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);
}
