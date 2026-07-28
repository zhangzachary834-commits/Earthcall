#pragma once

#include "ECA.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"
#include "MathBinding.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "json.hpp"

#include <string>
#include <vector>
#include <ctime>

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
        Publish = 10,  // MINT an event: laws stop merely consuming the event
                       // vocabulary and start authoring it — perception laws
                       // ("publish contact-perceived(a, b) whenever they
                       // overlap") become ordinary text. Cascades stay under
                       // the anti-Babel ceiling (kMaxChainRounds).
        // ------------------------------------------------------------------
        // Creation from nothing. Spawn instantiates a REMEMBERED thing (an
        // ObjectConcept captured from a selection); these three let a law
        // author a being it was never shown:
        //
        //   Create      mint a generic Object of an authored shape kind and
        //               place it in the World. Its children run WITH THE
        //               NEWBORN AS SUBJECT, so the same Set/Map/AddProperty/
        //               AddElement vocabulary shapes it — creation and
        //               modification are one language.
        //   AddProperty grant a being a property it did not have. The
        //               first-mover registry is the vocabulary the engine
        //               gave; this is the vocabulary a Person adds. Persisted
        //               with the being, refused where it would shadow a
        //               registered name (a silent shadow is a trap).
        //   AddElement  compose: put a being inside another's element
        //               Formation. What a thing is MADE OF becomes authorable.
        //
        // ...and their counterparts, because a world that can only grow is
        // not a world a Person can keep:
        //
        //   RemoveProperty  take a granted property back. An AUTHORED property
        //                   is erased outright. A FIRST-MOVER property cannot
        //                   be erased — the member exists in C++ — so it is
        //                   CLEARED to its empty value instead, and the record
        //                   says which of the two happened. Honest either way.
        //   RemoveElement   decompose: take a being back out of a container.
        //                   The element itself keeps living; only the
        //                   membership ends.
        //   Destroy         remove an Object from the World — the delete tool
        //                   as law-text. Every element Formation that held it
        //                   releases it first, so no Formation is left
        //                   pointing at a being that no longer exists.
        // ------------------------------------------------------------------
        Create = 11,
        AddProperty = 12,
        AddElement = 13,
        RemoveProperty = 14,
        RemoveElement = 15,
        Destroy = 16
    };

    struct ExecutedEvent {
        std::string actionName;
        Singular* target;
        std::time_t timestamp;
    };

    static const char* kindName(Kind k);

    Kind kind = Kind::Set;

    PropertyPath path;               // what changes
    PropertyValue operand;           // Set value / Add delta / Scale factor / Lerp target
    double factor = 1.0;             // Lerp blend

    CurveModel curve;                // Drive
    PropertyPath input;              // Drive domain; empty => event timestamp (seconds)

    std::string conceptId;           // Spawn (reserved)
    PropertyPath spawnParentPath;    // Spawn (optional parent object)
    PropertyPath spawnPlacementPath; // Spawn (optional placement coordinate)

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

    // Create payload. The newborn is a plain Object of this shape kind
    // (Object::ShapeKind as int — APPEND-ONLY over there too), placed by
    // spawnPlacementPath (vec3 or mat4 read off the subject; absent = the
    // subject's own position) and optionally parented by spawnParentPath.
    // createType labels it (Object::objectType) so conditions can select the
    // kind of thing a law makes. Placement/parent slots are shared with
    // Spawn deliberately: same question, same answer.
    int createShapeKind = 0;
    std::string createType;

    // AddProperty / RemoveProperty payload. `path` names WHOSE property (the
    // ordinary referent vocabulary: plain = subject, @being-id,
    // @event.subject/object) and propertyName is the leaf. For AddProperty,
    // `operand` is its opening value — a property granted without a value is
    // a name with nothing behind it.
    std::string propertyName;

    // AddElement / RemoveElement payload. Both are participant tokens in the
    // same vocabulary Publish uses: "" = the law's subject,
    // "@event.subject"/"@event.object", or a being id. containerToken must
    // resolve to an Object (only Objects hold elements today); elementToken
    // may be any Singular. Destroy uses elementToken for its victim, so
    // "destroy whatever I collided with" is Destroy("@event.object").
    std::string containerToken;
    std::string elementToken;

    std::vector<ActionNode> children;   // Sequence / Parallel; Create (on the newborn)

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
    static ActionNode sequence(std::vector<ActionNode> children);
    static ActionNode parallel(std::vector<ActionNode> children);
    static ActionNode spawn(const std::string& conceptId, const std::string& spawnParentPath = "");
    static ActionNode publish(const std::string& type,
                              const std::string& subjectToken = "",
                              const std::string& objectToken = "");

    // Creation, composition, and their counterparts. `shapeKind` is
    // Object::ShapeKind as int (0 = Cube); `children` shape the newborn.
    static ActionNode create(int shapeKind = 0,
                             const std::string& createType = "",
                             std::vector<ActionNode> children = {});
    static ActionNode addProperty(const std::string& ownerPath,
                                  const std::string& propertyName,
                                  PropertyValue initial);
    static ActionNode removeProperty(const std::string& ownerPath,
                                     const std::string& propertyName);
    static ActionNode addElement(const std::string& containerToken,
                                 const std::string& elementToken);
    static ActionNode removeElement(const std::string& containerToken,
                                    const std::string& elementToken);
    static ActionNode destroy(const std::string& targetToken = "");
};

// A law's action model is the root of one such tree.
using ActionModel = ActionNode;
