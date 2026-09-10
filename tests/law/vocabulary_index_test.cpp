// The vocabulary index must never narrow.
//
// FORMATION_RETE.md §3.0 and §8 rung 2. `sweepSubjects` no longer walks
// `Universe::beings()` per law; it reads a per-property-name index built once
// per structural change. That is a strict win on cost and a standing risk to
// correctness, because the failure mode is not a wrong answer — it is a law
// that reaches nobody and reports nothing. `PROPHETIC_RETE.md` §2: widen where
// uncertain, never narrow.
//
// So every section here asks the same question in a different world: does the
// law still reach the beings it is about? The oracle is the law firing — if a
// being that satisfies the condition is missing from the candidate set, the
// law never touches it and `shape.fillet` stays 0.
//
// The cases are chosen to be exactly the ones an index gets wrong:
//   A. a property that exists statically vs one granted at runtime
//   B. a property GRANTED after the index was already built
//   C. a law authored after the world, naming a name never indexed
//   D. a being admitted to the world after the index was built
//   E. a property REMOVED — the law must stop reaching it
//   F. a law with no required properties at all — still about everyone
//   G. a required name nobody carries — nobody, and no crash

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

bool nearf(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

double filletOf(Object& obj) {
    PropertyValue v;
    if (PropertyPath::parse("shape.fillet").getValue(obj, v) != PropertyPath::PathResult::Ok) {
        return -12345.0;
    }
    double out = -12345.0;
    propertyValueToNumber(v, out);
    return out;
}

void resetFillet(Object& obj) {
    PropertyPath::parse("shape.fillet").setValue(obj, PropertyValue(0.0f));
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "vocabulary_index_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "vocabulary_index_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "vocabulary_index_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object carrier;    // carries `beacon` from the start
        Object bystander;  // never carries it
        Object latecomer;  // granted `beacon` after the index is built
        Object newcomer;   // admitted to the world after the index is built
        Object loser;      // carries `beacon`, then loses it

        for (Object* o : {&author, &carrier, &bystander, &latecomer, &newcomer, &loser}) {
            resetFillet(*o);
        }
        carrier.setDynamicProperty("beacon", PropertyValue(1.0));
        loser.setDynamicProperty("beacon", PropertyValue(1.0));

        // `newcomer` is deliberately OUT of the world at first.
        std::vector<Singular*> population{&author, &carrier, &bystander, &latecomer, &loser};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();

        auto beaconLaw = mgr.createLaw("beacon-law", {&author});
        beaconLaw->setActivation(Law::Activation::WhileTrue);
        beaconLaw->setConditionModel(ConditionNode::compare(
            "beacon", ConditionNode::Op::Gt, PropertyValue(0.0)));
        beaconLaw->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        // --------------------------------------------------------------
        // A. The base case: static world, index built, law reaches exactly
        //    the beings carrying its vocabulary.
        // --------------------------------------------------------------
        mgr.tick();
        assert(nearf(filletOf(carrier), 0.5) && "a carrier must be reached");
        assert(nearf(filletOf(loser), 0.5) && "a carrier must be reached");
        assert(nearf(filletOf(bystander), 0.0) && "a non-carrier must not be");

        // --------------------------------------------------------------
        // B. A PROPERTY GRANTED AT RUNTIME. `latecomer` gains `beacon` after
        //    the index already exists. Singular::setDynamicProperty bumps the
        //    structural revision on a FIRST grant, which is what forces the
        //    rebuild — if that link ever breaks, this being stays invisible
        //    to the law forever and nothing says so.
        // --------------------------------------------------------------
        latecomer.setDynamicProperty("beacon", PropertyValue(1.0));
        mgr.tick();
        assert(nearf(filletOf(latecomer), 0.5) &&
               "a property granted at runtime must reach the index");

        // --------------------------------------------------------------
        // C. A LAW AUTHORED AFTER THE WORLD, naming a vocabulary the index
        //    has never held. The index is keyed on names some law requires,
        //    so a new name must force a rebuild the same way a structural
        //    change does — otherwise the new law finds an empty entry and
        //    reaches nobody.
        // --------------------------------------------------------------
        carrier.setDynamicProperty("ember", PropertyValue(2.0));
        auto emberLaw = mgr.createLaw("ember-law", {&author});
        emberLaw->setActivation(Law::Activation::WhileTrue);
        emberLaw->setConditionModel(ConditionNode::compare(
            "ember", ConditionNode::Op::Gt, PropertyValue(0.0)));
        emberLaw->setActionModel(ActionNode::set("shape.r", PropertyValue(3.0f)));
        resetFillet(carrier);
        mgr.tick();
        assert(nearf(filletOf(carrier), 0.5) &&
               "a law naming a brand-new vocabulary must not disturb the old one");

        // --------------------------------------------------------------
        // D. A BEING ADMITTED AFTER THE INDEX WAS BUILT.
        // --------------------------------------------------------------
        newcomer.setDynamicProperty("beacon", PropertyValue(1.0));
        population.push_back(&newcomer);
        Universe::instance().bumpStructuralRevision();   // what Zone::addObject does
        mgr.tick();
        assert(nearf(filletOf(newcomer), 0.5) &&
               "a being admitted after the index was built must be reached");

        // --------------------------------------------------------------
        // E. A PROPERTY REMOVED. The law must stop reaching `loser` — and
        //    this is the one direction where the index could be too WIDE
        //    rather than too narrow, which is safe but should still be right:
        //    couldApplyTo re-filters whatever the index proposes.
        // --------------------------------------------------------------
        resetFillet(loser);
        loser.removeDynamicProperty("beacon");
        mgr.tick();
        assert(nearf(filletOf(loser), 0.0) &&
               "a being that lost the vocabulary must not be reached");
        assert(nearf(filletOf(carrier), 0.5) && "the others are untouched by that");

        // --------------------------------------------------------------
        // F. A LAW ABOUT EVERYONE must still reach everyone.
        //
        //    Worth stating because it surprised this test's author:
        //    `rebuildRequiredProperties` collects paths from the ACTION as well
        //    as the condition, and keys on each path's ROOT segment. So an
        //    `IsKind` law — which asks about the being itself and contributes
        //    no condition path — still requires `shape`, because its action
        //    writes `shape.fillet`. The vocabulary a law states is what it
        //    READS AND WRITES, not only what it tests.
        //
        //    That makes this the index's widest case: one name that nearly
        //    every being carries, so the index must return nearly everyone
        //    rather than quietly collapsing to the small entries beside it.
        // --------------------------------------------------------------
        for (Object* o : {&carrier, &bystander, &latecomer, &newcomer, &loser}) {
            resetFillet(*o);
        }
        beaconLaw->setEnabled(false);
        auto everyoneLaw = mgr.createLaw("everyone-law", {&author});
        everyoneLaw->setActivation(Law::Activation::WhileTrue);
        everyoneLaw->setConditionModel(
            ConditionNode::isKind(ConditionNode::BeingKind::Object));
        everyoneLaw->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));
        mgr.tick();
        {
            const auto& req = everyoneLaw->requiredProperties();
            assert(req.size() == 1 && req[0] == "shape" &&
                   "an IsKind law still states the vocabulary its ACTION writes");
        }
        for (Object* o : {&carrier, &bystander, &latecomer, &newcomer, &loser}) {
            assert(nearf(filletOf(*o), 0.5) &&
                   "a law about everyone must still reach everyone");
        }
        everyoneLaw->setEnabled(false);

        // --------------------------------------------------------------
        // G. A REQUIRED NAME NOBODY CARRIES. The index has no entry, which is
        //    a provably-IMPOSSIBLE narrowing — the only kind §3.0 permits —
        //    so the law reaches nobody, and does so without walking the world.
        // --------------------------------------------------------------
        for (Object* o : {&carrier, &bystander}) resetFillet(*o);
        auto ghostLaw = mgr.createLaw("ghost-law", {&author});
        ghostLaw->setActivation(Law::Activation::WhileTrue);
        ghostLaw->setConditionModel(ConditionNode::compare(
            "nobody-carries-this", ConditionNode::Op::Gt, PropertyValue(0.0)));
        ghostLaw->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.9f)));
        mgr.tick();
        assert(nearf(filletOf(carrier), 0.0) && nearf(filletOf(bystander), 0.0) &&
               "a law whose vocabulary nobody carries reaches nobody");

        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("vocabulary_index_test: OK\n");
    return 0;
}
