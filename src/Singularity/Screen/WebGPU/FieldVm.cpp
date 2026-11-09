#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <glm/gtc/noise.hpp>

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

bool appendVm(FieldVmProgram& out, uint32_t& stackDepth,
              FieldVmInstruction inst, uint32_t pops = 0,
              uint32_t pushes = 1) {
    if (stackDepth < pops) {
        out.ok = false;
        out.error = "field VM internal stack underflow while lowering OntoMath";
        return false;
    }
    if (out.instructions.size() >= kFieldVmMaxInstructions) {
        out.ok = false;
        out.error = "field VM instruction budget exceeded";
        return false;
    }
    const uint32_t next = stackDepth - pops + pushes;
    if (next > kFieldVmMaxStackDepth) {
        out.ok = false;
        out.error = "field VM stack budget exceeded";
        return false;
    }
    out.instructions.push_back(inst);
    stackDepth = next;
    out.maxStackDepth = std::max(out.maxStackDepth, stackDepth);
    return true;
}

bool pushMathConstant(double value, FieldVmProgram& out, uint32_t& stackDepth) {
    if (!std::isfinite(value)) {
        out.ok = false;
        out.error = "field VM refuses non-finite OntoMath literal";
        return false;
    }
    const float f = static_cast<float>(value);
    if (!std::isfinite(f)) {
        out.ok = false;
        out.error = "field VM refuses OntoMath literal outside float32 range";
        return false;
    }
    FieldVmInstruction inst;
    inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::Constant);
    inst.a.x = f;
    return appendVm(out, stackDepth, inst);
}

bool pushCoord(int axis, const glm::vec3& offset,
               FieldVmProgram& out, uint32_t& stackDepth) {
    FieldVmInstruction inst;
    inst.meta.x = static_cast<uint32_t>(
        axis == 0 ? FieldVmOpcode::CoordX
                  : axis == 1 ? FieldVmOpcode::CoordY
                              : FieldVmOpcode::CoordZ);
    inst.a.x = offset[axis];
    return appendVm(out, stackDepth, inst);
}

bool emitMathScalar(const OntoMath::MathNode& node, const glm::vec3& offset,
                    FieldVmProgram& out, uint32_t& stackDepth);
bool emitMathVectorComponent(const OntoMath::MathNode& node, int axis,
                             const glm::vec3& offset,
                             FieldVmProgram& out, uint32_t& stackDepth);

OntoMath::TypeResult fieldMathType(const OntoMath::MathNode& node,
                                   const std::string& path = "field-vm") {
    OntoMath::TypeEnv env{
        {OntoMath::kAmbientPointVar, OntoMath::MathType::vector(3)},
        {"x", OntoMath::ValueKind::Scalar},
        {"y", OntoMath::ValueKind::Scalar},
        {"z", OntoMath::ValueKind::Scalar},
    };
    return node.typeOf(env, path, false);
}

bool appendUnary(FieldVmOpcode op, FieldVmProgram& out, uint32_t& stackDepth) {
    FieldVmInstruction inst;
    inst.meta.x = static_cast<uint32_t>(op);
    return appendVm(out, stackDepth, inst, 1, 1);
}

bool appendBinary(FieldVmOpcode op, FieldVmProgram& out, uint32_t& stackDepth) {
    FieldVmInstruction inst;
    inst.meta.x = static_cast<uint32_t>(op);
    return appendVm(out, stackDepth, inst, 2, 1);
}

bool emitScalarFormVm(const OntoMath::ScalarForm& form, const glm::vec3& offset,
                      FieldVmProgram& out, uint32_t& stackDepth) {
    if (form.terms.empty()) return pushMathConstant(0.0, out, stackDepth);

    bool firstTerm = true;
    for (const auto& term : form.terms) {
        if (!pushMathConstant(term.coefficient, out, stackDepth)) return false;

        for (const auto& [name, exponent] : term.factors) {
            const int axis = name == "x" ? 0 : name == "y" ? 1 : name == "z" ? 2 : -1;
            if (axis < 0) {
                out.ok = false;
                out.error = "field VM Expr ScalarForm names unbound variable '" + name + "'";
                return false;
            }
            if (!pushCoord(axis, offset, out, stackDepth)) return false;
            if (exponent != 1.0) {
                if (!pushMathConstant(exponent, out, stackDepth) ||
                    !appendBinary(FieldVmOpcode::MathRawPow, out, stackDepth)) {
                    return false;
                }
            }
            if (!appendBinary(FieldVmOpcode::MathMul, out, stackDepth)) return false;
        }

        for (const auto& tf : term.trans) {
            const int axis = tf.variable == "x" ? 0 : tf.variable == "y" ? 1 :
                             tf.variable == "z" ? 2 : -1;
            if (axis < 0) {
                out.ok = false;
                out.error = "field VM Expr transcendental names unbound variable '" +
                            tf.variable + "'";
                return false;
            }
            if (!pushCoord(axis, offset, out, stackDepth)) return false;
            if (tf.scale != 1.0) {
                if (!pushMathConstant(tf.scale, out, stackDepth) ||
                    !appendBinary(FieldVmOpcode::MathMul, out, stackDepth)) {
                    return false;
                }
            }
            if (tf.shift != 0.0) {
                if (!pushMathConstant(tf.shift, out, stackDepth) ||
                    !appendBinary(FieldVmOpcode::MathAdd, out, stackDepth)) {
                    return false;
                }
            }
            FieldVmOpcode op = FieldVmOpcode::MathSin;
            switch (tf.kind) {
                case OntoMath::TransFactor::Kind::Sin: op = FieldVmOpcode::MathSin; break;
                case OntoMath::TransFactor::Kind::Cos: op = FieldVmOpcode::MathCos; break;
                case OntoMath::TransFactor::Kind::Exp: op = FieldVmOpcode::MathExp; break;
                case OntoMath::TransFactor::Kind::Ln:  op = FieldVmOpcode::MathLog; break;
            }
            if (!appendUnary(op, out, stackDepth) ||
                !appendBinary(FieldVmOpcode::MathMul, out, stackDepth)) {
                return false;
            }
        }

        if (!firstTerm &&
            !appendBinary(FieldVmOpcode::MathAdd, out, stackDepth)) {
            return false;
        }
        firstTerm = false;
    }
    return true;
}

bool emitMathVectorComponent(const OntoMath::MathNode& node, int axis,
                             const glm::vec3& offset,
                             FieldVmProgram& out, uint32_t& stackDepth) {
    using Op = OntoMath::MathNode::Op;
    if (axis < 0 || axis > 2) {
        out.ok = false;
        out.error = "field VM internal vector component axis is invalid";
        return false;
    }

    const auto child = [&](std::size_t i) -> const OntoMath::MathNode* {
        return i < node.children.size() && node.children[i] ? node.children[i].get() : nullptr;
    };
    const auto requireChild = [&](std::size_t i) -> const OntoMath::MathNode* {
        const auto* c = child(i);
        if (!c) {
            out.ok = false;
            out.error = "field VM Expr node is missing child " + std::to_string(i);
        }
        return c;
    };

    switch (node.op) {
        case Op::ValueLeaf:
            if (node.variableName == OntoMath::kAmbientPointVar)
                return pushCoord(axis, offset, out, stackDepth);
            out.ok = false;
            out.error = "field VM Expr vector names unbound value '" +
                        node.variableName + "'";
            return false;

        case Op::VectorConstruct: {
            const auto* c = requireChild(static_cast<std::size_t>(axis));
            return c && emitMathScalar(*c, offset, out, stackDepth);
        }

        case Op::Add:
        case Op::Sub:
        case Op::Hadamard: {
            const auto* a = requireChild(0);
            const auto* b = requireChild(1);
            if (!a || !b ||
                !emitMathVectorComponent(*a, axis, offset, out, stackDepth) ||
                !emitMathVectorComponent(*b, axis, offset, out, stackDepth)) {
                return false;
            }
            return appendBinary(
                node.op == Op::Add ? FieldVmOpcode::MathAdd :
                node.op == Op::Sub ? FieldVmOpcode::MathSub :
                                     FieldVmOpcode::MathMul,
                out, stackDepth);
        }

        case Op::Union:
        case Op::Intersection: {
            const auto* a = requireChild(0);
            const auto* b = requireChild(1);
            if (!a || !b) return false;
            const auto ta = fieldMathType(*a, "field-vm.minmax[0]");
            const auto tb = fieldMathType(*b, "field-vm.minmax[1]");
            if (!ta || !tb) {
                out.ok = false;
                out.error = "field VM Expr cannot prove vector min/max operand types";
                return false;
            }
            if (ta.kind == OntoMath::ValueKind::Vector) {
                if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth)) return false;
            } else if (!emitMathScalar(*a, offset, out, stackDepth)) return false;
            if (tb.kind == OntoMath::ValueKind::Vector) {
                if (!emitMathVectorComponent(*b, axis, offset, out, stackDepth)) return false;
            } else if (!emitMathScalar(*b, offset, out, stackDepth)) return false;
            return appendBinary(
                node.op == Op::Union ? FieldVmOpcode::MathMin
                                     : FieldVmOpcode::MathMax,
                out, stackDepth);
        }

        case Op::Scale: {
            const auto* a = requireChild(0);
            const auto* b = requireChild(1);
            if (!a || !b) return false;
            const auto ta = fieldMathType(*a, "field-vm.Scale[0]");
            const auto tb = fieldMathType(*b, "field-vm.Scale[1]");
            if (!ta || !tb) {
                out.ok = false;
                out.error = "field VM Expr Scale cannot prove operand types";
                return false;
            }
            if (ta.kind == OntoMath::ValueKind::Vector &&
                tb.kind == OntoMath::ValueKind::Scalar) {
                if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth) ||
                    !emitMathScalar(*b, offset, out, stackDepth)) return false;
            } else if (ta.kind == OntoMath::ValueKind::Scalar &&
                       tb.kind == OntoMath::ValueKind::Vector) {
                if (!emitMathScalar(*a, offset, out, stackDepth) ||
                    !emitMathVectorComponent(*b, axis, offset, out, stackDepth)) return false;
            } else {
                out.ok = false;
                out.error = "field VM Expr vector Scale requires Scalar×Vector or Vector×Scalar";
                return false;
            }
            return appendBinary(FieldVmOpcode::MathMul, out, stackDepth);
        }

        case Op::Cross: {
            const auto* a = requireChild(0);
            const auto* b = requireChild(1);
            if (!a || !b) return false;
            const int a0 = (axis + 1) % 3, a1 = (axis + 2) % 3;
            if (!emitMathVectorComponent(*a, a0, offset, out, stackDepth) ||
                !emitMathVectorComponent(*b, a1, offset, out, stackDepth) ||
                !appendBinary(FieldVmOpcode::MathMul, out, stackDepth) ||
                !emitMathVectorComponent(*a, a1, offset, out, stackDepth) ||
                !emitMathVectorComponent(*b, a0, offset, out, stackDepth) ||
                !appendBinary(FieldVmOpcode::MathMul, out, stackDepth) ||
                !appendBinary(FieldVmOpcode::MathSub, out, stackDepth)) {
                return false;
            }
            return true;
        }

        case Op::Normalize: {
            const auto* a = requireChild(0);
            if (!a) return false;
            for (int c = 0; c < 3; ++c)
                if (!emitMathVectorComponent(*a, c, offset, out, stackDepth)) return false;
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathNormalizeComponent);
            inst.meta.y = static_cast<uint32_t>(axis);
            return appendVm(out, stackDepth, inst, 3, 1);
        }

        case Op::Project: {
            const auto* a = requireChild(0);
            const auto* b = requireChild(1);
            if (!a || !b) return false;
            for (int c = 0; c < 3; ++c)
                if (!emitMathVectorComponent(*a, c, offset, out, stackDepth)) return false;
            for (int c = 0; c < 3; ++c)
                if (!emitMathVectorComponent(*b, c, offset, out, stackDepth)) return false;
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathProjectComponent);
            inst.meta.y = static_cast<uint32_t>(axis);
            return appendVm(out, stackDepth, inst, 6, 1);
        }

        case Op::Div:
        case Op::Pow: {
            const auto* a = requireChild(0);
            const auto* b = requireChild(1);
            if (!a || !b) return false;
            const auto tb = fieldMathType(*b, "field-vm.vector-rhs");
            if (!tb) {
                out.ok = false;
                out.error = "field VM Expr cannot prove vector arithmetic rhs type";
                return false;
            }
            if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth))
                return false;
            if (tb.kind == OntoMath::ValueKind::Vector) {
                if (!emitMathVectorComponent(*b, axis, offset, out, stackDepth))
                    return false;
            } else {
                if (!emitMathScalar(*b, offset, out, stackDepth)) return false;
            }
            return appendBinary(node.op == Op::Div ? FieldVmOpcode::MathDiv
                                                   : FieldVmOpcode::MathPow,
                                out, stackDepth);
        }

        case Op::Abs:
        case Op::Sqrt:
        case Op::Map: {
            const auto* a = requireChild(0);
            if (!a || !emitMathVectorComponent(*a, axis, offset, out, stackDepth))
                return false;
            FieldVmOpcode op = node.op == Op::Abs ? FieldVmOpcode::MathAbs
                               : node.op == Op::Sqrt ? FieldVmOpcode::MathSqrt
                               : node.stringArg == "Round" ? FieldVmOpcode::MathRound
                               : node.stringArg == "Floor" ? FieldVmOpcode::MathFloor
                               : FieldVmOpcode::Constant;
            if (node.op == Op::Map && op == FieldVmOpcode::Constant) {
                out.ok = false;
                out.error = "field VM Expr Map supports only Round/Floor";
                return false;
            }
            return appendUnary(op, out, stackDepth);
        }

        case Op::Clamp: {
            const auto* v = requireChild(0);
            const auto* lo = requireChild(1);
            const auto* hi = requireChild(2);
            if (!v || !lo || !hi ||
                !emitMathVectorComponent(*v, axis, offset, out, stackDepth))
                return false;
            const auto tLo = fieldMathType(*lo, "field-vm.Clamp[1]");
            const auto tHi = fieldMathType(*hi, "field-vm.Clamp[2]");
            if (!tLo || !tHi) {
                out.ok = false;
                out.error = "field VM Expr Clamp cannot prove bound types";
                return false;
            }
            if (tLo.kind == OntoMath::ValueKind::Vector) {
                if (!emitMathVectorComponent(*lo, axis, offset, out, stackDepth)) return false;
            } else if (!emitMathScalar(*lo, offset, out, stackDepth)) return false;
            if (tHi.kind == OntoMath::ValueKind::Vector) {
                if (!emitMathVectorComponent(*hi, axis, offset, out, stackDepth)) return false;
            } else if (!emitMathScalar(*hi, offset, out, stackDepth)) return false;
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathClamp);
            return appendVm(out, stackDepth, inst, 3, 1);
        }

        default:
            out.ok = false;
            out.error = "field VM Expr does not admit vector op " +
                        std::string(OntoMath::mathOpName(node.op));
            return false;
    }
}

bool emitMathScalar(const OntoMath::MathNode& node, const glm::vec3& offset,
                    FieldVmProgram& out, uint32_t& stackDepth) {
    using Op = OntoMath::MathNode::Op;
    const auto child = [&](std::size_t i) -> const OntoMath::MathNode* {
        if (i >= node.children.size() || !node.children[i]) {
            out.ok = false;
            out.error = "field VM Expr node is missing child " + std::to_string(i);
            return nullptr;
        }
        return node.children[i].get();
    };

    switch (node.op) {
        case Op::ScalarLeaf:
            return emitScalarFormVm(node.scalarForm, offset, out, stackDepth);

        case Op::ValueLeaf:
            if (node.variableName == "x") return pushCoord(0, offset, out, stackDepth);
            if (node.variableName == "y") return pushCoord(1, offset, out, stackDepth);
            if (node.variableName == "z") return pushCoord(2, offset, out, stackDepth);
            out.ok = false;
            out.error = "field VM Expr scalar names unbound value '" +
                        node.variableName + "'";
            return false;

        case Op::Component: {
            const auto* a = child(0);
            const int axis = node.stringArg == "x" ? 0 : node.stringArg == "y" ? 1 :
                             node.stringArg == "z" ? 2 : -1;
            if (!a || axis < 0) {
                if (axis < 0) {
                    out.ok = false;
                    out.error = "field VM Expr Component names invalid axis '" +
                                node.stringArg + "'";
                }
                return false;
            }
            return emitMathVectorComponent(*a, axis, offset, out, stackDepth);
        }

        case Op::Add:
        case Op::Sub:
        case Op::Scale:
        case Op::Div:
        case Op::Pow:
        case Op::Union:
        case Op::Intersection: {
            const auto* a = child(0);
            const auto* b = child(1);
            if (!a || !b ||
                !emitMathScalar(*a, offset, out, stackDepth) ||
                !emitMathScalar(*b, offset, out, stackDepth)) return false;
            const FieldVmOpcode op =
                node.op == Op::Add ? FieldVmOpcode::MathAdd :
                node.op == Op::Sub ? FieldVmOpcode::MathSub :
                node.op == Op::Scale ? FieldVmOpcode::MathMul :
                node.op == Op::Div ? FieldVmOpcode::MathDiv :
                node.op == Op::Pow ? FieldVmOpcode::MathPow :
                node.op == Op::Union ? FieldVmOpcode::MathMin :
                                       FieldVmOpcode::MathMax;
            return appendBinary(op, out, stackDepth);
        }

        case Op::Difference: {
            const auto* a = child(0);
            const auto* b = child(1);
            if (!a || !b ||
                !emitMathScalar(*a, offset, out, stackDepth) ||
                !emitMathScalar(*b, offset, out, stackDepth) ||
                !pushMathConstant(-1.0, out, stackDepth) ||
                !appendBinary(FieldVmOpcode::MathMul, out, stackDepth)) return false;
            return appendBinary(FieldVmOpcode::MathMax, out, stackDepth);
        }

        case Op::Dot: {
            const auto* a = child(0);
            const auto* b = child(1);
            if (!a || !b) return false;
            for (int axis = 0; axis < 3; ++axis) {
                if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth) ||
                    !emitMathVectorComponent(*b, axis, offset, out, stackDepth) ||
                    !appendBinary(FieldVmOpcode::MathMul, out, stackDepth))
                    return false;
                if (axis > 0 &&
                    !appendBinary(FieldVmOpcode::MathAdd, out, stackDepth))
                    return false;
            }
            return true;
        }

        case Op::Length: {
            const auto* a = child(0);
            if (!a) return false;
            for (int axis = 0; axis < 3; ++axis)
                if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth)) return false;
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathLength3);
            return appendVm(out, stackDepth, inst, 3, 1);
        }

        case Op::Distance: {
            const auto* a = child(0);
            const auto* b = child(1);
            if (!a || !b) return false;
            for (int axis = 0; axis < 3; ++axis) {
                if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth) ||
                    !emitMathVectorComponent(*b, axis, offset, out, stackDepth) ||
                    !appendBinary(FieldVmOpcode::MathSub, out, stackDepth))
                    return false;
            }
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathLength3);
            return appendVm(out, stackDepth, inst, 3, 1);
        }

        case Op::Abs:
        case Op::Sqrt:
        case Op::Tan: {
            const auto* a = child(0);
            if (!a || !emitMathScalar(*a, offset, out, stackDepth)) return false;
            return appendUnary(node.op == Op::Abs ? FieldVmOpcode::MathAbs :
                               node.op == Op::Sqrt ? FieldVmOpcode::MathSqrt :
                                                   FieldVmOpcode::MathTan,
                               out, stackDepth);
        }

        case Op::Clamp: {
            const auto* v = child(0);
            const auto* lo = child(1);
            const auto* hi = child(2);
            if (!v || !lo || !hi ||
                !emitMathScalar(*v, offset, out, stackDepth) ||
                !emitMathScalar(*lo, offset, out, stackDepth) ||
                !emitMathScalar(*hi, offset, out, stackDepth)) return false;
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathClamp);
            return appendVm(out, stackDepth, inst, 3, 1);
        }

        case Op::Noise: {
            const auto* a = child(0);
            if (!a) return false;
            for (int axis = 0; axis < 3; ++axis)
                if (!emitMathVectorComponent(*a, axis, offset, out, stackDepth)) return false;
            FieldVmInstruction inst;
            inst.meta.x = static_cast<uint32_t>(FieldVmOpcode::MathNoise3);
            return appendVm(out, stackDepth, inst, 3, 1);
        }

        default:
            out.ok = false;
            out.error = "field VM Expr does not admit scalar op " +
                        std::string(OntoMath::mathOpName(node.op));
            return false;
    }
}

bool emitPiecewiseVm(const geom::SdfNode& node,
                             const OntoMath::Piecewise& pw,
                             FieldVmProgram& out,
                             uint32_t& stackDepth) {
    if (pw.pieces.empty()) {
        return pushMathConstant(1e9, out, stackDepth);
    }

    bool hasIntervalPiece = false;
    for (const auto& piece : pw.pieces) {
        hasIntervalPiece = hasIntervalPiece ||
            (!piece.whereLEZero && (piece.hasLo || piece.hasHi));
    }

    int axis = -1;
    if (hasIntervalPiece) {
        axis = pw.inputVariable == "x" ? 0 :
               pw.inputVariable == "y" ? 1 :
               pw.inputVariable == "z" ? 2 : -1;
        if (axis < 0) {
            out.ok = false;
            out.error =
                "field VM Piecewise interval input must be local x, y, or z";
            return false;
        }
    }

    const uint32_t incomingDepth = stackDepth;
    std::vector<std::size_t> endJumps;
    std::optional<std::size_t> pendingFalseBranch;
    bool catchAll = false;

    for (std::size_t i = 0; i < pw.pieces.size(); ++i) {
        if (pendingFalseBranch) {
            out.instructions[*pendingFalseBranch].meta.w =
                static_cast<uint32_t>(out.instructions.size());
            pendingFalseBranch.reset();
        }

        const auto& piece = pw.pieces[i];
        if (piece.guard || piece.call || piece.fold) {
            out.ok = false;
            out.error =
                "field VM Piecewise refuses subject/world guards, calls, and folds";
            return false;
        }
        if (!piece.mathNode) {
            out.ok = false;
            out.error = "field VM Piecewise piece " + std::to_string(i) +
                        " has no authored value";
            return false;
        }

        const auto type =
            fieldMathType(*piece.mathNode, "field-vm.piece[" + std::to_string(i) + "]");
        if (!type) {
            out.ok = false;
            out.error = "field VM Piecewise value type refusal: " +
                        OntoMath::formatTypeDiagnostic(type.diagnostic);
            return false;
        }
        if (type.kind != OntoMath::ValueKind::Scalar &&
            type.kind != OntoMath::ValueKind::ScalarField) {
            out.ok = false;
            out.error = "field VM Piecewise value must be scalar";
            return false;
        }

        const bool pureGuard = piece.whereLEZero != nullptr;
        const bool intervalGate =
            !pureGuard && (piece.hasLo || piece.hasHi);
        const bool gated = pureGuard || intervalGate;

        if (pureGuard) {
            const auto guardType = fieldMathType(
                *piece.whereLEZero,
                "field-vm.piece[" + std::to_string(i) + "].where");
            if (!guardType ||
                (guardType.kind != OntoMath::ValueKind::Scalar &&
                 guardType.kind != OntoMath::ValueKind::ScalarField)) {
                out.ok = false;
                out.error =
                    "field VM Piecewise pure where guard must type-check as scalar";
                return false;
            }

            // Lower the predicate into a completely isolated straight-line
            // math slice. It is data for BranchWhereLEZero, not execution on
            // the enclosing CSG stack.
            FieldVmProgram predicate;
            uint32_t predicateDepth = 0;
            if (!emitMathScalar(
                    *piece.whereLEZero, node.offset,
                    predicate, predicateDepth) ||
                !predicate.ok || predicateDepth != 1u ||
                predicate.instructions.empty()) {
                out.ok = false;
                out.error =
                    "field VM Piecewise pure where guard lowering failed: " +
                    predicate.error;
                return false;
            }
            for (const auto& predInst : predicate.instructions) {
                const uint32_t raw = predInst.meta.x;
                if (raw < static_cast<uint32_t>(FieldVmOpcode::Constant) ||
                    raw > static_cast<uint32_t>(FieldVmOpcode::MathRawPow)) {
                    out.ok = false;
                    out.error =
                        "field VM Piecewise pure where guard escaped straight-line math";
                    return false;
                }
            }
            if (out.instructions.size() + 1u +
                    predicate.instructions.size() >
                kFieldVmMaxInstructions) {
                out.ok = false;
                out.error =
                    "field VM instruction budget exceeded by Piecewise pure guard";
                return false;
            }

            FieldVmInstruction branch;
            branch.meta.x =
                static_cast<uint32_t>(FieldVmOpcode::BranchWhereLEZero);
            branch.meta.y =
                static_cast<uint32_t>(predicate.instructions.size());
            if (!appendVm(out, stackDepth, branch, 0, 0)) return false;
            pendingFalseBranch = out.instructions.size() - 1u;

            out.instructions.insert(
                out.instructions.end(),
                predicate.instructions.begin(),
                predicate.instructions.end());
            out.maxStackDepth =
                std::max(out.maxStackDepth, predicate.maxStackDepth);
        } else if (intervalGate) {
            const float lo = static_cast<float>(piece.lo);
            const float hi = static_cast<float>(piece.hi);
            if ((piece.hasLo && (!std::isfinite(piece.lo) || !std::isfinite(lo))) ||
                (piece.hasHi && (!std::isfinite(piece.hi) || !std::isfinite(hi)))) {
                out.ok = false;
                out.error =
                    "field VM Piecewise refuses non-finite/out-of-f32 interval bound";
                return false;
            }

            FieldVmInstruction branch;
            branch.meta.x =
                static_cast<uint32_t>(FieldVmOpcode::BranchInterval);
            branch.meta.y = static_cast<uint32_t>(axis);
            branch.meta.z =
                (piece.hasLo ? 1u : 0u) |
                (piece.hasHi ? 2u : 0u) |
                (piece.includeLo ? 4u : 0u) |
                (piece.includeHi ? 8u : 0u);
            branch.a.x = node.offset[axis];
            branch.a.y = lo;
            branch.a.z = hi;
            if (!appendVm(out, stackDepth, branch, 0, 0)) return false;
            pendingFalseBranch = out.instructions.size() - 1u;
        }

        if (!emitMathScalar(*piece.mathNode, node.offset, out, stackDepth)) {
            return false;
        }
        if (stackDepth != incomingDepth + 1u) {
            out.ok = false;
            out.error =
                "field VM Piecewise piece did not preserve its incoming stack prefix";
            return false;
        }

        if (!gated) {
            // First unconditional piece is the CPU's permanent catch-all.
            catchAll = true;
            break;
        }

        FieldVmInstruction jump;
        jump.meta.x = static_cast<uint32_t>(FieldVmOpcode::Jump);
        if (!appendVm(out, stackDepth, jump, 0, 0)) return false;
        endJumps.push_back(out.instructions.size() - 1u);

        // The next authored piece is an alternative path, not a sequential
        // continuation of this selected value. Preserve any enclosing CSG
        // operands that were already live before this Piecewise leaf.
        stackDepth = incomingDepth;
    }

    if (pendingFalseBranch) {
        out.instructions[*pendingFalseBranch].meta.w =
            static_cast<uint32_t>(out.instructions.size());
        pendingFalseBranch.reset();
    }

    if (!catchAll) {
        stackDepth = incomingDepth;
        if (!pushMathConstant(1e9, out, stackDepth)) return false;
    }

    const uint32_t endTarget =
        static_cast<uint32_t>(out.instructions.size());
    for (const std::size_t jumpIndex : endJumps) {
        out.instructions[jumpIndex].meta.y = endTarget;
    }
    stackDepth = incomingDepth + 1u;
    return true;
}

bool emitExprVm(const geom::SdfNode& node,
                FieldVmProgram& out, uint32_t& stackDepth) {
    if (node.piecewise) {
        return emitPiecewiseVm(
            node, *node.piecewise, out, stackDepth);
    }

    const OntoMath::MathNode* root = node.mathNode.get();
    if (!root) {
        out.ok = false;
        out.error =
            "field VM Expr has no OntoMath AST; legacy SdfToken RPN is not a VM authority";
        return false;
    }

    const auto type = fieldMathType(*root);
    if (!type) {
        out.ok = false;
        out.error = "field VM Expr type refusal: " +
                    OntoMath::formatTypeDiagnostic(type.diagnostic);
        return false;
    }
    if (type.kind != OntoMath::ValueKind::Scalar &&
        type.kind != OntoMath::ValueKind::ScalarField) {
        out.ok = false;
        out.error = "field VM Expr root must be scalar, got " +
                    std::string(OntoMath::valueKindName(type.kind));
        return false;
    }
    return emitMathScalar(*root, node.offset, out, stackDepth);
}

bool emitConvexVm(const geom::SdfNode& node,
                  FieldVmProgram& out,
                  uint32_t& stackDepth) {
    if (node.planes.empty()) {
        out.ok = false;
        out.error = "field VM Convex requires at least one authored plane";
        return false;
    }

    bool first = true;
    for (std::size_t i = 0; i < node.planes.size(); ++i) {
        const glm::vec4 pl = node.planes[i];
        const glm::vec3 n(pl);
        if (!std::isfinite(pl.x) || !std::isfinite(pl.y) ||
            !std::isfinite(pl.z) || !std::isfinite(pl.w)) {
            out.ok = false;
            out.error = "field VM Convex refuses non-finite plane " +
                        std::to_string(i);
            return false;
        }
        const float nLen = glm::length(n);
        if (!std::isfinite(nLen)) {
            out.ok = false;
            out.error = "field VM Convex refuses non-finite normal length";
            return false;
        }
        // n dot (p - offset) - d, lowered only through existing generic scalar
        // execution. The authored plane vector remains the source of truth.
        if (!pushCoord(0, node.offset, out, stackDepth) ||
            !pushMathConstant(pl.x, out, stackDepth) ||
            !appendBinary(FieldVmOpcode::MathMul, out, stackDepth) ||
            !pushCoord(1, node.offset, out, stackDepth) ||
            !pushMathConstant(pl.y, out, stackDepth) ||
            !appendBinary(FieldVmOpcode::MathMul, out, stackDepth) ||
            !appendBinary(FieldVmOpcode::MathAdd, out, stackDepth) ||
            !pushCoord(2, node.offset, out, stackDepth) ||
            !pushMathConstant(pl.z, out, stackDepth) ||
            !appendBinary(FieldVmOpcode::MathMul, out, stackDepth) ||
            !appendBinary(FieldVmOpcode::MathAdd, out, stackDepth) ||
            !pushMathConstant(pl.w, out, stackDepth) ||
            !appendBinary(FieldVmOpcode::MathSub, out, stackDepth)) {
            return false;
        }
        if (!first &&
            !appendBinary(FieldVmOpcode::MathMax, out, stackDepth)) {
            return false;
        }
        first = false;
    }

    return true;
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
                return emitExprVm(node, out, stackDepth);
            case geom::SdfPrim::Convex:
                return emitConvexVm(node, out, stackDepth);
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

FieldMarchPolicy inspectMarchPolicyNode(const geom::SdfNode& node) {
    FieldMarchPolicy policy;

    if (node.op == geom::SdfOp::Leaf) {
        if (node.prim == geom::SdfPrim::Expr) {
            policy.needsGradientStep = true;
            return policy;
        }
        if (node.prim == geom::SdfPrim::Convex) {
            float maxNormalLength = 0.0f;
            for (std::size_t i = 0; i < node.planes.size(); ++i) {
                const glm::vec4 pl = node.planes[i];
                if (!std::isfinite(pl.x) || !std::isfinite(pl.y) ||
                    !std::isfinite(pl.z) || !std::isfinite(pl.w)) {
                    policy.ok = false;
                    policy.error =
                        "field march policy refuses non-finite Convex plane " +
                        std::to_string(i);
                    return policy;
                }
                const float nLen = glm::length(glm::vec3(pl));
                if (!std::isfinite(nLen)) {
                    policy.ok = false;
                    policy.error =
                        "field march policy refuses non-finite Convex normal length";
                    return policy;
                }
                maxNormalLength = std::max(maxNormalLength, nLen);
            }
            policy.marchLipschitz =
                std::max(policy.marchLipschitz, maxNormalLength);
        }
        return policy;
    }

    // Mirror evalSdf's malformed-binary semantics: fewer than two children
    // delegates to the first child when one exists, otherwise the constant 1.
    if (node.children.size() < 2 || !node.children[0] || !node.children[1]) {
        if (!node.children.empty() && node.children[0]) {
            return inspectMarchPolicyNode(*node.children[0]);
        }
        return policy;
    }

    const FieldMarchPolicy a = inspectMarchPolicyNode(*node.children[0]);
    if (!a.ok) return a;
    const FieldMarchPolicy b = inspectMarchPolicyNode(*node.children[1]);
    if (!b.ok) return b;

    policy.needsGradientStep =
        a.needsGradientStep || b.needsGradientStep;
    policy.marchLipschitz =
        std::max(a.marchLipschitz, b.marchLipschitz);
    return policy;
}

} // namespace

FieldMarchPolicy inspectFieldMarchPolicy(const geom::SdfNode& root) {
    return inspectMarchPolicyNode(root);
}

FieldVmProgram lowerFieldVm(const geom::SdfNode& root) {
    FieldVmProgram out;
    const FieldMarchPolicy policy = inspectFieldMarchPolicy(root);
    if (!policy.ok) {
        out.ok = false;
        out.error = policy.error;
        return out;
    }
    out.needsGradientStep = policy.needsGradientStep;
    out.marchLipschitz = policy.marchLipschitz;

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

namespace {

struct PredicateVmResult {
    bool valid = false;   // bytecode/execution shape is well-formed
    bool defined = false; // authored predicate produced a value (ln domain etc.)
    float value = 0.0f;
};

PredicateVmResult evaluatePredicateVmSlice(
    const std::vector<FieldVmInstruction>& instructions,
    std::size_t begin, std::size_t count, const glm::vec3& p) {

    if (count == 0 || begin > instructions.size() ||
        count > instructions.size() - begin) {
        return {};
    }

    std::array<float, kFieldVmMaxStackDepth> stack{};
    uint32_t sp = 0;
    auto push = [&](float value) -> bool {
        if (sp >= kFieldVmMaxStackDepth) return false;
        stack[sp++] = value; // NaN/inf are authored numeric outcomes here.
        return true;
    };

    for (std::size_t k = 0; k < count; ++k) {
        const FieldVmInstruction& inst = instructions[begin + k];
        const auto op = static_cast<FieldVmOpcode>(inst.meta.x);
        float value = 0.0f;

        if (op == FieldVmOpcode::Constant ||
            op == FieldVmOpcode::CoordX ||
            op == FieldVmOpcode::CoordY ||
            op == FieldVmOpcode::CoordZ) {
            if (op == FieldVmOpcode::Constant) value = inst.a.x;
            else if (op == FieldVmOpcode::CoordX) value = p.x - inst.a.x;
            else if (op == FieldVmOpcode::CoordY) value = p.y - inst.a.x;
            else value = p.z - inst.a.x;
            if (!push(value)) return {};
            continue;
        }

        if (op == FieldVmOpcode::MathAdd ||
            op == FieldVmOpcode::MathSub ||
            op == FieldVmOpcode::MathMul ||
            op == FieldVmOpcode::MathDiv ||
            op == FieldVmOpcode::MathPow ||
            op == FieldVmOpcode::MathRawPow ||
            op == FieldVmOpcode::MathMin ||
            op == FieldVmOpcode::MathMax) {
            if (sp < 2) return {};
            const float b = stack[--sp];
            const float a = stack[--sp];
            if (op == FieldVmOpcode::MathAdd) value = a + b;
            else if (op == FieldVmOpcode::MathSub) value = a - b;
            else if (op == FieldVmOpcode::MathMul) value = a * b;
            else if (op == FieldVmOpcode::MathDiv)
                value = std::abs(b) <
                        static_cast<float>(OntoMath::kDegenerateDivisor)
                    ? 0.0f : a / b;
            else if (op == FieldVmOpcode::MathPow)
                value = (a < 0.0f && std::floor(b) != b)
                    ? 0.0f : std::pow(a, b);
            else if (op == FieldVmOpcode::MathRawPow)
                value = std::pow(a, b);
            else if (op == FieldVmOpcode::MathMin)
                value = std::min(a, b);
            else
                value = std::max(a, b);
            if (!push(value)) return {};
            continue;
        }

        if (op == FieldVmOpcode::MathAbs ||
            op == FieldVmOpcode::MathSqrt ||
            op == FieldVmOpcode::MathTan ||
            op == FieldVmOpcode::MathSin ||
            op == FieldVmOpcode::MathCos ||
            op == FieldVmOpcode::MathExp ||
            op == FieldVmOpcode::MathLog ||
            op == FieldVmOpcode::MathRound ||
            op == FieldVmOpcode::MathFloor) {
            if (sp < 1) return {};
            const float a = stack[--sp];
            if (op == FieldVmOpcode::MathAbs) value = std::abs(a);
            else if (op == FieldVmOpcode::MathSqrt)
                value = std::sqrt(std::max(a, 0.0f));
            else if (op == FieldVmOpcode::MathTan) value = std::tan(a);
            else if (op == FieldVmOpcode::MathSin) value = std::sin(a);
            else if (op == FieldVmOpcode::MathCos) value = std::cos(a);
            else if (op == FieldVmOpcode::MathExp) value = std::exp(a);
            else if (op == FieldVmOpcode::MathLog) {
                // CPU TransFactor::Ln is undefined iff arg <= 0. NaN is NOT
                // caught by that comparison and remains a produced numeric
                // value, so preserve that exact distinction.
                if (a <= 0.0f) {
                    return PredicateVmResult{true, false, 0.0f};
                }
                value = std::log(a);
            } else if (op == FieldVmOpcode::MathRound)
                value = std::round(a);
            else
                value = std::floor(a);
            if (!push(value)) return {};
            continue;
        }

        if (op == FieldVmOpcode::MathClamp) {
            if (sp < 3) return {};
            const float hi = stack[--sp];
            const float lo = stack[--sp];
            const float a = stack[--sp];
            if (!push(glm::clamp(a, lo, hi))) return {};
            continue;
        }

        if (op == FieldVmOpcode::MathNoise3 ||
            op == FieldVmOpcode::MathLength3 ||
            op == FieldVmOpcode::MathNormalizeComponent) {
            if (sp < 3) return {};
            const float z = stack[--sp];
            const float y = stack[--sp];
            const float x = stack[--sp];
            const glm::vec3 v(x, y, z);
            if (op == FieldVmOpcode::MathNoise3) {
                value = glm::perlin(v);
            } else {
                const float length = glm::length(v);
                if (op == FieldVmOpcode::MathLength3) value = length;
                else {
                    if (inst.meta.y > 2u) return {};
                    value = length < OntoMath::kDegenerateVectorLength
                        ? 0.0f : v[inst.meta.y] / length;
                }
            }
            if (!push(value)) return {};
            continue;
        }

        if (op == FieldVmOpcode::MathProjectComponent) {
            if (sp < 6 || inst.meta.y > 2u) return {};
            glm::vec3 b;
            b.z = stack[--sp]; b.y = stack[--sp]; b.x = stack[--sp];
            glm::vec3 a;
            a.z = stack[--sp]; a.y = stack[--sp]; a.x = stack[--sp];
            const float length = glm::length(b);
            value = length < OntoMath::kDegenerateVectorLength
                ? 0.0f
                : b[inst.meta.y] *
                    (glm::dot(a, b) / glm::dot(b, b));
            if (!push(value)) return {};
            continue;
        }

        // Geometry, Piecewise control, or unknown opcodes are never lawful
        // inside a pure mathematical predicate slice.
        return {};
    }

    if (sp != 1u) return {};
    return PredicateVmResult{true, true, stack[0]};
}

} // namespace

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
        if (sp >= kFieldVmMaxStackDepth || !std::isfinite(value)) return false;
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
            case FieldVmOpcode::MathAdd:
                value = a + b;
                break;
            case FieldVmOpcode::MathSub:
                value = a - b;
                break;
            case FieldVmOpcode::MathMul:
                value = a * b;
                break;
            case FieldVmOpcode::MathDiv:
                value = std::abs(b) < static_cast<float>(OntoMath::kDegenerateDivisor)
                    ? 0.0f : a / b;
                break;
            case FieldVmOpcode::MathPow:
                value = (a < 0.0f && std::floor(b) != b) ? 0.0f : std::pow(a, b);
                break;
            case FieldVmOpcode::MathRawPow:
                value = std::pow(a, b);
                break;
            case FieldVmOpcode::MathMin:
                value = std::min(a, b);
                break;
            case FieldVmOpcode::MathMax:
                value = std::max(a, b);
                break;
            default:
                return false;
        }
        return push(value);
    };

    for (std::size_t ip = 0; ip < program.instructions.size(); ++ip) {
        const FieldVmInstruction& inst = program.instructions[ip];
        const auto op = static_cast<FieldVmOpcode>(inst.meta.x);

        if (op == FieldVmOpcode::BranchInterval) {
            const uint32_t axis = inst.meta.y;
            const uint32_t flags = inst.meta.z;
            const uint32_t target = inst.meta.w;
            if (axis > 2u || target <= ip ||
                target > program.instructions.size()) {
                return std::nullopt;
            }
            const float coordinate = p[axis] - inst.a.x;
            if (!std::isfinite(coordinate)) return std::nullopt;
            bool inside = true;
            if ((flags & 1u) != 0u) {
                inside = inside &&
                    (((flags & 4u) != 0u)
                         ? coordinate >= inst.a.y
                         : coordinate > inst.a.y);
            }
            if ((flags & 2u) != 0u) {
                inside = inside &&
                    (((flags & 8u) != 0u)
                         ? coordinate <= inst.a.z
                         : coordinate < inst.a.z);
            }
            if (!inside) ip = static_cast<std::size_t>(target - 1u);
            continue;
        }

        if (op == FieldVmOpcode::Jump) {
            const uint32_t target = inst.meta.y;
            if (target <= ip || target > program.instructions.size()) {
                return std::nullopt;
            }
            ip = static_cast<std::size_t>(target - 1u);
            continue;
        }

        if (op == FieldVmOpcode::BranchWhereLEZero) {
            const uint32_t predicateCount = inst.meta.y;
            const uint32_t falseTarget = inst.meta.w;
            const std::size_t predicateBegin = ip + 1u;
            const std::size_t predicateEnd =
                predicateBegin + static_cast<std::size_t>(predicateCount);
            if (predicateCount == 0u ||
                predicateEnd > program.instructions.size() ||
                falseTarget <= ip ||
                falseTarget > program.instructions.size() ||
                falseTarget < predicateEnd) {
                return std::nullopt;
            }
            const PredicateVmResult predicate =
                evaluatePredicateVmSlice(
                    program.instructions,
                    predicateBegin, predicateCount, p);
            if (!predicate.valid) return std::nullopt;

            if (!predicate.defined || predicate.value > 0.0f) {
                ip = static_cast<std::size_t>(falseTarget - 1u);
            } else {
                // CPU Piece::applies is !(value > 0), not IEEE value <= 0.
                // Skip the side-slice and continue at this piece's value.
                ip = predicateEnd - 1u;
            }
            continue;
        }

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
            case FieldVmOpcode::MathAdd:
            case FieldVmOpcode::MathSub:
            case FieldVmOpcode::MathMul:
            case FieldVmOpcode::MathDiv:
            case FieldVmOpcode::MathPow:
            case FieldVmOpcode::MathRawPow:
            case FieldVmOpcode::MathMin:
            case FieldVmOpcode::MathMax:
                if (!applyBinary(op, inst.a.x)) return std::nullopt;
                break;

            case FieldVmOpcode::Constant:
                if (!push(inst.a.x)) return std::nullopt;
                break;
            case FieldVmOpcode::CoordX:
                if (!push(p.x - inst.a.x)) return std::nullopt;
                break;
            case FieldVmOpcode::CoordY:
                if (!push(p.y - inst.a.x)) return std::nullopt;
                break;
            case FieldVmOpcode::CoordZ:
                if (!push(p.z - inst.a.x)) return std::nullopt;
                break;

            case FieldVmOpcode::MathAbs:
            case FieldVmOpcode::MathSqrt:
            case FieldVmOpcode::MathTan:
            case FieldVmOpcode::MathSin:
            case FieldVmOpcode::MathCos:
            case FieldVmOpcode::MathExp:
            case FieldVmOpcode::MathLog:
            case FieldVmOpcode::MathRound:
            case FieldVmOpcode::MathFloor: {
                if (sp < 1) return std::nullopt;
                const float a = stack[--sp];
                float value = 0.0f;
                if (op == FieldVmOpcode::MathAbs) value = std::abs(a);
                else if (op == FieldVmOpcode::MathSqrt)
                    value = std::sqrt(std::max(a, 0.0f));
                else if (op == FieldVmOpcode::MathTan) value = std::tan(a);
                else if (op == FieldVmOpcode::MathSin) value = std::sin(a);
                else if (op == FieldVmOpcode::MathCos) value = std::cos(a);
                else if (op == FieldVmOpcode::MathExp) value = std::exp(a);
                else if (op == FieldVmOpcode::MathLog) {
                    if (!(a > 0.0f)) return std::nullopt;
                    value = std::log(a);
                } else if (op == FieldVmOpcode::MathRound) value = std::round(a);
                else value = std::floor(a);
                if (!push(value)) return std::nullopt;
                break;
            }

            case FieldVmOpcode::MathClamp: {
                if (sp < 3) return std::nullopt;
                const float hi = stack[--sp];
                const float lo = stack[--sp];
                const float value = stack[--sp];
                if (!push(glm::clamp(value, lo, hi))) return std::nullopt;
                break;
            }

            case FieldVmOpcode::MathNoise3:
            case FieldVmOpcode::MathLength3:
            case FieldVmOpcode::MathNormalizeComponent: {
                if (sp < 3) return std::nullopt;
                const float z = stack[--sp];
                const float y = stack[--sp];
                const float x = stack[--sp];
                const glm::vec3 v(x, y, z);
                float value = 0.0f;
                if (op == FieldVmOpcode::MathNoise3) {
                    value = glm::perlin(v);
                } else {
                    const float length = glm::length(v);
                    if (op == FieldVmOpcode::MathLength3) {
                        value = length;
                    } else {
                        const uint32_t axis = inst.meta.y;
                        if (axis > 2u) return std::nullopt;
                        value = length < OntoMath::kDegenerateVectorLength
                            ? 0.0f : v[axis] / length;
                    }
                }
                if (!push(value)) return std::nullopt;
                break;
            }

            case FieldVmOpcode::MathProjectComponent: {
                if (sp < 6) return std::nullopt;
                glm::vec3 b;
                b.z = stack[--sp]; b.y = stack[--sp]; b.x = stack[--sp];
                glm::vec3 a;
                a.z = stack[--sp]; a.y = stack[--sp]; a.x = stack[--sp];
                const uint32_t axis = inst.meta.y;
                if (axis > 2u) return std::nullopt;
                const float length = glm::length(b);
                const float value = length < OntoMath::kDegenerateVectorLength
                    ? 0.0f
                    : b[axis] * (glm::dot(a, b) / glm::dot(b, b));
                if (!push(value)) return std::nullopt;
                break;
            }

            default:
                return std::nullopt;
        }
    }

    if (sp != 1) return std::nullopt;
    return stack[0];
}

std::string fieldVmMathModule() {
    std::string module = R"WGSL(
struct FieldVmInstructionData {
    meta: vec4<u32>,
    a: vec4<f32>,
    b: vec4<f32>,
};

struct FieldVmResult {
    value: f32,
    known: f32,
};

fn fieldVmFinite(value: f32) -> bool {
    return value == value && abs(value) <= 3.402823e38;
}

struct FieldVmPredicateResult {
    value: f32,
    defined: f32,
    valid: f32,
};

fn fieldVmEvalPredicate(
    instructionBase: u32,
    instructionCount: u32,
    p: vec3<f32>) -> FieldVmPredicateResult {

    if (instructionCount == 0u || instructionCount > 1024u) {
        return FieldVmPredicateResult(0.0, 0.0, 0.0);
    }

    var stack: array<f32, 128>;
    var sp = 0u;

    for (var i = 0u; i < instructionCount; i = i + 1u) {
        let inst = fieldVmInstructions[instructionBase + i];
        let op = inst.meta.x;
        var value = 0.0;

        if (op >= 40u && op <= 43u) {
            if (sp >= 128u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            if (op == 40u) {
                value = inst.a.x;
            } else if (op == 41u) {
                value = p.x - inst.a.x;
            } else if (op == 42u) {
                value = p.y - inst.a.x;
            } else {
                value = p.z - inst.a.x;
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if ((op >= 44u && op <= 48u) ||
            (op >= 56u && op <= 57u) || op == 65u) {
            if (sp < 2u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            let b = stack[sp - 1u];
            let a = stack[sp - 2u];
            sp = sp - 2u;

            if (op == 44u) {
                value = a + b;
            } else if (op == 45u) {
                value = a - b;
            } else if (op == 46u) {
                value = a * b;
            } else if (op == 47u) {
                value = select(0.0, a / b, abs(b) >= FIELD_VM_DIV_EPS);
            } else if (op == 48u) {
                value = ontoPow(a, b);
            } else if (op == 65u) {
                value = ontoRawPow(a, b);
            } else if (op == 56u) {
                value = min(a, b);
            } else {
                value = max(a, b);
            }

            if (sp >= 128u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if ((op >= 49u && op <= 55u) ||
            (op >= 60u && op <= 61u)) {
            if (sp < 1u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            let a = stack[sp - 1u];
            if (op == 49u) {
                value = abs(a);
            } else if (op == 50u) {
                value = sqrt(max(a, 0.0));
            } else if (op == 51u) {
                value = tan(a);
            } else if (op == 52u) {
                value = sin(a);
            } else if (op == 53u) {
                value = cos(a);
            } else if (op == 54u) {
                value = exp(a);
            } else if (op == 55u) {
                // Match CPU TransFactor::Ln exactly: undefined iff a <= 0.
                // NaN does not satisfy that comparison and remains a produced
                // numeric value for Piece::applies' later !(value > 0) test.
                if (a <= 0.0) {
                    return FieldVmPredicateResult(0.0, 0.0, 1.0);
                }
                value = log(a);
            } else if (op == 60u) {
                value = round(a);
            } else {
                value = floor(a);
            }
            stack[sp - 1u] = value;
            continue;
        }

        if (op == 58u) {
            if (sp < 3u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            let hi = stack[sp - 1u];
            let lo = stack[sp - 2u];
            let a = stack[sp - 3u];
            sp = sp - 3u;
            if (sp >= 128u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            stack[sp] = clamp(a, lo, hi);
            sp = sp + 1u;
            continue;
        }

        if (op == 59u || op == 62u || op == 63u) {
            if (sp < 3u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            let v = vec3<f32>(
                stack[sp - 3u], stack[sp - 2u], stack[sp - 1u]);
            sp = sp - 3u;

            if (op == 59u) {
                value = cnoise3(v);
            } else {
                let len = length(v);
                if (op == 62u) {
                    value = len;
                } else {
                    let axis = inst.meta.y;
                    if (axis > 2u) {
                        return FieldVmPredicateResult(0.0, 0.0, 0.0);
                    }
                    value = select(
                        0.0, v[axis] / len, len >= FIELD_VM_VECTOR_EPS);
                }
            }

            if (sp >= 128u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if (op == 64u) {
            if (sp < 6u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            let a = vec3<f32>(
                stack[sp - 6u], stack[sp - 5u], stack[sp - 4u]);
            let b = vec3<f32>(
                stack[sp - 3u], stack[sp - 2u], stack[sp - 1u]);
            sp = sp - 6u;
            let axis = inst.meta.y;
            if (axis > 2u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            let len = length(b);
            value = select(
                0.0,
                b[axis] * (dot(a, b) / dot(b, b)),
                len >= FIELD_VM_VECTOR_EPS);
            if (sp >= 128u) {
                return FieldVmPredicateResult(0.0, 0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        return FieldVmPredicateResult(0.0, 0.0, 0.0);
    }

    if (sp != 1u) {
        return FieldVmPredicateResult(0.0, 0.0, 0.0);
    }
    return FieldVmPredicateResult(stack[0], 1.0, 1.0);
}

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

        if (op == 66u) {
            let axis = inst.meta.y;
            let flags = inst.meta.z;
            let target = inst.meta.w;
            if (axis > 2u || target <= i || target > instructionCount) {
                return FieldVmResult(0.0, 0.0);
            }
            let coordinate = p[axis] - inst.a.x;
            if (!fieldVmFinite(coordinate)) {
                return FieldVmResult(0.0, 0.0);
            }
            var inside = true;
            if ((flags & 1u) != 0u) {
                inside = inside && select(
                    coordinate > inst.a.y,
                    coordinate >= inst.a.y,
                    (flags & 4u) != 0u);
            }
            if ((flags & 2u) != 0u) {
                inside = inside && select(
                    coordinate < inst.a.z,
                    coordinate <= inst.a.z,
                    (flags & 8u) != 0u);
            }
            if (!inside) {
                i = target - 1u;
            }
            continue;
        }

        if (op == 67u) {
            let target = inst.meta.y;
            if (target <= i || target > instructionCount) {
                return FieldVmResult(0.0, 0.0);
            }
            i = target - 1u;
            continue;
        }

        if (op == 68u) {
            let predicateCount = inst.meta.y;
            let falseTarget = inst.meta.w;
            let predicateBegin = i + 1u;
            if (predicateCount == 0u ||
                predicateBegin > instructionCount ||
                predicateCount > instructionCount - predicateBegin) {
                return FieldVmResult(0.0, 0.0);
            }
            let predicateEnd = predicateBegin + predicateCount;
            if (falseTarget <= i || falseTarget > instructionCount ||
                falseTarget < predicateEnd) {
                return FieldVmResult(0.0, 0.0);
            }

            let predicate = fieldVmEvalPredicate(
                programBase + predicateBegin, predicateCount, p);
            if (predicate.valid < 0.5) {
                return FieldVmResult(0.0, 0.0);
            }

            if (predicate.defined < 0.5 || predicate.value > 0.0) {
                i = falseTarget - 1u;
            } else {
                // Piece::applies uses !(value > 0), not IEEE value <= 0.
                i = predicateEnd - 1u;
            }
            continue;
        }

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
            if (!fieldVmFinite(value)) {
                return FieldVmResult(0.0, 0.0);
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
            if (!fieldVmFinite(value)) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if (op >= 40u && op <= 43u) {
            if (sp >= 128u) {
                return FieldVmResult(0.0, 0.0);
            }
            if (op == 40u) {
                value = inst.a.x;
            } else if (op == 41u) {
                value = p.x - inst.a.x;
            } else if (op == 42u) {
                value = p.y - inst.a.x;
            } else {
                value = p.z - inst.a.x;
            }
            if (!fieldVmFinite(value)) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if ((op >= 44u && op <= 48u) || (op >= 56u && op <= 57u) ||
            op == 65u) {
            if (sp < 2u) {
                return FieldVmResult(0.0, 0.0);
            }
            let b = stack[sp - 1u];
            let a = stack[sp - 2u];
            sp = sp - 2u;
            if (op == 44u) {
                value = a + b;
            } else if (op == 45u) {
                value = a - b;
            } else if (op == 46u) {
                value = a * b;
            } else if (op == 47u) {
                value = select(0.0, a / b, abs(b) >= FIELD_VM_DIV_EPS);
            } else if (op == 48u) {
                value = ontoPow(a, b);
            } else if (op == 65u) {
                value = ontoRawPow(a, b);
            } else if (op == 56u) {
                value = min(a, b);
            } else {
                value = max(a, b);
            }
            if (!fieldVmFinite(value) || sp >= 128u) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if ((op >= 49u && op <= 55u) || (op >= 60u && op <= 61u)) {
            if (sp < 1u) {
                return FieldVmResult(0.0, 0.0);
            }
            let a = stack[sp - 1u];
            if (op == 49u) {
                value = abs(a);
            } else if (op == 50u) {
                value = sqrt(max(a, 0.0));
            } else if (op == 51u) {
                value = tan(a);
            } else if (op == 52u) {
                value = sin(a);
            } else if (op == 53u) {
                value = cos(a);
            } else if (op == 54u) {
                value = exp(a);
            } else if (op == 55u) {
                if (!(a > 0.0)) {
                    return FieldVmResult(0.0, 0.0);
                }
                value = log(a);
            } else if (op == 60u) {
                value = round(a);
            } else {
                value = floor(a);
            }
            if (!fieldVmFinite(value)) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp - 1u] = value;
            continue;
        }

        if (op == 58u) {
            if (sp < 3u) {
                return FieldVmResult(0.0, 0.0);
            }
            let hi = stack[sp - 1u];
            let lo = stack[sp - 2u];
            let a = stack[sp - 3u];
            sp = sp - 3u;
            value = clamp(a, lo, hi);
            if (!fieldVmFinite(value) || sp >= 128u) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if (op == 59u || op == 62u || op == 63u) {
            if (sp < 3u) {
                return FieldVmResult(0.0, 0.0);
            }
            let v = vec3<f32>(
                stack[sp - 3u], stack[sp - 2u], stack[sp - 1u]);
            sp = sp - 3u;
            if (op == 59u) {
                value = cnoise3(v);
            } else {
                let len = length(v);
                if (op == 62u) {
                    value = len;
                } else {
                    let axis = inst.meta.y;
                    if (axis > 2u) {
                        return FieldVmResult(0.0, 0.0);
                    }
                    value = select(
                        0.0, v[axis] / len, len >= FIELD_VM_VECTOR_EPS);
                }
            }
            if (!fieldVmFinite(value) || sp >= 128u) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        if (op == 64u) {
            if (sp < 6u) {
                return FieldVmResult(0.0, 0.0);
            }
            let a = vec3<f32>(
                stack[sp - 6u], stack[sp - 5u], stack[sp - 4u]);
            let b = vec3<f32>(
                stack[sp - 3u], stack[sp - 2u], stack[sp - 1u]);
            sp = sp - 6u;
            let axis = inst.meta.y;
            if (axis > 2u) {
                return FieldVmResult(0.0, 0.0);
            }
            let len = length(b);
            value = select(
                0.0,
                b[axis] * (dot(a, b) / dot(b, b)),
                len >= FIELD_VM_VECTOR_EPS);
            if (!fieldVmFinite(value) || sp >= 128u) {
                return FieldVmResult(0.0, 0.0);
            }
            stack[sp] = value;
            sp = sp + 1u;
            continue;
        }

        return FieldVmResult(0.0, 0.0);
    }

    if (sp != 1u || !fieldVmFinite(stack[0])) {
        return FieldVmResult(0.0, 0.0);
    }
    return FieldVmResult(stack[0], 1.0);
}
)WGSL";
    const auto replaceAll = [](std::string& text,
                               const std::string& needle,
                               const std::string& value) {
        std::size_t at = 0;
        while ((at = text.find(needle, at)) != std::string::npos) {
            text.replace(at, needle.size(), value);
            at += value.size();
        }
    };
    replaceAll(module, "FIELD_VM_DIV_EPS",
               std::to_string(OntoMath::kDegenerateDivisor));
    replaceAll(module, "FIELD_VM_VECTOR_EPS",
               std::to_string(OntoMath::kDegenerateVectorLength));
    return module;
}

std::string fieldVmBoundMathModule(uint32_t group, uint32_t binding) {
    std::string module = fieldVmMathModule();
    const std::string marker =
        "struct FieldVmResult {\n"
        "    value: f32,\n"
        "    known: f32,\n"
        "};\n";
    const std::size_t at = module.find(marker);
    if (at == std::string::npos) return "// REFUSED: field VM binding marker missing\n";
    const std::size_t insertAt = at + marker.size();
    const std::string declaration =
        "@group(" + std::to_string(group) + ") @binding(" +
        std::to_string(binding) +
        ") var<storage, read> fieldVmInstructions: array<FieldVmInstructionData>;\n";
    module.insert(insertAt, declaration);
    return module;
}

} // namespace sdfwgsl
