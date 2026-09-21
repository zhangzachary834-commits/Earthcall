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
    // 2. retractFactsAbout / being release clears pointer index entries
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
    // 3. clearFacts purges pointer index completely
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
    // 4. Runtime-granted property fallback seeding through LawManager
    // ---------------------------------------------------------------------
    Object authorObj;
    authorObj.setObjectID("test-author");

    LawManager manager;
    manager.connectToEventBus();

    auto law = manager.createLaw("test-pos-law", {&authorObj});
    law->setLawIdentifier("test-pos-law");
    law->setActivation(Law::Activation::WhileTrue);
    law->setConditionModel(ConditionNode::compare(
        "position", ConditionNode::Op::Gt, PropertyValue(0.0f)));

    auto testObj = std::make_shared<Object>();
    testObj->setObjectID("runtime-granted-obj");
    testObj->setPosition(glm::vec3(1.0f, 1.0f, 1.0f));

    std::vector<Singular*> worldBeings = { testObj.get() };
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.insert(out.end(), worldBeings.begin(), worldBeings.end());
    });

    // Seed initial facts & prophetic index
    manager.tick();

    // Modify existing property triggers pointer fast-path
    Singular::notifyPropertyChanged(testObj.get(), "position");
    assert(manager.rete().hasDirtyFacts());

    // ---------------------------------------------------------------------
    // 5. A/B Performance benchmark: Pointer fast-path vs String lookup
    // ---------------------------------------------------------------------
    ReteNetwork benchRete;
    Object benchSubject;
    benchSubject.setObjectID("bench-object-id-string-for-testing");
    const FactPtr benchPos = createPositionFact(benchSubject, "bench-pos");
    benchRete.assertFact(benchPos);

    constexpr int kIterations = 500000;

    // Benchmark String Path
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        benchRete.markFactDirty(benchSubject.getIdentifier(), "position");
        benchPos->dirty = false;
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    // Benchmark Pointer Fast-Path
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        benchRete.markFactDirty(&benchSubject, "position");
        benchPos->dirty = false;
    }
    auto t3 = std::chrono::high_resolution_clock::now();

    double stringMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double pointerMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::printf("Benchmark over %d iterations:\n", kIterations);
    std::printf("  String Path : %.2f ms\n", stringMs);
    std::printf("  Pointer Path: %.2f ms\n", pointerMs);
    if (pointerMs > 0.0) {
        std::printf("  Speedup     : %.2fx faster\n", stringMs / pointerMs);
    }

    std::puts("rete_state_ptr_index_test: ALL OK");
    return 0;
}
