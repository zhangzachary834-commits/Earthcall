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
};
@group(1) @binding(0) var<storage, read> instances: array<SdfInstanceData>;
// (hMin, hMax) per cell, conservative -- see geom::computeHeightGrid.
@group(1) @binding(1) var<storage, read> heightCells: array<vec2<f32>>;
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
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos) {
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
    e.refuse("a field expression names the variable '" + var +
             "', which has no binding in a shader; only the ambient point "
             "(p, x, y, z) is bound here");
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
    std::string inVar = (pw.inputVariable == "x") ? (pt + ".x") : (pw.inputVariable == "y") ? (pt + ".y") : (pw.inputVariable == "z") ? (pt + ".z") : "0.0";
    
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
    viewProj:  mat4x4<f32>,
    lightPos:  vec4<f32>,
    eyePos:    vec4<f32>,
    // x = distance to the camera's far plane, in WORLD units. Everything past it
    // is clipped before it is ever seen, so marching there is work with no
    // possible output. Unlike a hardcoded horizon constant this is not a quality
    // setting -- it cannot remove anything a Person could have seen.
    limits:    vec4<f32>,
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
    // Clamp to far plane so proxy geometry is never lost to far-plane clipping.
    o.clip.z = min(o.clip.z, o.clip.w * 0.999999);
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
// fieldEval is emitted before this block.

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
        if (!(yHi < cell.x - 1e-4 || yLo > cell.y + 1e-4)) {
            return vec2<f32>(t, 1.0); // candidate cell -- hand off to the fine marcher
        }

        t = tCellExit;
        if (tMaxX < tMaxZ) { ix = ix + stepX; } else { iz = iz + stepZ; }
    }
    return vec2<f32>(t, 1.0); // guard exhausted: fail open, never an unverified miss
}

@fragment
fn fs(in: VSOut) -> FSOut {
    g_instIdx = in.instIdx;
    let inst    = instances[in.instIdx];
    let roWorld = u.eyePos.xyz;
    let rdWorld = normalize(in.worldPos - roWorld);
    let ro      = (inst.invModel * vec4<f32>(roWorld, 1.0)).xyz;
    let rdField = (inst.invModel * vec4<f32>(rdWorld, 0.0)).xyz;
    let rd      = normalize(rdField);
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
    var volumetric_scatter = 0.0;
    var first_hit_t = -1.0;
    
    // Enhanced Sphere Tracing (Over-Relaxation) state:
    var omega = select(1.0, 1.4, damping > 0.5);
    var prev_d = 1e10;
    var candidate_step = 0.0;
    
    for (var i = 0; i < 192; i = i + 1) {
        if (t > maxDist) { break; }
        let p = ro + rd * t;
        
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
            t = t + candidate_step;
        }
        
        // Volumetric Field Accumulation
        let density = fieldEval(p);
        if (density > 0.0) {
            if (first_hit_t < 0.0) { first_hit_t = t; }
            let step_size = max(abs(d), current_eps); // Optical depth uses absolute distance to next bound or small step
            let extinction = max(density * 0.5, 1e-6); // Tunable constant
            
            let old_t = transmittance;
            transmittance *= exp(-extinction * step_size);
            
            // Analytical integration prevents double attenuation across large steps
            volumetric_scatter += (density / extinction) * (old_t - transmittance);
        }
        
        // Early exit if the field is fully opaque or ray exits the bounded volume
        if (transmittance < 0.01) { break; }
        if (t > maxDist) { break; }
    }
    if (!hit && transmittance > 0.99) { discard; }

    var out: FSOut;
    
    if (!hit) {
        // Resolve depth/normal garbage when early-exiting (volumetric only, no hard surface hit)
        // Set depth to the first volumetric hit so it occludes correctly
        let final_alpha = 1.0 - transmittance;
        let c = vec3<f32>(1.0, 1.0, 1.0) * volumetric_scatter;
        if (final_alpha > 0.0) {
            out.color = vec4<f32>(c / final_alpha, final_alpha);
        } else {
            out.color = vec4<f32>(0.0);
        }
        if (first_hit_t >= 0.0) {
            let hit_p = ro + rd * first_hit_t;
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
    
    let diff = max(dot(nw, L), 0.0);
    let lit  = inst.shading.x + inst.shading.y * diff;
    let spec = inst.shading.z * pow(max(dot(nw, H), 0.0), max(inst.shading.w, 1.0)) * step(0.0001, diff);

    let clip = u.viewProj * vec4<f32>(pw, 1.0);

    // Combine hard surface with accumulated volumetric scatter
    let base_rgb = inst.baseColor.rgb * lit + vec3<f32>(spec);
    let field_rgb = vec3<f32>(1.0, 1.0, 1.0) * volumetric_scatter; // Could be colored by the field later
    
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

Program compile(const geom::SdfNode& root, const geom::FieldNode* fieldNode) {
    Emit e;

    const bool hasAnalyticGrad = (root.op == geom::SdfOp::Leaf &&
                                  root.prim == geom::SdfPrim::Expr &&
                                  root.mathNode &&
                                  astContainsNoise(*root.mathNode) &&
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

    Program prog;
    prog.wgsl = kPrimitives;

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

    // --- Dual-Path Field Compiler ---
    prog.wgsl += "\nfn fieldEval(p: vec3<f32>) -> f32 {\n";
    if (fieldNode && fieldNode->field) {
        if (fieldNode->field->mode == OntoMath::ScalarField::EvaluationMode::AST) {
            prog.wgsl += "    // Path B: AST-Driven evaluation\n";
            emitPiecewise(fieldNode->field->astDefinition, e, "p", "f32", prog.wgsl);
        } else {
            std::string baseDensity = e.param(fieldNode->field->baseDensity);
            std::string freq = e.param(fieldNode->field->frequency);
            std::string amp = e.param(fieldNode->field->amplitude);
            
            prog.wgsl += "    // Path A: Hardcoded procedural evaluation\n";
            prog.wgsl += "    let rawDensity = " + baseDensity + " + sin(p.x * " + freq + ") * " + amp + ";\n";
            prog.wgsl += "    return max(rawDensity, 0.0);\n";
        }
    } else {
        prog.wgsl += "    return 0.0;\n";
    }
    prog.wgsl += "}\n";

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

    prog.wgsl += kMarcher;
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

} // namespace sdfwgsl
