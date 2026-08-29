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
    pad0: u32,
    pad1: u32,
    pad2: u32,
};
@group(1) @binding(0) var<storage, read> instances: array<SdfInstanceData>;
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
fn mod289_2(x: vec2<f32>) -> vec2<f32> {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
fn mod289_3(x: vec3<f32>) -> vec3<f32> {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
fn permute3(x: vec3<f32>) -> vec3<f32> {
    return mod289_3(((x * 34.0) + 1.0) * x);
}
fn permute4(x: vec4<f32>) -> vec4<f32> {
    return mod289(((x * 34.0) + 1.0) * x);
}

fn taylorInvSqrt(r: vec4<f32>) -> vec4<f32> {
    return 1.79284291400159 - 0.85373472095314 * r;
}

fn snoise2(v: vec2<f32>) -> f32 {
    let C = vec4<f32>(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
    var i = floor(v + dot(v, C.yy));
    let x0 = v - i + dot(i, C.xx);
    var i1 = select(vec2<f32>(0.0, 1.0), vec2<f32>(1.0, 0.0), x0.x > x0.y);
    var x12 = x0.xyxy + C.xxzz;
    x12.x = x12.x - i1.x;
    x12.y = x12.y - i1.y;
    i = mod289_2(i);
    let p = permute3(permute3(i.y + vec3<f32>(0.0, i1.y, 1.0)) + i.x + vec3<f32>(0.0, i1.x, 1.0));
    var m = max(0.5 - vec3<f32>(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), vec3<f32>(0.0));
    m = m * m;
    m = m * m;
    let x = 2.0 * fract(p * C.www) - 1.0;
    let h = abs(x) - 0.5;
    let ox = floor(x + 0.5);
    let a0 = x - ox;
    m = m * (1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h));
    let g = vec3<f32>(a0.x * x0.x + h.x * x0.y, a0.y * x12.x + h.y * x12.y, a0.z * x12.z + h.z * x12.w);
    return 70.0 * dot(m, g);
}

fn cnoise2(P: vec2<f32>) -> f32 {
    let Pi = floor(P.xyxy) + vec4<f32>(0.0, 0.0, 1.0, 1.0);
    let Pf = fract(P.xyxy) - vec4<f32>(0.0, 0.0, 1.0, 1.0);
    let Pi_mod = mod289(Pi);
    let ix = Pi_mod.xzxz;
    let iy = Pi_mod.yyww;
    let fx = Pf.xzxz;
    let fy = Pf.yyww;

    let i = permute4(permute4(ix) + iy);
    var gx = 2.0 * fract(i * 0.0243902439) - 1.0;
    let gy = abs(gx) - 0.5;
    let tx = floor(gx + 0.5);
    gx = gx - tx;

    var g00 = vec2<f32>(gx.x, gy.x);
    var g10 = vec2<f32>(gx.y, gy.y);
    var g01 = vec2<f32>(gx.z, gy.z);
    var g11 = vec2<f32>(gx.w, gy.w);

    let norm = taylorInvSqrt(vec4<f32>(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 = g00 * norm.x;
    g10 = g10 * norm.y;
    g01 = g01 * norm.z;
    g11 = g11 * norm.w;

    let n00 = dot(g00, vec2<f32>(fx.x, fy.x));
    let n10 = dot(g10, vec2<f32>(fx.y, fy.y));
    let n01 = dot(g01, vec2<f32>(fx.z, fy.z));
    let n11 = dot(g11, vec2<f32>(fx.w, fy.w));

    let fade_xy = fx.xy * fx.xy * fx.xy * (fx.xy * (fx.xy * 6.0 - 15.0) + 10.0);
    let n_x = mix(vec2<f32>(n00, n01), vec2<f32>(n10, n11), fade_xy.x);
    let n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}

fn snoise3(v: vec3<f32>) -> f32 {
    let C = vec2<f32>(1.0 / 6.0, 1.0 / 3.0);
    let D = vec4<f32>(0.0, 0.5, 1.0, 2.0);

    var i = floor(v + dot(v, C.yyy));
    let x0 = v - i + dot(i, C.xxx);

    let g = step(x0.yzx, x0.xyz);
    let l = 1.0 - g;
    let i1 = min(g.xyz, l.zxy);
    let i2 = max(g.xyz, l.zxy);

    let x1 = x0 - i1 + C.xxx;
    let x2 = x0 - i2 + C.yyy;
    let x3 = x0 - D.yyy;

    i = mod289_3(i);
    let p = permute4(permute4(permute4(
                i.z + vec4<f32>(0.0, i1.z, i2.z, 1.0))
              + i.y + vec4<f32>(0.0, i1.y, i2.y, 1.0))
              + i.x + vec4<f32>(0.0, i1.x, i2.x, 1.0));

    let n_ = 0.142857142857;
    let ns = n_ * D.wyz - D.xzx;

    let j = p - 49.0 * floor(p * ns.z * ns.z);

    let x_ = floor(j * ns.z);
    let y_ = floor(j - 7.0 * x_);

    let x = x_ * ns.x + ns.yyyy;
    let y = y_ * ns.x + ns.yyyy;
    let h = 1.0 - abs(x) - abs(y);

    let b0 = vec4<f32>(x.xy, y.xy);
    let b1 = vec4<f32>(x.zw, y.zw);

    let s0 = floor(b0) * 2.0 + 1.0;
    let s1 = floor(b1) * 2.0 + 1.0;
    let sh = -step(h, vec4<f32>(0.0));

    let a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    let a1 = b1.xzyw + s1.xzyw * sh.zzww;

    var p0 = vec3<f32>(a0.xy, h.x);
    var p1 = vec3<f32>(a0.zw, h.y);
    var p2 = vec3<f32>(a1.xy, h.z);
    var p3 = vec3<f32>(a1.zw, h.w);

    let norm = taylorInvSqrt(vec4<f32>(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 = p0 * norm.x;
    p1 = p1 * norm.y;
    p2 = p2 * norm.z;
    p3 = p3 * norm.w;

    var m = max(0.6 - vec4<f32>(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), vec4<f32>(0.0));
    m = m * m;
    return 42.0 * dot(m * m, vec4<f32>(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

fn cnoise3(P: vec3<f32>) -> f32 {
    return snoise3(P);
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
    pad:       vec4<f32>,
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

fn sdfGrad(p: vec3<f32>) -> vec3<f32> {
    let e = 1e-3;
    return vec3<f32>(
        sdfEval(p + vec3<f32>(e, 0.0, 0.0)) - sdfEval(p - vec3<f32>(e, 0.0, 0.0)),
        sdfEval(p + vec3<f32>(0.0, e, 0.0)) - sdfEval(p - vec3<f32>(0.0, e, 0.0)),
        sdfEval(p + vec3<f32>(0.0, 0.0, e)) - sdfEval(p - vec3<f32>(0.0, 0.0, e))) / (2.0 * e);
}

fn sdfNormal(p: vec3<f32>) -> vec3<f32> {
    // Tetrahedral gradient (4 evaluations instead of 6) for isotropic precision and 33% lower cost
    let e = 1e-3;
    let k0 = vec3<f32>( 1.0, -1.0, -1.0);
    let k1 = vec3<f32>(-1.0, -1.0,  1.0);
    let k2 = vec3<f32>(-1.0,  1.0, -1.0);
    let k3 = vec3<f32>( 1.0,  1.0,  1.0);
    let g = k0 * sdfEval(p + k0 * e) +
            k1 * sdfEval(p + k1 * e) +
            k2 * sdfEval(p + k2 * e) +
            k3 * sdfEval(p + k3 * e);
    let l = length(g);
    if (l < 1e-8) { return vec3<f32>(0.0, 1.0, 0.0); }
    return g / l;
}

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

@fragment
fn fs(in: VSOut) -> FSOut {
    g_instIdx = in.instIdx;
    let inst    = instances[in.instIdx];
    let roWorld = u.eyePos.xyz;
    let rdWorld = normalize(in.worldPos - roWorld);
    let ro      = (inst.invModel * vec4<f32>(roWorld, 1.0)).xyz;
    let rd      = normalize((inst.invModel * vec4<f32>(rdWorld, 0.0)).xyz);
    let eps     = inst.misc.y;
    let damping = inst.misc.w;
    let box     = rayAabb(ro, rd, inst.extents.xyz);
    // Miss the cube, or the whole slab is behind the eye.
    if (box.y < box.x || box.y < 0.0) { discard; }

    var t = max(box.x, 0.0);
    let maxDist = min(box.y, inst.misc.z);

    // Heightfield planar leap: If ray starts above the upper extent and points down, leap to top plane in 1 step
    if (damping < 0.5 && rd.y < -1e-4 && (ro.y + rd.y * t) > inst.extents.y) {
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
    
    for (var i = 0; i < 48; i = i + 1) {
        if (t > maxDist) { break; }
        let p = ro + rd * t;
        
        // Analytical early-exit: If ray is above maximum height and traveling upwards, it can never hit ground
        if (damping < 0.5 && rd.y > 1e-4 && p.y > inst.extents.y) {
            break;
        }

        let raw = sdfEval(p);
        
        // Scale epsilon by distance (cone stepping) to prevent infinite steps on the horizon
        let current_eps = max(eps, t * 0.001);

        var d = raw;

        if (damping < 0.5) {
            // Implicit ASTs & heightfield terrains: Coarse-to-fine rapid convergence
            if (d <= 0.0 || abs(d) < current_eps * 2.0) {
                hit = true;
                if (d < 0.0 && prev_d > 0.0 && candidate_step > 0.0) {
                    // Secant root refinement for exact sub-pixel boundary hit
                    let frac = clamp(prev_d / (prev_d - d), 0.0, 1.0);
                    t = (t - candidate_step) + candidate_step * frac;
                }
                break;
            }
            
            // Dynamic terrain step: scale aggressively with distance and clearance
            let s = max(d * 0.85, max(0.4, t * 0.02));
            candidate_step = s;
            prev_d = d;
            t = t + s;
        } else {
            // Exact manifold SDFs: Keinert et al. Over-Relaxation
            if (d < current_eps) { hit = true; break; }
            
            if (d + prev_d < candidate_step) {
                // Overstep detected: roll back candidate leap
                t = t - (omega - 1.0) * prev_d;
                candidate_step = d;
                prev_d = d;
                omega = 1.0;
            } else {
                candidate_step = omega * d;
                prev_d = d;
                omega = 1.4;
            }
            t = t + max(candidate_step, current_eps);
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
    const std::string result = emitNode(root, e);

    // Order matters: WGSL has no forward declarations, so a function must appear
    // before anything that calls it. Primitives, then the generated sdfEval, then
    // the marcher which calls both.
    Program prog;
    prog.wgsl  = kPrimitives;
    prog.wgsl += "\nfn sdfEval(p: vec3<f32>) -> f32 {\n";
    prog.wgsl += e.body;
    prog.wgsl += "    return " + result + ";\n}\n";

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

    // A storage array of length zero is invalid, and a field with no parameters at
    // all is possible (a bare degenerate tree). One unused float keeps the binding
    // legal without the shader having to know.
    if (prog.params.empty()) prog.params.push_back(0.0f);
    return prog;
}

} // namespace sdfwgsl
