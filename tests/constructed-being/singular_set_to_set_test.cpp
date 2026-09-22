// Singular set-to-set creation (LAW_AND_CREATION_SYSTEM.md §7, and the
// manifesto's "Object/Singular set-to-set creation").
//
// This file replaces synthesis_system_test.cpp. The system it tested — a
// second Concept type, a second registry, a second mapping struct and a second
// governance rule, all reached through ActionNode::Synthesize — was a parallel
// implementation of what ObjectConcept already does. Synthesize is now a
// visible composition of Create and the ordinary property actions; its live
// inputs are ordinary @event paths, not a hidden ObjectConcept invocation.
//
// Exercises: capture over beings of any kind, the per-member property
// snapshot (a concept remembers VALUES, not only a recipe), the birth
// refusals, provenance surviving a save/load round trip, the transfer gate
// over multivariable mappings, and Synthesize deriving from the live input
// set the event names.

#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

// Colour through the PROPERTY surface — the same door set-to-set walks
// through, so the test exercises what the system exercises.
void setColor(Singular& being, const glm::vec3& c) {
    assert(PropertyPath::parse("color").setValue(being, PropertyValue(c)) ==
           PropertyPath::PathResult::Ok);
}

glm::vec3 colorOf(Singular& being) {
    PropertyValue v;
    assert(PropertyPath::parse("color").getValue(being, v) == PropertyPath::PathResult::Ok);
    return std::get<glm::vec3>(v);
}

// A concept remembers what its members WERE, not merely how they were shaped:
// instantiated with no live source set at all, the newborn still comes out
// carrying the colour and the authored vocabulary of the being it abstracts.
void testCapturedStateTravels() {
    Object source;
    source.setShape(Object::ShapeKind::Sphere, [] {
        Object::ShapeParams p; p.r = 0.75f; return p; }());
    source.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
    setColor(source, glm::vec3(0.9f, 0.1f, 0.1f));
    source.setDynamicProperty("acoustic.amplitude", PropertyValue(0.5));

    auto concept = ObjectConcept::captureFrom({&source}, "red-sphere");
    assert(concept->members().size() == 1);
    assert(concept->members()[0].beingKind == ConditionNode::BeingKind::Object);
    assert(concept->members()[0].hasGeometry);

    // No source set: the OLD behaviour produced a bare grey sphere, because
    // mappings are the only thing that ever carried a value and mappings only
    // run against a live set.
    auto newborns = concept->instantiate(glm::mat4(1.0f));
    assert(newborns.size() == 1);
    assert(nearf(colorOf(*newborns[0]).r, 0.9f));
    assert(nearf(colorOf(*newborns[0]).g, 0.1f));
    PropertyValue amp;
    assert(PropertyPath::parse("acoustic.amplitude")
               .getValue(*newborns[0], amp) == PropertyPath::PathResult::Ok);
    assert(nearf(static_cast<float>(std::get<double>(amp)), 0.5f));

    // The pose is the PLACEMENT's, never the source's remembered world
    // position — the snapshot must not fight the author's placement.
    assert(nearf(newborns[0]->getPosition().x, 0.0f));
    std::cout << "  captured state travels to a newborn with no source set OK\n";
}

// The snapshot survives the world file, kind and all.
void testCaptureRoundTrip() {
    Object source;
    setColor(source, glm::vec3(0.2f, 0.4f, 0.8f));
    source.setDynamicProperty("ritual.phase", PropertyValue(std::string("waxing")));
    auto concept = ObjectConcept::captureFrom({&source}, "blue-thing");

    auto reborn = ObjectConcept::fromJson(concept->toJson());
    assert(reborn->members().size() == 1);
    assert(reborn->members()[0].beingKind == ConditionNode::BeingKind::Object);

    auto newborns = reborn->instantiate(glm::mat4(1.0f));
    assert(newborns.size() == 1);
    assert(nearf(colorOf(*newborns[0]).b, 0.8f));
    PropertyValue phase;
    assert(PropertyPath::parse("ritual.phase")
               .getValue(*newborns[0], phase) == PropertyPath::PathResult::Ok);
    assert(std::get<std::string>(phase) == "waxing");
    std::cout << "  member kind and captured state round-trip through JSON OK\n";
}

// Ancestry is part of what a being is. toJson always wrote provenance and
// fromJson never read it back, so every abstracted-from and authored-by edge
// died at the next load — and the anti-Babel ceilings are predicates over
// exactly those chains.
void testProvenanceSurvivesLoad() {
    Object author, source;
    auto concept = ObjectConcept::captureFrom({&source}, "traced", &author);
    const std::size_t captured = concept->provenance().getAll().size();
    assert(captured == 2);   // abstracted-from + authored-by

    auto reborn = ObjectConcept::fromJson(concept->toJson());
    assert(reborn->provenance().getAll().size() == captured);
    bool sawAuthor = false;
    for (const auto& edge : reborn->provenance().getAll()) {
        if (edge && edge->type == "authored-by" && edge->bId() == author.getIdentifier()) {
            sawAuthor = true;
        }
    }
    assert(sawAuthor);
    std::cout << "  provenance survives the save/load round trip OK\n";
}

// Every Singular may be a SOURCE — the manifesto's layers 4 and 5. A Person
// carries a property surface like anything else, and set-to-set reads it
// through the same gate.
void testNonObjectSource() {
    Person person(Soul("zack"), Body::createBasicAvatar("Voxel"), "default");
    Object cube;

    std::vector<Singular*> mixed{&person, &cube};
    auto concept = ObjectConcept::captureFromBeings(mixed, "person-and-cube");
    assert(concept->members().size() == 2);
    assert(concept->members()[0].beingKind == ConditionNode::BeingKind::Person);
    assert(concept->members()[1].beingKind == ConditionNode::BeingKind::Object);

    // A Person is never instantiated. The member is refused, not silently
    // downgraded into a cube — so one newborn, from the cube alone.
    auto newborns = concept->instantiate(glm::mat4(1.0f));
    assert(newborns.size() == 1);
    std::cout << "  a Person may be a source and is never a birth OK\n";
}

// The gate applies to the paths a mapping actually READS. A multivariable
// mapping leaves the legacy single `source` empty, and canTransfer({}) is
// false, so gating on that field alone refused every bindings-authored
// mapping before it began.
void testBindingsOnlyMappingTransfers() {
    Object source;
    source.setPosition(glm::vec3(4.0f, 0.0f, 0.0f));
    auto concept = ObjectConcept::captureFrom({&source}, "derived");

    PropertyMapping m;                       // m.source deliberately empty
    m.bindings["a"] = PropertyPath::parse("position.x");
    m.target = PropertyPath::parse("position.y");
    m.hasExact = true;
    m.exact = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("a")));
    concept->addMapping(m);

    std::vector<Object*> sources{&source};
    auto newborns = concept->instantiate(glm::mat4(1.0f), &sources);
    assert(newborns.size() == 1);
    assert(nearf(newborns[0]->getPosition().y, 4.0f));
    std::cout << "  a bindings-only mapping passes the gate and transfers OK\n";
}

// A closed gate still refuses. `enabled` is Gated by default, and a snapshot
// must never remember what it may not take.
void testClosedGateRefusesCapture() {
    Object source;
    source.setDynamicProperty("enabled", PropertyValue(true));
    auto concept = ObjectConcept::captureFrom({&source}, "gated");
    assert(concept->members()[0].captured.count("enabled") == 0);
    std::cout << "  a Gated property is not remembered by a concept OK\n";
}

// Synthesize is no longer an ObjectConcept shortcut. A Create child establishes
// the newborn; its Map child reads the LIVE input set with an @event-qualified
// PropertyPath and writes directly to that newborn. AddProperty demonstrates
// that the newborn's vocabulary is shaped by the same ordinary action tree.
void testSynthesizeBirthsIntoWorld() {
    Zone world("test-zone", "default");
    Object subject;
    subject.setPosition(glm::vec3(7.0f, 0.0f, 0.0f));

    ActionNode map;
    map.kind = ActionNode::Kind::Map;
    map.path = PropertyPath::parse("derived-from-x");
    map.bindings["x"] = PropertyPath::parse("@event.subject.position.x");
    map.mapFunction = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("x")));

    ActionNode grant = ActionNode::addProperty("", "derived-from-x", PropertyValue(0.0));
    ActionNode create = ActionNode::create(0, "derived", {grant, map});
    ActionNode node;
    node.kind = ActionNode::Kind::Synthesize;
    node.children.push_back(create);
    const nlohmann::json persisted = node.toJson();
    assert(persisted.contains("children"));
    assert(!persisted.contains("conceptId"));
    assert(ActionNode::fromJson(persisted).children.size() == 1);

    // Opening and saving an old kind-17 law must not erase the concept text
    // that needs deliberate re-authoring into this visible composition.
    ActionNode legacy;
    legacy.kind = ActionNode::Kind::Synthesize;
    legacy.conceptId = "old-concept";
    const nlohmann::json preservedLegacy = legacy.toJson();
    assert(preservedLegacy.value("conceptId", std::string()) == "old-concept");
    assert(ActionNode::fromJson(preservedLegacy).conceptId == "old-concept");
    auto executor = node.compile();

    const std::size_t before = world.getOwnedObjects().size();
    ECA::Event event{"beings-met", &subject, nullptr, 0};
    Universe::EventScope eventScope(event.subject, event.object);
    executor(event, world);

    assert(world.getOwnedObjects().size() == before + 1);
    const auto& born = world.getOwnedObjects().back();
    PropertyValue derived;
    assert(PropertyPath::parse("derived-from-x").getValue(*born, derived) ==
           PropertyPath::PathResult::Ok);
    assert(nearf(static_cast<float>(std::get<double>(derived)), 7.0f));
    std::cout << "  Synthesize composes Create, AddProperty, and Map over live inputs OK\n";
}

// An un-migrated (or simply empty) Synthesize has no composed Create to run.
// It must refuse loudly and do nothing — no newborn, no ExecutedEvent — never
// guess at a legacy concept that isn't there.
void testSynthesizeRefusesWithNoChildren() {
    Zone world("test-zone", "default");
    Object subject;

    ActionNode node;
    node.kind = ActionNode::Kind::Synthesize;   // no children, no conceptId
    auto executor = node.compile();

    ActionNode::TraceScope trace;
    const std::size_t before = world.getOwnedObjects().size();
    ECA::Event event{"beings-met", &subject, nullptr, 0};
    Universe::EventScope eventScope(event.subject, event.object);
    executor(event, world);

    assert(world.getOwnedObjects().size() == before);
    assert(!trace.trace().anyWrote());
    std::cout << "  Synthesize with no composed Create refuses and births nothing OK\n";
}

// One Synthesize, several Create children: the composition is not limited to
// a single newborn. Each Create is an independent birth into the same World.
void testSynthesizeComposesMultipleCreates() {
    Zone world("test-zone", "default");
    Object subject;

    ActionNode labelFirst = ActionNode::addProperty("", "label", PropertyValue(std::string("first")));
    ActionNode labelSecond = ActionNode::addProperty("", "label", PropertyValue(std::string("second")));
    ActionNode first = ActionNode::create(0, "", {labelFirst});
    ActionNode second = ActionNode::create(0, "", {labelSecond});
    ActionNode node;
    node.kind = ActionNode::Kind::Synthesize;
    node.children.push_back(first);
    node.children.push_back(second);
    auto executor = node.compile();

    const std::size_t before = world.getOwnedObjects().size();
    ECA::Event event{"beings-met", &subject, nullptr, 0};
    Universe::EventScope eventScope(event.subject, event.object);
    executor(event, world);

    assert(world.getOwnedObjects().size() == before + 2);
    PropertyValue labelA, labelB;
    assert(PropertyPath::parse("label").getValue(*world.getOwnedObjects()[before], labelA) ==
           PropertyPath::PathResult::Ok);
    assert(PropertyPath::parse("label").getValue(*world.getOwnedObjects()[before + 1], labelB) ==
           PropertyPath::PathResult::Ok);
    assert(std::get<std::string>(labelA) == "first");
    assert(std::get<std::string>(labelB) == "second");
    std::cout << "  Synthesize composes multiple Create children into multiple newborns OK\n";
}

// The live input set is BOTH event participants, not only the subject.
// A Map bound to @event.object must read the OTHER being the moment brought
// together, exactly as @event.subject does.
void testSynthesizeReadsEventObject() {
    Zone world("test-zone", "default");
    Object subject;
    Object object;
    object.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));

    ActionNode map;
    map.kind = ActionNode::Kind::Map;
    map.path = PropertyPath::parse("derived-from-object-y");
    map.bindings["y"] = PropertyPath::parse("@event.object.position.y");
    map.mapFunction = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("y")));
    ActionNode grant = ActionNode::addProperty("", "derived-from-object-y", PropertyValue(0.0));
    ActionNode create = ActionNode::create(0, "derived-from-object", {grant, map});

    ActionNode node;
    node.kind = ActionNode::Kind::Synthesize;
    node.children.push_back(create);
    auto executor = node.compile();

    ECA::Event event{"beings-met", &subject, &object, 0};
    Universe::EventScope eventScope(event.subject, event.object);
    executor(event, world);

    const auto& born = world.getOwnedObjects().back();
    PropertyValue derived;
    assert(PropertyPath::parse("derived-from-object-y").getValue(*born, derived) ==
           PropertyPath::PathResult::Ok);
    assert(nearf(static_cast<float>(std::get<double>(derived)), 5.0f));
    std::cout << "  Synthesize reads @event.object through a composed Map OK\n";
}

} // namespace

int main() {
    std::cout << "Running Singular set-to-set tests...\n";
    testCapturedStateTravels();
    testCaptureRoundTrip();
    testProvenanceSurvivesLoad();
    testNonObjectSource();
    testBindingsOnlyMappingTransfers();
    testClosedGateRefusesCapture();
    testSynthesizeBirthsIntoWorld();
    testSynthesizeRefusesWithNoChildren();
    testSynthesizeComposesMultipleCreates();
    testSynthesizeReadsEventObject();
    std::cout << "All Singular set-to-set tests passed.\n";
    return 0;
}
