#pragma once

#include "ECA.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "json.hpp"

#include <string>
#include <vector>

// The law's action as data (LAW_AND_CREATION_SYSTEM.md §2b): a mutation tree
// over PropertyPaths, serializable and Person-authorable, compiled once into
// the ECA::ActionExecutor slot.
//
// Drive is the gradient law: the written value is curve(input) evaluated
// continuously — "glow brighter AS the person approaches", not "glow when
// near". Discrete actions are the degenerate case (constant curve behind a
// condition).
struct ActionNode {
    // Serialized as ints — APPEND-ONLY.
    enum class Kind {
        Set = 0,       // path := operand
        Add = 1,       // path := path + operand        (numeric)
        Scale = 2,     // path := path * operand        (numeric)
        Lerp = 3,      // path := path + (operand-path)*factor   (numeric)
        Drive = 4,     // path := curve(input)
        Sequence = 5,  // children in order
        Parallel = 6,  // children conceptually simultaneous (same order today;
                       // the distinction matters when actions become async)
        Spawn = 7      // reserved: instantiate from an ObjectConcept (commit 6)
    };

    Kind kind = Kind::Set;

    PropertyPath path;               // what changes
    PropertyValue operand;           // Set value / Add delta / Scale factor / Lerp target
    double factor = 1.0;             // Lerp blend

    CurveModel curve;                // Drive
    PropertyPath input;              // Drive domain; empty => event timestamp (seconds)

    std::string conceptId;           // Spawn (reserved)

    std::vector<ActionNode> children;   // Sequence / Parallel

    nlohmann::json toJson() const;
    static ActionNode fromJson(const nlohmann::json& j);

    // Tree → closure, once. The tree remains the law's text.
    ECA::ActionExecutor compile() const;

    // One-line human summary for ApplicationRecord logs.
    std::string describe() const;

    // Factories.
    static ActionNode set(const std::string& dottedPath, PropertyValue v);
    static ActionNode add(const std::string& dottedPath, double delta);
    static ActionNode scale(const std::string& dottedPath, double factor);
    static ActionNode drive(const std::string& dottedPath, CurveModel curve,
                            const std::string& inputPath = "");
    static ActionNode sequence(std::vector<ActionNode> children);
};

// A law's action model is the root of one such tree.
using ActionModel = ActionNode;
