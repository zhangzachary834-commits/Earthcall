// A successful PropertyPath write is not complete until every Law that hears
// that property can react to the new value.
//
// WHY THIS TEST EXISTS (2026-09-15, GPT-5.6 Sol, following Claude Opus 5's
// 2026-09-14 reachability probe in
// agent intercom/communication-threads/ontomath-light-and-image/OntoMath_Image_Ingestion_Phase_1_Update.md):
// the Phase-2 PropertyPath rewrite made pointer traversal correct while briefly
// announcing dotted dynamic writes under only their final segment. A write to
// `image.pixelWidth` therefore read back as 200 while the Rete fact remained
// forever at 50. Value-correct tests stayed green while authored Laws went deaf.
//
// This is deliberately a LAW-FIRES oracle, not another value-readback test.
// It guards three distinct seams:
//   1. a plain dynamic property (control / baseline),
//   2. a flat dotted dynamic key (`image.pixelWidth`), and
//   3. a component write reached through a PropertyDict + Singular pointer,
//      where notification must move to the nested Singular and retain that
//      being's full dotted key (`region.tint`, not `g` or the macro path).
//
// If PropertyPath::resolve/setValue, Singular::notifyPropertyChanged, the
// ChangeFeed, Prophetic relevance filtering, or Rete property-state addressing
// disagree about either OWNER or NAME, one of these witnesses stays at zero.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

void writeNumber(Singular& being, const char* path, double value) {
    const auto result = PropertyPath::parse(path).setValue(
        being, PropertyValue(static_cast<float>(value)));
    assert((result == PropertyPath::PathResult::Ok ||
            result == PropertyPath::PathResult::Unchanged) &&
           "test setup write failed");
}

double readNumber(Singular& being, const char* path) {
    PropertyValue value;
    const auto result = PropertyPath::parse(path).getValue(being, value);
    assert(result == PropertyPath::PathResult::Ok && "test witness read failed");
    double number = 0.0;
    assert(propertyValueToNumber(value, number) && "test witness was not numeric");
    return number;
}

bool near(double a, double b) {
    return std::fabs(a - b) < 1e-4;
}

class MacroImageObject : public Object {
public:
    using Object::Object;
    Singular* region = nullptr;
    Singular* readRegion() const { return region; }
protected:
    void buildProperties() override {
        Object::buildProperties();
        registerProperty(std::make_unique<ComputedProperty<MacroImageObject, Singular*>>(
            "region", this, &MacroImageObject::readRegion));
    }
};

} // namespace

int main() {
    Object author;
    author.setObjectID("author.dynamic-property-reachability");

    Object plain;
    plain.setObjectID("subject.plain-dynamic");
    plain.setDynamicProperty("beacon", PropertyValue(50.0f));
    writeNumber(plain, "position.z", 0.0);

    Object dotted;
    dotted.setObjectID("subject.dotted-dynamic");
    dotted.setDynamicProperty("image.pixelWidth", PropertyValue(50.0f));
    writeNumber(dotted, "position.z", 0.0);

    MacroImageObject macro;
    macro.setObjectID("image.reachability-macro");
    Object region;
    region.setObjectID("image.reachability-macro.region.sky");
    region.setDynamicProperty("region.tint", PropertyValue(glm::vec3(0.1f, 0.2f, 0.3f)));
    writeNumber(region, "position.z", 0.0);
    macro.region = &region;

    std::vector<Singular*> population{&author, &plain, &dotted, &macro, &region};
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        for (Singular* being : population) out.push_back(being);
    });
    Universe::instance().setClock(100.0, 0.1);

    {
        LawManager manager;
        manager.connectToEventBus();

        auto plainLaw = manager.createLaw("dynamic reachability: plain", {&author});
        plainLaw->setLawIdentifier("test-dynamic-reachability-plain");
        plainLaw->setActivation(Law::Activation::WhileTrue);
        plainLaw->setConditionModel(ConditionNode::compare(
            "beacon", ConditionNode::Op::Gt, PropertyValue(100.0f)));
        plainLaw->setActionModel(ActionNode::add("position.z", 1.0));

        auto dottedLaw = manager.createLaw("dynamic reachability: dotted", {&author});
        dottedLaw->setLawIdentifier("test-dynamic-reachability-dotted");
        dottedLaw->setActivation(Law::Activation::WhileTrue);
        dottedLaw->setConditionModel(ConditionNode::compare(
            "image.pixelWidth", ConditionNode::Op::Gt, PropertyValue(100.0f)));
        dottedLaw->setActionModel(ActionNode::add("position.z", 1.0));

        auto regionLaw = manager.createLaw("dynamic reachability: nested owner", {&author});
        regionLaw->setLawIdentifier("test-dynamic-reachability-region");
        regionLaw->setActivation(Law::Activation::WhileTrue);
        regionLaw->setConditionModel(ConditionNode::compare(
            "region.tint.g", ConditionNode::Op::Gt, PropertyValue(0.8f)));
        regionLaw->setActionModel(ActionNode::add("position.z", 1.0));

        // Seed/compile the live Rete while every condition is false. This is the
        // state in which the old bug became permanent: a stale false fact never
        // entered terminal memory even after the underlying value changed.
        manager.tick();
        assert(near(readNumber(plain, "position.z"), 0.0));
        assert(near(readNumber(dotted, "position.z"), 0.0));
        assert(near(readNumber(region, "position.z"), 0.0));

        // LawManager's per-tick state seeding may be skipped when the Universe
        // structure is unchanged. These value-only writes must still wake the
        // Rete via the property-change callback, without changing that revision.
        const uint64_t seededRevision = Universe::instance().structuralRevision();

        const auto plainResult = PropertyPath::parse("beacon").setValue(
            plain, PropertyValue(200.0f));
        const auto dottedResult = PropertyPath::parse("image.pixelWidth").setValue(
            dotted, PropertyValue(200.0f));
        const auto regionResult = PropertyPath::parse(
            "region.region.tint.g").setValue(
                macro, PropertyValue(0.9f));

        assert(plainResult == PropertyPath::PathResult::Ok);
        assert(dottedResult == PropertyPath::PathResult::Ok);
        assert(regionResult == PropertyPath::PathResult::Ok);
        assert(Universe::instance().structuralRevision() == seededRevision &&
               "value-only property writes must not be mistaken for structural edits");

        // Readback is useful diagnosis, but NOT the verdict. The historical bug
        // passed these assertions. The three action witnesses below are the gate.
        assert(near(readNumber(plain, "beacon"), 200.0));
        assert(near(readNumber(dotted, "image.pixelWidth"), 200.0));
        assert(near(readNumber(macro, "region.region.tint.g"), 0.9));

        manager.tick();

        const double plainFires = readNumber(plain, "position.z");
        const double dottedFires = readNumber(dotted, "position.z");
        const double regionFires = readNumber(region, "position.z");

        std::printf("plain dynamic write        -> Law witness %.0f (expected 1)\n", plainFires);
        std::printf("dotted dynamic write       -> Law witness %.0f (expected 1)\n", dottedFires);
        std::printf("nested dotted component    -> Law witness %.0f (expected 1)\n", regionFires);

        assert(near(plainFires, 1.0) &&
               "baseline dynamic property write did not reach its Law");
        assert(near(dottedFires, 1.0) &&
               "dotted dynamic property changed but its Law stayed deaf");
        assert(near(regionFires, 1.0) &&
               "nested write notified the wrong owner/name; region Law stayed deaf");

        // The opposite half of the revision gate: a new being admitted by the
        // provider AFTER a steady tick must receive its initial state facts.
        // No object-created ECA echo is published here, so a working gate must
        // rely on the explicit structural revision signal rather than events.
        Object newcomer;
        newcomer.setObjectID("subject.newborn-revision-gate");
        newcomer.setDynamicProperty("beacon", PropertyValue(200.0f));
        writeNumber(newcomer, "position.z", 0.0);
        population.push_back(&newcomer);
        Universe::instance().bumpStructuralRevision();
        manager.tick();
        assert(near(readNumber(newcomer, "position.z"), 1.0) &&
               "structural revision must seed a newly admitted being");
        population.pop_back();

        // ---------------------------------------------------------------------
        // ValueCell Multi-Alias Rete Fan-Out Witness (Rung 1):
        // Object A and Object B share a scalar cell ("scalarVal") and a vector cell ("color").
        // A WhileTrue law watches Object B ("scalarVal" > 50.0 and "color.r" > 0.5).
        // Mutating through Object A must fan out change notifications to Object B
        // and wake Object B's law in the Prophetic Rete without alias deafness.
        // ---------------------------------------------------------------------
        Object sharedA;
        sharedA.setObjectID("subject.shared-cell-a");
        Object sharedB;
        sharedB.setObjectID("subject.shared-cell-b");

        // Allocate shared scalar cell and shared vector cell
        CellId scalarCell = PropertyCellTable::instance().allocateCell(
            PropertyValue(10.0), CellOwnershipMode::Shared, &sharedA, Earthcall::StringInterner::intern("scalarVal"));
        sharedA.bindPropertyCell("scalarVal", scalarCell, CellOwnershipMode::Shared);
        sharedB.bindPropertyCell("scalarVal", scalarCell, CellOwnershipMode::Shared);

        CellId vectorCell = PropertyCellTable::instance().allocateCell(
            PropertyValue(glm::vec3(0.0f, 0.0f, 0.0f)), CellOwnershipMode::Shared, &sharedA, Earthcall::StringInterner::intern("color"));
        sharedA.bindPropertyCell("color", vectorCell, CellOwnershipMode::Shared);
        sharedB.bindPropertyCell("color", vectorCell, CellOwnershipMode::Shared);

        writeNumber(sharedA, "position.z", 0.0);
        writeNumber(sharedB, "position.z", 0.0);

        population.push_back(&sharedA);
        population.push_back(&sharedB);
        Universe::instance().bumpStructuralRevision();

        auto sharedScalarLaw = manager.createLaw("shared cell: scalar fanout", {&author});
        sharedScalarLaw->setLawIdentifier("test-shared-cell-scalar");
        sharedScalarLaw->setActivation(Law::Activation::WhileTrue);
        sharedScalarLaw->setConditionModel(ConditionNode::compare(
            "scalarVal", ConditionNode::Op::Gt, PropertyValue(50.0)));
        sharedScalarLaw->setActionModel(ActionNode::add("position.z", 1.0));

        auto sharedVectorLaw = manager.createLaw("shared cell: vector fanout", {&author});
        sharedVectorLaw->setLawIdentifier("test-shared-cell-vector");
        sharedVectorLaw->setActivation(Law::Activation::WhileTrue);
        sharedVectorLaw->setConditionModel(ConditionNode::compare(
            "color.r", ConditionNode::Op::Gt, PropertyValue(0.5f)));
        sharedVectorLaw->setActionModel(ActionNode::add("position.z", 1.0));

        manager.tick();
        assert(near(readNumber(sharedA, "position.z"), 0.0));
        assert(near(readNumber(sharedB, "position.z"), 0.0));

        // Now mutate strictly through sharedA!
        const auto setScalarRes = PropertyPath::parse("scalarVal").setValue(sharedA, PropertyValue(75.0));
        assert(setScalarRes == PropertyPath::PathResult::Ok);
        const auto setVecRes = PropertyPath::parse("color.r").setValue(sharedA, PropertyValue(0.9f));
        assert(setVecRes == PropertyPath::PathResult::Ok);

        // Verify sharedB reads the updated values
        PropertyValue bScalar, bColorR;
        assert(PropertyPath::parse("scalarVal").getValue(sharedB, bScalar) == PropertyPath::PathResult::Ok);
        assert(std::get<double>(bScalar) == 75.0);
        assert(PropertyPath::parse("color.r").getValue(sharedB, bColorR) == PropertyPath::PathResult::Ok);
        assert(std::get<float>(bColorR) == 0.9f);

        // Tick manager and assert both laws fired for sharedB!
        manager.tick();
        const double sharedAFires = readNumber(sharedA, "position.z");
        const double sharedBFires = readNumber(sharedB, "position.z");
        std::printf("shared-cell writes on A -> A fires %.0f, B fires %.0f (expected 2 on B)\n", sharedAFires, sharedBFires);
        assert(near(sharedBFires, 2.0) && "mutations on shared cell via A failed to wake WhileTrue Law on B (alias deafness gap)");

        population.pop_back(); // pop sharedB
        population.pop_back(); // pop sharedA
    }

    Universe::instance().setProvider(nullptr);
    std::printf("dynamic_property_reachability_test: OK\n");
    return 0;
}
