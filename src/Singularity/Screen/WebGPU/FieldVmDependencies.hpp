#pragma once

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
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
    // Exact caller-owned provenance/revision stamps for each supplied value.
    // The value map says "what"; this map says "whose/current-under-which
    // authority". Snapshot identity requires both for reusable evidence.
    const std::map<std::string, std::string>* propertyBindingAuthorities = nullptr;
    const std::map<std::string, std::string>* namedValueAuthorities = nullptr;
    bool subjectAvailable = false;
    // A subject alone does not prove Related/Overlaps/quantifier/world guards
    // are executable. The caller must separately attest that the condition
    // calculus has its authoritative query context.
    bool worldConditionContextAvailable = false;
    bool functionRegistryAvailable = false;
    bool worldEnumerationAvailable = false;

    // Opaque exact authority stamps supplied by the caller. Preflight never
    // invents these. A valid runtime snapshot for an external context requires
    // both availability above and a non-empty identity here, so two subjects or
    // two world/query regimes cannot share retained execution evidence merely
    // because both were "available".
    std::string subjectIdentity;
    std::string worldConditionContextIdentity;
    std::string functionRegistryIdentity;
    std::string worldEnumerationIdentity;
};

struct FieldVmDependencyAssessment {
    bool ready = false;
    std::vector<std::string> missing;
    std::vector<std::string> nonFinite;
    std::vector<std::string> outOfFloat32Range;
    std::vector<std::string> unsupported;
    std::vector<FieldVmDependency> unresolvedContext;
};

struct FieldVmDependencySnapshot {
    bool valid = false;
    // Exact textual encoding of manifest entries plus execution-relevant f32
    // bit patterns / opaque external-authority stamps. This is intentionally
    // not a reduced hash: collisions must never authorize stale evidence.
    std::string identity;
    FieldVmDependencyAssessment assessment;
    std::vector<std::string> missingAuthorityIdentity;
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

enum class FieldVmDependencyValueStatus {
    Ready,
    NonFinite,
    OutOfFloat32Range,
    Unsupported,
};

inline uint32_t fieldVmFloatBits(float value) {
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

inline std::string fieldVmHex32(uint32_t value) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(8, '0');
    for (int i = 7; i >= 0; --i) {
        out[static_cast<std::size_t>(i)] = kHex[value & 0xfu];
        value >>= 4u;
    }
    return out;
}

inline FieldVmDependencyValueStatus fieldVmDependencyValueToken(
    const PropertyValue& value, std::string* token = nullptr) {
    double scalar = 0.0;
    if (propertyValueToNumber(value, scalar)) {
        if (!std::isfinite(scalar))
            return FieldVmDependencyValueStatus::NonFinite;
        const float f = static_cast<float>(scalar);
        if (!std::isfinite(f))
            return FieldVmDependencyValueStatus::OutOfFloat32Range;
        if (token) *token = "s:" + fieldVmHex32(fieldVmFloatBits(f));
        return FieldVmDependencyValueStatus::Ready;
    }
    if (const auto* v = std::get_if<glm::vec3>(&value)) {
        if (!std::isfinite(v->x) || !std::isfinite(v->y) ||
            !std::isfinite(v->z))
            return FieldVmDependencyValueStatus::NonFinite;
        if (token) {
            *token = "v:" +
                fieldVmHex32(fieldVmFloatBits(v->x)) + ":" +
                fieldVmHex32(fieldVmFloatBits(v->y)) + ":" +
                fieldVmHex32(fieldVmFloatBits(v->z));
        }
        return FieldVmDependencyValueStatus::Ready;
    }
    return FieldVmDependencyValueStatus::Unsupported;
}

inline void appendFieldVmIdentityPart(std::string& out,
                                      const std::string& value) {
    out += std::to_string(value.size());
    out += ":";
    out += value;
    out += ";";
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
            const auto status =
                detail::fieldVmDependencyValueToken(it->second);
            switch (status) {
                case detail::FieldVmDependencyValueStatus::Ready:
                    break;
                case detail::FieldVmDependencyValueStatus::NonFinite:
                    out.nonFinite.push_back(d.key);
                    break;
                case detail::FieldVmDependencyValueStatus::OutOfFloat32Range:
                    out.outOfFloat32Range.push_back(d.key);
                    break;
                case detail::FieldVmDependencyValueStatus::Unsupported:
                    out.unsupported.push_back(d.key);
                    break;
            }
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
                out.outOfFloat32Range.empty() && out.unsupported.empty() &&
                out.unresolvedContext.empty();
    return out;
}

inline FieldVmDependencySnapshot snapshotFieldVmDependencies(
    const FieldVmDependencyManifest& manifest,
    const FieldVmDependencyContext& context = {}) {
    FieldVmDependencySnapshot out;
    out.assessment = assessFieldVmDependencies(manifest, context);
    if (!out.assessment.ready) return out;

    std::string identity;
    const auto add = [&](const std::string& part) {
        detail::appendFieldVmIdentityPart(identity, part);
    };
    const auto readToken =
        [&](const std::map<std::string, PropertyValue>* provider,
            const std::string& key) -> std::string {
            if (!provider) return {};
            const auto it = provider->find(key);
            if (it == provider->end()) return {};
            std::string token;
            if (detail::fieldVmDependencyValueToken(it->second, &token) !=
                detail::FieldVmDependencyValueStatus::Ready) {
                return {};
            }
            return token;
        };
    const auto readAuthority =
        [&](const std::map<std::string, std::string>* provider,
            const std::string& key,
            const std::string& label) -> std::string {
            if (!provider) {
                out.missingAuthorityIdentity.push_back(label + ":" + key);
                return {};
            }
            const auto it = provider->find(key);
            if (it == provider->end() || it->second.empty()) {
                out.missingAuthorityIdentity.push_back(label + ":" + key);
                return {};
            }
            return it->second;
        };
    const auto requireAuthorityIdentity =
        [&](bool available, const std::string& stamp,
            const std::string& label) -> bool {
            if (!available || stamp.empty()) {
                out.missingAuthorityIdentity.push_back(label);
                return false;
            }
            add(stamp);
            return true;
        };

    add("field-vm-dependency-snapshot:v1");
    bool valid = manifest.structurallyValid();
    for (const auto& d : manifest.dependencies) {
        add(std::to_string(static_cast<unsigned>(d.kind)));
        add(d.key);
        switch (d.kind) {
            case FieldVmDependencyKind::PropertyPath: {
                const std::string token =
                    readToken(context.propertyBindings, d.key);
                const std::string authority =
                    readAuthority(context.propertyBindingAuthorities,
                                  d.key, "property-binding");
                if (token.empty() || authority.empty()) {
                    valid = false;
                } else {
                    add(token);
                    add(authority);
                }
                break;
            }
            case FieldVmDependencyKind::NamedValue: {
                const std::string token =
                    readToken(context.namedValues, d.key);
                const std::string authority =
                    readAuthority(context.namedValueAuthorities,
                                  d.key, "named-value");
                if (token.empty() || authority.empty()) {
                    valid = false;
                } else {
                    add(token);
                    add(authority);
                }
                break;
            }
            case FieldVmDependencyKind::WorldGuard:
                valid &= requireAuthorityIdentity(
                    context.subjectAvailable, context.subjectIdentity,
                    "subject");
                valid &= requireAuthorityIdentity(
                    context.worldConditionContextAvailable,
                    context.worldConditionContextIdentity,
                    "world-condition-context");
                break;
            case FieldVmDependencyKind::FunctionCall:
                valid &= requireAuthorityIdentity(
                    context.functionRegistryAvailable,
                    context.functionRegistryIdentity,
                    "function-registry");
                break;
            case FieldVmDependencyKind::WorldFold:
                valid &= requireAuthorityIdentity(
                    context.worldEnumerationAvailable,
                    context.worldEnumerationIdentity,
                    "world-enumeration");
                break;
            case FieldVmDependencyKind::PureGuard:
                add("local-pure-control");
                break;
        }
    }

    out.valid = valid && out.missingAuthorityIdentity.empty();
    if (out.valid) out.identity = std::move(identity);
    return out;
}

} // namespace sdfwgsl
