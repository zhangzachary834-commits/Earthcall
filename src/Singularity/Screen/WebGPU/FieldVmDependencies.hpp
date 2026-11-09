#pragma once

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace sdfwgsl {

// Kernel preflight only. These names classify what authored field mathematics
// would need from an execution/query context; they are NOT new authored kinds
// and do not grant Field-VM admission.
enum class FieldVmDependencyKind : unsigned {
    PropertyPath = 1,
    NamedValue = 2,
    WorldGuard = 3,
    PureGuard = 4,
    FunctionCall = 5,
    WorldFold = 6,
};

struct FieldVmDependency {
    FieldVmDependencyKind kind = FieldVmDependencyKind::NamedValue;
    std::string key;

    bool operator==(const FieldVmDependency& other) const {
        return kind == other.kind && key == other.key;
    }
};

struct FieldVmDependencyManifest {
    std::vector<FieldVmDependency> dependencies;
    std::vector<std::string> structuralErrors;

    bool structurallyValid() const { return structuralErrors.empty(); }

    bool requiresExternalAuthority() const {
        for (const auto& d : dependencies) {
            switch (d.kind) {
                case FieldVmDependencyKind::PropertyPath:
                case FieldVmDependencyKind::NamedValue:
                case FieldVmDependencyKind::WorldGuard:
                case FieldVmDependencyKind::FunctionCall:
                case FieldVmDependencyKind::WorldFold:
                    return true;
                case FieldVmDependencyKind::PureGuard:
                    break;
            }
        }
        return false;
    }
};

// Explicit query/binding context for readiness PRECHECKS. Supplying this does
// not make lowerFieldVm accept dependencies; it only proves that the authority
// a future consumer would need is present and finite.
struct FieldVmDependencyContext {
    const std::map<std::string, PropertyValue>* propertyBindings = nullptr;
    const std::map<std::string, PropertyValue>* namedValues = nullptr;
    bool subjectAvailable = false;
    // A subject alone does not prove Related/Overlaps/quantifier/world guards
    // are executable. The caller must separately attest that the condition
    // calculus has its authoritative query context.
    bool worldConditionContextAvailable = false;
    bool functionRegistryAvailable = false;
    bool worldEnumerationAvailable = false;
};

struct FieldVmDependencyAssessment {
    bool ready = false;
    std::vector<std::string> missing;
    std::vector<std::string> nonFinite;
    std::vector<std::string> unsupported;
    std::vector<FieldVmDependency> unresolvedContext;
};

namespace detail {

inline bool fieldVmLocalValueName(const std::string& name) {
    return name == "x" || name == "y" || name == "z" ||
           name == OntoMath::kAmbientPointVar;
}

inline void addFieldVmDependency(
    FieldVmDependencyManifest& out,
    std::set<std::pair<unsigned, std::string>>& seen,
    FieldVmDependencyKind kind,
    const std::string& key) {
    const auto id = std::make_pair(static_cast<unsigned>(kind), key);
    if (seen.insert(id).second) out.dependencies.push_back({kind, key});
}

inline void inspectFieldVmMathDependencies(
    const OntoMath::MathNode& node,
    FieldVmDependencyManifest& out,
    std::set<std::pair<unsigned, std::string>>& seen) {
    using Op = OntoMath::MathNode::Op;

    if (node.op == Op::Bind) {
        if (node.propertyPath.empty()) {
            out.structuralErrors.push_back("Bind has an empty PropertyPath");
        } else {
            addFieldVmDependency(out, seen,
                                 FieldVmDependencyKind::PropertyPath,
                                 node.propertyPath);
        }
    }

    if (node.op == Op::ValueLeaf &&
        !fieldVmLocalValueName(node.variableName)) {
        if (node.variableName.empty()) {
            out.structuralErrors.push_back("ValueLeaf has an empty variable name");
        } else {
            addFieldVmDependency(out, seen,
                                 FieldVmDependencyKind::NamedValue,
                                 node.variableName);
        }
    }

    if (node.op == Op::ScalarLeaf) {
        for (const auto& term : node.scalarForm.terms) {
            for (const auto& [name, exponent] : term.factors) {
                (void)exponent;
                if (!fieldVmLocalValueName(name)) {
                    addFieldVmDependency(out, seen,
                                         FieldVmDependencyKind::NamedValue,
                                         name);
                }
            }
            for (const auto& tf : term.trans) {
                if (!fieldVmLocalValueName(tf.variable)) {
                    addFieldVmDependency(out, seen,
                                         FieldVmDependencyKind::NamedValue,
                                         tf.variable);
                }
            }
        }
    }

    for (const auto& child : node.children) {
        if (child) inspectFieldVmMathDependencies(*child, out, seen);
    }
}

inline bool finiteFieldVmDependencyValue(const PropertyValue& value,
                                         bool& supported) {
    double scalar = 0.0;
    if (propertyValueToNumber(value, scalar)) {
        supported = true;
        return std::isfinite(scalar);
    }
    if (const auto* v = std::get_if<glm::vec3>(&value)) {
        supported = true;
        return std::isfinite(v->x) && std::isfinite(v->y) &&
               std::isfinite(v->z);
    }
    supported = false;
    return false;
}

} // namespace detail

inline FieldVmDependencyManifest
inspectFieldVmDependencies(const geom::SdfNode& root) {
    FieldVmDependencyManifest out;
    std::set<std::pair<unsigned, std::string>> seen;

    std::function<void(const geom::SdfNode&)> visit =
        [&](const geom::SdfNode& node) {
            if (node.op == geom::SdfOp::Leaf &&
                node.prim == geom::SdfPrim::Expr) {
                if (node.mathNode) {
                    detail::inspectFieldVmMathDependencies(
                        *node.mathNode, out, seen);
                }
                if (node.piecewise) {
                    const auto& pw = *node.piecewise;
                    if (!detail::fieldVmLocalValueName(pw.inputVariable)) {
                        detail::addFieldVmDependency(
                            out, seen, FieldVmDependencyKind::NamedValue,
                            pw.inputVariable);
                    }
                    for (std::size_t i = 0; i < pw.pieces.size(); ++i) {
                        const auto& piece = pw.pieces[i];
                        const std::string key = "piece[" + std::to_string(i) + "]";
                        if (piece.mathNode) {
                            detail::inspectFieldVmMathDependencies(
                                *piece.mathNode, out, seen);
                        }
                        if (piece.whereLEZero) {
                            detail::addFieldVmDependency(
                                out, seen, FieldVmDependencyKind::PureGuard,
                                key + ".whereLEZero");
                            detail::inspectFieldVmMathDependencies(
                                *piece.whereLEZero, out, seen);
                        }
                        if (piece.guard) {
                            detail::addFieldVmDependency(
                                out, seen, FieldVmDependencyKind::WorldGuard,
                                key + ".guard");
                        }
                        if (piece.call) {
                            detail::addFieldVmDependency(
                                out, seen, FieldVmDependencyKind::FunctionCall,
                                key + ".call");
                        }
                        if (piece.fold) {
                            detail::addFieldVmDependency(
                                out, seen, FieldVmDependencyKind::WorldFold,
                                key + ".fold");
                        }
                    }
                }
            }

            for (const auto& child : node.children) {
                if (child) visit(*child);
            }
        };

    visit(root);
    return out;
}

inline FieldVmDependencyAssessment assessFieldVmDependencies(
    const FieldVmDependencyManifest& manifest,
    const FieldVmDependencyContext& context = {}) {
    FieldVmDependencyAssessment out;

    if (!manifest.structurallyValid()) {
        out.unsupported = manifest.structuralErrors;
    }

    const auto checkMapValue =
        [&](const std::map<std::string, PropertyValue>* provider,
            const FieldVmDependency& d) {
            if (!provider) {
                out.missing.push_back(d.key);
                return;
            }
            const auto it = provider->find(d.key);
            if (it == provider->end()) {
                out.missing.push_back(d.key);
                return;
            }
            bool supported = false;
            const bool finite =
                detail::finiteFieldVmDependencyValue(it->second, supported);
            if (!supported) out.unsupported.push_back(d.key);
            else if (!finite) out.nonFinite.push_back(d.key);
        };

    for (const auto& d : manifest.dependencies) {
        switch (d.kind) {
            case FieldVmDependencyKind::PropertyPath:
                checkMapValue(context.propertyBindings, d);
                break;
            case FieldVmDependencyKind::NamedValue:
                checkMapValue(context.namedValues, d);
                break;
            case FieldVmDependencyKind::WorldGuard:
                if (!context.subjectAvailable ||
                    !context.worldConditionContextAvailable)
                    out.unresolvedContext.push_back(d);
                break;
            case FieldVmDependencyKind::FunctionCall:
                if (!context.functionRegistryAvailable)
                    out.unresolvedContext.push_back(d);
                break;
            case FieldVmDependencyKind::WorldFold:
                if (!context.worldEnumerationAvailable)
                    out.unresolvedContext.push_back(d);
                break;
            case FieldVmDependencyKind::PureGuard:
                // Pure guards are local mathematical control. Their presence is
                // recorded, but they need no invented subject/world provider.
                break;
        }
    }

    out.ready = out.missing.empty() && out.nonFinite.empty() &&
                out.unsupported.empty() && out.unresolvedContext.empty();
    return out;
}

} // namespace sdfwgsl
