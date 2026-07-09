#pragma once

#include "ECA.hpp"
#include "Form/Object/Geometry/Sdf.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"
#include "json.hpp"

#include <string>
#include <vector>

// The law's condition as data (LAW_AND_CREATION_SYSTEM.md §2a): an expression
// tree over PropertyPaths, serializable and Person-authorable, compiled once
// into the ECA::ConditionPredicate slot. The SdfNode move applied to
// predicates — the tree is the law's text; the closure is derived.
struct ConditionNode {
    // Serialized as ints — both enums are APPEND-ONLY.
    enum class Kind { Compare = 0, InRegion = 1, Related = 2, All = 3, Any = 4, Not = 5 };
    enum class Op { Eq = 0, Ne = 1, Lt = 2, Le = 3, Gt = 4, Ge = 5, Near = 6, InRange = 7 };

    Kind kind = Kind::Compare;

    // Compare payload.
    PropertyPath path;               // lhs
    Op op = Op::Eq;
    PropertyValue operand;           // rhs literal…
    PropertyPath operandPath;        // …or rhs read live from another property
    double tolerance = 0.0;          // Near
    PropertyValue lo, hi;            // InRange

    // InRegion payload — a shape IS the condition. Authored with the ordinary
    // shape tools in projection mode; evalSdf(region, probe) < 0 is the test.
    geom::SdfNode region;
    PropertyPath probe;              // point tested; defaults to "position"

    // Related payload — graph-shaped conditions ("x touching y"). Recorded in
    // the model now; resolution against the relation graph lands with the
    // event/Rete wiring (commit 4).
    std::string relationType;
    std::string otherId;

    std::vector<ConditionNode> children;   // All / Any / Not

    nlohmann::json toJson() const;
    static ConditionNode fromJson(const nlohmann::json& j);

    // Tree → closure, once. The tree remains the law's text.
    ECA::ConditionPredicate compile() const;

    // One-line human summary for ApplicationRecord logs.
    std::string describe() const;

    // Factories (mirror SdfNode::leaf/binary).
    static ConditionNode compare(const std::string& dottedPath, Op op, PropertyValue rhs);
    static ConditionNode comparePaths(const std::string& dottedPath, Op op, const std::string& rhsPath);
    static ConditionNode inRegion(geom::SdfNode region, const std::string& probePath = "position");
    static ConditionNode all(std::vector<ConditionNode> children);
    static ConditionNode any(std::vector<ConditionNode> children);
    static ConditionNode negate(ConditionNode child);
};

// A law's condition model is the root of one such tree.
using ConditionModel = ConditionNode;
