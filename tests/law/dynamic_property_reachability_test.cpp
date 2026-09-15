// A successful PropertyPath write is not complete until every Law that hears
// that property can react to the new value.
//
// WHY THIS TEST EXISTS (2026-09-15, GPT-5.6 Sol, following Claude Opus 5's
// 2026-09-14 reachability probe in
// agent intercom/communication-threads/OntoMath_Image_Ingestion_Phase_1_Update.md):
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

    Object macro;
    macro.setObjectID("image.reachability-macro");
    Object region;
    region.setObjectID("image.reachability-macro.region.sky");
    region.setDynamicProperty("region.tint", PropertyValue(glm::vec3(0.1f, 0.2f, 0.3f)));
    writeNumber(region, "position.z", 0.0);

    auto regions = std::make_shared<PropertyDict>();
    regions->elements["sky"] = PropertyValue(static_cast<Singular*>(&region));
    macro.setDynamicProperty("image.regions", PropertyValue(regions));

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

        const auto plainResult = PropertyPath::parse("beacon").setValue(
            plain, PropertyValue(200.0f));
        const auto dottedResult = PropertyPath::parse("image.pixelWidth").setValue(
            dotted, PropertyValue(200.0f));
        const auto regionResult = PropertyPath::parse(
            "image.regions.sky.region.tint.g").setValue(
                macro, PropertyValue(0.9f));

        assert(plainResult == PropertyPath::PathResult::Ok);
        assert(dottedResult == PropertyPath::PathResult::Ok);
        assert(regionResult == PropertyPath::PathResult::Ok);

        // Readback is useful diagnosis, but NOT the verdict. The historical bug
        // passed these assertions. The three action witnesses below are the gate.
        assert(near(readNumber(plain, "beacon"), 200.0));
        assert(near(readNumber(dotted, "image.pixelWidth"), 200.0));
        assert(near(readNumber(macro, "image.regions.sky.region.tint.g"), 0.9));

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
    }

    Universe::instance().setProvider(nullptr);
    std::printf("dynamic_property_reachability_test: OK\n");
    return 0;
}
