#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"

#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace sdfwgsl {
namespace {

// ---------------------------------------------------------------------------
// The primitive library. Every function here is a LINE-BY-LINE transcription of
// its counterpart in Sdf.cpp — same formula, same epsilons, same degenerate-case
// branches. That is not stylistic: the raymarched surface and the tessellated
// mesh must be the same surface, or a field would visibly change shape when the
// backend changed. If you edit a formula in Sdf.cpp, edit it here too.
//
// WGSL note: select(falseValue, trueValue, condition) — the operand order is the
// reverse of a C ternary, which is an easy way to invert a sign by accident.
// ---------------------------------------------------------------------------
const char* kPrimitives = R"WGSL(
struct SdfInstanceData {
    model: mat4x4<f32>,
    invModel: mat4x4<f32>,
    baseColor: vec4<f32>,
    shading: vec4<f32>,
    extents: vec4<f32>,
    misc: vec4<f32>,
    paramOffset: u32,
    // Min/max heightfield grid (rendering-optimization Phase C): this
    // instance's cells live at heightCells[heightGridOffset ..
    // +heightGridDimX*heightGridDimZ), row-major z-major, covering
    // [-extents.x,extents.x] x [-extents.z,extents.z] in field-local space.
    // heightGridDimX/Z == 0 means "no grid" -- take the unmodified marcher.
    heightGridOffset: u32,
    heightGridDimX: u32,
    heightGridDimZ: u32,
    // Fixed-depth conservative positive-proof bit grid. A zero bit means
    // "no GPU skip proof; exact authored marching owns this cell."
    rangeProofWordOffset: u32,
    rangeProofWordCount: u32,
    rangeTraversalEnabled: u32,
    rangeProofDepth: u32,
};
@group(1) @binding(0) var<storage, read> instances: array<SdfInstanceData>;
// (hMin, hMax) per cell, conservative -- see geom::computeHeightGrid.
@group(1) @binding(1) var<storage, read> heightCells: array<vec2<f32>>;
// Positive-outside proof bitmap, packed 32 regular depth-N cells per u32.
@group(1) @binding(2) var<storage, read> rangeProofWords: array<u32>;
var<private> g_instIdx: u32;

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

// Capped cone along Z: base radius r1 at z=-h, apex at z=+h.
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
struct PerlinJet {
    value: f32,
    grad: vec3<f32>,
};
struct StepSample {
    raw: f32,
    gradLen: f32,
};

fn cnoise3_grad(P: vec3<f32>) -> PerlinJet {
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

    var g000 = vec3<f32>(gx0.x, gy0.x, gz0.x);
    var g100 = vec3<f32>(gx0.y, gy0.y, gz0.y);
    var g010 = vec3<f32>(gx0.z, gy0.z, gz0.z);
    var g110 = vec3<f32>(gx0.w, gy0.w, gz0.w);
    var g001 = vec3<f32>(gx1.x, gy1.x, gz1.x);
    var g101 = vec3<f32>(gx1.y, gy1.y, gz1.y);
    var g011 = vec3<f32>(gx1.z, gy1.z, gz1.z);
    var g111 = vec3<f32>(gx1.w, gy1.w, gz1.w);

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
    let dfade_xyz = 30.0 * Pf0 * Pf0 * (Pf0 - vec3<f32>(1.0)) * (Pf0 - vec3<f32>(1.0));

    let u = fade_xyz.x;
    let v = fade_xyz.y;
    let w = fade_xyz.z;

    let n_z0 = mix(n000, n001, w);
    let n_z1 = mix(n100, n101, w);
    let n_z2 = mix(n010, n011, w);
    let n_z3 = mix(n110, n111, w);

    let n_yz0 = mix(n_z0, n_z2, v);
    let n_yz1 = mix(n_z1, n_z3, v);

    let n_xyz = mix(n_yz0, n_yz1, u);
    let val = 2.2 * n_xyz;

    let dx_fade = dfade_xyz.x * (n_yz1 - n_yz0);

    let n_x_y0 = mix(n_z0, n_z1, u);
    let n_x_y1 = mix(n_z2, n_z3, u);
    let dy_fade = dfade_xyz.y * (n_x_y1 - n_x_y0);

    let n_xy_z0 = mix(mix(n000, n100, u), mix(n010, n110, u), v);
    let n_xy_z1 = mix(mix(n001, n101, u), mix(n011, n111, u), v);
    let dz_fade = dfade_xyz.z * (n_xy_z1 - n_xy_z0);

    let g_z0 = mix(g000, g001, w);
    let g_z1 = mix(g100, g101, w);
    let g_z2 = mix(g010, g011, w);
    let g_z3 = mix(g110, g111, w);

    let g_yz0 = mix(g_z0, g_z2, v);
    let g_yz1 = mix(g_z1, g_z3, v);

    let g_interp = mix(g_yz0, g_yz1, u);

    let grad = 2.2 * (g_interp + vec3<f32>(dx_fade, dy_fade, dz_fade));
    return PerlinJet(val, grad);
}
)WGSL";

// A WGSL float literal for a C++ constant, so a threshold shared with the CPU
// evaluator cannot drift by being typed twice. %.9g round-trips an f32 exactly.
std::string wgslLiteral(double v) {
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%.9g", v);
    std::string s = buf;
    // WGSL needs a decimal point or exponent to read a literal as f32.
    if (s.find_first_of(".eE") == std::string::npos) {
        s += ".0";
    }
    return s;
}

// Emitter state: appends generated statements and collects the parameter values
// those statements will read back out of the buffer.
struct Emit {
    std::string        body;
    std::vector<float> params;
    int                next = 0; // next `let dN` temporary
    bool               sawExpr = false; // an implicit leaf appeared -> not a distance
    bool               bindTime = false; // expression-context capability, not authored state
    bool               bindOmega = false; // Rung-6 source angular radiance context
    bool               readOmega = false; // structural witness for source singularity handling
    bool               bindEmissionOmega = false; // V4 E_v owns a distinct omega context
    bool               readEmissionOmega = false;
    bool               bindPhaseDirections = false; // V3 Phi admits wi/wo, never source omega
    bool               readWi = false;
    bool               readWo = false;
    // Ambient temporal coordinate for the expression currently being emitted.
    // Rungs 3-6 use the historical global uniform; Rung 7 temporarily points
    // this at one source record while lowering that source's rho/chi/alpha.
    std::string        timeExpression = "u.radianceTime.x";

    // The refusal (see Program::ok). Once set it is never overwritten: the
    // FIRST thing the compiler could not honour is the one worth reporting;
    // everything after it is downstream noise.
    bool        refused = false;
    std::string refusal;

    void refuse(const std::string& why) {
        if (!refused) { refused = true; refusal = why; }
    }

    // Record a number and return the WGSL expression that reads it.
    std::string param(float v) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "P.v[instances[g_instIdx].paramOffset + %zu]", params.size());
        params.push_back(v);
        return buf;
    }
    std::string param3(const glm::vec3& v) {
        // Three scalars rather than a packed vec3: a std430 array<f32> has no
        // alignment surprises, and nothing here is bandwidth-bound.
        std::string x = param(v.x), y = param(v.y), z = param(v.z);
        return "vec3<f32>(" + x + ", " + y + ", " + z + ")";
    }
    std::string fresh() { return "d" + std::to_string(next++); }
    void line(const std::string& s) { body += "    " + s + "\n"; }
};

// RPN -> straight-line WGSL. WGSL has no recursion and no dynamic stack, so the
// stack is unwound at COMPILE time: each token pops expression strings and pushes
// the combined string. Bounded because the RPN is finite.
std::string emitRpn(const std::vector<geom::SdfToken>& rpn, Emit& e,
                    const std::string& pt) {
    std::vector<std::string> st;
    auto pop = [&]() -> std::string {
        if (st.empty()) return "0.0";
        std::string t = st.back(); st.pop_back(); return t;
    };
    for (const geom::SdfToken& t : rpn) {
        using K = geom::SdfToken::Kind;
        switch (t.kind) {
            case K::Num: st.push_back(e.param(t.num)); break;
            case K::X:   st.push_back(pt + ".x"); break;
            case K::Y:   st.push_back(pt + ".y"); break;
            case K::Z:   st.push_back(pt + ".z"); break;
            case K::Neg: { auto a = pop(); st.push_back("(-(" + a + "))"); break; }
            case K::Add: { auto b = pop(); auto a = pop(); st.push_back("((" + a + ") + (" + b + "))"); break; }
            case K::Sub: { auto b = pop(); auto a = pop(); st.push_back("((" + a + ") - (" + b + "))"); break; }
            case K::Mul: { auto b = pop(); auto a = pop(); st.push_back("((" + a + ") * (" + b + "))"); break; }
            case K::Div: { auto b = pop(); auto a = pop(); st.push_back("((" + a + ") / (" + b + "))"); break; }
            case K::Pow: { auto b = pop(); auto a = pop(); st.push_back("pow((" + a + "), (" + b + "))"); break; }
            case K::Sin:  { auto a = pop(); st.push_back("sin("  + a + ")"); break; }
            case K::Cos:  { auto a = pop(); st.push_back("cos("  + a + ")"); break; }
            case K::Tan:  { auto a = pop(); st.push_back("tan("  + a + ")"); break; }
            case K::Sqrt: { auto a = pop(); st.push_back("sqrt(max(" + a + ", 0.0))"); break; }
            case K::Abs:  { auto a = pop(); st.push_back("abs("  + a + ")"); break; }
            case K::Exp:  { auto a = pop(); st.push_back("exp("  + a + ")"); break; }
            case K::Log:  { auto a = pop(); st.push_back("log(max(" + a + ", 1e-8))"); break; }
        }
    }
    return st.empty() ? "0.0" : st.back();
}

// The ambient point's components, as WGSL. A variable a field expression can
// name but this compiler cannot bind is a REFUSAL, not "0.0": substituting zero
// silently reinterprets f(t) as f(0), which is a different field.
std::string pointComponent(const std::string& var, Emit& e, const std::string& pt) {
    if (var == "x" || var == "y" || var == "z") return "(" + pt + ")." + var;
    if (var == OntoMath::kTimeVar) {
        if (e.bindTime) return e.timeExpression;
        e.refuse("a field expression names temporal variable 't', but this shader "
                 "expression context does not bind the temporal coordinate");
        return "0.0";
    }
    if (var == OntoMath::kOmegaXVar ||
        var == OntoMath::kOmegaYVar ||
        var == OntoMath::kOmegaZVar) {
        if (e.bindEmissionOmega) {
            e.readEmissionOmega = true;
            if (var == OntoMath::kOmegaXVar) return "omega.x";
            if (var == OntoMath::kOmegaYVar) return "omega.y";
            return "omega.z";
        }
        e.readOmega = true;
        if (e.bindOmega) {
            if (var == OntoMath::kOmegaXVar) return "omega.x";
            if (var == OntoMath::kOmegaYVar) return "omega.y";
            return "omega.z";
        }
        e.refuse("a field expression names angular coordinate '" + var +
                 "', but this shader expression context does not bind omega");
        return "0.0";
    }
    if (var == OntoMath::kWiXVar ||
        var == OntoMath::kWiYVar ||
        var == OntoMath::kWiZVar) {
        e.readWi = true;
        if (e.bindPhaseDirections) {
            if (var == OntoMath::kWiXVar) return "wi.x";
            if (var == OntoMath::kWiYVar) return "wi.y";
            return "wi.z";
        }
        e.refuse("a field expression names phase incoming direction '" + var +
                 "', but this shader expression context does not bind wi");
        return "0.0";
    }
    if (var == OntoMath::kWoXVar ||
        var == OntoMath::kWoYVar ||
        var == OntoMath::kWoZVar) {
        e.readWo = true;
        if (e.bindPhaseDirections) {
            if (var == OntoMath::kWoXVar) return "wo.x";
            if (var == OntoMath::kWoYVar) return "wo.y";
            return "wo.z";
        }
        e.refuse("a field expression names phase outgoing direction '" + var +
                 "', but this shader expression context does not bind wo");
        return "0.0";
    }
    e.refuse("a field expression names the variable '" + var +
             "', which has no binding in this shader expression context");
    return "0.0";
}

std::string emitScalarForm(const OntoMath::ScalarForm& sf, Emit& e, const std::string& pt) {
    if (sf.terms.empty()) return "0.0";
    std::string res = "";
    for (size_t i = 0; i < sf.terms.size(); ++i) {
        const auto& term = sf.terms[i];
        std::string termStr = e.param(term.coefficient);
        for (const auto& [var, exp] : term.factors) {
            std::string wgslVar = pointComponent(var, e, pt);
            if (exp == 1.0) termStr += " * " + wgslVar;
            else termStr += " * pow(" + wgslVar + ", " + e.param(exp) + ")";
        }
        for (const auto& tr : term.trans) {
            std::string wgslVar = pointComponent(tr.variable, e, pt);
            std::string inner = "(" + e.param(tr.scale) + " * " + wgslVar + " + " + e.param(tr.shift) + ")";
            if (tr.kind == OntoMath::TransFactor::Kind::Sin) termStr += " * sin(" + inner + ")";
            else if (tr.kind == OntoMath::TransFactor::Kind::Cos) termStr += " * cos(" + inner + ")";
            else if (tr.kind == OntoMath::TransFactor::Kind::Exp) termStr += " * exp(" + inner + ")";
            else if (tr.kind == OntoMath::TransFactor::Kind::Ln) termStr += " * log(max(" + inner + ", 1e-8))";
        }
        if (i > 0) res += " + ";
        res += "(" + termStr + ")";
    }
    return res;
}

// THE ONE RULE OF THIS FUNCTION: never invent a number.
//
// It used to answer the literal string "0.0" for seven operations and for every
// variable it could not bind. Zero is not a neutral placeholder — for a signed
// distance it means "on the surface", for a density it means "empty here" —
// and downstream nothing can tell an invented zero from an authored one. Where
// this compiler cannot honour an authored expression it calls e.refuse(), and
// the whole pipeline is declined with a reason. The string it returns after a
// refusal exists only to keep the recursion well-formed; it is never used.
std::string emitMathNode(const OntoMath::MathNode& node, Emit& e, const std::string& pt) {
    using Op = OntoMath::MathNode::Op;

    // Arity is checked by the type pass before we get here, but a malformed
    // tree must not be a buffer overrun in the meantime.
    const auto arg = [&](std::size_t i) -> std::string {
        if (i >= node.children.size() || !node.children[i]) {
            e.refuse(std::string(OntoMath::mathOpName(node.op)) + " is missing argument " +
                     std::to_string(static_cast<int>(i)));
            return "0.0";
        }
        return emitMathNode(*node.children[i], e, pt);
    };

    switch (node.op) {
        case Op::ScalarLeaf:
            return emitScalarForm(node.scalarForm, e, pt);
        case Op::ValueLeaf:
            if (node.variableName == OntoMath::kAmbientPointVar) return "(" + pt + ")";
            return pointComponent(node.variableName, e, pt);
        case Op::VectorConstruct: {
            if (node.children.size() != 3) {
                e.refuse("VectorConstruct needs exactly 3 components, got " +
                         std::to_string(node.children.size()));
                return "vec3<f32>(0.0)";
            }
            return "vec3<f32>(" + arg(0) + ", " + arg(1) + ", " + arg(2) + ")";
        }
        case Op::Component: {
            if (node.stringArg != "x" && node.stringArg != "y" && node.stringArg != "z") {
                e.refuse("Component names axis '" + node.stringArg + "'; expected x, y or z");
                return "0.0";
            }
            return "(" + arg(0) + ")." + node.stringArg;
        }
        case Op::Add:      return "(" + arg(0) + " + " + arg(1) + ")";
        case Op::Sub:      return "(" + arg(0) + " - " + arg(1) + ")";
        // Scale covers scalar*vector, vector*scalar AND scalar*scalar; WGSL's
        // `*` is the same three, which is why the CPU evaluator was widened to
        // match instead of the emitter being narrowed.
        case Op::Scale:    return "(" + arg(0) + " * " + arg(1) + ")";
        case Op::Dot:      return "dot(" + arg(0) + ", " + arg(1) + ")";
        case Op::Cross:    return "cross(" + arg(0) + ", " + arg(1) + ")";
        case Op::Hadamard: return "(" + arg(0) + " * " + arg(1) + ")";
        case Op::Normalize:return "normalize(" + arg(0) + ")";
        case Op::Length:   return "length(" + arg(0) + ")";
        case Op::Map: {
            // WGSL's builtins are lower-case. Emitting the author-facing name
            // verbatim produced `Round(...)`, which is not a WGSL function at
            // all — the shader failed to compile rather than computing anything.
            if (node.stringArg == "Round") return "round(" + arg(0) + ")";
            if (node.stringArg == "Floor") return "floor(" + arg(0) + ")";
            e.refuse("Map names function '" + node.stringArg +
                     "', which this build does not define (Round, Floor)");
            return "vec3<f32>(0.0)";
        }
        case Op::Stochastic: {
            // Used to emit "1.0" while the CPU drew from a distribution: the
            // two paths did not merely differ, they were unrelated. A stateless
            // fragment shader has no reproducible draw to offer, so it declines.
            e.refuse("Stochastic cannot be compiled to WGSL: a fragment shader has "
                     "no reproducible random draw, and the CPU evaluator's draw "
                     "cannot be reproduced here");
            return "0.0";
        }
        case Op::Project: {
            // proj_b(a) = b * (a.b / b.b), guarded on the UNSQUARED length of b
            // with the same threshold the CPU uses (OntoMath::
            // kDegenerateVectorLength). Without the guard, b = 0 gives 0/0 = NaN
            // and the NaN propagates through the entire field; with the CPU's
            // old squared test the two paths disagreed for any |b| < 1e-3.
            // Below the threshold the answer is exact, not a fallback:
            // span{0} = {0}, and the projection onto the zero subspace is zero.
            const std::string va = arg(0);
            const std::string vb = arg(1);
            return "select(vec3<f32>(0.0), ((" + vb + ") * (dot(" + va + ", " + vb +
                   ") / dot(" + vb + ", " + vb + "))), length(" + vb + ") >= " +
                   wgslLiteral(OntoMath::kDegenerateVectorLength) + ")";
        }
        case Op::Distance: return "distance(" + arg(0) + ", " + arg(1) + ")";
        case Op::Div: {
            const std::string b = arg(1);
            return "select(0.0, ((" + arg(0) + ") / (" + b + ")), abs(" + b + ") >= " + wgslLiteral(OntoMath::kDegenerateDivisor) + ")";
        }
        case Op::Pow:   return "pow((" + arg(0) + "), (" + arg(1) + "))";
        case Op::Abs:   return "abs(" + arg(0) + ")";
        case Op::Clamp: return "clamp((" + arg(0) + "), (" + arg(1) + "), (" + arg(2) + "))";
        case Op::Sqrt:  return "sqrt(max((" + arg(0) + "), 0.0))";
        case Op::Tan:   return "tan(" + arg(0) + ")";
        case Op::Noise: return "cnoise3(" + arg(0) + ")";

        // --- CSG over signed distance --------------------------------------
        // Character-identical to what emitNode() below emits for
        // geom::SdfOp::Union / Intersect / Subtract, and to what evalSdf
        // computes on the CPU. One vocabulary, written in two syntaxes.
        case Op::Union:        return "min(" + arg(0) + ", " + arg(1) + ")";
        case Op::Intersection: return "max(" + arg(0) + ", " + arg(1) + ")";
        case Op::Difference:   return "max(" + arg(0) + ", -(" + arg(1) + "))";

        // --- Sampling a field expression elsewhere -------------------------
        // SDF(f, q) is f with the ambient point substituted by q. The emitter
        // is already parameterised on the point expression, so this is exactly
        // a re-emission of the subtree at a different point — the same thing
        // MathNode::evaluate does by rebinding p/x/y/z.
        case Op::SDF: {
            if (node.children.size() != 2 || !node.children[0] || !node.children[1]) {
                e.refuse("SDF needs exactly 2 arguments (field expression, point)");
                return "0.0";
            }
            const std::string q = "(" + emitMathNode(*node.children[1], e, pt) + ")";
            return "(" + emitMathNode(*node.children[0], e, q) + ")";
        }
        case Op::Gradient: {
            // Central differences with OntoMath::kGradientEpsilon — the same
            // step the marcher's sdfGrad/sdfNormal and geom::sdfNormal use, and
            // the same one MathNode::evaluate uses, so a gradient does not
            // change value with the backend.
            //
            // Cost: the field subtree is re-emitted SIX times. That is inherent
            // to a language with no closures; a Gradient of a Gradient is 36
            // copies. Authors should not nest them deeply.
            if (node.children.size() != 2 || !node.children[0] || !node.children[1]) {
                e.refuse("Gradient needs exactly 2 arguments (field expression, point)");
                return "vec3<f32>(0.0)";
            }
            const std::string q = "(" + emitMathNode(*node.children[1], e, pt) + ")";
            const std::string eps = wgslLiteral(OntoMath::kGradientEpsilon);
            const auto at = [&](int axis, int sign) {
                std::string d[3] = {"0.0", "0.0", "0.0"};
                d[axis] = (sign < 0 ? "-" : "") + eps;
                return "(" + q + " + vec3<f32>(" + d[0] + ", " + d[1] + ", " + d[2] + "))";
            };
            std::string comps[3];
            for (int axis = 0; axis < 3; ++axis) {
                const std::string hi = emitMathNode(*node.children[0], e, at(axis, +1));
                const std::string lo = emitMathNode(*node.children[0], e, at(axis, -1));
                comps[axis] = "((" + hi + ") - (" + lo + "))";
            }
            return "(vec3<f32>(" + comps[0] + ", " + comps[1] + ", " + comps[2] +
                   ") / (2.0 * " + eps + "))";
        }

        // --- Declared, not implemented, on EITHER path ----------------------
        case Op::Raycast:
            e.refuse("Raycast has no implementation on either path: it needs a "
                     "marching budget and a hit epsilon that nothing in this tree "
                     "authors. It evaluates to nullopt on the CPU, so the shader "
                     "declines rather than answering a number the CPU would not.");
            return "0.0";
        case Op::LineIntegral:
            e.refuse("LineIntegral has no implementation on either path: it needs a "
                     "curve parameterization and a quadrature rule that nothing in "
                     "this tree authors. It evaluates to nullopt on the CPU, so the "
                     "shader declines rather than answering a number the CPU would not.");
            return "0.0";
        case Op::Unsupported:
            e.refuse("the field expression contains an operation this build does not "
                     "know (preserved verbatim from the save); it cannot be compiled");
            return "0.0";
    }
    e.refuse("the field expression contains an operation outside the enumeration");
    return "0.0";
}

void emitPiecewise(const OntoMath::Piecewise& pw, Emit& e, const std::string& pt, const std::string& outType, std::string& outBody) {
    // Piecewise interval bounds live on the same authored coordinate vocabulary
    // as the value expression itself. In particular, rho(p,t) may cut pieces
    // along t. An unbound coordinate refuses through pointComponent(); it is
    // never silently reinterpreted as the scalar zero.
    std::string inVar = pointComponent(pw.inputVariable, e, pt);
    
    for (size_t i = 0; i < pw.pieces.size(); ++i) {
        const auto& piece = pw.pieces[i];
        if (!piece.mathNode) continue;
        
        std::string cond = "";
        if (piece.hasLo) cond += inVar + " >= " + e.param(piece.lo);
        if (piece.hasHi) {
            if (!cond.empty()) cond += " && ";
            cond += inVar + " <= " + e.param(piece.hi);
        }
        
        std::string val = emitMathNode(*piece.mathNode, e, pt);
        
        if (cond.empty()) {
            outBody += "    return " + val + ";\n";
            return;
        } else {
            outBody += "    if (" + cond + ") { return " + val + "; }\n";
        }
    }
    
    if (outType == "vec3<f32>") {
        outBody += "    return vec3<f32>(0.0);\n";
    } else {
        outBody += "    return 0.0;\n";
    }
}

enum class JetKind { Scalar, Vector };
struct JetExpr {
    JetKind kind = JetKind::Scalar;
    std::string value;
    std::string grad;
    std::string gradX;
    std::string gradY;
    std::string gradZ;
};

bool astContainsNoise(const OntoMath::MathNode& node) {
    if (node.op == OntoMath::MathNode::Op::Noise) return true;
    for (const auto& c : node.children) {
        if (c && astContainsNoise(*c)) return true;
    }
    return false;
}

bool isDifferentiableAst(const OntoMath::MathNode& node) {
    switch (node.op) {
        case OntoMath::MathNode::Op::ScalarLeaf: {
            for (const auto& term : node.scalarForm.terms) {
                if (!term.factors.empty() || !term.trans.empty()) {
                    return false;
                }
            }
            return true;
        }
        case OntoMath::MathNode::Op::ValueLeaf:
            return (node.variableName == OntoMath::kAmbientPointVar ||
                    node.variableName == "x" ||
                    node.variableName == "y" ||
                    node.variableName == "z");
        case OntoMath::MathNode::Op::VectorConstruct: {
            if (node.children.size() != 3) return false;
            for (const auto& c : node.children) {
                if (!c || !isDifferentiableAst(*c)) return false;
            }
            return true;
        }
        case OntoMath::MathNode::Op::Component: {
            if (node.children.empty() || !node.children[0]) return false;
            if (node.stringArg != "x" && node.stringArg != "y" && node.stringArg != "z") return false;
            return isDifferentiableAst(*node.children[0]);
        }
        case OntoMath::MathNode::Op::Add:
        case OntoMath::MathNode::Op::Sub:
        case OntoMath::MathNode::Op::Scale: {
            if (node.children.size() != 2 || !node.children[0] || !node.children[1]) return false;
            return isDifferentiableAst(*node.children[0]) && isDifferentiableAst(*node.children[1]);
        }
        case OntoMath::MathNode::Op::Noise: {
            if (node.children.empty() || !node.children[0]) return false;
            return isDifferentiableAst(*node.children[0]);
        }
        case OntoMath::MathNode::Op::Dot: {
            if (node.children.size() != 2 || !node.children[0] || !node.children[1]) return false;
            return isDifferentiableAst(*node.children[0]) && isDifferentiableAst(*node.children[1]);
        }
        case OntoMath::MathNode::Op::Length: {
            if (node.children.size() != 1 || !node.children[0]) return false;
            return isDifferentiableAst(*node.children[0]);
        }
        default:
            return false;
    }
}

JetExpr emitMathNodeGrad(const OntoMath::MathNode& node, Emit& e,
                         const std::string& pt, std::string& body, int& nextVar) {
    using Op = OntoMath::MathNode::Op;
    switch (node.op) {
        case Op::ScalarLeaf: {
            if (node.scalarForm.terms.empty()) {
                return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
            }
            std::string v = "";
            for (size_t ti = 0; ti < node.scalarForm.terms.size(); ++ti) {
                const auto& term = node.scalarForm.terms[ti];
                if (!term.factors.empty() || !term.trans.empty()) {
                    e.refuse("ScalarLeaf with non-constant factors reached analytic gradient emitter");
                    return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
                }
                std::string tStr = e.param(static_cast<float>(term.coefficient));
                if (ti == 0) v = tStr;
                else v += " + " + tStr;
            }
            return JetExpr{ JetKind::Scalar, "(" + v + ")", "vec3<f32>(0.0)", "", "", "" };
        }
        case Op::ValueLeaf: {
            if (node.variableName == OntoMath::kAmbientPointVar) {
                return JetExpr{ JetKind::Vector, "(" + pt + ")", "",
                                "vec3<f32>(1.0, 0.0, 0.0)",
                                "vec3<f32>(0.0, 1.0, 0.0)",
                                "vec3<f32>(0.0, 0.0, 1.0)" };
            }
            if (node.variableName == "x") {
                return JetExpr{ JetKind::Scalar, "(" + pt + ").x", "vec3<f32>(1.0, 0.0, 0.0)", "", "", "" };
            }
            if (node.variableName == "y") {
                return JetExpr{ JetKind::Scalar, "(" + pt + ").y", "vec3<f32>(0.0, 1.0, 0.0)", "", "", "" };
            }
            if (node.variableName == "z") {
                return JetExpr{ JetKind::Scalar, "(" + pt + ").z", "vec3<f32>(0.0, 0.0, 1.0)", "", "", "" };
            }
            e.refuse("unsupported variable in analytic gradient emitter: " + node.variableName);
            return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
        }
        case Op::VectorConstruct: {
            if (node.children.size() != 3 || !node.children[0] || !node.children[1] || !node.children[2]) {
                e.refuse("VectorConstruct arity invalid in analytic gradient emitter");
                return JetExpr{ JetKind::Vector, "vec3<f32>(0.0)", "", "vec3<f32>(0.0)", "vec3<f32>(0.0)", "vec3<f32>(0.0)" };
            }
            JetExpr c0 = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            JetExpr c1 = emitMathNodeGrad(*node.children[1], e, pt, body, nextVar);
            JetExpr c2 = emitMathNodeGrad(*node.children[2], e, pt, body, nextVar);
            std::string v = "vec3<f32>(" + c0.value + ", " + c1.value + ", " + c2.value + ")";
            return JetExpr{ JetKind::Vector, v, "", c0.grad, c1.grad, c2.grad };
        }
        case Op::Component: {
            if (node.children.empty() || !node.children[0]) {
                e.refuse("Component missing argument in analytic gradient emitter");
                return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
            }
            JetExpr c = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            if (node.stringArg == "x") return JetExpr{ JetKind::Scalar, "(" + c.value + ").x", c.gradX, "", "", "" };
            if (node.stringArg == "y") return JetExpr{ JetKind::Scalar, "(" + c.value + ").y", c.gradY, "", "", "" };
            if (node.stringArg == "z") return JetExpr{ JetKind::Scalar, "(" + c.value + ").z", c.gradZ, "", "", "" };
            e.refuse("Component invalid axis in analytic gradient emitter: " + node.stringArg);
            return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
        }
        case Op::Add: {
            JetExpr a = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            JetExpr b = emitMathNodeGrad(*node.children[1], e, pt, body, nextVar);
            if (a.kind == JetKind::Scalar && b.kind == JetKind::Scalar) {
                return JetExpr{ JetKind::Scalar, "(" + a.value + " + " + b.value + ")",
                                "(" + a.grad + " + " + b.grad + ")", "", "", "" };
            } else if (a.kind == JetKind::Vector && b.kind == JetKind::Vector) {
                return JetExpr{ JetKind::Vector, "(" + a.value + " + " + b.value + ")", "",
                                "(" + a.gradX + " + " + b.gradX + ")",
                                "(" + a.gradY + " + " + b.gradY + ")",
                                "(" + a.gradZ + " + " + b.gradZ + ")" };
            }
            e.refuse("type mismatch in Add for analytic gradient emitter");
            return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
        }
        case Op::Sub: {
            JetExpr a = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            JetExpr b = emitMathNodeGrad(*node.children[1], e, pt, body, nextVar);
            if (a.kind == JetKind::Scalar && b.kind == JetKind::Scalar) {
                return JetExpr{ JetKind::Scalar, "(" + a.value + " - " + b.value + ")",
                                "(" + a.grad + " - " + b.grad + ")", "", "", "" };
            } else if (a.kind == JetKind::Vector && b.kind == JetKind::Vector) {
                return JetExpr{ JetKind::Vector, "(" + a.value + " - " + b.value + ")", "",
                                "(" + a.gradX + " - " + b.gradX + ")",
                                "(" + a.gradY + " - " + b.gradY + ")",
                                "(" + a.gradZ + " - " + b.gradZ + ")" };
            }
            e.refuse("type mismatch in Sub for analytic gradient emitter");
            return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
        }
        case Op::Scale: {
            JetExpr a = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            JetExpr b = emitMathNodeGrad(*node.children[1], e, pt, body, nextVar);
            if (a.kind == JetKind::Scalar && b.kind == JetKind::Scalar) {
                std::string v = "(" + a.value + " * " + b.value + ")";
                std::string g = "((" + a.value + ") * (" + b.grad + ") + (" + b.value + ") * (" + a.grad + "))";
                return JetExpr{ JetKind::Scalar, v, g, "", "", "" };
            } else if (a.kind == JetKind::Scalar && b.kind == JetKind::Vector) {
                std::string v = "((" + a.value + ") * (" + b.value + "))";
                std::string gx = "((" + a.value + ") * (" + b.gradX + ") + (" + b.value + ").x * (" + a.grad + "))";
                std::string gy = "((" + a.value + ") * (" + b.gradY + ") + (" + b.value + ").y * (" + a.grad + "))";
                std::string gz = "((" + a.value + ") * (" + b.gradZ + ") + (" + b.value + ").z * (" + a.grad + "))";
                return JetExpr{ JetKind::Vector, v, "", gx, gy, gz };
            } else if (a.kind == JetKind::Vector && b.kind == JetKind::Scalar) {
                std::string v = "((" + a.value + ") * (" + b.value + "))";
                std::string gx = "((" + b.value + ") * (" + a.gradX + ") + (" + a.value + ").x * (" + b.grad + "))";
                std::string gy = "((" + b.value + ") * (" + a.gradY + ") + (" + a.value + ").y * (" + b.grad + "))";
                std::string gz = "((" + b.value + ") * (" + a.gradZ + ") + (" + a.value + ").z * (" + b.grad + "))";
                return JetExpr{ JetKind::Vector, v, "", gx, gy, gz };
            }
            e.refuse("type mismatch in Scale for analytic gradient emitter");
            return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
        }
        case Op::Noise: {
            JetExpr q = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            std::string jv = "j" + std::to_string(nextVar++);
            body += "    let " + jv + " = cnoise3_grad(" + q.value + ");\n";
            std::string v = jv + ".value";
            std::string g = "((" + jv + ".grad.x * (" + q.gradX + ")) + (" +
                                  jv + ".grad.y * (" + q.gradY + ")) + (" +
                                  jv + ".grad.z * (" + q.gradZ + ")))";
            return JetExpr{ JetKind::Scalar, v, g, "", "", "" };
        }
        case Op::Dot: {
            JetExpr a = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            JetExpr b = emitMathNodeGrad(*node.children[1], e, pt, body, nextVar);
            if (a.kind != JetKind::Vector || b.kind != JetKind::Vector) {
                e.refuse("Dot expects vector operands in analytic gradient emitter");
                return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
            }
            std::string v = "dot(" + a.value + ", " + b.value + ")";
            std::string g = "(((" + a.value + ").x * (" + b.gradX + ") + (" + b.value + ").x * (" + a.gradX + ")) + " +
                            "((" + a.value + ").y * (" + b.gradY + ") + (" + b.value + ").y * (" + a.gradY + ")) + " +
                            "((" + a.value + ").z * (" + b.gradZ + ") + (" + b.value + ").z * (" + a.gradZ + ")))";
            return JetExpr{ JetKind::Scalar, v, g, "", "", "" };
        }
        case Op::Length: {
            JetExpr a = emitMathNodeGrad(*node.children[0], e, pt, body, nextVar);
            if (a.kind != JetKind::Vector) {
                e.refuse("Length expects vector operand in analytic gradient emitter");
                return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
            }
            std::string v = "length(" + a.value + ")";
            std::string dg = "(((" + a.value + ").x * (" + a.gradX + ")) + ((" + a.value + ").y * (" + a.gradY + ")) + ((" + a.value + ").z * (" + a.gradZ + ")))";
            std::string g = "select(vec3<f32>(0.0), (" + dg + ") / max(length(" + a.value + "), 1e-6), length(" + a.value + ") > 1e-6)";
            return JetExpr{ JetKind::Scalar, v, g, "", "", "" };
        }
        default:
            e.refuse(std::string("unsupported operation in analytic gradient emitter: ") + OntoMath::mathOpName(node.op));
            return JetExpr{ JetKind::Scalar, "0.0", "vec3<f32>(0.0)", "", "", "" };
    }
}

// Emit one node, returning the name of the temporary holding its distance.
std::string emitNode(const geom::SdfNode& n, Emit& e) {
    const std::string out = e.fresh();

    if (n.op == geom::SdfOp::Leaf) {
        // Leaf placement: the CPU evaluates every primitive at (world - offset).
        const std::string off = e.param3(n.offset);
        const std::string lp = e.fresh();
        e.line("let " + lp + " = p - " + off + ";");

        switch (n.prim) {
            case geom::SdfPrim::Sphere:
                e.line("let " + out + " = sdSphere(" + lp + ", " + e.param(n.dims.x) + ");");
                break;
            case geom::SdfPrim::Box:
                e.line("let " + out + " = sdBox(" + lp + ", " + e.param3(n.dims) + ");");
                break;
            case geom::SdfPrim::RoundBox:
                e.line("let " + out + " = sdRoundBox(" + lp + ", " + e.param3(n.dims) + ", " + e.param(n.p0) + ");");
                break;
            case geom::SdfPrim::Ellipsoid:
                e.line("let " + out + " = sdEllipsoid(" + lp + ", " + e.param3(n.dims) + ");");
                break;
            case geom::SdfPrim::Cylinder:
                e.line("let " + out + " = sdCylinder(" + lp + ", " + e.param(n.dims.x) + ", " + e.param(n.dims.y) + ");");
                break;
            case geom::SdfPrim::Cone:
                e.line("let " + out + " = sdCone(" + lp + ", " + e.param(n.dims.x) + ", " + e.param(n.dims.y) + ");");
                break;
            case geom::SdfPrim::Torus:
                e.line("let " + out + " = sdTorus(" + lp + ", " + e.param(n.dims.x) + ", " + e.param(n.dims.y) + ");");
                break;
            case geom::SdfPrim::Expr: {
                // The implicit f(x,y,z)=0 surface. NOTE: this is an iso-surface
                // value, not a true distance, so sphere tracing must take damped
                // steps through it — see the marcher's use of kExprDamping.
                e.sawExpr = true;
                if (n.mathNode) {
                    e.line("let " + out + " = " + emitMathNode(*n.mathNode, e, lp) + ";");
                } else if (!n.rpn.empty()) {
                    e.line("let " + out + " = " + emitRpn(n.rpn, e, lp) + ";");
                } else {
                    e.line("let " + out + " = 1e9;");
                }
                break;
            }
            case geom::SdfPrim::Convex: {
                // max over outward face half-spaces. Exact inside and on faces; a
                // valid 1-Lipschitz bound outside, matching the CPU.
                if (n.planes.empty()) { e.line("let " + out + " = 1e9;"); break; }
                e.line("var " + out + "_acc = -1e9;");
                for (const glm::vec4& pl : n.planes) {
                    const std::string nx = e.param(pl.x), ny = e.param(pl.y),
                                      nz = e.param(pl.z), d = e.param(pl.w);
                    e.line(out + "_acc = max(" + out + "_acc, dot(" + lp +
                           ", vec3<f32>(" + nx + ", " + ny + ", " + nz + ")) - " + d + ");");
                }
                e.line("let " + out + " = " + out + "_acc;");
                break;
            }
        }
        return out;
    }

    // Operators. Mirrors evalSdf's degenerate handling exactly: fewer than two
    // children falls through to the first child, or 1.0 when there is none.
    if (n.children.size() < 2 || !n.children[0] || !n.children[1]) {
        if (!n.children.empty() && n.children[0]) {
            const std::string a = emitNode(*n.children[0], e);
            e.line("let " + out + " = " + a + ";");
        } else {
            e.line("let " + out + " = 1.0;");
        }
        return out;
    }

    const std::string a = emitNode(*n.children[0], e);
    const std::string b = emitNode(*n.children[1], e);
    switch (n.op) {
        case geom::SdfOp::Morph:
            e.line("let " + out + " = mix(" + a + ", " + b + ", clamp(" + e.param(n.t) + ", 0.0, 1.0));");
            break;
        case geom::SdfOp::Intersect:
            e.line("let " + out + " = max(" + a + ", " + b + ");");
            break;
        case geom::SdfOp::Subtract:
            e.line("let " + out + " = max(" + a + ", -(" + b + "));");
            break;
        case geom::SdfOp::SmoothUnion:
            e.line("let " + out + " = sminK(" + a + ", " + b + ", " + e.param(n.t) + ");");
            break;
        case geom::SdfOp::Union:
        default:
            e.line("let " + out + " = min(" + a + ", " + b + ");");
            break;
    }
    return out;
}

// Validate the authored vector channel before lowering it. The generic emitter
// can print vector syntax, but a chroma expression has a stronger contract than
// "something that happens to parse": every authored piece must actually be a
// Vector and every Piecewise feature must have a GPU realization. An absent chi
// is handled outside this helper as the legacy light.color default.
bool validateVectorPiecewise(const OntoMath::Piecewise& pw, bool bindTime,
                             std::string& error) {
    OntoMath::TypeEnv env{
        {OntoMath::kAmbientPointVar, OntoMath::ValueKind::Vector},
        {"x", OntoMath::ValueKind::Scalar},
        {"y", OntoMath::ValueKind::Scalar},
        {"z", OntoMath::ValueKind::Scalar}
    };
    if (bindTime) env[OntoMath::kTimeVar] = OntoMath::ValueKind::Scalar;

    if (pw.pieces.empty()) {
        error = "authored vector expression has no pieces";
        return false;
    }
    for (std::size_t i = 0; i < pw.pieces.size(); ++i) {
        const auto& piece = pw.pieces[i];
        if (piece.guard || piece.whereLEZero || piece.call || piece.fold) {
            error = "piece " + std::to_string(i) +
                    " uses Piecewise semantics the WGSL expression channel does not implement";
            return false;
        }
        if (!piece.mathNode) {
            error = "piece " + std::to_string(i) + " has no authored value";
            return false;
        }
        OntoMath::ValueKind kind = OntoMath::ValueKind::Unknown;
        std::string typeError;
        if (!piece.mathNode->checkTypes(env, typeError, &kind, false)) {
            error = typeError;
            return false;
        }
        if (kind != OntoMath::ValueKind::Vector) {
            error = "piece " + std::to_string(i) + " must evaluate to Vector, got " +
                    std::string(OntoMath::valueKindName(kind));
            return false;
        }
    }
    error.clear();
    return true;
}

// Validate the authored angular source factor before lowering. Alpha is scalar
// and is the only Screen-radiance expression context that admits omega.x/y/z.
bool validateAngularPiecewise(const OntoMath::Piecewise& pw, std::string& error) {
    OntoMath::TypeEnv env{
        {OntoMath::kAmbientPointVar, OntoMath::ValueKind::Vector},
        {"x", OntoMath::ValueKind::Scalar},
        {"y", OntoMath::ValueKind::Scalar},
        {"z", OntoMath::ValueKind::Scalar},
        {OntoMath::kTimeVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kOmegaXVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kOmegaYVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kOmegaZVar, OntoMath::ValueKind::Scalar}
    };

    if (pw.pieces.empty()) {
        error = "authored angular expression has no pieces";
        return false;
    }
    for (std::size_t i = 0; i < pw.pieces.size(); ++i) {
        const auto& piece = pw.pieces[i];
        if (piece.guard || piece.whereLEZero || piece.call || piece.fold) {
            error = "piece " + std::to_string(i) +
                    " uses Piecewise semantics the WGSL expression channel does not implement";
            return false;
        }
        if (!piece.mathNode) {
            error = "piece " + std::to_string(i) + " has no authored value";
            return false;
        }
        OntoMath::ValueKind kind = OntoMath::ValueKind::Unknown;
        std::string typeError;
        if (!piece.mathNode->checkTypes(env, typeError, &kind, false)) {
            error = typeError;
            return false;
        }
        if (kind != OntoMath::ValueKind::Scalar) {
            error = "piece " + std::to_string(i) + " must evaluate to Scalar, got " +
                    std::string(OntoMath::valueKindName(kind));
            return false;
        }
    }
    error.clear();
    return true;
}


// V3 phase has its own directional vocabulary. It is scalar medium truth and
// therefore must never gain source-alpha's omega binding by accident.
bool validatePhasePiecewise(const OntoMath::Piecewise& pw, std::string& error) {
    OntoMath::TypeEnv env{
        {OntoMath::kAmbientPointVar, OntoMath::ValueKind::Vector},
        {"x", OntoMath::ValueKind::Scalar},
        {"y", OntoMath::ValueKind::Scalar},
        {"z", OntoMath::ValueKind::Scalar},
        {OntoMath::kTimeVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kWiXVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kWiYVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kWiZVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kWoXVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kWoYVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kWoZVar, OntoMath::ValueKind::Scalar}
    };

    if (pw.pieces.empty()) {
        error = "authored phase expression has no pieces";
        return false;
    }
    for (std::size_t i = 0; i < pw.pieces.size(); ++i) {
        const auto& piece = pw.pieces[i];
        if (piece.guard || piece.whereLEZero || piece.call || piece.fold) {
            error = "piece " + std::to_string(i) +
                    " uses Piecewise semantics the WGSL expression channel does not implement";
            return false;
        }
        if (!piece.mathNode) {
            error = "piece " + std::to_string(i) + " has no authored value";
            return false;
        }
        OntoMath::ValueKind kind = OntoMath::ValueKind::Unknown;
        std::string typeError;
        if (!piece.mathNode->checkTypes(env, typeError, &kind, false)) {
            error = typeError;
            return false;
        }
        if (kind != OntoMath::ValueKind::Scalar) {
            error = "piece " + std::to_string(i) + " must evaluate to Scalar, got " +
                    std::string(OntoMath::valueKindName(kind));
            return false;
        }
    }
    error.clear();
    return true;
}

// V4 self-emission is vector-valued medium truth with an emission-owned omega
// context. Its physical omega is world-space sample -> eye; it is not source
// alpha's source -> receiver authored invariant.
bool validateEmissionPiecewise(const OntoMath::Piecewise& pw, std::string& error) {
    OntoMath::TypeEnv env{
        {OntoMath::kAmbientPointVar, OntoMath::ValueKind::Vector},
        {"x", OntoMath::ValueKind::Scalar},
        {"y", OntoMath::ValueKind::Scalar},
        {"z", OntoMath::ValueKind::Scalar},
        {OntoMath::kTimeVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kOmegaXVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kOmegaYVar, OntoMath::ValueKind::Scalar},
        {OntoMath::kOmegaZVar, OntoMath::ValueKind::Scalar}
    };

    if (pw.pieces.empty()) {
        error = "authored volume emission expression has no pieces";
        return false;
    }
    for (std::size_t i = 0; i < pw.pieces.size(); ++i) {
        const auto& piece = pw.pieces[i];
        if (piece.guard || piece.whereLEZero || piece.call || piece.fold) {
            error = "piece " + std::to_string(i) +
                    " uses Piecewise semantics the WGSL expression channel does not implement";
            return false;
        }
        if (!piece.mathNode) {
            error = "piece " + std::to_string(i) + " has no authored value";
            return false;
        }
        OntoMath::ValueKind kind = OntoMath::ValueKind::Unknown;
        std::string typeError;
        if (!piece.mathNode->checkTypes(env, typeError, &kind, false)) {
            error = typeError;
            return false;
        }
        if (kind != OntoMath::ValueKind::Vector) {
            error = "piece " + std::to_string(i) + " must evaluate to Vector, got " +
                    std::string(OntoMath::valueKindName(kind));
            return false;
        }
    }
    error.clear();
    return true;
}

// The raymarcher. Rasterises the field's bounding box and sphere-traces the true
// eye ray per fragment, in FIELD space.
//
// The cube is only the rasterisation domain. The field is evaluated in field
// space, and an implicit like a gyroid is periodic and defined EVERYWHERE —
// marching from the eye therefore hits sheets between the camera and the
// object, which is why a Gyroid looked like a flashing cube of holes. Enter
// and leave at the analytic AABB; the rasterised face is not the ray origin.
const char* kMarcher = R"WGSL(
struct RU {
    viewProj:       mat4x4<f32>,
    invViewProj:    mat4x4<f32>,
    lightPos:       vec4<f32>,
    eyePos:         vec4<f32>,
    lightAmbient:   vec4<f32>,
    lightDiffuse:   vec4<f32>,
    lightSpecular:  vec4<f32>,
    // x = lighting enabled; y = Rung-8 derived visibility enabled.
    // Visibility is execution state, not authored source state.
    lightControl:   vec4<f32>,
    // x/y/z/w = source intensity/ambient/diffuse/specular. These are used
    // only when authored chi is present; the no-chi branch keeps the exact
    // pre-Rung-5 color-bearing light uniforms below.
    radianceSourceCoefficients: vec4<f32>,
    // x = distance to the camera's far plane, in WORLD units.
    // y = viewport width in pixels.
    // z = viewport height in pixels.
    // w = authorable space distortion factor (e.g. Far Lands Zone).
    limits:      vec4<f32>,
    // x = admitted radiance-source temporal coordinate; y = its delta.
    // z/w reserved. Authored rho(p,t) reads t from radianceTime.x.
    radianceTime: vec4<f32>,
    // Independent participating-medium coordinate. D(p,t) never borrows
    // radianceTime merely because both channels happen to read canonical t.
    volumeTime: vec4<f32>,
};
@group(0) @binding(0) var<uniform> u: RU;
struct Params { v: array<f32> };
@group(0) @binding(1) var<storage, read> P: Params;

struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldPos: vec3<f32>,
    @location(1) @interpolate(flat) instIdx: u32,
};

@vertex
fn vs(@location(0) pos: vec3<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var o: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos * inst.extents.xyz, 1.0);
    o.clip = u.viewProj * world;
    // Clamp to far plane only when in front of camera (w > 0) so proxy geometry is never
    // lost to far-plane clipping without inverting clip depth for vertices behind the camera plane.
    if (o.clip.w > 0.0) {
        o.clip.z = min(o.clip.z, o.clip.w * 0.999999);
    }
    o.worldPos = world.xyz;
    o.instIdx = instIdx;
    return o;
}

// sdfGrad, sdfNormal and sdfEvalGrad emitted dynamically above before kMarcher

struct FSOut {
    @location(0) color: vec4<f32>,
    @builtin(frag_depth) depth: f32,
};

// Dual-Path WGSL Field Evaluator
// This function is generated dynamically based on whether the Law system 
// provides a hardcoded parameter path or an AST-driven piecewise definition.
// volumeDensityEval is emitted before this block.

fn rayAabb(ro: vec3<f32>, rd: vec3<f32>, b: vec3<f32>) -> vec2<f32> {
    // Slab method. A zero direction would NaN the inverse; nudge it.
    let rds = select(rd, vec3<f32>(1e-8), abs(rd) < vec3<f32>(1e-8));
    let inv = 1.0 / rds;
    let t0 = (-b - ro) * inv;
    let t1 = ( b - ro) * inv;
    let tmin = min(t0, t1);
    let tmax = max(t0, t1);
    let tEnter = max(max(tmin.x, tmin.y), tmin.z);
    let tExit  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2<f32>(tEnter, tExit);
}

// Min/max heightfield grid DDA skip (rendering-optimization Phase C). Walks
// the ray's XZ footprint across a uniform grid of conservative (hMin,hMax)
// bounds (Amanatides & Woo 1987) and skips whole cells the ray's own height
// range there cannot intersect -- a cell bound is PROVEN by
// geom::computeHeightGrid, not guessed here, so a skip can never remove real
// geometry. Returns (t, found): found < 0.5 is a PROVEN miss -- the caller can
// skip the fine marcher for the whole ray -- and found >= 0.5 means resume the
// unmodified fine marcher at t. On exhausting the guard budget without either
// (should not happen: a correct 2D DDA crosses at most dimX+dimZ grid lines)
// this fails OPEN (found=1 at the last t reached), never claiming a miss it
// has not actually verified.
fn heightGridAdvance(inst: SdfInstanceData, ro: vec3<f32>, rd: vec3<f32>,
                     tStart: f32, tMax: f32) -> vec2<f32> {
    let dimX = inst.heightGridDimX;
    let dimZ = inst.heightGridDimZ;
    if (dimX == 0u || dimZ == 0u) { return vec2<f32>(tStart, 1.0); }

    let halfX = inst.extents.x;
    let halfZ = inst.extents.z;
    let cellX = (2.0 * halfX) / f32(dimX);
    let cellZ = (2.0 * halfZ) / f32(dimZ);

    var t = tStart;
    var ix = clamp(i32(floor((ro.x + rd.x * t + halfX) / cellX)), 0, i32(dimX) - 1);
    var iz = clamp(i32(floor((ro.z + rd.z * t + halfZ) / cellZ)), 0, i32(dimZ) - 1);
    let stepX = select(-1, 1, rd.x >= 0.0);
    let stepZ = select(-1, 1, rd.z >= 0.0);
    let hasRdX = abs(rd.x) > 1e-8;
    let hasRdZ = abs(rd.z) > 1e-8;
    let invRdX = select(0.0, 1.0 / rd.x, hasRdX);
    let invRdZ = select(0.0, 1.0 / rd.z, hasRdZ);

    let maxSteps = i32(dimX) + i32(dimZ) + 4;
    for (var guard = 0; guard < maxSteps; guard = guard + 1) {
        if (t >= tMax) { return vec2<f32>(tMax, 0.0); }
        if (ix < 0 || ix >= i32(dimX) || iz < 0 || iz >= i32(dimZ)) {
            return vec2<f32>(tMax, 0.0); // left the grid footprint: nothing to find
        }

        let nx = -halfX + f32(select(ix, ix + 1, stepX > 0)) * cellX;
        let nz = -halfZ + f32(select(iz, iz + 1, stepZ > 0)) * cellZ;
        let tMaxX = select(1e30, (nx - ro.x) * invRdX, hasRdX);
        let tMaxZ = select(1e30, (nz - ro.z) * invRdZ, hasRdZ);
        let tCellExit = min(min(tMaxX, tMaxZ), tMax);

        let cell = heightCells[inst.heightGridOffset + u32(iz) * dimX + u32(ix)];
        let y0 = ro.y + rd.y * t;
        let y1 = ro.y + rd.y * tCellExit;
        let yLo = min(y0, y1);
        let yHi = max(y0, y1);
        if (!(yHi < cell.x || yLo > cell.y)) {
            return vec2<f32>(t, 1.0); // candidate cell -- hand off to the fine marcher
        }

        t = tCellExit;
        if (tMaxX < tMaxZ) { ix = ix + stepX; } else { iz = iz + stepZ; }
    }
    return vec2<f32>(t, 1.0); // guard exhausted: fail open, never an unverified miss
}

// Generic spatial-Prophetic traversal over a fixed-depth proof bitmap.
// The CPU adaptive hierarchy is still the theorem. Its proved-positive cells
// are conservatively expanded into regular depth-N cells before upload.
// Therefore a set bit permits skipping exactly one regular cell; a clear bit
// carries no negative information and hands that cell to the exact marcher.
//
// This removes root-to-leaf pointer chasing from every hierarchy query. The
// only slab intersection is for the one regular cell containing the current
// ray point. Exact split-plane ownership follows ray direction so a boundary
// cannot repeatedly select the cell the ray just exited.
fn rangeGridAxisIndex(coord: f32, halfExtent: f32,
                      dir: f32, dim: u32) -> u32 {
    let e = abs(halfExtent);
    let denom = max(2.0 * e, 1e-8);
    let scaled = clamp(((coord + e) / denom) * f32(dim),
                       0.0, f32(dim));
    let floored = floor(scaled);
    var idx = u32(min(floored, f32(dim - 1u)));
    if (scaled == floored && dir < 0.0 && idx > 0u) {
        idx = idx - 1u;
    }
    return idx;
}

fn rangeCandidate(inst: SdfInstanceData, ro: vec3<f32>, rd: vec3<f32>,
                  tStart: f32, tMax: f32) -> vec3<f32> {
    if (inst.rangeTraversalEnabled == 0u ||
        inst.rangeProofWordCount == 0u ||
        inst.rangeProofDepth == 0u ||
        inst.rangeProofDepth > 10u) {
        return vec3<f32>(tStart, tMax, 1.0);
    }

    let dim = 1u << inst.rangeProofDepth;
    let cellCount = dim * dim * dim;
    let neededWords = (cellCount + 31u) >> 5u;
    if (inst.rangeProofWordCount < neededWords) {
        return vec3<f32>(tStart, tMax, 1.0);
    }

    let extent = abs(inst.extents.xyz);
    if (any(extent <= vec3<f32>(0.0))) {
        return vec3<f32>(tStart, tMax, 1.0);
    }
    let cellSize = (2.0 * extent) / f32(dim);
    var t = tStart;

    // At depth 6 a straight ray crosses at most 190 regular cells. If a future
    // deeper proof grid exceeds this guard, the unvisited remainder fails open
    // to exact marching rather than silently disappearing.
    for (var skipGuard = 0; skipGuard < 192; skipGuard = skipGuard + 1) {
        if (t >= tMax) {
            return vec3<f32>(tMax, tMax, 0.0);
        }

        let p = ro + rd * t;
        if (any(p < -extent) || any(p > extent)) {
            return vec3<f32>(t, tMax, 1.0);
        }

        let ix = rangeGridAxisIndex(p.x, extent.x, rd.x, dim);
        let iy = rangeGridAxisIndex(p.y, extent.y, rd.y, dim);
        let iz = rangeGridAxisIndex(p.z, extent.z, rd.z, dim);
        let linear = ix + dim * (iy + dim * iz);
        let localWord = linear >> 5u;
        if (localWord >= inst.rangeProofWordCount) {
            return vec3<f32>(t, tMax, 1.0);
        }

        let bit = 1u << (linear & 31u);
        let provedPositive =
            (rangeProofWords[inst.rangeProofWordOffset + localWord] & bit) != 0u;

        let cellMin =
            -extent + vec3<f32>(f32(ix), f32(iy), f32(iz)) * cellSize;
        let cellMax = cellMin + cellSize;

        // We need only the selected cell's EXIT. The old slab helper computed
        // both entry and exit even though this ray point already owns the cell.
        // Per axis, max((bmin-ro)/rd, (bmax-ro)/rd) is exactly the forward
        // face: bmax for a positive safe direction, bmin for a negative one.
        // Keep the same near-zero substitution and arithmetic order, but skip
        // the unused entry-face work. This is not a DDA; one classified cell
        // still hands a clear bit straight back to the exact authored marcher.
        let rds = select(rd, vec3<f32>(1e-8), abs(rd) < vec3<f32>(1e-8));
        let invRd = 1.0 / rds;
        let exitFace = select(cellMin, cellMax, rds >= vec3<f32>(0.0));
        let axisExit = (exitFace - ro) * invRd;
        let cellExit = min(
            min(min(axisExit.x, axisExit.y), axisExit.z),
            tMax);
        if (cellExit <= t) {
            return vec3<f32>(t, tMax, 1.0);
        }

        if (!provedPositive) {
            // A clear bit says only that the proof grid grants no skip here.
            // Preserve the exact marcher's authority over this interval.
            return vec3<f32>(t, cellExit, 1.0);
        }

        // The CPU theorem proved f>0 throughout this regular cell. Advance to
        // its exact exit without evaluating the authored field.
        t = cellExit;
    }

    return vec3<f32>(t, tMax, 1.0);
}

// Rung 8 exact baseline for the geometry this shader actually owns.
//
// This is deliberately DERIVED transport: it reads the already-authored SDF
// between the receiver and source and never writes or reinterprets rho/chi/alpha.
// lightControl.y=0 is the exact Rung-7 compatibility law V=1.
//
// Scope is intentionally honest. A generated SDF pipeline can evaluate its own
// authored geometry; it cannot yet name arbitrary differently-structured SDF
// pipelines elsewhere in the Zone. Scene-wide transport needs a shared scene
// geometry representation rather than pretending those other beings are visible
// here. Until that exists, this function is not enabled globally by EngineRender.
fn sourceTransportSignedStep(p: vec3<f32>, damping: f32) -> f32 {
    if (damping < 0.5) {
        let s = sdfSampleStep(p);
        var gradLen = s.gradLen;
        if (gradLen <= 1e-6) {
            let ge = 1e-3;
            let raw = s.raw;
            let g = vec3<f32>(
                sdfEval(p + vec3<f32>(ge, 0.0, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, ge, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, 0.0, ge)) - raw) / ge;
            gradLen = length(g);
        }
        return select(s.raw, s.raw / gradLen, gradLen > 1e-6);
    }
    return sdfEval(p);
}

fn sourceVisibility(surfacePoint: vec3<f32>, surfaceNormal: vec3<f32>, sourceWorld: vec3<f32>) -> f32 {
    if (u.lightControl.y < 0.5) { return 1.0; }

    let inst = instances[g_instIdx];
    let sourceField = (inst.invModel * vec4<f32>(sourceWorld, 1.0)).xyz;
    let toSource = sourceField - surfacePoint;
    let sourceDistance = length(toSource);
    let surfaceEps = max(inst.misc.y, 1e-4);
    let bias = surfaceEps * 4.0;
    if (sourceDistance <= bias * 2.0) { return 1.0; }

    let initialDir = toSource / sourceDistance;
    let damping = inst.misc.w;

    // The primary marcher may terminate just inside the zero set (for example,
    // over-relaxation followed by secant correction). A tiny ray-direction bias
    // then begins the transport query inside its own receiver and manufactures a
    // self-shadow. Escape only when the source ray points outward through the
    // receiver's local SDF normal. Back-facing/inward rays remain inside real
    // geometry and are therefore still blocked by the receiver itself.
    let surfaceSignedStep = sourceTransportSignedStep(surfacePoint, damping);
    var origin = surfacePoint + initialDir * bias;
    if (dot(surfaceNormal, initialDir) > 0.0) {
        let penetration = max(-surfaceSignedStep, 0.0);
        origin = surfacePoint + surfaceNormal * (penetration + bias);
    }

    let remaining = sourceField - origin;
    let rayLength = length(remaining);
    if (rayLength <= bias) { return 1.0; }
    let shadowDir = remaining / rayLength;

    // Restrict the query to authored geometry inside this instance's domain.
    // A source outside the box is fine: leaving the box unobstructed proves this
    // instance contributes no blocker beyond that exit.
    let bounds = rayAabb(origin, shadowDir, inst.extents.xyz);
    if (bounds.y < bounds.x || bounds.y <= 0.0) { return 1.0; }

    var tShadow = max(bounds.x, 0.0);
    let maxShadow = min(bounds.y, rayLength - bias);
    if (maxShadow <= tShadow) { return 1.0; }

    // Match the primary renderer's finite exact-march budget. This baseline uses
    // no proof-grid skip, no penumbra estimate, and no percentage heuristic.
    for (var shadowStep = 0; shadowStep < 192; shadowStep = shadowStep + 1) {
        if (tShadow >= maxShadow) { return 1.0; }

        let pShadow = origin + shadowDir * tShadow;
        let currentEps = max(surfaceEps, tShadow * 0.001);
        let dShadow = sourceTransportSignedStep(pShadow, damping);

        if (dShadow <= 0.0 || abs(dShadow) < currentEps) { return 0.0; }
        tShadow = tShadow + max(dShadow, currentEps);
    }

    // The primary marcher uses the same bounded iteration contract. If the
    // budget is exhausted before the segment is decided, fail conservatively:
    // never invent an unobstructed path that was not actually traversed.
    return 0.0;
}

@fragment
fn fs(in: VSOut) -> FSOut {
    g_instIdx = in.instIdx;
    let inst    = instances[in.instIdx];
    let roWorld = u.eyePos.xyz;

    // Derive primary ray direction: use exact screen NDC unprojection when viewport
    // dimensions are available to eliminate proxy cube clipping and perspective non-linearities.
    var rdWorld: vec3<f32>;
    if (u.limits.y > 0.0 && u.limits.z > 0.0) {
        let ndc = vec4<f32>(
            (in.clip.x / u.limits.y) * 2.0 - 1.0,
            (1.0 - (in.clip.y / u.limits.z)) * 2.0 - 1.0,
            1.0,
            1.0
        );
        let worldPt = u.invViewProj * ndc;
        rdWorld = normalize(worldPt.xyz / worldPt.w - roWorld);
    } else {
        rdWorld = normalize(in.worldPos - roWorld);
    }

    let ro      = (inst.invModel * vec4<f32>(roWorld, 1.0)).xyz;
    let rdField = (inst.invModel * vec4<f32>(rdWorld, 0.0)).xyz;
    var rd      = normalize(rdField);

    // Authorable Far Lands space distortion (Part 2):
    if (u.limits.w > 1e-4) {
        let warp = sin(ro.xyz * 0.05 + vec3<f32>(0.0, rdField.y * 2.0, 0.0)) * u.limits.w;
        rd = normalize(rd + warp);
    }
    // t is measured in FIELD units; the far plane is a WORLD distance. The
    // unnormalised field-space direction is exactly the conversion factor along
    // this ray, so this stays right under any invertible model transform,
    // non-uniform scale included.
    let farField = u.limits.x * length(rdField);
    let isHeightfield = inst.misc.x > 0.5;
    let eps     = inst.misc.y;
    let damping = inst.misc.w;
    let box     = rayAabb(ro, rd, inst.extents.xyz);
    // Miss the cube, or the whole slab is behind the eye.
    if (box.y < box.x || box.y < 0.0) { discard; }

    var t = max(box.x, 0.0);
    // Three bounds, and they are measured from two different origins -- getting
    // that wrong is what made every small analytic shape vanish (Bugs.md #20).
    //   box.y     : where the ray leaves the bounding cube. From the EYE.
    //   farField  : the camera's far plane. Also from the eye.
    //   misc.z    : how far to march INSIDE the volume, a LENGTH (maxDim * 8) --
    //               so it has to be offset by the entry point. Compared against
    //               `t` directly, as it briefly was, it stops being a budget and
    //               becomes "objects further than maxDim * 8 from the camera do
    //               not exist": for a chess piece maxDim is ~0.6, so pieces
    //               beyond ~4.8 units entered the loop with t already past
    //               maxDist, broke on the first iteration and discarded. The
    //               noise floor's budget is 8000, which is why terrain looked
    //               fine and only the small things went missing.
    let maxDist = min(min(box.y, t + inst.misc.z), farField);

    // Min/max heightfield grid skip (Phase C): for a proven heightfield
    // (inst.misc.x > 0.5, with dimensions nonzero when traversal is enabled),
    // fast-forward t past a stretch the grid proves cannot contain the surface
    // BEFORE paying for the fine per-step marcher below, which this does not
    // modify. A whole-ray DDA miss deliberately falls open to that marcher;
    // see the guarded hand-off below.
    if (inst.heightGridDimX > 0u || (isHeightfield && damping < 0.5)) {
        let adv = heightGridAdvance(inst, ro, rd, t, maxDist);
        if (adv.y >= 0.5) {
            // DDA cell ownership and the fine marcher's hit test use finite f32
            // precision. Resume one of the marcher's own distance-scaled hit
            // tolerances before the candidate boundary: the preceding cells are
            // proved empty, so this cannot add a false hit, and it prevents a root
            // on the shared boundary from belonging to neither traversal.
            let boundaryEps = max(eps, adv.x * 0.001);
            t = max(t, adv.x - boundaryEps);
        }
        // A full-ray DDA miss once disagreed with the generic renderer at
        // grazing boundaries. Until that proof is independently repaired and
        // covered, do not discard: leave `t` at the original AABB entry and let
        // the exact generic marcher decide. The grid may accelerate a candidate
        // hand-off, never become an authority to erase a rendered root.
    }

    // Heightfield planar leap: If ray starts above the upper extent and points down, leap to top plane in 1 step
    if ((isHeightfield && damping < 0.5) && rd.y < -1e-4 && (ro.y + rd.y * t) > inst.extents.y) {
        let planeT = (inst.extents.y - ro.y) / rd.y;
        t = max(t, planeT);
    }

    var hit = false;
    var transmittance = 1.0;
    var volumetric_scatter = vec3<f32>(0.0);
    var volumetric_emission = vec3<f32>(0.0);
    // First ray coordinate at which the authored medium was actually sampled
    // with positive density. This is NOT a hard-surface hit.
    var first_density_t = -1.0;
    
    // Enhanced Sphere Tracing (Over-Relaxation) state:
    var omega = select(1.0, 1.4, damping > 0.5);
    var prev_d = 1e10;
    var candidate_step = 0.0;

    // When range traversal is active, exact marching owns only the current
    // ambiguous leaf. Crossing its exit asks the hierarchy for the next
    // candidate interval; proved-empty cells between them are skipped without
    // calling sdfEval/sdfSampleStep.
    var rangeCellExit = t;
    var rangeCandidateActive = false;
    
    for (var i = 0; i < 192; i = i + 1) {
        if (t > maxDist) { break; }

        if (inst.rangeTraversalEnabled != 0u &&
            (!rangeCandidateActive || t >= rangeCellExit)) {
            let candidate = rangeCandidate(inst, ro, rd, t, maxDist);
            if (candidate.z < 0.5) {
                t = maxDist + 1.0;
                break;
            }
            let oldT = t;
            t = max(t, candidate.x);
            if (t > oldT) {
                // A proof-authorized spatial jump is not a marcher step. Any
                // secant / over-relaxation history describes the old sample
                // pair and must not be reused as though candidate_step bridged
                // this larger distance.
                prev_d = 1e10;
                candidate_step = 0.0;
                omega = select(1.0, 1.4, damping > 0.5);
            }
            rangeCellExit = max(t, candidate.y);
            rangeCandidateActive = true;
            if (t > maxDist) { break; }
        }

        // Keep the coordinate at which this iteration's medium sample is
        // evaluated. Surface marching may advance t by a gradient-corrected,
        // damped, or over-relaxed amount below; transport must integrate the
        // interval that was ACTUALLY traversed, not reuse raw SDF magnitude.
        let sample_t = t;
        let p = ro + rd * sample_t;
        
        // Analytical early-exit: If ray is above maximum height and traveling upwards, it can never hit ground
        if (isHeightfield && rd.y > 1e-4 && p.y > inst.extents.y) {
            break;
        }

        let current_eps = max(eps, t * 0.001);
        var d = 0.0;
        var raw = 0.0;

        if (damping < 0.5) {
            let s = sdfSampleStep(p);
            raw = s.raw;
            var gl = s.gradLen;
            if (gl <= 1e-6) {
                let ge = 1e-3;
                let g = vec3<f32>(
                    sdfEval(p + vec3<f32>(ge, 0.0, 0.0)) - raw,
                    sdfEval(p + vec3<f32>(0.0, ge, 0.0)) - raw,
                    sdfEval(p + vec3<f32>(0.0, 0.0, ge)) - raw) / ge;
                gl = length(g);
            }
            d = select(raw, raw / gl, gl > 1e-6);

            if (d <= 0.0 || abs(d) < current_eps) {
                hit = true;
                if (d < 0.0 && prev_d > 0.0 && candidate_step > 0.0) {
                    let frac = clamp(prev_d / (prev_d - d), 0.0, 1.0);
                    t = (t - candidate_step) + candidate_step * frac;
                }
                break;
            }

            candidate_step = max(d, current_eps);
            // Do not clamp the exact marcher's lawful step to octree-cell
            // boundaries. The hierarchy may skip cells it proved zero-free,
            // but ambiguous space must preserve the baseline march trajectory.
            prev_d = d;
            t = t + candidate_step;
        } else {
            raw = sdfEval(p);
            d = raw;

            if (d <= 0.0 || abs(d) < current_eps) {
                hit = true;
                if (d < 0.0 && prev_d > 0.0 && candidate_step > 0.0) {
                    let frac = clamp(prev_d / (prev_d - d), 0.0, 1.0);
                    t = (t - candidate_step) + candidate_step * frac;
                }
                break;
            }

            if (omega > 1.0 && d + prev_d < candidate_step) {
                t = t - candidate_step + prev_d;
                omega = 1.0;
                candidate_step = 0.0;
                continue;
            }

            prev_d = d;
            candidate_step = max(omega * d, current_eps);
            // Same rule for distance-field marching: cell boundaries are not
            // authored geometry and may not perturb the exact baseline step.
            t = t + candidate_step;
        }
        
        // Volumetric Field Accumulation
        let density = volumeDensityEval(p);
        // t may advance beyond maxDist on the last surface-march step. Medium
        // transport owns only the bounded interval [sample_t, maxDist].
        let marched_field_distance = max(min(t, maxDist) - sample_t, 0.0);
        if (density > 0.0 && marched_field_distance > 0.0) {
            if (first_density_t < 0.0) { first_density_t = sample_t; }
            // V1: sigma_t is independently authored when present; otherwise
            // volumeExtinctionEval preserves the exact pre-V1 0.5*D contract.
            let extinction = max(volumeExtinctionEval(p, density), 1e-6);
            
            let old_t = transmittance;
            transmittance *= exp(-extinction * marched_field_distance);
            
            // V2: sigma_s controls scattering magnitude while C_v controls medium
            // chroma. Compatibility (sigma_s=D, C_v=white) is byte-for-byte
            // equivalent to the historical white term.
            let scattering = max(volumeScatteringEval(p, density), 0.0);
            let mediumChroma = volumeChromaEval(p);
            if (HAS_AUTHORED_VOLUME_PHASE) {
                let mediumWorldP = (inst.model * vec4<f32>(p, 1.0)).xyz;
                let wiDelta = mediumWorldP - u.lightPos.xyz;
                let woDelta = u.eyePos.xyz - mediumWorldP;
                let wiLen = length(wiDelta);
                let woLen = length(woDelta);
                let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, SOURCE_DIRECTION_EPS),
                                wiLen > SOURCE_DIRECTION_EPS);
                let wo = select(vec3<f32>(0.0), woDelta / max(woLen, SOURCE_DIRECTION_EPS),
                                woLen > SOURCE_DIRECTION_EPS);
                var phase = 0.0;
                if ((!VOLUME_PHASE_READS_WI || wiLen > SOURCE_DIRECTION_EPS) &&
                    woLen > SOURCE_DIRECTION_EPS) {
                    phase = max(volumePhaseEval(p, wi, wo), 0.0);
                }
                volumetric_scatter +=
                    mediumChroma * (scattering / extinction) * phase *
                    (old_t - transmittance);
            } else {
                // Exact V2 compatibility arithmetic: no extra multiply-by-one.
                volumetric_scatter +=
                    mediumChroma * (scattering / extinction) * (old_t - transmittance);
            }

            if (HAS_AUTHORED_VOLUME_EMISSION) {
                let emissionWorldP = (inst.model * vec4<f32>(p, 1.0)).xyz;
                let emissionDelta = u.eyePos.xyz - emissionWorldP;
                let emissionLen = length(emissionDelta);
                let emissionOmega = select(
                    vec3<f32>(0.0),
                    emissionDelta / max(emissionLen, SOURCE_DIRECTION_EPS),
                    emissionLen > SOURCE_DIRECTION_EPS);
                var emitted = vec3<f32>(0.0);
                if (!VOLUME_EMISSION_READS_OMEGA || emissionLen > SOURCE_DIRECTION_EPS) {
                    emitted = max(volumeEmissionEval(p, emissionOmega), vec3<f32>(0.0));
                }
                volumetric_emission += emitted * ((old_t - transmittance) / extinction);
            }
        }
        
        // Early exit if the field is fully opaque or ray exits the bounded volume
        if (transmittance < 0.01) { break; }
        if (t > maxDist) { break; }
    }
    if (!hit && transmittance > 0.99 && !HAS_AUTHORED_VOLUME_EMISSION) { discard; }

    var out: FSOut;
    
    if (!hit) {
        // Volumetric-only output still uses the legacy shared SDF pipeline.
        // first_density_t records the sampled medium coordinate truthfully, but
        // V0c MUST NOT activate this path in production until volume composition
        // no longer treats a translucent sample as an opaque depth owner.
        let final_alpha = 1.0 - transmittance;
        let c = volumetric_scatter + volumetric_emission;
        if (final_alpha > 0.0) {
            out.color = vec4<f32>(c / final_alpha, final_alpha);
        } else {
            out.color = vec4<f32>(0.0);
        }
        if (first_density_t >= 0.0) {
            let hit_p = ro + rd * first_density_t;
            let hit_w = (inst.model * vec4<f32>(hit_p, 1.0)).xyz;
            let hit_c = u.viewProj * vec4<f32>(hit_w, 1.0);
            out.depth = hit_c.z / hit_c.w;
        } else {
            out.depth = 1.0; 
        }
        return out;
    }

    let pf = ro + rd * t;                                // field-space hit
    let pw = (inst.model * vec4<f32>(pf, 1.0)).xyz;         // world-space hit
    let nf = sdfNormal(pf);
    // Normals transform by the inverse-transpose; invModel transposed gives it
    // without shipping another matrix.
    let nw = normalize((transpose(inst.invModel) * vec4<f32>(nf, 0.0)).xyz);

    let L = normalize(u.lightPos.xyz - pw);
    let V = normalize(u.eyePos.xyz - pw);
    let H = normalize(L + V);

    // Evaluate the Person-authored source invariants in source-relative world
    // coordinates. rho is scalar, chi is vec3, and alpha is a third independent
    // scalar over the normalized WORLD-space source -> receiver direction.
    let sourceDelta = pw - u.lightPos.xyz;
    let radialRadiance = max(lightRadiance(sourceDelta), 0.0);
    var angularRadiance = 1.0;
    if (HAS_AUTHORED_ANGULAR) {
        if (ANGULAR_READS_OMEGA) {
            let directionLength = length(sourceDelta);
            if (directionLength > SOURCE_DIRECTION_EPS) {
                let emissionOmega = sourceDelta / directionLength;
                angularRadiance = max(lightAngular(sourceDelta, emissionOmega), 0.0);
            } else {
                // omega is undefined at the source singularity. Refuse this
                // directional sample by contributing zero; never invent an axis.
                angularRadiance = 0.0;
            }
        } else {
            // A direction-independent authored alpha does not require omega.
            angularRadiance = max(lightAngular(sourceDelta, vec3<f32>(0.0)), 0.0);
        }
    }
    let shapedRadiance = radialRadiance * angularRadiance;
    let pathVisibility = sourceVisibility(pf, nf, u.lightPos.xyz);
    let directRadiance = shapedRadiance * pathVisibility;
    let diff = max(dot(nw, L), 0.0);
    let specShape = inst.shading.z *
        pow(max(dot(nw, H), 0.0), max(inst.shading.w, 1.0)) *
        step(0.0001, diff);

    var ambientTerm: vec3<f32>;
    var diffuseTerm: vec3<f32>;
    var specTerm: vec3<f32>;
    if (HAS_AUTHORED_CHROMA) {
        let sourceChroma = lightChroma(pw - u.lightPos.xyz);
        let c = u.radianceSourceCoefficients;
        // Separate the legacy scalar coefficients from chroma only on this new
        // path. This realizes rho * chi without multiplying legacy light.color
        // a second time, including when one legacy color channel is exactly zero.
        let ambientEnvelope = sourceChroma * vec3<f32>((c.x * c.y) / 0.2);
        let diffuseEnvelope = sourceChroma * vec3<f32>((c.x * c.z) / 0.8);
        let specularEnvelope = sourceChroma * vec3<f32>(c.x * c.w);
        ambientTerm = inst.shading.x * ambientEnvelope;
        diffuseTerm = inst.shading.y * diffuseEnvelope * diff * directRadiance;
        specTerm = specularEnvelope * specShape * directRadiance;
    } else {
        // EXACT compatibility branch from Rung 4. No authored chi means
        // constant legacy light.color, already carried by these uniforms.
        let ambientEnvelope  = u.lightAmbient.rgb / vec3<f32>(0.2);
        let diffuseEnvelope  = u.lightDiffuse.rgb / vec3<f32>(0.8);
        let specularEnvelope = u.lightSpecular.rgb;
        ambientTerm = inst.shading.x * ambientEnvelope;
        diffuseTerm = inst.shading.y * diffuseEnvelope * diff * directRadiance;
        specTerm = specularEnvelope * specShape * directRadiance;
    }

    let clip = u.viewProj * vec4<f32>(pw, 1.0);

    let surfaceColor = sdfColor(pf);
    let litRgb = surfaceColor * (ambientTerm + diffuseTerm) + specTerm;
    let base_rgb = mix(surfaceColor, litRgb, u.lightControl.x);
    let field_rgb = volumetric_scatter;
    
    let final_alpha = clamp(inst.baseColor.a + (1.0 - transmittance), 0.0, 1.0);
    let final_rgb = base_rgb * transmittance + field_rgb;
    
    if (final_alpha > 0.0) {
        out.color = vec4<f32>(final_rgb / final_alpha, final_alpha);
    } else {
        out.color = vec4<f32>(0.0);
    }
    
    // Depth from the ACTUAL hit, so a raymarched field occludes and is occluded by
    // ordinary meshes correctly instead of by its bounding box.
    out.depth = clip.z / clip.w;
    return out;
}
)WGSL";

} // namespace

ScalarExpressionLayout inspectScalarExpression(const OntoMath::Piecewise* expr,
                                               bool bindTime) {
    Emit e;
    e.bindTime = bindTime;
    std::string body;

    // No authored expression is a real structural state: compile() emits the
    // compatibility return 1.0 path. Give it an explicit identity so switching
    // between no radiance and authored radiance cannot look like a value edit.
    if (!expr || expr->pieces.empty()) {
        return ScalarExpressionLayout{"<legacy-radiance:1.0>", 0, true, ""};
    }

    emitPiecewise(*expr, e, "p", "f32", body);

    ScalarExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

ScalarExpressionLayout inspectDensityExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return ScalarExpressionLayout{"<volume-density:none>", 0, true, ""};
    }

    Emit e;
    e.bindTime = true;
    e.timeExpression = "u.volumeTime.x";
    std::string body;
    emitPiecewise(*expr, e, "p", "f32", body);

    ScalarExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

ScalarExpressionLayout inspectExtinctionExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return ScalarExpressionLayout{"<volume-extinction:compat-0.5-density>", 0, true, ""};
    }

    Emit e;
    e.bindTime = true;
    e.timeExpression = "u.volumeTime.x";
    std::string body;
    emitPiecewise(*expr, e, "p", "f32", body);

    ScalarExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

ScalarExpressionLayout inspectScatteringExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return ScalarExpressionLayout{"<volume-scattering:compat-density>", 0, true, ""};
    }

    Emit e;
    e.bindTime = true;
    e.timeExpression = "u.volumeTime.x";
    std::string body;
    emitPiecewise(*expr, e, "p", "f32", body);

    ScalarExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

VectorExpressionLayout inspectVolumeChromaExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return VectorExpressionLayout{"<volume-chroma:compat-white>", 0, true, ""};
    }

    std::string validationError;
    if (!validateVectorPiecewise(*expr, true, validationError)) {
        return VectorExpressionLayout{"", 0, false, validationError};
    }

    Emit e;
    e.bindTime = true;
    e.timeExpression = "u.volumeTime.x";
    std::string body;
    emitPiecewise(*expr, e, "p", "vec3<f32>", body);

    VectorExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

VectorExpressionLayout inspectVectorExpression(const OntoMath::Piecewise* expr,
                                               bool bindTime) {
    // Absence is not refusal: it means the historical authored light.color is
    // the constant chroma. Presence, however, must be honored or refused.
    if (!expr || expr->pieces.empty()) {
        return VectorExpressionLayout{"<legacy-chroma:light.color>", 0, true, ""};
    }

    std::string validationError;
    if (!validateVectorPiecewise(*expr, bindTime, validationError)) {
        return VectorExpressionLayout{"", 0, false, validationError};
    }

    Emit e;
    e.bindTime = bindTime;
    std::string body;
    emitPiecewise(*expr, e, "p", "vec3<f32>", body);

    VectorExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

AngularExpressionLayout inspectAngularExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return AngularExpressionLayout{"<legacy-angular:1.0>", 0, false, true, ""};
    }

    std::string validationError;
    if (!validateAngularPiecewise(*expr, validationError)) {
        return AngularExpressionLayout{"", 0, false, false, validationError};
    }

    Emit e;
    e.bindTime = true;
    e.bindOmega = true;
    std::string body;
    emitPiecewise(*expr, e, "p", "f32", body);

    AngularExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.readsOmega = e.readOmega;
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}


// Architectural note — GPT-5.6 Sol ("The Sun"), 2026-09-24:
// OntoMath's directional coordinates are shared mathematical grammar, not shared
// semantic authority. Phi_medium(p, wi, wo, t) owns participating-medium
// redirection. Rung 9 surface response f_r(p, n, wi, wo, t) may reuse this
// directional lowering machinery, but it must enter through its own
// material-response admission context. Reusing wi/wo must never let phase state
// stand in for surface response, or vice versa.
//
// Minimum–maximum principle: keep one expressive directional language while
// preserving distinct irreducible predicates for source emission, medium
// scattering, and receiving-surface response.
PhaseExpressionLayout inspectPhaseExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return PhaseExpressionLayout{"<volume-phase:1.0>", 0, false, false, true, ""};
    }

    std::string validationError;
    if (!validatePhasePiecewise(*expr, validationError)) {
        return PhaseExpressionLayout{"", 0, false, false, false, validationError};
    }

    Emit e;
    e.bindTime = true;
    e.bindPhaseDirections = true;
    std::string body;
    emitPiecewise(*expr, e, "p", "f32", body);

    PhaseExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.readsWi = e.readWi;
    layout.readsWo = e.readWo;
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

EmissionExpressionLayout inspectEmissionExpression(const OntoMath::Piecewise* expr) {
    if (!expr || expr->pieces.empty()) {
        return EmissionExpressionLayout{"<volume-emission:absent>", 0, false, true, ""};
    }

    std::string validationError;
    if (!validateEmissionPiecewise(*expr, validationError)) {
        return EmissionExpressionLayout{"", 0, false, false, validationError};
    }

    Emit e;
    e.bindTime = true;
    e.bindEmissionOmega = true;
    std::string body;
    emitPiecewise(*expr, e, "p", "vec3<f32>", body);

    EmissionExpressionLayout layout;
    layout.structure = std::move(body);
    layout.parameterCount = e.params.size();
    layout.readsOmega = e.readEmissionOmega;
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

ScalarExpressionLayout inspectOccluderLayout(const geom::SdfNode* root) {
    if (!geom::isSdfActive(root)) {
        return ScalarExpressionLayout{"<volume-occluder:absent>", 0, true, ""};
    }
    Emit e;
    (void)emitNode(*root, e);
    ScalarExpressionLayout layout;
    layout.structure = std::move(e.body);
    layout.parameterCount = e.params.size();
    layout.ok = !e.refused;
    layout.error = e.refusal;
    return layout;
}

ParameterBlock collectParams(const geom::SdfNode& root,
                             const geom::FieldNode* fieldNode,
                             const OntoMath::Piecewise* colorExpr,
                             const OntoMath::Piecewise* radianceExpr,
                             const OntoMath::Piecewise* chromaExpr,
                             const OntoMath::Piecewise* angularExpr,
                             const std::vector<Rendering::RadianceSourceBinding>* radianceSources,
                             const OntoMath::Piecewise* densityExpr,
                             DensityInputKind densityKind,
                             const OntoMath::Piecewise* extinctionExpr,
                             const OntoMath::Piecewise* scatteringExpr,
                             const OntoMath::Piecewise* volumeChromaExpr,
                             const OntoMath::Piecewise* phaseExpr,
                             const OntoMath::Piecewise* emissionExpr) {
    Emit e;

    const bool hasAnalyticGrad = (root.op == geom::SdfOp::Leaf &&
                                  root.prim == geom::SdfPrim::Expr &&
                                  root.mathNode &&
                                  isDifferentiableAst(*root.mathNode));

    // Follow compile()'s exact traversal order so parameter indices remain a
    // structural contract. We deliberately do not append kPrimitives/kMarcher
    // or assemble a complete shader module on this value-only path.
    if (hasAnalyticGrad) {
        e.sawExpr = true;
        const std::string off = e.param3(root.offset);
        const std::string lp = e.fresh();
        std::string throwawayBody = "    let " + lp + " = p - " + off + ";\n";
        int nextVar = 0;
        (void)emitMathNodeGrad(*root.mathNode, e, lp, throwawayBody, nextVar);
    } else {
        (void)emitNode(root, e);
    }

    std::string throwaway;
    if (densityKind == DensityInputKind::Authored &&
        densityExpr && !densityExpr->pieces.empty()) {
        const std::string previousTimeExpression = e.timeExpression;
        e.timeExpression = "u.volumeTime.x";
        e.bindTime = true;
        emitPiecewise(*densityExpr, e, "p", "f32", throwaway);
        e.bindTime = false;
        e.timeExpression = previousTimeExpression;
    } else if (densityKind == DensityInputKind::LegacyField &&
               fieldNode && fieldNode->field) {
        // LEGACY ONLY: old callers may still project generic ScalarField
        // mathematics as density. Explicit None must never fall through here.
        if (fieldNode->field->mode == OntoMath::ScalarField::EvaluationMode::AST) {
            emitPiecewise(fieldNode->field->astDefinition, e, "p", "f32", throwaway);
        } else {
            (void)e.param(fieldNode->field->baseDensity);
            (void)e.param(fieldNode->field->frequency);
            (void)e.param(fieldNode->field->amplitude);
        }
    }

    if (extinctionExpr && !extinctionExpr->pieces.empty()) {
        const std::string previousTimeExpression = e.timeExpression;
        e.timeExpression = "u.volumeTime.x";
        e.bindTime = true;
        emitPiecewise(*extinctionExpr, e, "p", "f32", throwaway);
        e.bindTime = false;
        e.timeExpression = previousTimeExpression;
    }

    if (scatteringExpr && !scatteringExpr->pieces.empty()) {
        const std::string previousTimeExpression = e.timeExpression;
        e.timeExpression = "u.volumeTime.x";
        e.bindTime = true;
        emitPiecewise(*scatteringExpr, e, "p", "f32", throwaway);
        e.bindTime = false;
        e.timeExpression = previousTimeExpression;
    }

    if (volumeChromaExpr && !volumeChromaExpr->pieces.empty()) {
        std::string validationError;
        if (!validateVectorPiecewise(*volumeChromaExpr, true, validationError)) {
            e.refuse("volume chroma: " + validationError);
        } else {
            const std::string previousTimeExpression = e.timeExpression;
            e.timeExpression = "u.volumeTime.x";
            e.bindTime = true;
            emitPiecewise(*volumeChromaExpr, e, "p", "vec3<f32>", throwaway);
            e.bindTime = false;
            e.timeExpression = previousTimeExpression;
        }
    }

    if (phaseExpr && !phaseExpr->pieces.empty()) {
        std::string validationError;
        if (!validatePhasePiecewise(*phaseExpr, validationError)) {
            e.refuse("volume phase: " + validationError);
        } else {
            const std::string previousTimeExpression = e.timeExpression;
            e.timeExpression = "u.volumeTime.x";
            e.bindTime = true;
            e.bindPhaseDirections = true;
            emitPiecewise(*phaseExpr, e, "p", "f32", throwaway);
            e.bindPhaseDirections = false;
            e.bindTime = false;
            e.timeExpression = previousTimeExpression;
        }
    }

    if (emissionExpr && !emissionExpr->pieces.empty()) {
        std::string validationError;
        if (!validateEmissionPiecewise(*emissionExpr, validationError)) {
            e.refuse("volume emission: " + validationError);
        } else {
            const std::string previousTimeExpression = e.timeExpression;
            e.timeExpression = "u.volumeTime.x";
            e.bindTime = true;
            e.bindEmissionOmega = true;
            emitPiecewise(*emissionExpr, e, "p", "vec3<f32>", throwaway);
            e.bindEmissionOmega = false;
            e.bindTime = false;
            e.timeExpression = previousTimeExpression;
        }
    }

    if (fieldNode && fieldNode->vectorField) {
        if (fieldNode->vectorField->mode == OntoMath::VectorField::EvaluationMode::AST) {
            emitPiecewise(fieldNode->vectorField->astDefinition, e, "p", "vec3<f32>", throwaway);
        } else {
            (void)e.param(fieldNode->vectorField->baseFlowX);
            (void)e.param(fieldNode->vectorField->baseFlowY);
            (void)e.param(fieldNode->vectorField->baseFlowZ);
            (void)e.param(fieldNode->vectorField->frequency);
            (void)e.param(fieldNode->vectorField->amplitude);
        }
    }

    if (colorExpr && !colorExpr->pieces.empty()) {
        emitPiecewise(*colorExpr, e, "p", "vec3<f32>", throwaway);
    }
    const bool multiSource = radianceSources && radianceSources->size() > 1;
    if (multiSource) {
        for (std::size_t i = 0; i < radianceSources->size(); ++i) {
            const auto& source = (*radianceSources)[i];
            e.timeExpression = "RS[" + std::to_string(i) + "u].time.x";

            if (source.radianceExpr && !source.radianceExpr->pieces.empty()) {
                e.bindTime = true;
                emitPiecewise(*source.radianceExpr, e, "p", "f32", throwaway);
                e.bindTime = false;
            }
            if (source.chromaExpr && !source.chromaExpr->pieces.empty()) {
                std::string validationError;
                if (!validateVectorPiecewise(*source.chromaExpr, true, validationError)) {
                    e.refuse("source[" + std::to_string(i) + "] chroma: " + validationError);
                } else {
                    e.bindTime = true;
                    emitPiecewise(*source.chromaExpr, e, "p", "vec3<f32>", throwaway);
                    e.bindTime = false;
                }
            }
            if (source.angularExpr && !source.angularExpr->pieces.empty()) {
                std::string validationError;
                if (!validateAngularPiecewise(*source.angularExpr, validationError)) {
                    e.refuse("source[" + std::to_string(i) + "] angular: " + validationError);
                } else {
                    e.bindTime = true;
                    e.bindOmega = true;
                    emitPiecewise(*source.angularExpr, e, "p", "f32", throwaway);
                    e.bindOmega = false;
                    e.bindTime = false;
                }
            }
        }
        e.timeExpression = "u.radianceTime.x";
    } else {
        if (radianceExpr && !radianceExpr->pieces.empty()) {
            e.bindTime = true;
            emitPiecewise(*radianceExpr, e, "p", "f32", throwaway);
            e.bindTime = false;
        }
        if (chromaExpr && !chromaExpr->pieces.empty()) {
            std::string validationError;
            if (!validateVectorPiecewise(*chromaExpr, true, validationError)) {
                e.refuse("chroma: " + validationError);
            } else {
                e.bindTime = true;
                emitPiecewise(*chromaExpr, e, "p", "vec3<f32>", throwaway);
                e.bindTime = false;
            }
        }
        if (angularExpr && !angularExpr->pieces.empty()) {
            std::string validationError;
            if (!validateAngularPiecewise(*angularExpr, validationError)) {
                e.refuse("angular: " + validationError);
            } else {
                e.bindTime = true;
                e.bindOmega = true;
                emitPiecewise(*angularExpr, e, "p", "f32", throwaway);
                e.bindOmega = false;
                e.bindTime = false;
            }
        }
    }

    ParameterBlock block;
    block.ok = !e.refused;
    block.error = e.refusal;
    block.values = std::move(e.params);
    if (block.values.empty()) block.values.push_back(0.0f);
    return block;
}

bool eraseWgslSpan(std::string& source,
                   const std::string& beginMarker,
                   const std::string& endMarker) {
    const std::size_t begin = source.find(beginMarker);
    if (begin == std::string::npos) return false;
    const std::size_t end = source.find(endMarker, begin + beginMarker.size());
    if (end == std::string::npos) return false;
    source.erase(begin, end - begin);
    return true;
}

bool buildPrimitiveSource(const CompileOptions& options,
                          std::string& primitives,
                          std::string& error) {
    primitives = kPrimitives;
    if (options.emitRangeTraversal) {
        error.clear();
        return true;
    }

    // NO-PROOF-SHADER keeps the exact instance byte stride but removes proof
    // semantics from WGSL. Four reserved u32 slots occupy the same trailing
    // layout as the production proof fields, so CPU submission remains an
    // apples-to-apples comparator while the shader no longer declares proof
    // state or the range-proof storage binding.
    const std::string proofFields =
        "    // Fixed-depth conservative positive-proof bit grid. A zero bit means\n"
        "    // \"no GPU skip proof; exact authored marching owns this cell.\"\n"
        "    rangeProofWordOffset: u32,\n"
        "    rangeProofWordCount: u32,\n"
        "    rangeTraversalEnabled: u32,\n"
        "    rangeProofDepth: u32,\n";
    const std::string reservedFields =
        "    // Reserved benchmark slots: preserve SdfInstanceData byte stride\n"
        "    // without exposing dormant proof semantics to the shader.\n"
        "    reserved0: u32,\n"
        "    reserved1: u32,\n"
        "    reserved2: u32,\n"
        "    reserved3: u32,\n";
    const std::size_t fieldsAt = primitives.find(proofFields);
    if (fieldsAt == std::string::npos) {
        error = "range-proof primitive-layout seam no longer matches kPrimitives";
        return false;
    }
    primitives.replace(fieldsAt, proofFields.size(), reservedFields);

    const std::string proofBinding =
        "// Positive-outside proof bitmap, packed 32 regular depth-N cells per u32.\n"
        "@group(1) @binding(2) var<storage, read> rangeProofWords: array<u32>;\n";
    const std::size_t bindingAt = primitives.find(proofBinding);
    if (bindingAt == std::string::npos) {
        error = "range-proof storage-binding seam no longer matches kPrimitives";
        return false;
    }
    primitives.erase(bindingAt, proofBinding.size());

    if (primitives.find("rangeProof") != std::string::npos ||
        primitives.find("rangeTraversal") != std::string::npos) {
        error = "NO-PROOF primitive source still exposes proof symbols";
        return false;
    }

    error.clear();
    return true;
}

bool buildMarcherSource(const CompileOptions& options,
                        std::string& marcher,
                        std::string& error) {
    marcher = kMarcher;
    if (options.emitRangeTraversal) {
        error.clear();
        return true;
    }

    // Keep the SdfInstanceData/storage ABI identical across benchmark arms and
    // remove only the executable proof path. That isolates dormant shader cost
    // from CPU submission, buffer-layout, or bind-group differences.
    const bool removedFunctions = eraseWgslSpan(
        marcher,
        "// Generic spatial-Prophetic traversal over a fixed-depth proof bitmap.\n",
        "// Rung 8 exact baseline for the geometry this shader actually owns.\n");
    const bool removedState = eraseWgslSpan(
        marcher,
        "    // When range traversal is active, exact marching owns only the current\n",
        "    for (var i = 0; i < 192; i = i + 1) {\n");
    const bool removedBranch = eraseWgslSpan(
        marcher,
        "        if (inst.rangeTraversalEnabled != 0u &&\n",
        "        // Keep the coordinate at which this iteration's medium sample is\n");

    if (!removedFunctions || !removedState || !removedBranch) {
        error = "range-traversal benchmark seam no longer matches kMarcher";
        return false;
    }
    if (marcher.find("fn rangeCandidate(") != std::string::npos ||
        marcher.find("inst.rangeTraversalEnabled != 0u") != std::string::npos) {
        error = "range-traversal benchmark seam left executable proof code behind";
        return false;
    }

    error.clear();
    return true;
}

Program compileWithOptions(const geom::SdfNode& root,
                const CompileOptions& options,
                const geom::FieldNode* fieldNode,
                const OntoMath::Piecewise* colorExpr,
                const OntoMath::Piecewise* radianceExpr,
                const OntoMath::Piecewise* chromaExpr,
                const OntoMath::Piecewise* angularExpr,
                const std::vector<Rendering::RadianceSourceBinding>* radianceSources,
                const OntoMath::Piecewise* densityExpr,
                DensityInputKind densityKind,
                const OntoMath::Piecewise* extinctionExpr,
                const OntoMath::Piecewise* scatteringExpr,
                const OntoMath::Piecewise* volumeChromaExpr,
                const OntoMath::Piecewise* phaseExpr,
                const OntoMath::Piecewise* emissionExpr) {
    Emit e;

    const bool hasAnalyticGrad = (root.op == geom::SdfOp::Leaf &&
                                  root.prim == geom::SdfPrim::Expr &&
                                  root.mathNode &&
                                  isDifferentiableAst(*root.mathNode));

    std::string evalGradFunc;
    if (hasAnalyticGrad) {
        e.sawExpr = true;
        const std::string off = e.param3(root.offset);
        const std::string lp = e.fresh();
        std::string gradBody = "    let " + lp + " = p - " + off + ";\n";
        int nextVar = 0;
        JetExpr je = emitMathNodeGrad(*root.mathNode, e, lp, gradBody, nextVar);
        evalGradFunc = "\nfn sdfEvalGrad(p: vec3<f32>) -> PerlinJet {\n" +
                       gradBody +
                       "    return PerlinJet(" + je.value + ", " + je.grad + ");\n}\n";
    }

    std::string primitiveSource;
    std::string primitiveError;
    if (!buildPrimitiveSource(options, primitiveSource, primitiveError)) {
        e.refuse(primitiveError);
    }

    Program prog;
    prog.wgsl = primitiveSource;

    if (hasAnalyticGrad) {
        prog.wgsl += evalGradFunc;
        prog.wgsl += "\nfn sdfEval(p: vec3<f32>) -> f32 {\n"
                     "    return sdfEvalGrad(p).value;\n}\n";
        prog.wgsl += "\nfn sdfSampleStep(p: vec3<f32>) -> StepSample {\n"
                     "    let jet = sdfEvalGrad(p);\n"
                     "    return StepSample(jet.value, length(jet.grad));\n}\n";
        prog.wgsl += "\nfn sdfNormal(p: vec3<f32>) -> vec3<f32> {\n"
                     "    let jet = sdfEvalGrad(p);\n"
                     "    let gl = length(jet.grad);\n"
                     "    return select(vec3<f32>(0.0, 1.0, 0.0), jet.grad / gl, gl > 1e-8);\n}\n";
    } else {
        const std::string result = emitNode(root, e);
        prog.wgsl += "\nfn sdfEval(p: vec3<f32>) -> f32 {\n";
        prog.wgsl += e.body;
        prog.wgsl += "    return " + result + ";\n}\n";
        prog.wgsl += "\nfn sdfSampleStep(p: vec3<f32>) -> StepSample {\n"
                     "    let raw = sdfEval(p);\n"
                     "    return StepSample(raw, 0.0);\n}\n";
        prog.wgsl += "\nfn sdfNormal(p: vec3<f32>) -> vec3<f32> {\n"
                     "    let e = 1e-3;\n"
                     "    let k0 = vec3<f32>( 1.0, -1.0, -1.0);\n"
                     "    let k1 = vec3<f32>(-1.0, -1.0,  1.0);\n"
                     "    let k2 = vec3<f32>(-1.0,  1.0, -1.0);\n"
                     "    let k3 = vec3<f32>( 1.0,  1.0,  1.0);\n"
                     "    return normalize(\n"
                     "        k0 * sdfEval(p + k0 * e) +\n"
                     "        k1 * sdfEval(p + k1 * e) +\n"
                     "        k2 * sdfEval(p + k2 * e) +\n"
                     "        k3 * sdfEval(p + k3 * e));\n"
                     "}\n";
    }

    // --- Volumetric V0 Density Compiler ---
    // Explicit volume.density.ast wins. The generic FieldNode scalar path below
    // is retained only as named legacy compatibility until saves migrate.
    prog.wgsl += "\nfn volumeDensityEval(p: vec3<f32>) -> f32 {\n";
    if (densityKind == DensityInputKind::Authored &&
        densityExpr && !densityExpr->pieces.empty()) {
        const std::string previousTimeExpression = e.timeExpression;
        e.timeExpression = "u.volumeTime.x";
        e.bindTime = true;
        prog.wgsl += "    // V0: explicit authored D(p,t)\n";
        emitPiecewise(*densityExpr, e, "p", "f32", prog.wgsl);
        e.bindTime = false;
        e.timeExpression = previousTimeExpression;
    } else if (densityKind == DensityInputKind::LegacyField &&
               fieldNode && fieldNode->field) {
        if (fieldNode->field->mode == OntoMath::ScalarField::EvaluationMode::AST) {
            prog.wgsl += "    // LEGACY density projection from generic field.ast\n";
            emitPiecewise(fieldNode->field->astDefinition, e, "p", "f32", prog.wgsl);
        } else {
            std::string baseDensity = e.param(fieldNode->field->baseDensity);
            std::string freq = e.param(fieldNode->field->frequency);
            std::string amp = e.param(fieldNode->field->amplitude);

            prog.wgsl += "    // LEGACY procedural density projection\n";
            prog.wgsl += "    let rawDensity = " + baseDensity + " + sin(p.x * " + freq + ") * " + amp + ";\n";
            prog.wgsl += "    return max(rawDensity, 0.0);\n";
        }
    } else {
        prog.wgsl += "    return 0.0;\n";
    }
    prog.wgsl += "}\n";

    // --- Volumetric V1 Extinction Compiler ---
    // Absence is the historical compatibility law; presence is sole sigma_t authority.
    prog.wgsl += "\nfn volumeExtinctionEval(p: vec3<f32>, compatibilityDensity: f32) -> f32 {\n";
    if (extinctionExpr && !extinctionExpr->pieces.empty()) {
        const std::string previousTimeExpression = e.timeExpression;
        e.timeExpression = "u.volumeTime.x";
        e.bindTime = true;
        prog.wgsl += "    // V1: explicit authored sigma_t(p,t)\n";
        emitPiecewise(*extinctionExpr, e, "p", "f32", prog.wgsl);
        e.bindTime = false;
        e.timeExpression = previousTimeExpression;
    } else {
        prog.wgsl += "    // V1 compatibility: preserve pre-V1 extinction exactly\n";
        prog.wgsl += "    return compatibilityDensity * 0.5;\n";
    }
    prog.wgsl += "}\n";

    // --- Volumetric V2 Scattering Compiler ---
    // Absence preserves the historical sigma_s = D compatibility law.
    prog.wgsl += "\nfn volumeScatteringEval(p: vec3<f32>, compatibilityDensity: f32) -> f32 {\n";
    if (scatteringExpr && !scatteringExpr->pieces.empty()) {
        const std::string previousTimeExpression = e.timeExpression;
        e.timeExpression = "u.volumeTime.x";
        e.bindTime = true;
        prog.wgsl += "    // V2: explicit authored sigma_s(p,t)\n";
        emitPiecewise(*scatteringExpr, e, "p", "f32", prog.wgsl);
        e.bindTime = false;
        e.timeExpression = previousTimeExpression;
    } else {
        prog.wgsl += "    // V2 compatibility: preserve pre-V2 scattering exactly\n";
        prog.wgsl += "    return compatibilityDensity;\n";
    }
    prog.wgsl += "}\n";

    // --- Volumetric V2 Medium Chroma Compiler ---
    // C_v is medium-owned chroma; it does not alias source/light chi.
    prog.wgsl += "\nfn volumeChromaEval(p: vec3<f32>) -> vec3<f32> {\n";
    if (volumeChromaExpr && !volumeChromaExpr->pieces.empty()) {
        std::string validationError;
        if (!validateVectorPiecewise(*volumeChromaExpr, true, validationError)) {
            e.refuse("volume chroma: " + validationError);
            prog.wgsl += "    return vec3<f32>(0.0);\n";
        } else {
            const std::string previousTimeExpression = e.timeExpression;
            e.timeExpression = "u.volumeTime.x";
            e.bindTime = true;
            prog.wgsl += "    // V2: explicit authored C_v(p,t)\n";
            emitPiecewise(*volumeChromaExpr, e, "p", "vec3<f32>", prog.wgsl);
            e.bindTime = false;
            e.timeExpression = previousTimeExpression;
        }
    } else {
        prog.wgsl += "    // V2 compatibility: neutral white medium chroma\n";
        prog.wgsl += "    return vec3<f32>(1.0);\n";
    }
    prog.wgsl += "}\n";

    // --- Volumetric V3 Phase Compiler ---
    // Phi is medium-owned angular scattering. Absent phase is exact identity 1.
    prog.wgsl += "\nfn volumePhaseEval(p: vec3<f32>, wi: vec3<f32>, wo: vec3<f32>) -> f32 {\n";
    if (phaseExpr && !phaseExpr->pieces.empty()) {
        std::string validationError;
        if (!validatePhasePiecewise(*phaseExpr, validationError)) {
            e.refuse("volume phase: " + validationError);
            prog.wgsl += "    return 0.0;\n";
        } else {
            const std::string previousTimeExpression = e.timeExpression;
            e.timeExpression = "u.volumeTime.x";
            e.bindTime = true;
            e.bindPhaseDirections = true;
            prog.wgsl += "    // V3: explicit authored Phi(p,wi,wo,t)\n";
            emitPiecewise(*phaseExpr, e, "p", "f32", prog.wgsl);
            e.bindPhaseDirections = false;
            e.bindTime = false;
            e.timeExpression = previousTimeExpression;
        }
    } else {
        prog.wgsl += "    // V3 compatibility: exact isotropic identity\n";
        prog.wgsl += "    return 1.0;\n";
    }
    prog.wgsl += "}\n";
    prog.wgsl += "\nconst HAS_AUTHORED_VOLUME_PHASE: bool = ";
    prog.wgsl += (phaseExpr && !phaseExpr->pieces.empty()) ? "true;\n" : "false;\n";
    prog.wgsl += "const VOLUME_PHASE_READS_WI: bool = ";
    prog.wgsl += e.readWi ? "true;\n" : "false;\n";

    // --- Volumetric V4 Self-Emission Compiler ---
    prog.wgsl += "\nfn volumeEmissionEval(p: vec3<f32>, omega: vec3<f32>) -> vec3<f32> {\n";
    if (emissionExpr && !emissionExpr->pieces.empty()) {
        std::string validationError;
        if (!validateEmissionPiecewise(*emissionExpr, validationError)) {
            e.refuse("volume emission: " + validationError);
            prog.wgsl += "    return vec3<f32>(0.0);\n";
        } else {
            const std::string previousTimeExpression = e.timeExpression;
            e.timeExpression = "u.volumeTime.x";
            e.bindTime = true;
            e.bindEmissionOmega = true;
            prog.wgsl += "    // V4: explicit authored E_v(p,omega,t)\n";
            emitPiecewise(*emissionExpr, e, "p", "vec3<f32>", prog.wgsl);
            e.bindEmissionOmega = false;
            e.bindTime = false;
            e.timeExpression = previousTimeExpression;
        }
    } else {
        prog.wgsl += "    return vec3<f32>(0.0);\n";
    }
    prog.wgsl += "}\n";
    prog.wgsl += "\nconst HAS_AUTHORED_VOLUME_EMISSION: bool = ";
    prog.wgsl += (emissionExpr && !emissionExpr->pieces.empty()) ? "true;\n" : "false;\n";
    prog.wgsl += "const VOLUME_EMISSION_READS_OMEGA: bool = ";
    prog.wgsl += e.readEmissionOmega ? "true;\n" : "false;\n";

    // --- Dual-Path Vector Field Compiler ---
    prog.wgsl += "\nfn vectorFieldEval(p: vec3<f32>) -> vec3<f32> {\n";
    if (fieldNode && fieldNode->vectorField) {
        if (fieldNode->vectorField->mode == OntoMath::VectorField::EvaluationMode::AST) {
            prog.wgsl += "    // Path B: AST-Driven evaluation\n";
            emitPiecewise(fieldNode->vectorField->astDefinition, e, "p", "vec3<f32>", prog.wgsl);
        } else {
            std::string baseFlowX = e.param(fieldNode->vectorField->baseFlowX);
            std::string baseFlowY = e.param(fieldNode->vectorField->baseFlowY);
            std::string baseFlowZ = e.param(fieldNode->vectorField->baseFlowZ);
            std::string freq = e.param(fieldNode->vectorField->frequency);
            std::string amp = e.param(fieldNode->vectorField->amplitude);
            
            prog.wgsl += "    // Path A: Hardcoded procedural evaluation\n";
            prog.wgsl += "    let rawFlowX = " + baseFlowX + " + sin(p.y * " + freq + ") * " + amp + ";\n";
            prog.wgsl += "    let rawFlowY = " + baseFlowY + " + cos(p.z * " + freq + ") * " + amp + ";\n";
            prog.wgsl += "    let rawFlowZ = " + baseFlowZ + " + sin(p.x * " + freq + ") * " + amp + ";\n";
            prog.wgsl += "    return vec3<f32>(rawFlowX, rawFlowY, rawFlowZ);\n";
        }
    } else {
        prog.wgsl += "    return vec3<f32>(0.0);\n";
    }
    prog.wgsl += "}\n";

    std::string colorBody = "";
    if (colorExpr && !colorExpr->pieces.empty()) {
        emitPiecewise(*colorExpr, e, "p", "vec3<f32>", colorBody);
    } else {
        colorBody = "    return instances[g_instIdx].baseColor.xyz;\n";
    }
    prog.wgsl += "\nfn sdfColor(p: vec3<f32>) -> vec3<f32> {\n" + colorBody + "}\n";

    std::string selectedMarcher;
    std::string marcherError;
    if (!buildMarcherSource(options, selectedMarcher, marcherError)) {
        e.refuse(marcherError);
    }

    const bool multiSource = radianceSources && radianceSources->size() > 1;
    if (!multiSource) {
        std::string radianceBody;
        if (radianceExpr && !radianceExpr->pieces.empty()) {
            e.bindTime = true;
            emitPiecewise(*radianceExpr, e, "p", "f32", radianceBody);
            e.bindTime = false;
        } else {
            radianceBody = "    return 1.0;\n";
        }
        prog.wgsl += "\nfn lightRadiance(p: vec3<f32>) -> f32 {\n" + radianceBody + "}\n";

        std::string chromaBody;
        if (chromaExpr && !chromaExpr->pieces.empty()) {
            std::string validationError;
            if (!validateVectorPiecewise(*chromaExpr, true, validationError)) {
                e.refuse("chroma: " + validationError);
            } else {
                e.bindTime = true;
                emitPiecewise(*chromaExpr, e, "p", "vec3<f32>", chromaBody);
                e.bindTime = false;
            }
        } else {
            // Multiplicative identity. EngineRender preserves legacy light.color in
            // the historical light uniforms when chi is absent.
            chromaBody = "    return vec3<f32>(1.0);\n";
        }
        prog.wgsl += "\nfn lightChroma(p: vec3<f32>) -> vec3<f32> {\n" + chromaBody + "}\n";
        prog.wgsl += std::string("\nconst HAS_AUTHORED_CHROMA: bool = ") +
                     ((chromaExpr && !chromaExpr->pieces.empty()) ? "true;\n" : "false;\n");

        std::string angularBody;
        bool angularReadsOmega = false;
        if (angularExpr && !angularExpr->pieces.empty()) {
            std::string validationError;
            if (!validateAngularPiecewise(*angularExpr, validationError)) {
                e.refuse("angular: " + validationError);
            } else {
                e.bindTime = true;
                e.bindOmega = true;
                e.readOmega = false;
                emitPiecewise(*angularExpr, e, "p", "f32", angularBody);
                angularReadsOmega = e.readOmega;
                e.bindOmega = false;
                e.bindTime = false;
            }
        } else {
            angularBody = "    return 1.0;\n";
        }
        prog.wgsl += "\nfn lightAngular(p: vec3<f32>, omega: vec3<f32>) -> f32 {\n" +
                     angularBody + "}\n";
        prog.wgsl += std::string("\nconst HAS_AUTHORED_ANGULAR: bool = ") +
                     ((angularExpr && !angularExpr->pieces.empty()) ? "true;\n" : "false;\n");
        prog.wgsl += std::string("const ANGULAR_READS_OMEGA: bool = ") +
                     (angularReadsOmega ? "true;\n" : "false;\n");
        prog.wgsl += "const SOURCE_DIRECTION_EPS: f32 = " +
                     wgslLiteral(OntoMath::kDirectionEpsilon) + ";\n";

        // Deliberately retain the historical marcher source verbatim in the
        // zero/one-source case. Rung 7 is additive composition, not a rewrite
        // of the already-proven Rungs 3-6 path.
        prog.wgsl += selectedMarcher;
    } else {
        prog.wgsl +=
            "\nstruct RadianceSourceData {\n"
            "    position: vec4<f32>,\n"
            "    ambient: vec4<f32>,\n"
            "    diffuse: vec4<f32>,\n"
            "    specular: vec4<f32>,\n"
            "    coefficients: vec4<f32>,\n"
            "    time: vec4<f32>,\n"
            "    control: vec4<f32>,\n"
            "};\n"
            "@group(0) @binding(2) var<storage, read> RS: array<RadianceSourceData>;\n";

        std::vector<bool> angularReadsOmega;
        angularReadsOmega.reserve(radianceSources->size());

        for (std::size_t i = 0; i < radianceSources->size(); ++i) {
            const auto& source = (*radianceSources)[i];
            const std::string suffix = std::to_string(i);
            e.timeExpression = "RS[" + suffix + "u].time.x";

            std::string radianceBody;
            if (source.radianceExpr && !source.radianceExpr->pieces.empty()) {
                e.bindTime = true;
                emitPiecewise(*source.radianceExpr, e, "p", "f32", radianceBody);
                e.bindTime = false;
            } else {
                radianceBody = "    return 1.0;\n";
            }
            prog.wgsl += "\nfn lightRadiance_" + suffix +
                         "(p: vec3<f32>) -> f32 {\n" + radianceBody + "}\n";

            std::string chromaBody;
            if (source.chromaExpr && !source.chromaExpr->pieces.empty()) {
                std::string validationError;
                if (!validateVectorPiecewise(*source.chromaExpr, true, validationError)) {
                    e.refuse("source[" + suffix + "] chroma: " + validationError);
                } else {
                    e.bindTime = true;
                    emitPiecewise(*source.chromaExpr, e, "p", "vec3<f32>", chromaBody);
                    e.bindTime = false;
                }
            } else {
                chromaBody = "    return vec3<f32>(1.0);\n";
            }
            prog.wgsl += "\nfn lightChroma_" + suffix +
                         "(p: vec3<f32>) -> vec3<f32> {\n" + chromaBody + "}\n";

            std::string angularBody;
            bool readsOmega = false;
            if (source.angularExpr && !source.angularExpr->pieces.empty()) {
                std::string validationError;
                if (!validateAngularPiecewise(*source.angularExpr, validationError)) {
                    e.refuse("source[" + suffix + "] angular: " + validationError);
                } else {
                    e.bindTime = true;
                    e.bindOmega = true;
                    e.readOmega = false;
                    emitPiecewise(*source.angularExpr, e, "p", "f32", angularBody);
                    readsOmega = e.readOmega;
                    e.bindOmega = false;
                    e.bindTime = false;
                }
            } else {
                angularBody = "    return 1.0;\n";
            }
            angularReadsOmega.push_back(readsOmega);
            prog.wgsl += "\nfn lightAngular_" + suffix +
                         "(p: vec3<f32>, omega: vec3<f32>) -> f32 {\n" +
                         angularBody + "}\n";
        }
        e.timeExpression = "u.radianceTime.x";
        prog.wgsl += "const SOURCE_DIRECTION_EPS: f32 = " +
                     wgslLiteral(OntoMath::kDirectionEpsilon) + ";\n";

        std::string marcher = selectedMarcher;
        const std::string lightingBegin =
            "    let L = normalize(u.lightPos.xyz - pw);\n";
        const std::string lightingEnd =
            "    let clip = u.viewProj * vec4<f32>(pw, 1.0);\n";
        const std::size_t lightAt = marcher.find(lightingBegin);
        const std::size_t clipAt = marcher.find(lightingEnd, lightAt);
        if (lightAt == std::string::npos || clipAt == std::string::npos) {
            e.refuse("Rung 7 compiler could not find the historical lighting seam");
        } else {
            std::string sum;
            sum += "    let V = normalize(u.eyePos.xyz - pw);\n";
            sum += "    var ambientTerm = vec3<f32>(0.0);\n";
            sum += "    var diffuseTerm = vec3<f32>(0.0);\n";
            sum += "    var specTerm = vec3<f32>(0.0);\n";

            for (std::size_t i = 0; i < radianceSources->size(); ++i) {
                const auto& source = (*radianceSources)[i];
                const std::string s = std::to_string(i);
                sum += "    {\n";
                sum += "        let source = RS[" + s + "u];\n";
                sum += "        if (source.control.x > 0.5) {\n";
                sum += "            let sourceDelta = pw - source.position.xyz;\n";
                sum += "            let sourceDistance = length(sourceDelta);\n";
                sum += "            var Ls = vec3<f32>(0.0);\n";
                sum += "            if (sourceDistance > SOURCE_DIRECTION_EPS) { "
                       "Ls = -sourceDelta / sourceDistance; }\n";
                sum += "            var Hs = V;\n";
                sum += "            let halfVector = Ls + V;\n";
                sum += "            let halfLength = length(halfVector);\n";
                sum += "            if (halfLength > SOURCE_DIRECTION_EPS) { "
                       "Hs = halfVector / halfLength; }\n";
                sum += "            let radialRadiance = max(lightRadiance_" + s +
                       "(sourceDelta), 0.0);\n";
                sum += "            var angularRadiance = 1.0;\n";

                if (source.angularExpr && !source.angularExpr->pieces.empty()) {
                    if (angularReadsOmega[i]) {
                        sum += "            if (sourceDistance > SOURCE_DIRECTION_EPS) {\n";
                        sum += "                angularRadiance = max(lightAngular_" + s +
                               "(sourceDelta, sourceDelta / sourceDistance), 0.0);\n";
                        sum += "            } else { angularRadiance = 0.0; }\n";
                    } else {
                        sum += "            angularRadiance = max(lightAngular_" + s +
                               "(sourceDelta, vec3<f32>(0.0)), 0.0);\n";
                    }
                }

                sum += "            let shapedRadiance = radialRadiance * angularRadiance;\n";
                sum += "            let pathVisibility = sourceVisibility(pf, nf, source.position.xyz);\n";
                sum += "            let directRadiance = shapedRadiance * pathVisibility;\n";
                sum += "            let diff = max(dot(nw, Ls), 0.0);\n";
                sum += "            let specShape = inst.shading.z * "
                       "pow(max(dot(nw, Hs), 0.0), max(inst.shading.w, 1.0)) * "
                       "step(0.0001, diff);\n";

                if (source.chromaExpr && !source.chromaExpr->pieces.empty()) {
                    sum += "            let sourceChroma = lightChroma_" + s +
                           "(sourceDelta);\n";
                    sum += "            let c = source.coefficients;\n";
                    sum += "            let ambientEnvelope = sourceChroma * "
                           "vec3<f32>((c.x * c.y) / 0.2);\n";
                    sum += "            let diffuseEnvelope = sourceChroma * "
                           "vec3<f32>((c.x * c.z) / 0.8);\n";
                    sum += "            let specularEnvelope = sourceChroma * "
                           "vec3<f32>(c.x * c.w);\n";
                } else {
                    sum += "            let ambientEnvelope = source.ambient.rgb / "
                           "vec3<f32>(0.2);\n";
                    sum += "            let diffuseEnvelope = source.diffuse.rgb / "
                           "vec3<f32>(0.8);\n";
                    sum += "            let specularEnvelope = source.specular.rgb;\n";
                }
                sum += "            ambientTerm += inst.shading.x * ambientEnvelope;\n";
                sum += "            diffuseTerm += inst.shading.y * diffuseEnvelope * "
                       "diff * directRadiance;\n";
                sum += "            specTerm += specularEnvelope * specShape * "
                       "directRadiance;\n";
                sum += "        }\n";
                sum += "    }\n";
            }
            marcher.replace(lightAt, clipAt - lightAt, sum);
        }

        prog.wgsl += marcher;
    }
    prog.params = std::move(e.params);
    prog.needsGradientStep = e.sawExpr;

    if (e.refused) {
        prog.ok = false;
        prog.error = e.refusal;
        prog.wgsl = "// REFUSED: " + e.refusal + "\n";
    }

    // A storage array of length zero is invalid, and a field with no parameters at
    // all is possible (a bare degenerate tree). One unused float keeps the binding
    // legal without the shader having to know.
    if (prog.params.empty()) prog.params.push_back(0.0f);
    return prog;
}

Program compile(const geom::SdfNode& root,
                const geom::FieldNode* fieldNode,
                const OntoMath::Piecewise* colorExpr,
                const OntoMath::Piecewise* radianceExpr,
                const OntoMath::Piecewise* chromaExpr,
                const OntoMath::Piecewise* angularExpr,
                const std::vector<Rendering::RadianceSourceBinding>* radianceSources,
                const OntoMath::Piecewise* densityExpr,
                DensityInputKind densityKind,
                const OntoMath::Piecewise* extinctionExpr,
                const OntoMath::Piecewise* scatteringExpr,
                const OntoMath::Piecewise* volumeChromaExpr,
                const OntoMath::Piecewise* phaseExpr,
                const OntoMath::Piecewise* emissionExpr) {
    return compileWithOptions(
        root, CompileOptions{}, fieldNode, colorExpr, radianceExpr, chromaExpr,
        angularExpr, radianceSources, densityExpr, densityKind, extinctionExpr,
        scatteringExpr, volumeChromaExpr, phaseExpr, emissionExpr);
}

ParameterBlock collectVolumeParams(const OntoMath::Piecewise* densityExpr,
                                   const OntoMath::Piecewise* extinctionExpr,
                                   const OntoMath::Piecewise* scatteringExpr,
                                   const OntoMath::Piecewise* volumeChromaExpr,
                                   const OntoMath::Piecewise* phaseExpr,
                                   const OntoMath::Piecewise* emissionExpr,
                                   const geom::SdfNode* occluderSdf,
                                   const OntoMath::Piecewise* lightRadianceExpr,
                                   const OntoMath::Piecewise* lightChromaExpr,
                                   const OntoMath::Piecewise* lightAngularExpr) {
    Emit e;
    e.bindTime = true;
    e.timeExpression = "instances[g_instIdx].time.x";

    std::string throwaway;
    if (densityExpr && !densityExpr->pieces.empty()) {
        emitPiecewise(*densityExpr, e, "p", "f32", throwaway);
    }
    if (extinctionExpr && !extinctionExpr->pieces.empty()) {
        emitPiecewise(*extinctionExpr, e, "p", "f32", throwaway);
    }
    if (scatteringExpr && !scatteringExpr->pieces.empty()) {
        emitPiecewise(*scatteringExpr, e, "p", "f32", throwaway);
    }
    if (volumeChromaExpr && !volumeChromaExpr->pieces.empty()) {
        std::string validationError;
        if (!validateVectorPiecewise(*volumeChromaExpr, true, validationError)) {
            e.refuse("volume chroma: " + validationError);
        } else {
            emitPiecewise(*volumeChromaExpr, e, "p", "vec3<f32>", throwaway);
        }
    }

    if (phaseExpr && !phaseExpr->pieces.empty()) {
        std::string validationError;
        if (!validatePhasePiecewise(*phaseExpr, validationError)) {
            e.refuse("volume phase: " + validationError);
        } else {
            e.bindPhaseDirections = true;
            emitPiecewise(*phaseExpr, e, "p", "f32", throwaway);
            e.bindPhaseDirections = false;
        }
    }

    if (emissionExpr && !emissionExpr->pieces.empty()) {
        std::string validationError;
        if (!validateEmissionPiecewise(*emissionExpr, validationError)) {
            e.refuse("volume emission: " + validationError);
        } else {
            e.bindEmissionOmega = true;
            emitPiecewise(*emissionExpr, e, "p", "vec3<f32>", throwaway);
            e.bindEmissionOmega = false;
        }
    }

    if (geom::isSdfActive(occluderSdf)) {
        (void)emitNode(*occluderSdf, e);
    }

    const std::string mediumTimeExpression = e.timeExpression;
    e.timeExpression = "u.sourceTime.x";
    if (lightRadianceExpr && !lightRadianceExpr->pieces.empty()) {
        emitPiecewise(*lightRadianceExpr, e, "p", "f32", throwaway);
    }
    if (lightChromaExpr && !lightChromaExpr->pieces.empty()) {
        std::string validationError;
        if (!validateVectorPiecewise(*lightChromaExpr, true, validationError)) {
            e.refuse("light chroma: " + validationError);
        } else {
            emitPiecewise(*lightChromaExpr, e, "p", "vec3<f32>", throwaway);
        }
    }
    if (lightAngularExpr && !lightAngularExpr->pieces.empty()) {
        e.bindOmega = true;
        emitPiecewise(*lightAngularExpr, e, "p", "f32", throwaway);
        e.bindOmega = false;
    }
    e.timeExpression = mediumTimeExpression;

    ParameterBlock block;
    block.ok = !e.refused;
    block.error = e.refusal;
    block.values = std::move(e.params);
    if (block.values.empty()) block.values.push_back(0.0f);
    return block;
}

namespace {
const char* kVolumePerlinNoise = R"WGSL(
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
)WGSL";

const char* kSdfPrimitivesMath = R"WGSL(
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
)WGSL";
} // namespace

Program compileVolume(const OntoMath::Piecewise* densityExpr,
                      const OntoMath::Piecewise* extinctionExpr,
                      const OntoMath::Piecewise* scatteringExpr,
                      const OntoMath::Piecewise* volumeChromaExpr,
                      const OntoMath::Piecewise* phaseExpr,
                      const OntoMath::Piecewise* emissionExpr,
                      const geom::SdfNode* occluderSdf,
                      const OntoMath::Piecewise* lightRadianceExpr,
                      const OntoMath::Piecewise* lightChromaExpr,
                      const OntoMath::Piecewise* lightAngularExpr) {
    Emit e;
    e.bindTime = true;
    e.timeExpression = "instances[g_instIdx].time.x";

    Program prog;
    prog.wgsl = kVolumePerlinNoise;
    prog.wgsl += kSdfPrimitivesMath;
    prog.wgsl += R"WGSL(
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
    // x = max shadow steps, y = local volumetric visibility enabled, z/w reserved.
    volumeControl: vec4<f32>,
};

struct VolumeInstanceData {
    origin: vec4<f32>,
    halfExtent: vec4<f32>,
    time: vec4<f32>,
    paramOffset: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
};

struct Params { v: array<f32> };

@group(0) @binding(0) var<uniform> u: VolumeGlobals;
@group(0) @binding(1) var<storage, read> P: Params;
@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;
@group(1) @binding(0) var<storage, read> instances: array<VolumeInstanceData>;

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
)WGSL";

    std::string densityBody;
    if (densityExpr && !densityExpr->pieces.empty()) {
        emitPiecewise(*densityExpr, e, "p", "f32", densityBody);
    } else {
        densityBody = "    return 0.0;\n";
    }
    prog.wgsl += "\nfn volumeDensityEval(p: vec3<f32>) -> f32 {\n" +
                 densityBody + "}\n";

    std::string extinctionBody;
    if (extinctionExpr && !extinctionExpr->pieces.empty()) {
        emitPiecewise(*extinctionExpr, e, "p", "f32", extinctionBody);
    } else {
        extinctionBody =
            "    // V1 compatibility: exact pre-V1 extinction law\n"
            "    return compatibilityDensity * 0.5;\n";
    }
    prog.wgsl +=
        "\nfn volumeExtinctionEval(p: vec3<f32>, compatibilityDensity: f32) -> f32 {\n" +
        extinctionBody + "}\n";

    std::string scatteringBody;
    if (scatteringExpr && !scatteringExpr->pieces.empty()) {
        emitPiecewise(*scatteringExpr, e, "p", "f32", scatteringBody);
    } else {
        scatteringBody =
            "    // V2 compatibility: exact pre-V2 scattering coefficient\n"
            "    return compatibilityDensity;\n";
    }
    prog.wgsl +=
        "\nfn volumeScatteringEval(p: vec3<f32>, compatibilityDensity: f32) -> f32 {\n" +
        scatteringBody + "}\n";

    std::string volumeChromaBody;
    if (volumeChromaExpr && !volumeChromaExpr->pieces.empty()) {
        std::string validationError;
        if (!validateVectorPiecewise(*volumeChromaExpr, true, validationError)) {
            e.refuse("volume chroma: " + validationError);
            volumeChromaBody = "    return vec3<f32>(0.0);\n";
        } else {
            emitPiecewise(*volumeChromaExpr, e, "p", "vec3<f32>", volumeChromaBody);
        }
    } else {
        volumeChromaBody =
            "    // V2 compatibility: neutral white medium chroma\n"
            "    return vec3<f32>(1.0);\n";
    }
    prog.wgsl +=
        "\nfn volumeChromaEval(p: vec3<f32>) -> vec3<f32> {\n" +
        volumeChromaBody + "}\n";

    std::string phaseBody;
    if (phaseExpr && !phaseExpr->pieces.empty()) {
        std::string validationError;
        if (!validatePhasePiecewise(*phaseExpr, validationError)) {
            e.refuse("volume phase: " + validationError);
            phaseBody = "    return 0.0;\n";
        } else {
            e.bindPhaseDirections = true;
            emitPiecewise(*phaseExpr, e, "p", "f32", phaseBody);
            e.bindPhaseDirections = false;
        }
    } else {
        phaseBody =
            "    // V3 compatibility: exact isotropic identity\n"
            "    return 1.0;\n";
    }
    prog.wgsl +=
        "\nfn volumePhaseEval(p: vec3<f32>, wi: vec3<f32>, wo: vec3<f32>) -> f32 {\n" +
        phaseBody + "}\n";
    prog.wgsl += "\nconst HAS_AUTHORED_VOLUME_PHASE: bool = ";
    prog.wgsl += (phaseExpr && !phaseExpr->pieces.empty()) ? "true;\n" : "false;\n";
    prog.wgsl += "const VOLUME_PHASE_READS_WI: bool = ";
    prog.wgsl += e.readWi ? "true;\n" : "false;\n";

    std::string emissionBody;
    if (emissionExpr && !emissionExpr->pieces.empty()) {
        std::string validationError;
        if (!validateEmissionPiecewise(*emissionExpr, validationError)) {
            e.refuse("volume emission: " + validationError);
            emissionBody = "    return vec3<f32>(0.0);\n";
        } else {
            e.bindEmissionOmega = true;
            emitPiecewise(*emissionExpr, e, "p", "vec3<f32>", emissionBody);
            e.bindEmissionOmega = false;
        }
    } else {
        emissionBody = "    return vec3<f32>(0.0);\n";
    }
    prog.wgsl +=
        "\nfn volumeEmissionEval(p: vec3<f32>, omega: vec3<f32>) -> vec3<f32> {\n" +
        emissionBody + "}\n";
    prog.wgsl += "\nconst HAS_AUTHORED_VOLUME_EMISSION: bool = ";
    prog.wgsl += (emissionExpr && !emissionExpr->pieces.empty()) ? "true;\n" : "false;\n";
    prog.wgsl += "const VOLUME_EMISSION_READS_OMEGA: bool = ";
    prog.wgsl += e.readEmissionOmega ? "true;\n" : "false;\n";

    // Source rho/chi/alpha retain the admitted source's own Timeline even while
    // they are consumed by participating-medium transport. The surrounding
    // medium evaluators continue to use instances[g_instIdx].time.x.
    const std::string mediumTimeExpression = e.timeExpression;
    e.timeExpression = "u.sourceTime.x";

    std::string lightRadianceBody;
    if (lightRadianceExpr && !lightRadianceExpr->pieces.empty()) {
        emitPiecewise(*lightRadianceExpr, e, "p", "f32", lightRadianceBody);
    } else {
        lightRadianceBody = "    return 1.0;\n";
    }
    prog.wgsl += "\nfn lightRadianceEval(p: vec3<f32>) -> f32 {\n" + lightRadianceBody + "}\n";
    prog.wgsl += "const HAS_AUTHORED_LIGHT_RADIANCE: bool = ";
    prog.wgsl += (lightRadianceExpr && !lightRadianceExpr->pieces.empty()) ? "true;\n" : "false;\n";

    std::string lightChromaBody;
    if (lightChromaExpr && !lightChromaExpr->pieces.empty()) {
        std::string validationError;
        if (!validateVectorPiecewise(*lightChromaExpr, true, validationError)) {
            e.refuse("light chroma: " + validationError);
            lightChromaBody = "    return vec3<f32>(1.0);\n";
        } else {
            emitPiecewise(*lightChromaExpr, e, "p", "vec3<f32>", lightChromaBody);
        }
    } else {
        lightChromaBody = "    return u.incidentColor.xyz;\n";
    }
    prog.wgsl += "\nfn lightChromaEval(p: vec3<f32>) -> vec3<f32> {\n" + lightChromaBody + "}\n";
    prog.wgsl += "const HAS_AUTHORED_LIGHT_CHROMA: bool = ";
    prog.wgsl += (lightChromaExpr && !lightChromaExpr->pieces.empty()) ? "true;\n" : "false;\n";

    std::string lightAngularBody;
    if (lightAngularExpr && !lightAngularExpr->pieces.empty()) {
        e.bindOmega = true;
        emitPiecewise(*lightAngularExpr, e, "p", "f32", lightAngularBody);
        e.bindOmega = false;
    } else {
        lightAngularBody = "    return 1.0;\n";
    }
    prog.wgsl += "\nfn lightAngularEval(p: vec3<f32>, omega: vec3<f32>) -> f32 {\n" + lightAngularBody + "}\n";
    prog.wgsl += "const HAS_AUTHORED_LIGHT_ANGULAR: bool = ";
    prog.wgsl += (lightAngularExpr && !lightAngularExpr->pieces.empty()) ? "true;\n" : "false;\n";
    e.timeExpression = mediumTimeExpression;

    const bool hasOccluder = geom::isSdfActive(occluderSdf);
    if (hasOccluder) {
        std::string occluderResult = emitNode(*occluderSdf, e);
        prog.wgsl += "\nfn volumeSdfEval(p: vec3<f32>) -> f32 {\n" + e.body + "    return " + occluderResult + ";\n}\n";
        e.body.clear();
        prog.wgsl += R"WGSL(
fn volumeSourceVisibility(worldP: vec3<f32>, sourceWorld: vec3<f32>) -> f32 {
    if (u.volumeControl.y < 0.5) { return 1.0; }
    let toLight = sourceWorld - worldP;
    let distToLight = length(toLight);
    if (distToLight <= 1e-4) { return 1.0; }
    let lightDir = toLight / distToLight;
    let inst = instances[g_instIdx];
    var t = 0.05;
    var vis = 1.0;
    let maxT = distToLight - 0.05;
    let maxSteps = i32(u.volumeControl.x);
    let penumbraK = 16.0;
    for (var shadowStep = 0; shadowStep < 24; shadowStep = shadowStep + 1) {
        if (shadowStep >= maxSteps || t >= maxT) { break; }
        let curWorld = worldP + lightDir * t;
        let curLocal = curWorld - inst.origin.xyz;
        let d = volumeSdfEval(curLocal);
        if (d < 0.001) { return 0.0; }
        vis = min(vis, penumbraK * d / t);
        t += max(d, 0.02);
    }
    return clamp(vis, 0.0, 1.0);
}
const HAS_OCCLUDER_SDF: bool = true;
)WGSL";
    } else {
        prog.wgsl += R"WGSL(
fn volumeSdfEval(p: vec3<f32>) -> f32 { return 1e9; }
fn volumeSourceVisibility(worldP: vec3<f32>, sourceWorld: vec3<f32>) -> f32 { return 1.0; }
const HAS_OCCLUDER_SDF: bool = false;
)WGSL";
    }

    prog.wgsl += R"WGSL(
@fragment
fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {
    g_instIdx = in.instIdx;
    let inst = instances[in.instIdx];

    let ro = u.eyePos.xyz;
    let farNdc = vec4<f32>(
        (in.position.x / u.viewport.x) * 2.0 - 1.0,
        (1.0 - in.position.y / u.viewport.y) * 2.0 - 1.0,
        1.0,
        1.0);
    let farH = u.invViewProj * farNdc;
    let farWorld = farH.xyz / farH.w;
    let rd = normalize(farWorld - ro);

    let bounds = rayAabbWorld(
        ro, rd,
        inst.origin.xyz - inst.halfExtent.xyz,
        inst.origin.xyz + inst.halfExtent.xyz);

    var t0 = max(bounds.x, 0.0);
    var t1 = bounds.y;
    if (t1 <= t0) { discard; }

    let maxX = max(i32(u.viewport.x), 1) - 1;
    let maxY = max(i32(u.viewport.y), 1) - 1;
    let px = vec2<i32>(
        clamp(i32(floor(in.position.x)), 0, maxX),
        clamp(i32(floor(in.position.y)), 0, maxY));
    let sceneDepth = textureLoad(sceneDepthTex, px, 0);

    if (sceneDepth < 0.999999) {
        let opaqueWorld = worldAtDepth(in.position.xy, sceneDepth);
        let opaqueT = dot(opaqueWorld - ro, rd);
        t1 = min(t1, max(opaqueT, 0.0));
    }
    if (t1 <= t0) { discard; }

    let span = t1 - t0;
    let stepLength = span / 96.0;
    if (stepLength <= 0.0) { discard; }

    var transmittance = 1.0;
    var volumetricScatter = vec3<f32>(0.0);
    var volumetricEmission = vec3<f32>(0.0);

    for (var i = 0; i < 96; i = i + 1) {
        let sampleT = t0 + (f32(i) + 0.5) * stepLength;
        let worldP = ro + rd * sampleT;
        let p = worldP - inst.origin.xyz;
        let density = max(volumeDensityEval(p), 0.0);

        if (density > 0.0) {
            // V1: authored sigma_t(p,t) is independent from D. If absent,
            // the evaluator preserves the exact pre-V1 compatibility law.
            let extinction = max(volumeExtinctionEval(p, density), 1e-6);
            let oldT = transmittance;
            transmittance *= exp(-extinction * stepLength);

            let scattering = max(volumeScatteringEval(p, density), 0.0);
            let mediumChroma = volumeChromaEval(p);

            var incidentLi = vec3<f32>(1.0);
            var phase = 1.0;

            if (u.incidentSource.w > 0.5) {
                let sourceDelta = worldP - u.incidentSource.xyz;
                let sourceDist = length(sourceDelta);
                let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);

                var radialRad = 1.0;
                if (HAS_AUTHORED_LIGHT_RADIANCE) {
                    radialRad = max(lightRadianceEval(sourceDelta), 0.0);
                }

                var chroma = vec3<f32>(1.0);
                if (HAS_AUTHORED_LIGHT_CHROMA) {
                    chroma = max(lightChromaEval(sourceDelta), vec3<f32>(0.0));
                }

                var angular = 1.0;
                if (HAS_AUTHORED_LIGHT_ANGULAR) {
                    angular = max(lightAngularEval(sourceDelta, lightDir), 0.0);
                }

                let vis = volumeSourceVisibility(worldP, u.incidentSource.xyz);
                incidentLi = chroma * (radialRad * angular * vis);

                if (HAS_AUTHORED_VOLUME_PHASE) {
                    let wiDelta = worldP - u.incidentSource.xyz;
                    let woDelta = ro - worldP;
                    let wiLen = length(wiDelta);
                    let woLen = length(woDelta);
                    let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, 1e-8), wiLen > 1e-8);
                    let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8), woLen > 1e-8);
                    phase = 0.0;
                    if ((!VOLUME_PHASE_READS_WI || wiLen > 1e-8) && woLen > 1e-8) {
                        phase = max(volumePhaseEval(p, wi, wo), 0.0);
                    }
                }
            } else {
                if (HAS_AUTHORED_VOLUME_PHASE) {
                    let woDelta = ro - worldP;
                    let woLen = length(woDelta);
                    let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8), woLen > 1e-8);
                    phase = 0.0;
                    if (!VOLUME_PHASE_READS_WI && woLen > 1e-8) {
                        phase = max(volumePhaseEval(p, vec3<f32>(0.0), wo), 0.0);
                    }
                }
            }

            volumetricScatter +=
                mediumChroma * incidentLi * (scattering / extinction) * phase *
                (oldT - transmittance);

            if (HAS_AUTHORED_VOLUME_EMISSION) {
                let emissionDelta = ro - worldP;
                let emissionLen = length(emissionDelta);
                let emissionOmega = select(
                    vec3<f32>(0.0),
                    emissionDelta / max(emissionLen, 1e-8),
                    emissionLen > 1e-8);
                var emitted = vec3<f32>(0.0);
                if (!VOLUME_EMISSION_READS_OMEGA || emissionLen > 1e-8) {
                    emitted = max(volumeEmissionEval(p, emissionOmega), vec3<f32>(0.0));
                }
                volumetricEmission += emitted * ((oldT - transmittance) / extinction);
            }
        }

        if (transmittance < 0.01) { break; }
    }

    let alpha = 1.0 - transmittance;
    let integratedRgb = volumetricScatter + volumetricEmission;
    let emittedMagnitude = max(max(abs(volumetricEmission.x), abs(volumetricEmission.y)),
                               abs(volumetricEmission.z));
    if (alpha <= 1e-5 && emittedMagnitude <= 1e-6) { discard; }

    // Keep the analytically integrated medium contribution premultiplied.
    // The volume pipeline blends (ONE, ONE_MINUS_SRC_ALPHA), yielding exactly:
    // C_out = C_medium + T * C_scene.
    return vec4<f32>(integratedRgb, alpha);
}
)WGSL";

    prog.params = std::move(e.params);
    prog.needsGradientStep = false;

    if (e.refused) {
        prog.ok = false;
        prog.error = e.refusal;
        prog.wgsl = "// REFUSED: " + e.refusal + "\n";
    }

    if (prog.params.empty()) prog.params.push_back(0.0f);
    return prog;
}


Program compileVolumeSet(const std::vector<VolumeProgramInput>& media) {
    Program out;
    if (media.empty()) {
        out.ok = false;
        out.error = "volume set: no participating media";
        out.wgsl = "// REFUSED: " + out.error + "\n";
        return out;
    }
    if (media.size() == 1) {
        const auto& m = media.front();
        return compileVolume(m.densityExpr, m.extinctionExpr, m.scatteringExpr,
                             m.volumeChromaExpr, m.phaseExpr, m.emissionExpr,
                             m.occluderSdf, m.lightRadianceExpr, m.lightChromaExpr,
                             m.lightAngularExpr);
    }

    auto replaceAll = [](std::string& text,
                         const std::string& from,
                         const std::string& to) {
        std::size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos) {
            text.replace(pos, from.size(), to);
            pos += to.size();
        }
    };

    std::vector<Program> members;
    members.reserve(media.size());

    std::size_t firstEvalStart = std::string::npos;
    for (std::size_t i = 0; i < media.size(); ++i) {
        const auto& m = media[i];
        Program member =
            compileVolume(m.densityExpr, m.extinctionExpr, m.scatteringExpr,
                          m.volumeChromaExpr, m.phaseExpr, m.emissionExpr,
                          m.occluderSdf, m.lightRadianceExpr, m.lightChromaExpr,
                          m.lightAngularExpr);
        if (!member.ok) {
            out.ok = false;
            out.error = "volume set member " + std::to_string(i) + ": " + member.error;
            out.wgsl = "// REFUSED: " + out.error + "\n";
            return out;
        }

        const std::size_t evalStart = member.wgsl.find("\nfn volumeDensityEval");
        const std::size_t fragStart = member.wgsl.find("\n@fragment", evalStart);
        if (evalStart == std::string::npos || fragStart == std::string::npos) {
            out.ok = false;
            out.error =
                "volume set: internal compiler boundary missing for member " +
                std::to_string(i);
            out.wgsl = "// REFUSED: " + out.error + "\n";
            return out;
        }

        if (i == 0) {
            firstEvalStart = evalStart;
            out.wgsl = member.wgsl.substr(0, firstEvalStart);
        }

        std::string evalBlock =
            member.wgsl.substr(evalStart, fragStart - evalStart);
        const std::string suffix = "_" + std::to_string(i);

        // Rename only the per-medium evaluator/feature symbols. Shared structs,
        // bindings, ray helpers and noise live once in the prefix from member 0.
        replaceAll(evalBlock, "volumeDensityEval", "volumeDensityEval" + suffix);
        replaceAll(evalBlock, "volumeExtinctionEval", "volumeExtinctionEval" + suffix);
        replaceAll(evalBlock, "volumeScatteringEval", "volumeScatteringEval" + suffix);
        replaceAll(evalBlock, "volumeChromaEval", "volumeChromaEval" + suffix);
        replaceAll(evalBlock, "volumePhaseEval", "volumePhaseEval" + suffix);
        replaceAll(evalBlock, "HAS_AUTHORED_VOLUME_PHASE",
                   "HAS_AUTHORED_VOLUME_PHASE" + suffix);
        replaceAll(evalBlock, "VOLUME_PHASE_READS_WI",
                   "VOLUME_PHASE_READS_WI" + suffix);
        replaceAll(evalBlock, "volumeEmissionEval", "volumeEmissionEval" + suffix);
        replaceAll(evalBlock, "HAS_AUTHORED_VOLUME_EMISSION",
                   "HAS_AUTHORED_VOLUME_EMISSION" + suffix);
        replaceAll(evalBlock, "VOLUME_EMISSION_READS_OMEGA",
                   "VOLUME_EMISSION_READS_OMEGA" + suffix);
        replaceAll(evalBlock, "volumeSdfEval", "volumeSdfEval" + suffix);
        replaceAll(evalBlock, "volumeSourceVisibility", "volumeSourceVisibility" + suffix);
        replaceAll(evalBlock, "HAS_OCCLUDER_SDF", "HAS_OCCLUDER_SDF" + suffix);
        replaceAll(evalBlock, "lightRadianceEval", "lightRadianceEval" + suffix);
        replaceAll(evalBlock, "HAS_AUTHORED_LIGHT_RADIANCE", "HAS_AUTHORED_LIGHT_RADIANCE" + suffix);
        replaceAll(evalBlock, "lightChromaEval", "lightChromaEval" + suffix);
        replaceAll(evalBlock, "HAS_AUTHORED_LIGHT_CHROMA", "HAS_AUTHORED_LIGHT_CHROMA" + suffix);
        replaceAll(evalBlock, "lightAngularEval", "lightAngularEval" + suffix);
        replaceAll(evalBlock, "HAS_AUTHORED_LIGHT_ANGULAR", "HAS_AUTHORED_LIGHT_ANGULAR" + suffix);

        out.wgsl += evalBlock;
        members.push_back(std::move(member));
    }

    out.wgsl += R"WGSL(
@fragment
fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {
    // V5: instance 0 is a union-bounds header used only by the proxy vertex/ray
    // interval. Projected media begin at instance 1 and keep their own
    // origin/halfExtent/time/paramOffset.
    g_instIdx = 0u;
    let setInst = instances[0];

    let ro = u.eyePos.xyz;
    let farNdc = vec4<f32>(
        (in.position.x / u.viewport.x) * 2.0 - 1.0,
        (1.0 - in.position.y / u.viewport.y) * 2.0 - 1.0,
        1.0,
        1.0);
    let farH = u.invViewProj * farNdc;
    let farWorld = farH.xyz / farH.w;
    let rd = normalize(farWorld - ro);

    let bounds = rayAabbWorld(
        ro, rd,
        setInst.origin.xyz - setInst.halfExtent.xyz,
        setInst.origin.xyz + setInst.halfExtent.xyz);

    var t0 = max(bounds.x, 0.0);
    var t1 = bounds.y;
    if (t1 <= t0) { discard; }

    let maxX = max(i32(u.viewport.x), 1) - 1;
    let maxY = max(i32(u.viewport.y), 1) - 1;
    let px = vec2<i32>(
        clamp(i32(floor(in.position.x)), 0, maxX),
        clamp(i32(floor(in.position.y)), 0, maxY));
    let sceneDepth = textureLoad(sceneDepthTex, px, 0);

    if (sceneDepth < 0.999999) {
        let opaqueWorld = worldAtDepth(in.position.xy, sceneDepth);
        let opaqueT = dot(opaqueWorld - ro, rd);
        t1 = min(t1, max(opaqueT, 0.0));
    }
    if (t1 <= t0) { discard; }

    var transmittance = 1.0;
    var integratedRadiance = vec3<f32>(0.0);
)WGSL";

    // V5 local-quality policy: the union proxy is only a conservative draw/ray
    // envelope. Sampling is driven by the actually occupied ray intervals of
    // the admitted media, so empty distance between disjoint media consumes no
    // sample budget and cannot coarsen an already-existing medium.
    const std::size_t eventCount = media.size() * 2u;
    out.wgsl += "    var mediumEvents: array<f32, " +
                std::to_string(eventCount) + ">;\n";

    for (std::size_t i = 0; i < media.size(); ++i) {
        const std::string n = std::to_string(i);
        const std::string inst = std::to_string(i + 1) + "u";
        const std::string entryIndex = std::to_string(i * 2u) + "u";
        const std::string exitIndex = std::to_string(i * 2u + 1u) + "u";
        out.wgsl +=
            "    {\n"
            "        let eventInst" + n + " = instances[" + inst + "];\n"
            "        let eventBounds" + n + " = rayAabbWorld(\n"
            "            ro, rd,\n"
            "            eventInst" + n + ".origin.xyz - eventInst" + n + ".halfExtent.xyz,\n"
            "            eventInst" + n + ".origin.xyz + eventInst" + n + ".halfExtent.xyz);\n"
            "        var eventEnter" + n + " = max(eventBounds" + n + ".x, t0);\n"
            "        var eventExit" + n + " = min(eventBounds" + n + ".y, t1);\n"
            "        if (eventExit" + n + " <= eventEnter" + n + ") {\n"
            "            eventEnter" + n + " = t1;\n"
            "            eventExit" + n + " = t1;\n"
            "        }\n"
            "        mediumEvents[" + entryIndex + "] = eventEnter" + n + ";\n"
            "        mediumEvents[" + exitIndex + "] = eventExit" + n + ";\n"
            "    }\n";
    }

    out.wgsl +=
        "    for (var eventIdx = 1u; eventIdx < " + std::to_string(eventCount) +
        "u; eventIdx = eventIdx + 1u) {\n"
        "        let key = mediumEvents[eventIdx];\n"
        "        var insertIdx = eventIdx;\n"
        "        loop {\n"
        "            if (insertIdx == 0u) { break; }\n"
        "            let previousIdx = insertIdx - 1u;\n"
        "            if (mediumEvents[previousIdx] <= key) { break; }\n"
        "            mediumEvents[insertIdx] = mediumEvents[previousIdx];\n"
        "            insertIdx = previousIdx;\n"
        "        }\n"
        "        mediumEvents[insertIdx] = key;\n"
        "    }\n"
        "    for (var segmentIdx = 0u; segmentIdx + 1u < " +
        std::to_string(eventCount) +
        "u; segmentIdx = segmentIdx + 1u) {\n"
        "        let segmentStart = max(mediumEvents[segmentIdx], t0);\n"
        "        let segmentEnd = min(mediumEvents[segmentIdx + 1u], t1);\n"
        "        if (segmentEnd <= segmentStart + 1e-6) { continue; }\n"
        "        let segmentMidT = 0.5 * (segmentStart + segmentEnd);\n"
        "        let segmentMidP = ro + rd * segmentMidT;\n"
        "        var segmentOccupied = false;\n";

    for (std::size_t i = 0; i < media.size(); ++i) {
        const std::string n = std::to_string(i);
        const std::string inst = std::to_string(i + 1) + "u";
        out.wgsl +=
            "        {\n"
            "            let segmentInst" + n + " = instances[" + inst + "];\n"
            "            let segmentMin" + n + " = segmentInst" + n +
                ".origin.xyz - segmentInst" + n + ".halfExtent.xyz;\n"
            "            let segmentMax" + n + " = segmentInst" + n +
                ".origin.xyz + segmentInst" + n + ".halfExtent.xyz;\n"
            "            if (all(segmentMidP >= segmentMin" + n + ") && "
                "all(segmentMidP <= segmentMax" + n + ")) {\n"
            "                segmentOccupied = true;\n"
            "            }\n"
            "        }\n";
    }

    out.wgsl += R"WGSL(
        if (!segmentOccupied) { continue; }

        // Preserve the established one-medium local resolution inside each
        // occupied topological interval. Segment boundaries come only from
        // medium entry/exit events; the physical state remains one continuous
        // transmittance/radiance integral across all occupied segments.
        let segmentSpan = segmentEnd - segmentStart;
        let stepLength = segmentSpan / 96.0;
        if (stepLength <= 0.0) { continue; }

        for (var step = 0; step < 96; step = step + 1) {
            let sampleT = segmentStart + (f32(step) + 0.5) * stepLength;
            let worldP = ro + rd * sampleT;

            // Transport directions are properties of this world-space sample,
            // not of medium ordering. Per-medium Phi/E_v evaluators consume
            // them below only when authored structure actually reads them.
            let wiDelta = worldP - u.incidentSource.xyz;
            let woDelta = ro - worldP;
            let wiLen = length(wiDelta);
            let woLen = length(woDelta);
            let wi = select(vec3<f32>(0.0), wiDelta / max(wiLen, 1e-8),
                            u.incidentSource.w > 0.5 && wiLen > 1e-8);
            let wo = select(vec3<f32>(0.0), woDelta / max(woLen, 1e-8),
                            woLen > 1e-8);

            var totalExtinction = 0.0;
            var totalSource = vec3<f32>(0.0);
)WGSL";

    for (std::size_t i = 0; i < media.size(); ++i) {
        const std::string n = std::to_string(i);
        const std::string inst = std::to_string(i + 1) + "u";
        out.wgsl +=
            "        {\n"
            "            let mediumInst" + n + " = instances[" + inst + "];\n"
            "            let mediumMin" + n + " = mediumInst" + n +
                ".origin.xyz - mediumInst" + n + ".halfExtent.xyz;\n"
            "            let mediumMax" + n + " = mediumInst" + n +
                ".origin.xyz + mediumInst" + n + ".halfExtent.xyz;\n"
            "            if (all(worldP >= mediumMin" + n + ") && "
                "all(worldP <= mediumMax" + n + ")) {\n"
            "                g_instIdx = " + inst + ";\n"
            "                let p" + n + " = worldP - mediumInst" + n + ".origin.xyz;\n"
            "                let density" + n + " = max(volumeDensityEval_" + n +
                "(p" + n + "), 0.0);\n"
            "                if (density" + n + " > 0.0) {\n"
            "                    let extinction" + n +
                " = max(volumeExtinctionEval_" + n + "(p" + n + ", density" + n +
                "), 1e-6);\n"
            "                    totalExtinction += extinction" + n + ";\n"
            "                    let scattering" + n +
                " = max(volumeScatteringEval_" + n + "(p" + n + ", density" + n +
                "), 0.0);\n"
            "                    let mediumChroma" + n + " = volumeChromaEval_" + n +
                "(p" + n + ");\n"
            "                    var incidentLi" + n + " = vec3<f32>(1.0);\n"
            "                    if (u.incidentSource.w > 0.5) {\n"
            "                        let sourceDelta = worldP - u.incidentSource.xyz;\n"
            "                        let sourceDist = length(sourceDelta);\n"
            "                        let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);\n"
            "                        var radialRad = 1.0;\n"
            "                        if (HAS_AUTHORED_LIGHT_RADIANCE_" + n + ") {\n"
            "                            radialRad = max(lightRadianceEval_" + n + "(sourceDelta), 0.0);\n"
            "                        }\n"
            "                        var chroma = vec3<f32>(1.0);\n"
            "                        if (HAS_AUTHORED_LIGHT_CHROMA_" + n + ") {\n"
            "                            chroma = max(lightChromaEval_" + n + "(sourceDelta), vec3<f32>(0.0));\n"
            "                        }\n"
            "                        var angular = 1.0;\n"
            "                        if (HAS_AUTHORED_LIGHT_ANGULAR_" + n + ") {\n"
            "                            angular = max(lightAngularEval_" + n + "(sourceDelta, lightDir), 0.0);\n"
            "                        }\n"
            "                        let vis = volumeSourceVisibility_" + n + "(worldP, u.incidentSource.xyz);\n"
            "                        incidentLi" + n + " = chroma * (radialRad * angular * vis);\n"
            "                    }\n"
            "                    var phase" + n + " = 1.0;\n"
            "                    if (HAS_AUTHORED_VOLUME_PHASE_" + n + ") {\n"
            "                        phase" + n + " = 0.0;\n"
            "                        if ((!VOLUME_PHASE_READS_WI_" + n +
                " || (u.incidentSource.w > 0.5 && wiLen > 1e-8)) && woLen > 1e-8) {\n"
            "                            phase" + n + " = max(volumePhaseEval_" + n +
                "(p" + n + ", wi, wo), 0.0);\n"
            "                        }\n"
            "                    }\n"
            "                    var emitted" + n + " = vec3<f32>(0.0);\n"
            "                    if (HAS_AUTHORED_VOLUME_EMISSION_" + n + ") {\n"
            "                        if (!VOLUME_EMISSION_READS_OMEGA_" + n +
                " || woLen > 1e-8) {\n"
            "                            emitted" + n + " = max(volumeEmissionEval_" + n +
                "(p" + n + ", wo), vec3<f32>(0.0));\n"
            "                        }\n"
            "                    }\n"
            "                    totalSource += mediumChroma" + n + " * incidentLi" + n + " * scattering" + n +
                " * phase" + n + " + emitted" + n + ";\n"
            "                }\n"
            "            }\n"
            "        }\n";
    }

    out.wgsl += R"WGSL(
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

            if (transmittance < 0.01) { break; }
        }
        if (transmittance < 0.01) { break; }
    }

    let alpha = 1.0 - transmittance;
    let integratedMagnitude =
        max(max(abs(integratedRadiance.x), abs(integratedRadiance.y)),
            abs(integratedRadiance.z));
    if (alpha <= 1e-5 && integratedMagnitude <= 1e-6) { discard; }

    // One premultiplied answer for the whole admitted medium set:
    // C_out = C_media_set + T_set * C_scene.
    return vec4<f32>(integratedRadiance, alpha);
}
)WGSL";

    out.params.clear();
    for (const auto& member : members) {
        out.params.insert(out.params.end(), member.params.begin(), member.params.end());
    }
    if (out.params.empty()) out.params.push_back(0.0f);
    out.needsGradientStep = false;
    out.ok = true;
    out.error.clear();
    return out;
}


} // namespace sdfwgsl
