#pragma once
#include "Singularity/OntoMath/ScalarForm.hpp"

// A reusable mathematical decomposition: f_i = B * c_i. This office knows
// neither rendering nor clocks. The caller declares every coefficient symbol.
// Original multiplication order and complete source snapshots are retained;
// equality is structural, never inferred from samples or simplified f32 math.
// This real-arithmetic identity grants NO backend rounding/omission authority.
//
// The result owns its data. It remains a historical value after source edits or
// destruction, not a cache that silently follows a borrowed node. A consumer
// must rederive on source/binding revision changes. No device resources live here.
// Plain mathematical IR, not state carried by a being; no new persisted opcode.
// Human origin: Zach's OntoMath ownership correction and authored-faithfulness
// checkpoint. Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a /
// 2026-10-22. Continuation: ray-transport plan §6ck.
namespace OntoMath {
struct CommonScalarFactor {
    MathNode factor;
    std::vector<MathNode> coefficients;
    std::vector<MathNode> originalExpressions;
    std::vector<unsigned> factorChild; // original operand order, 0 or 1
    std::set<std::string> coefficientDependencies;
    TypeEnv coefficientTypes;
};

namespace common_factor_detail {
inline bool refuse(std::string* why, const std::string& reason) {
    if (why) *why = reason;
    return false;
}
// Kernel compiler ceilings, never authored approximation tolerances. Refuse
// before recursive type/JSON operations; exhausting a ceiling changes no math.
inline bool inspect(const MathNode& n, const TypeEnv& env, unsigned depth,
                    std::size_t& budget, std::string* why) {
    if (depth > 96 || budget == 0) return refuse(why, "shared-factor tree budget exceeded");
    --budget;
    if (n.unsupported) return refuse(why, "unsupported source payload");
    using O = MathNode::Op;
    switch (n.op) {
    case O::ScalarLeaf: case O::ValueLeaf: case O::VectorConstruct:
    case O::Component: case O::Add: case O::Sub: case O::Scale: case O::Dot:
    case O::Cross: case O::Hadamard: case O::Normalize: case O::Length:
    case O::Map: case O::Project: case O::Distance: case O::Union:
    case O::Intersection: case O::Difference: case O::Div: case O::Pow:
    case O::Abs: case O::Clamp: case O::Sqrt: case O::Tan: case O::Noise: break;
    default: return refuse(why, "operation not admitted by pure shared-factor analysis");
    }
    if ((n.op == O::ScalarLeaf || n.op == O::ValueLeaf) && !n.children.empty())
        return refuse(why, "leaf carries unexpected operands");
    if (n.op == O::ScalarLeaf) {
        for (const auto& term : n.scalarForm.terms) {
            if (budget == 0) return refuse(why, "shared-factor term budget exceeded");
            --budget;
            if (!std::isfinite(term.coefficient)) return refuse(why, "nonfinite source coefficient");
            for (const auto& [name, exponent] : term.factors) {
                if (budget == 0) return refuse(why, "shared-factor factor budget exceeded");
                --budget;
                const auto it = env.find(name);
                if (!std::isfinite(exponent) || it == env.end() || it->second.kind != ValueKind::Scalar)
                    return refuse(why, "scalar symbol not explicitly typed");
            }
            for (const auto& t : term.trans) {
                if (budget == 0) return refuse(why, "shared-factor transcendental budget exceeded");
                --budget;
                const auto it = env.find(t.variable);
                if (!std::isfinite(t.scale) || !std::isfinite(t.shift) || it == env.end() || it->second.kind != ValueKind::Scalar)
                    return refuse(why, "transcendental symbol not explicitly typed");
            }
        }
    }
    for (const auto& child : n.children)
        if (!child || !inspect(*child, env, depth + 1, budget, why))
            return refuse(why, "missing or unadmitted shared-factor operand");
    return true;
}
inline bool finiteValue(const PropertyValue& v) {
    if (const auto p = std::get_if<double>(&v)) return std::isfinite(*p);
    if (const auto p = std::get_if<float>(&v)) return std::isfinite(*p);
    if ((std::holds_alternative<int>(v) || std::holds_alternative<long>(v))) return true;
    if (const auto p = std::get_if<glm::vec3>(&v))
        return std::isfinite(p->x) && std::isfinite(p->y) && std::isfinite(p->z);
    return false;
}
} // namespace common_factor_detail

// Admit a common scalar child of two or more original Scale expressions.
// c_i may be scalar or vec3; remaining symbols must be explicitly admitted.
// No division, cancellation, positivity policy, world read, or symbol default.
inline std::optional<CommonScalarFactor> factorCommonScalar(
    const std::vector<const MathNode*>& expressions, const TypeEnv& env,
    const std::set<std::string>& allowedCoefficientSymbols, std::string* why = nullptr) {
    using namespace common_factor_detail;
    if (why) why->clear();
    if (expressions.size() < 2 || expressions.size() > 64) {
        refuse(why, "shared-factor expression count outside [2,64]"); return std::nullopt;
    }
    std::size_t budget = 16384;
    for (const auto* n : expressions) {
        if (!n || n->op != MathNode::Op::Scale || n->children.size() != 2 ||
            !inspect(*n, env, 0, budget, why)) {
            refuse(why, "requires bounded pure binary Scale expressions"); return std::nullopt;
        }
        // Judge the common subtree once below. Each Scale then has that
        // scalar operand plus its separately judged scalar/vector coefficient.
        // Rewalking every whole original tree repeats identical type work.
    }
    for (unsigned firstSide = 0; firstSide < 2; ++firstSide) {
        const auto& candidate = *expressions.front()->children[firstSide];
        const auto type = candidate.typeOf(env);
        if (!type || type.kind != ValueKind::Scalar) continue;
        const auto identity = candidate.toJson();
        CommonScalarFactor result;
        bool all = true;
        for (const auto* n : expressions) {
            bool matched = false;
            for (unsigned side = 0; side < 2; ++side) {
                if (identity != n->children[side]->toJson()) continue;
                const auto& coefficient = *n->children[1 - side];
                const auto coefficientType = coefficient.typeOf(env);
                if (!coefficientType || (coefficientType.kind != ValueKind::Scalar && coefficientType.kind != ValueKind::Vector)) continue;
                std::set<std::string> dependencies; coefficient.collectDependencies(dependencies);
                if (!std::includes(allowedCoefficientSymbols.begin(), allowedCoefficientSymbols.end(),
                                   dependencies.begin(), dependencies.end())) continue;
                result.coefficients.push_back(coefficient);
                result.originalExpressions.push_back(*n);
                result.factorChild.push_back(side);
                result.coefficientDependencies.insert(dependencies.begin(), dependencies.end());
                matched = true; break;
            }
            if (!matched) { all = false; break; }
        }
        if (all) {
            result.factor = candidate;
            for (const auto& name : result.coefficientDependencies) result.coefficientTypes.emplace(name, env.at(name));
            return result;
        }
    }
    refuse(why, "no structurally identical scalar factor with admitted coefficient symbols");
    return std::nullopt;
}

// Prepare only c_i through the ORIGINAL evaluator. No JSON, source matching,
// factor evaluation, hidden Timeline, or GPU/device storage here. Caller owns
// backend conversion/error accounting and any per-frame upload/residency.
inline std::optional<std::vector<PropertyValue>> evaluateFactorCoefficients(
    const CommonScalarFactor& recipe, const std::map<std::string, PropertyValue>& bindings,
    std::string* why = nullptr) {
    using namespace common_factor_detail;
    if (why) why->clear();
    for (const auto& name : recipe.coefficientDependencies) {
        const auto it = bindings.find(name);
        const auto expected = recipe.coefficientTypes.at(name).kind;
        if (it == bindings.end() || !finiteValue(it->second) ||
            (expected == ValueKind::Scalar && std::holds_alternative<glm::vec3>(it->second)) ||
            (expected == ValueKind::Vector && !std::holds_alternative<glm::vec3>(it->second))) {
            refuse(why, "missing or nonfinite coefficient binding: " + name); return std::nullopt;
        }
    }
    std::vector<PropertyValue> values; values.reserve(recipe.coefficients.size());
    for (const auto& n : recipe.coefficients) {
        const auto v = n.evaluate(bindings);
        if (!v || !finiteValue(*v)) {
            refuse(why, "coefficient evaluation refused or nonfinite"); return std::nullopt;
        }
        values.push_back(*v);
    }
    return values;
}
} // namespace OntoMath
