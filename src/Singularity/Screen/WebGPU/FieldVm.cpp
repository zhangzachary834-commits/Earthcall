#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace sdfwgsl {
namespace {

bool finiteValue(float v) {
    return std::isfinite(v);
}

bool finiteValue(const glm::vec3& v) {
    return finiteValue(v.x) && finiteValue(v.y) && finiteValue(v.z);
}

float sdSphereVm(const glm::vec3& p, float r) {
    return glm::length(p) - r;
}

float sdBoxVm(const glm::vec3& p, const glm::vec3& b) {
    const glm::vec3 q = glm::abs(p) - b;
    return glm::length(glm::max(q, glm::vec3(0.0f))) +
           std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
}

float sdRoundBoxVm(const glm::vec3& p, const glm::vec3& b, float r) {
    return sdBoxVm(p, glm::max(b - glm::vec3(r), glm::vec3(0.0f))) - r;
}

float sdEllipsoidVm(const glm::vec3& p, const glm::vec3& r) {
    const glm::vec3 rr = glm::max(r, glm::vec3(1e-4f));
    const float k0 = glm::length(p / rr);
    const float k1 = glm::length(p / (rr * rr));
    if (k1 < 1e-8f) return -std::min(rr.x, std::min(rr.y, rr.z));
    return k0 * (k0 - 1.0f) / k1;
}

float sdCylinderVm(const glm::vec3& p, float r, float h) {
    const glm::vec2 d =
        glm::abs(glm::vec2(glm::length(glm::vec2(p.x, p.y)), p.z)) -
        glm::vec2(r, h);
    return std::min(std::max(d.x, d.y), 0.0f) +
           glm::length(glm::max(d, glm::vec2(0.0f)));
}

float sdConeVm(const glm::vec3& p, float r1, float h) {
    const float r2 = 0.0f;
    const glm::vec2 q(glm::length(glm::vec2(p.x, p.y)), p.z);
    const glm::vec2 k1(r2, h);
    const glm::vec2 k2(r2 - r1, 2.0f * h);
    const glm::vec2 ca(
        q.x - std::min(q.x, (q.y < 0.0f) ? r1 : r2),
        std::abs(q.y) - h);
    const float denom = glm::dot(k2, k2);
    if (denom <= 1e-16f) return std::numeric_limits<float>::quiet_NaN();
    const glm::vec2 cb =
        q - k1 + k2 * glm::clamp(glm::dot(k1 - q, k2) / denom, 0.0f, 1.0f);
    const float sign = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f;
    return sign * std::sqrt(std::min(glm::dot(ca, ca), glm::dot(cb, cb)));
}

float sdTorusVm(const glm::vec3& p, float majorR, float minorR) {
    const glm::vec2 q(glm::length(glm::vec2(p.x, p.y)) - majorR, p.z);
    return glm::length(q) - minorR;
}

float smoothMinVm(float a, float b, float k) {
    if (k <= 1e-5f) return std::min(a, b);
    const float h = glm::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return glm::mix(b, a, h) - k * h * (1.0f - h);
}

bool emitFieldVm(const geom::SdfNode& node,
                 FieldVmProgram& out,
                 uint32_t& stackDepth) {
    if (out.instructions.size() >= kFieldVmMaxInstructions) {
        out.ok = false;
        out.error = "field VM instruction budget exceeded";
        return false;
    }

    if (node.op == geom::SdfOp::Leaf) {
        if (!finiteValue(node.dims) || !finiteValue(node.offset) ||
            !finiteValue(node.p0) || !finiteValue(node.p1)) {
            out.ok = false;
            out.error = "field VM refuses non-finite primitive payload";
            return false;
        }

        FieldVmInstruction inst;
        inst.a = glm::vec4(node.offset, 0.0f);
        inst.b = glm::vec4(node.dims, 0.0f);

        switch (node.prim) {
            case geom::SdfPrim::Sphere:
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Sphere);
                inst.a.w = node.dims.x;
                break;
            case geom::SdfPrim::Box:
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Box);
                break;
            case geom::SdfPrim::RoundBox:
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::RoundBox);
                inst.a.w = node.p0;
                break;
            case geom::SdfPrim::Ellipsoid:
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Ellipsoid);
                break;
            case geom::SdfPrim::Cylinder:
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Cylinder);
                inst.a.w = node.dims.x;
                inst.b.x = node.dims.y;
                break;
            case geom::SdfPrim::Cone:
                if (std::fabs(node.dims.y) <= 1e-8f) {
                    out.ok = false;
                    out.error = "field VM refuses degenerate cone half-height";
                    return false;
                }
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Cone);
                inst.a.w = node.dims.x;
                inst.b.x = node.dims.y;
                break;
            case geom::SdfPrim::Torus:
                inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Torus);
                inst.a.w = node.dims.x;
                inst.b.x = node.dims.y;
                break;
            case geom::SdfPrim::Expr:
                out.ok = false;
                out.error =
                    "field VM Expr lowering requires native OntoMath bytecode";
                return false;
            case geom::SdfPrim::Convex:
                out.ok = false;
                out.error =
                    "field VM Convex lowering requires a plane payload table";
                return false;
        }

        out.instructions.push_back(inst);
        ++stackDepth;
        out.maxStackDepth = std::max(out.maxStackDepth, stackDepth);
        if (stackDepth > kFieldVmMaxStackDepth) {
            out.ok = false;
            out.error = "field VM stack budget exceeded";
            return false;
        }
        return true;
    }

    if (node.children.size() < 2 || !node.children[0] || !node.children[1]) {
        out.ok = false;
        out.error = "field VM refuses malformed binary SDF operator";
        return false;
    }

    if (!emitFieldVm(*node.children[0], out, stackDepth) ||
        !emitFieldVm(*node.children[1], out, stackDepth)) {
        return false;
    }
    if (stackDepth < 2) {
        out.ok = false;
        out.error = "field VM internal stack underflow while lowering";
        return false;
    }

    FieldVmInstruction inst;
    switch (node.op) {
        case geom::SdfOp::Morph:
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Morph);
            inst.a.x = node.t;
            break;
        case geom::SdfOp::Union:
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Union);
            break;
        case geom::SdfOp::Intersect:
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Intersect);
            break;
        case geom::SdfOp::Subtract:
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Subtract);
            break;
        case geom::SdfOp::SmoothUnion:
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::SmoothUnion);
            inst.a.x = node.t;
            break;
        case geom::SdfOp::Leaf:
            out.ok = false;
            out.error = "field VM internal leaf/operator mismatch";
            return false;
    }

    if (!finiteValue(inst.a.x)) {
        out.ok = false;
        out.error = "field VM refuses non-finite operator parameter";
        return false;
    }
    if (out.instructions.size() >= kFieldVmMaxInstructions) {
        out.ok = false;
        out.error = "field VM instruction budget exceeded";
        return false;
    }

    out.instructions.push_back(inst);
    --stackDepth; // pop 2, push 1
    return true;
}

} // namespace

FieldVmProgram lowerFieldVm(const geom::SdfNode& root) {
    FieldVmProgram out;
    uint32_t stackDepth = 0;
    if (!emitFieldVm(root, out, stackDepth)) {
        out.instructions.clear();
        out.maxStackDepth = 0;
        return out;
    }
    if (stackDepth != 1 || out.instructions.empty()) {
        out.ok = false;
        out.error = "field VM lowering did not produce one terminal value";
        out.instructions.clear();
        out.maxStackDepth = 0;
    }
    return out;
}

std::optional<float> evaluateFieldVm(const FieldVmProgram& program,
                                     const glm::vec3& p) {
    if (!program.ok || program.instructions.empty() ||
        program.instructions.size() > kFieldVmMaxInstructions ||
        program.maxStackDepth > kFieldVmMaxStackDepth) {
        return std::nullopt;
    }

    std::array<float, kFieldVmMaxStackDepth> stack{};
    uint32_t sp = 0;

    auto push = [&](float value) -> bool {
        if (sp >= kFieldVmMaxStackDepth) return false;
        stack[sp++] = value;
        return true;
    };

    auto applyBinary = [&](FieldVmOpcode op, float parameter) -> bool {
        if (sp < 2) return false;
        const float b = stack[--sp];
        const float a = stack[--sp];
        float value = 0.0f;
        switch (op) {
            case FieldVmOpcode::Morph:
                value = glm::mix(a, b, glm::clamp(parameter, 0.0f, 1.0f));
                break;
            case FieldVmOpcode::Union:
                value = std::min(a, b);
                break;
            case FieldVmOpcode::Intersect:
                value = std::max(a, b);
                break;
            case FieldVmOpcode::Subtract:
                value = std::max(a, -b);
                break;
            case FieldVmOpcode::SmoothUnion:
                value = smoothMinVm(a, b, parameter);
                break;
            default:
                return false;
        }
        return push(value);
    };

    for (const FieldVmInstruction& inst : program.instructions) {
        const auto op = static_cast<FieldVmOpcode>(inst.meta.x);
        const glm::vec3 q = p - glm::vec3(inst.a);

        switch (op) {
            case FieldVmOpcode::Sphere:
                if (!push(sdSphereVm(q, inst.a.w))) return std::nullopt;
                break;
            case FieldVmOpcode::Box:
                if (!push(sdBoxVm(q, glm::vec3(inst.b)))) return std::nullopt;
                break;
            case FieldVmOpcode::RoundBox:
                if (!push(sdRoundBoxVm(q, glm::vec3(inst.b), inst.a.w)))
                    return std::nullopt;
                break;
            case FieldVmOpcode::Ellipsoid:
                if (!push(sdEllipsoidVm(q, glm::vec3(inst.b))))
                    return std::nullopt;
                break;
            case FieldVmOpcode::Cylinder:
                if (!push(sdCylinderVm(q, inst.a.w, inst.b.x)))
                    return std::nullopt;
                break;
            case FieldVmOpcode::Cone:
                if (!push(sdConeVm(q, inst.a.w, inst.b.x)))
                    return std::nullopt;
                break;
            case FieldVmOpcode::Torus:
                if (!push(sdTorusVm(q, inst.a.w, inst.b.x)))
                    return std::nullopt;
                break;

            case FieldVmOpcode::Morph:
            case FieldVmOpcode::Union:
            case FieldVmOpcode::Intersect:
            case FieldVmOpcode::Subtract:
            case FieldVmOpcode::SmoothUnion:
                if (!applyBinary(op, inst.a.x)) return std::nullopt;
                break;

            default:
                return std::nullopt;
        }
    }

    if (sp != 1) return std::nullopt;
    return stack[0];
}

std::string fieldVmMathModule() {
    return R"WGSL(
struct FieldVmInstructionData {
    meta: vec4<u32>,
    a: vec4<f32>,
    b: vec4<f32>,
};

struct FieldVmResult {
    value: f32,
    known: f32,
};

fn fieldVmEval(
    programBase: u32, instructionCount: u32, p: vec3<f32>) -> FieldVmResult {
    if (instructionCount == 0u || instructionCount > 1024u) {
        return FieldVmResult(0.0, 0.0);
    }

    var stack: array<f32, 128>;
    var sp = 0u;

    for (var i = 0u; i < instructionCount; i = i + 1u) {
        let inst = fieldVmInstructions[programBase + i];
        let op = inst.meta.x;
        var value = 0.0;

        if (op >= 1u && op <= 7u) {
            if (sp >= 128u) {
                return FieldVmResult(0.0, 0.0);
            }

            let q = p - inst.a.xyz;
            if (op == 1u) {
                value = sdSphere(q, inst.a.w);
            } else if (op == 2u) {
                value = sdBox(q, inst.b.xyz);
            } else if (op == 3u) {
                value = sdRoundBox(q, inst.b.xyz, inst.a.w);
            } else if (op == 4u) {
                value = sdEllipsoid(q, inst.b.xyz);
            } else if (op == 5u) {
                value = sdCylinder(q, inst.a.w, inst.b.x);
            } else if (op == 6u) {
                value = sdCone(q, inst.a.w, inst.b.x);
            } else {
                value = sdTorus(q, inst.a.w, inst.b.x);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if (op >= 20u && op <= 24u) {
            if (sp < 2u) {
                return FieldVmResult(0.0, 0.0);
            }
            let b = stack[sp - 1u];
            let a = stack[sp - 2u];
            sp = sp - 2u;

            if (op == 20u) {
                value = mix(a, b, clamp(inst.a.x, 0.0, 1.0));
            } else if (op == 21u) {
                value = min(a, b);
            } else if (op == 22u) {
                value = max(a, b);
            } else if (op == 23u) {
                value = max(a, -b);
            } else {
                let k = inst.a.x;
                if (k <= 1e-5) {
                    value = min(a, b);
                } else {
                    let h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
                    value = mix(b, a, h) - k * h * (1.0 - h);
                }
            }

            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        return FieldVmResult(0.0, 0.0);
    }

    if (sp != 1u) {
        return FieldVmResult(0.0, 0.0);
    }
    return FieldVmResult(stack[0], 1.0);
}
)WGSL";
}

} // namespace sdfwgsl
