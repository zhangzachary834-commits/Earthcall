#include "Rendering/WebGPU/SdfWgsl.hpp"

#include "Form/Object/Geometry/Sdf.hpp"
#include "Form/Object/Geometry/FieldNode.hpp"

#include <cstdio>
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
)WGSL";

// Emitter state: appends generated statements and collects the parameter values
// those statements will read back out of the buffer.
struct Emit {
    std::string        body;
    std::vector<float> params;
    int                next = 0; // next `let dN` temporary
    bool               sawExpr = false; // an implicit leaf appeared -> not a distance

    // Record a number and return the WGSL expression that reads it.
    std::string param(float v) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "P.v[%zu]", params.size());
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
                if (n.rpn.empty()) { e.line("let " + out + " = 1e9;"); break; }
                e.line("let " + out + " = " + emitRpn(n.rpn, e, lp) + ";");
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
// Why march from the eye rather than from the rasterised box face: the box is
// drawn with culling off, so which face produced a fragment is not knowable here,
// and starting at the far face would march backwards. Tracing the whole ray costs
// a few large steps through empty space, which is what sphere tracing is good at.
const char* kMarcher = R"WGSL(
struct RU {
    viewProj:  mat4x4<f32>,
    model:     mat4x4<f32>,
    invModel:  mat4x4<f32>,
    baseColor: vec4<f32>,
    lightPos:  vec4<f32>,
    shading:   vec4<f32>,   // x=ambient y=diffuse z=specular w=shininess
    eyePos:    vec4<f32>,
    misc:      vec4<f32>,   // x=extent y=surfaceEps z=maxDist w=exprDamping
};
@group(0) @binding(0) var<uniform> u: RU;
struct Params { v: array<f32> };
@group(0) @binding(1) var<storage, read> P: Params;

struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldPos: vec3<f32>,
};

@vertex
fn vs(@location(0) pos: vec3<f32>) -> VSOut {
    var o: VSOut;
    let world = u.model * vec4<f32>(pos * u.misc.x, 1.0);
    o.clip = u.viewProj * world;
    o.worldPos = world.xyz;
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
    // Central differences, same 1e-3 epsilon the CPU sdfNormal uses.
    let e = 1e-3;
    let g = vec3<f32>(
        sdfEval(p + vec3<f32>(e, 0.0, 0.0)) - sdfEval(p - vec3<f32>(e, 0.0, 0.0)),
        sdfEval(p + vec3<f32>(0.0, e, 0.0)) - sdfEval(p - vec3<f32>(0.0, e, 0.0)),
        sdfEval(p + vec3<f32>(0.0, 0.0, e)) - sdfEval(p - vec3<f32>(0.0, 0.0, e)));
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

@fragment
fn fs(in: VSOut) -> FSOut {
    // Ray in field space, so the marched distances are the field's own.
    let o4 = u.invModel * vec4<f32>(u.eyePos.xyz, 1.0);
    let t4 = u.invModel * vec4<f32>(in.worldPos, 1.0);
    let ro = o4.xyz;
    let rd = normalize(t4.xyz - ro);

    let eps     = u.misc.y;
    let maxDist = u.misc.z;
    let damping = u.misc.w;

    var t = 0.0;
    var hit = false;
    var transmittance = 1.0;
    var volumetric_scatter = 0.0;
    
    for (var i = 0; i < 192; i = i + 1) {
        let p = ro + rd * t;
        let raw = sdfEval(p);
        
        // Volumetric Field Accumulation
        let density = fieldEval(p);
        if (density > 0.0) {
            let step_size = max(raw, eps);
            let extinction = density * 0.5; // Tunable constant
            transmittance *= exp(-extinction * step_size);
            volumetric_scatter += density * step_size * transmittance;
        }

        var d = raw;
        if (damping > 0.5) {
            let g = sdfGrad(p);
            let gl = length(g);
            if (gl > 1e-6) { d = raw / gl; }
        }
        if (d < eps) { hit = true; break; }
        t = t + max(d, eps);
        
        // Early exit if the field is fully opaque
        if (transmittance < 0.01) { break; }
        if (t > maxDist) { break; }
    }
    if (!hit && transmittance > 0.99) { discard; }

    let pf = ro + rd * t;                                // field-space hit
    let pw = (u.model * vec4<f32>(pf, 1.0)).xyz;         // world-space hit
    let nf = sdfNormal(pf);
    // Normals transform by the inverse-transpose; invModel transposed gives it
    // without shipping another matrix.
    let nw = normalize((transpose(u.invModel) * vec4<f32>(nf, 0.0)).xyz);

    let L = normalize(u.lightPos.xyz - pw);
    let V = normalize(u.eyePos.xyz - pw);
    let H = normalize(L + V);
    
    var lit = 0.0;
    var spec = 0.0;
    
    // Only calculate hard surface lighting if we actually hit the SDF boundary
    if (hit) {
        let diff = max(dot(nw, L), 0.0);
        lit  = u.shading.x + u.shading.y * diff;
        spec = u.shading.z * pow(max(dot(nw, H), 0.0), max(u.shading.w, 1.0)) * step(0.0001, diff);
    }

    let clip = u.viewProj * vec4<f32>(pw, 1.0);

    var out: FSOut;
    // Combine hard surface with accumulated volumetric scatter
    let base_rgb = u.baseColor.rgb * lit + vec3<f32>(spec);
    let field_rgb = vec3<f32>(1.0, 1.0, 1.0) * volumetric_scatter; // Could be colored by the field later
    
    out.color = vec4<f32>(mix(base_rgb, field_rgb, 1.0 - transmittance), u.baseColor.a);
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

    Program prog;
    prog.wgsl  = kPrimitives;
    prog.wgsl += "\nfn sdfEval(p: vec3<f32>) -> f32 {\n";
    prog.wgsl += e.body;
    prog.wgsl += "    return " + result + ";\n}\n";

    // --- Dual-Path Field Compiler ---
    prog.wgsl += "\nfn fieldEval(p: vec3<f32>) -> f32 {\n";
    if (fieldNode && fieldNode->field) {
        // Here we branch on Path A vs Path B based on Law Integration logic.
        // For now, we deploy Path A (Hardcoded procedural fields driven by variables).
        // Path B (AST-Driven) would walk `fieldNode->field->astDefinition` and emit WGSL.
        
        std::string baseDensity = e.param(fieldNode->field->baseDensity);
        std::string freq = e.param(fieldNode->field->frequency);
        std::string amp = e.param(fieldNode->field->amplitude);
        
        prog.wgsl += "    // Path A: Hardcoded procedural evaluation\n";
        prog.wgsl += "    let rawDensity = " + baseDensity + " + sin(p.x * " + freq + ") * " + amp + ";\n";
        prog.wgsl += "    return max(rawDensity, 0.0);\n";
    } else {
        prog.wgsl += "    return 0.0;\n";
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
