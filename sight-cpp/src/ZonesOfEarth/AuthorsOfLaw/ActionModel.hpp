#pragma once

#include "ECA.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"
#include "MathBinding.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/Expression.hpp"
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
        Spawn = 7,     // instantiate from an ObjectConcept into a World
        Map = 8,       // path := f(bindings) — output governed by an authored
                       // OntoMath function of authored inputs (Drive's
                       // multivariate, piecewise, exact elder sibling)
        Flow = 9,      // path := path + f(bindings) * dt — the RATE form:
                       // the authored model is dp/dt, integrated each tick.
                       // Map authors the position F(t); Flow authors its
                       // derivative F'(t) — OntoMath's exact derivative/
                       // antiderivative make them exact counterparts.
        Publish = 10   // MINT an event: laws stop merely consuming the event
                       // vocabulary and start authoring it — perception laws
                       // ("publish contact-perceived(a, b) whenever they
                       // overlap") become ordinary text. Cascades stay under
                       // the anti-Babel ceiling (kMaxChainRounds).
    };

    Kind kind = Kind::Set;

    PropertyPath path;               // what changes
    PropertyValue operand;           // Set value / Add delta / Scale factor / Lerp target
    double factor = 1.0;             // Lerp blend

    CurveModel curve;                // Drive
    PropertyPath input;              // Drive domain; empty => event timestamp (seconds)

    std::string conceptId;           // Spawn (reserved)

    // Publish payload. Participant tokens: "" = the law's subject (for the
    // event's subject) / none (for its object); "@event.subject" and
    // "@event.object" = the TRIGGERING event's participants; anything else
    // = a being id. An unproven SUBJECT token publishes nothing.
    std::string eventType;
    std::string publishSubject;
    std::string publishObject;

    // Map payload: the authored function and where each of its variables
    // lives on the subject. Undefined math (unbound variable, outside every
    // piece) writes NOTHING — a law never manifests undefined values.
    OntoMath::Piecewise mapFunction;
    MathBindings bindings;

    std::vector<ActionNode> children;   // Sequence / Parallel

    nlohmann::json toJson() const;
    static ActionNode fromJson(const nlohmann::json& j);

    // Tree → closure, once. The tree remains the law's text.
    ECA::ActionExecutor compile() const;

    // One-line human summary for ApplicationRecord logs.
    std::string describe() const;

    // ------------------------------------------------------------------
    // Drive-session scans (a law that keeps applying after its trigger).
    // definedFor: can this action still act on the subject RIGHT NOW —
    // i.e. is any node's function defined at the current values of its
    // bound variables? The authored bounds are the duration, and ANY bound
    // variable may cut them: time, another being's position, the subject's
    // own state. Function-free leaves (Set/Add/...) have no bounds and are
    // always defined — an eternal drive unless the author bounds a function.
    // Call within the law's application context so time.sinceApplied (one
    // input variable among the rest) resolves.
    // referencesSinceApplied: does any node read the sinceApplied clock?
    // (a UI hint — such an action usually wants the law to drive).
    // ------------------------------------------------------------------
    bool referencesSinceApplied() const;
    bool definedFor(Singular& subject) const;

    // Factories.
    static ActionNode set(const std::string& dottedPath, PropertyValue v);
    static ActionNode add(const std::string& dottedPath, double delta);
    static ActionNode scale(const std::string& dottedPath, double factor);
    static ActionNode drive(const std::string& dottedPath, CurveModel curve,
                            const std::string& inputPath = "");
    static ActionNode map(const std::string& dottedPath, OntoMath::Piecewise function,
                          MathBindings bindings);
    static ActionNode flow(const std::string& dottedPath, OntoMath::Piecewise function,
                           MathBindings bindings);
    static ActionNode publish(const std::string& type,
                              const std::string& subjectToken = "",
                              const std::string& objectToken = "");
    static ActionNode sequence(std::vector<ActionNode> children);
};

// A law's action model is the root of one such tree.
using ActionModel = ActionNode;
