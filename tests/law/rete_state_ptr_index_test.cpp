// Dedicated unit test and invalidation witness for ReteNetwork's
// pointer-indexed state fact map (_stateFactsBySubjectPtrAttr) and the
// scalar pointer fast-path markFactDirty(const Singular*, const std::string&).

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

FactPtr createPositionFact(Object& object, const std::string& id) {
    Property* position = object.findProperty("position");
    assert(position);

    auto fact = std::make_shared<ReteFact>();
    fact->id = id;
    fact->type = "state";
    fact->subject = &object;
    fact->subjectId = object.getIdentifier();
    fact->attribute = "position";
    fact->value = propertyValueToJson(position->value());
    fact->isState = true;
    fact->dirty = false;
    return fact;
}

FactPtr createHealthFact(Object& object, const std::string& id) {
    auto fact = std::make_shared<ReteFact>();
    fact->id = id;
    fact->type = "state";
    fact->subject = &object;
    fact->subjectId = object.getIdentifier();
    fact->attribute = "health";
    fact->value = 100.0f;
    fact->isState = true;
    fact->dirty = false;
    return fact;
}

} // namespace

int main() {
    // ---------------------------------------------------------------------
    // 1. Pointer-path markFactDirty succeeds after assertion and fails after retractFact
    // ---------------------------------------------------------------------
    ReteNetwork rete;
    Object subject;
    subject.setObjectID("rete-ptr-test.subject");
    subject.setPosition(glm::vec3(1.0f, 2.0f, 3.0f));

    const FactPtr posFact = createPositionFact(subject, "pos-fact-1");
    const FactPtr hpFact = createHealthFact(subject, "hp-fact-1");

    // Before assertion: pointer fast path returns false
    assert(!rete.markFactDirty(&subject, "position"));
    assert(!rete.markFactDirty(subject.getIdentifier(), "position"));

    rete.assertFact(posFact);
    rete.assertFact(hpFact);

    // After assertion: both pointer fast path and string fallback succeed
    assert(rete.markFactDirty(&subject, "position"));
    assert(posFact->dirty);
    posFact->dirty = false;

    assert(rete.markFactDirty(&subject, "health"));
    assert(hpFact->dirty);
    hpFact->dirty = false;

    // Retracting one fact: position retracts, health remains
    assert(rete.retractFact(posFact->id));
    assert(!rete.markFactDirty(&subject, "position"));
    assert(!rete.markFactDirty(subject.getIdentifier(), "position"));
    assert(rete.markFactDirty(&subject, "health"));
    hpFact->dirty = false;

    // ---------------------------------------------------------------------
    // 2. retractStateFactsBySubject explicitly exercises state fact retraction
    // ---------------------------------------------------------------------
    Object subjectBySubj;
    subjectBySubj.setObjectID("rete-ptr-test.subjectBySubj");
    const FactPtr posFactBySubj = createPositionFact(subjectBySubj, "pos-fact-bysubj");
    const FactPtr hpFactBySubj = createHealthFact(subjectBySubj, "hp-fact-bysubj");
    rete.assertFact(posFactBySubj);
    rete.assertFact(hpFactBySubj);

    assert(rete.markFactDirty(&subjectBySubj, "position"));
    assert(rete.markFactDirty(&subjectBySubj, "health"));

    // Retract all state facts for subjectBySubj by subject ID
    rete.retractStateFactsBySubject(subjectBySubj.getIdentifier());
    assert(!rete.markFactDirty(&subjectBySubj, "position"));
    assert(!rete.markFactDirty(subjectBySubj.getIdentifier(), "position"));
    assert(!rete.markFactDirty(&subjectBySubj, "health"));
    assert(!rete.markFactDirty(subjectBySubj.getIdentifier(), "health"));

    // ---------------------------------------------------------------------
    // 3. retractFactsAbout / being release clears pointer index entries
    // ---------------------------------------------------------------------
    Object subject2;
    subject2.setObjectID("rete-ptr-test.subject2");
    const FactPtr posFact2 = createPositionFact(subject2, "pos-fact-2");
    rete.assertFact(posFact2);

    assert(rete.markFactDirty(&subject2, "position"));
    posFact2->dirty = false;

    // Retract facts about subject2
    rete.retractFactsAbout(&subject2);
    assert(!rete.markFactDirty(&subject2, "position"));
    assert(!rete.markFactDirty(subject2.getIdentifier(), "position"));

    // ---------------------------------------------------------------------
    // 4. clearFacts purges pointer index completely
    // ---------------------------------------------------------------------
    Object subject3;
    subject3.setObjectID("rete-ptr-test.subject3");
    const FactPtr posFact3 = createPositionFact(subject3, "pos-fact-3");
    rete.assertFact(posFact3);

    assert(rete.markFactDirty(&subject3, "position"));
    posFact3->dirty = false;

    rete.clearFacts();
    assert(!rete.markFactDirty(&subject3, "position"));
    assert(!rete.markFactDirty(&subject, "health"));

    // ---------------------------------------------------------------------
    // 5. True runtime-granted property fallback seeding through LawManager
    // ---------------------------------------------------------------------
    Object authorObj;
    authorObj.setObjectID("test-author");

    LawManager manager;
    manager.connectToEventBus();

    // Create a law reading a property "runtimeGranted" that does NOT exist on testObj yet
    auto law = manager.createLaw("test-runtime-granted-law", {&authorObj});
    law->setLawIdentifier("test-runtime-granted-law");
    law->setActivation(Law::Activation::WhileTrue);
    law->setConditionModel(ConditionNode::compare(
        "runtimeGranted", ConditionNode::Op::Gt, PropertyValue(0.0f)));

    auto testObj = std::make_shared<Object>();
    testObj->setObjectID("runtime-granted-obj");

    std::vector<Singular*> worldBeings = { testObj.get() };
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.insert(out.end(), worldBeings.begin(), worldBeings.end());
    });

    // Seed initial facts & prophetic index for testObj (runtimeGranted does not exist yet)
    manager.tick();

    // Before granting property: no fact for runtimeGranted
    assert(!manager.rete().markFactDirty(testObj.get(), "runtimeGranted"));

    // Grant property at runtime (setDynamicProperty triggers notifyPropertyChanged)
    testObj->setDynamicProperty("runtimeGranted", PropertyValue(42.0f));

    // Fallback branch in setPropertyChangeCallback seeded exactly one property-state fact
    assert(manager.rete().markFactDirty(testObj.get(), "runtimeGranted"));
    assert(manager.rete().hasDirtyFacts());

    // Count facts for runtimeGranted to ensure exactly 1 state fact exists
    std::size_t runtimeFactCount = 0;
    for (const auto& fact : manager.rete().facts()) {
        if (fact->subject == testObj.get() && fact->attribute == "runtimeGranted") {
            ++runtimeFactCount;
        }
    }
    assert(runtimeFactCount == 1);

    // Updating value again dirties the existing fact without adding duplicates
    manager.tick(); // clear dirty facts
    testObj->setDynamicProperty("runtimeGranted", PropertyValue(84.0f));
    assert(manager.rete().hasDirtyFacts()); // callback itself heard it

    runtimeFactCount = 0;
    for (const auto& fact : manager.rete().facts()) {
        if (fact->subject == testObj.get() && fact->attribute == "runtimeGranted") {
            ++runtimeFactCount;
        }
    }
    assert(runtimeFactCount == 1);

    // ---------------------------------------------------------------------
    // 6. A/B Performance benchmark: Pointer fast-path vs String lookup
    //    Isolates lookup & dirty-marking work by periodically evaluating dirty facts.
    // ---------------------------------------------------------------------
    constexpr int kIterations = 1000000;

    // Run 1: Fresh ReteNetwork for String Path
    ReteNetwork stringRete;
    Object stringSubject;
    stringSubject.setObjectID("bench-object-id-string-for-testing");
    const FactPtr stringPos = createPositionFact(stringSubject, "string-pos");
    stringRete.assertFact(stringPos);

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        stringRete.markFactDirty(stringSubject.getIdentifier(), "position");
        stringPos->dirty = false;
        if (i % 1000 == 0) stringRete.evaluateDirty();
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    // Run 2: Fresh ReteNetwork for Pointer Fast-Path
    ReteNetwork pointerRete;
    Object pointerSubject;
    pointerSubject.setObjectID("bench-object-id-string-for-testing");
    const FactPtr pointerPos = createPositionFact(pointerSubject, "pointer-pos");
    pointerRete.assertFact(pointerPos);

    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        pointerRete.markFactDirty(&pointerSubject, "position");
        pointerPos->dirty = false;
        if (i % 1000 == 0) pointerRete.evaluateDirty();
    }
    auto t3 = std::chrono::high_resolution_clock::now();

    double stringMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double pointerMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::printf("Benchmark over %d iterations (periodic dirty draining every 1000 cycles):\n", kIterations);
    std::printf("  String Path : %.2f ms\n", stringMs);
    std::printf("  Pointer Path: %.2f ms\n", pointerMs);
    if (pointerMs > 0.0) {
        std::printf("  Speedup     : %.2fx faster\n", stringMs / pointerMs);
    }

    std::puts("rete_state_ptr_index_test: ALL OK");
    return 0;
}
