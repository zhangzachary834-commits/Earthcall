// Hoisting a gate must never silence a law that should fire.
//
// FORMATION_RETE.md §8 rung 3. A conjunct like `@gate.open > 0` names one
// being, so its truth is the same for every subject — which makes it an
// all-or-nothing gate rather than a filter. `LawManager::gatesHold` evaluates
// it once and skips the whole subject loop when it is false. Measured: a shut
// gate went from 278 ms/tick at 480 beings to 0.18 ms.
//
// That is a large win bought with an assumption, and the assumption is exactly
// the kind this subsystem has been burned by: "nothing changes the gate while
// the sweep runs, and nothing else was depending on the sweep happening."
// Every section below is a way that could be false.
//
// The dangerous direction is a law that goes quiet and says nothing about it —
// PROPHETIC_RETE.md §2. So the oracle throughout is: did the law FIRE when it
// should have? `shape.fillet` starts at 0 and a firing law sets it to 0.5.

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
        std::fprintf(stderr, "gate_hoist_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "gate_hoist_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "gate_hoist_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object gate;
        Object subject;
        Object other;

        gate.setObjectID("gate");
        gate.setDynamicProperty("open", PropertyValue(1.0));
        subject.setObjectID("subject");
        subject.setDynamicProperty("beacon", PropertyValue(1.0));
        other.setObjectID("other");
        for (Object* o : {&author, &gate, &subject, &other}) resetFillet(*o);

        std::vector<Singular*> population{&author, &gate, &subject, &other};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();

        // ------------------------------------------------------------------
        // A. The gate decides, and KEEPS DECIDING. Shut, open, shut, open —
        //    the law must follow it every time. A hoist that latched would
        //    pass a single-transition test and fail this one.
        // ------------------------------------------------------------------
        auto gated = mgr.createLaw("gated", {&author});
        gated->setActivation(Law::Activation::WhileTrue);
        gated->setConditionModel(ConditionNode::all({
            ConditionNode::compare("@gate.open", ConditionNode::Op::Gt, PropertyValue(0.0)),
            ConditionNode::compare("beacon", ConditionNode::Op::Gt, PropertyValue(0.0))}));
        gated->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        mgr.tick();
        assert(nearf(filletOf(subject), 0.5) && "an open gate must let the law through");

        resetFillet(subject);
        gate.setDynamicProperty("open", PropertyValue(0.0));
        mgr.tick();
        assert(nearf(filletOf(subject), 0.0) && "a shut gate must stop the law");

        gate.setDynamicProperty("open", PropertyValue(1.0));
        mgr.tick();
        assert(nearf(filletOf(subject), 0.5) && "a REOPENED gate must let it through again");

        resetFillet(subject);
        gate.setDynamicProperty("open", PropertyValue(0.0));
        mgr.tick();
        assert(nearf(filletOf(subject), 0.0) && "and shut again");
        gate.setDynamicProperty("open", PropertyValue(1.0));

        // ------------------------------------------------------------------
        // B. THE EDGE SURVIVES THE GATE.
        //
        //    This section also found a bug older than the gate: it failed with
        //    the hoist disabled AND the vocabulary index disabled. A law's
        //    required vocabulary is a path ROOT (`shape`), but Object registers
        //    the property under its whole dotted name (`shape.fillet`), so
        //    couldApplyTo answered no for every being and any sweep-path law
        //    touching `shape.*` reached nobody. Fixed in beingCarriesProperty. This is the one that would have been
        //    a silent bug: skipping the subject loop also skips the release of
        //    everyone the law was holding, so an OnBecomeTrue law would come
        //    back from a shut gate still believing its subjects held — no
        //    false->true edge, and it never fires again.
        // ------------------------------------------------------------------
        gated->setEnabled(false);
        resetFillet(subject);
        auto edge = mgr.createLaw("edge", {&author});
        edge->setActivation(Law::Activation::OnBecomeTrue);
        edge->setConditionModel(ConditionNode::all({
            ConditionNode::compare("@gate.open", ConditionNode::Op::Gt, PropertyValue(0.0)),
            ConditionNode::compare("beacon", ConditionNode::Op::Gt, PropertyValue(0.0))}));
        edge->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        mgr.tick();
        assert(nearf(filletOf(subject), 0.5) && "the edge fires once on first hold");

        resetFillet(subject);
        mgr.tick();
        assert(nearf(filletOf(subject), 0.0) && "an edge does not re-fire while it holds");

        gate.setDynamicProperty("open", PropertyValue(0.0));
        mgr.tick();                       // gate shut: the hold must be released
        gate.setDynamicProperty("open", PropertyValue(1.0));
        mgr.tick();                       // reopened: a NEW false->true edge
        assert(nearf(filletOf(subject), 0.5) &&
               "a gate closing and reopening must re-arm the edge");
        edge->setEnabled(false);

        // ------------------------------------------------------------------
        // C. A GATE UNDER `Any` IS A DISJUNCT and decides nothing. "the gate is
        //    open OR this being has a beacon" is satisfiable with the gate shut,
        //    so hoisting it would silence a law that should fire — the precise
        //    narrowing §2 forbids.
        // ------------------------------------------------------------------
        resetFillet(subject);
        gate.setDynamicProperty("open", PropertyValue(0.0));
        auto disjunct = mgr.createLaw("disjunct", {&author});
        disjunct->setActivation(Law::Activation::WhileTrue);
        disjunct->setConditionModel(ConditionNode::any({
            ConditionNode::compare("@gate.open", ConditionNode::Op::Gt, PropertyValue(0.0)),
            ConditionNode::compare("beacon", ConditionNode::Op::Gt, PropertyValue(0.0))}));
        disjunct->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));
        mgr.tick();
        assert(nearf(filletOf(subject), 0.5) &&
               "a shut gate in a DISJUNCTION must not silence the law");
        disjunct->setEnabled(false);

        // ------------------------------------------------------------------
        // D. A LAW THAT WRITES THROUGH A QUALIFIED ROOT MUST NOT BE HOISTED.
        //    Evaluating the gate once assumes nothing moves it during the
        //    sweep — but this law's own action moves it, so the answer
        //    legitimately differs between the first subject and the second.
        //    gatesHold refuses to hoist any law that writes an `@` path.
        //
        //    Gate starts SHUT and the law opens it. Under a correct refusal the
        //    law runs, so it can open its own gate; under a wrong hoist it is
        //    skipped forever and the gate stays shut.
        // ------------------------------------------------------------------
        gate.setDynamicProperty("open", PropertyValue(0.0));
        auto selfOpening = mgr.createLaw("self-opening", {&author});
        selfOpening->setActivation(Law::Activation::WhileTrue);
        selfOpening->setConditionModel(ConditionNode::all({
            ConditionNode::compare("beacon", ConditionNode::Op::Gt, PropertyValue(0.0)),
            ConditionNode::compare("@gate.open", ConditionNode::Op::Lt, PropertyValue(0.5))}));
        selfOpening->setActionModel(ActionNode::set("@gate.open", PropertyValue(1.0)));
        mgr.tick();
        {
            PropertyValue v;
            double open = 0.0;
            PropertyPath::parse("open").getValue(gate, v);
            propertyValueToNumber(v, open);
            assert(open > 0.5 &&
                   "a law that writes its own gate must not be hoisted away");
        }
        selfOpening->setEnabled(false);

        // ------------------------------------------------------------------
        // E. `@event.*` IS NOT A GATE. It resolves through Universe's
        //    application event, which is only set inside applyTo — evaluated
        //    outside one it reads unset, so hoisting it would refuse every
        //    event law in the tree. isHoistableGate excludes the root by name.
        // ------------------------------------------------------------------
        {
            const auto eventCond = ConditionNode::compare(
                "@event.subject.beacon", ConditionNode::Op::Gt, PropertyValue(0.0));
            assert(!eventCond.isHoistableGate() &&
                   "@event.* must never be treated as a subject-independent gate");
            const auto worldCond = ConditionNode::compare(
                "@world.audio.level", ConditionNode::Op::Gt, PropertyValue(0.0));
            assert(!worldCond.isHoistableGate() &&
                   "@world.* reads through the subject and is not a gate");
            const auto ownCond = ConditionNode::compare(
                "beacon", ConditionNode::Op::Gt, PropertyValue(0.0));
            assert(!ownCond.isHoistableGate() && "an own-property test is not a gate");
            const auto realGate = ConditionNode::compare(
                "@gate.open", ConditionNode::Op::Gt, PropertyValue(0.0));
            assert(realGate.isHoistableGate() && "a plain @referent IS a gate");
        }

        // ------------------------------------------------------------------
        // F. A MISSING REFERENT SHUTS THE GATE, and that is the pre-existing
        //    answer, not a new one: resolveLawRoot returns nullptr for a being
        //    not in the world, lawGetValue fails, and the Compare was already
        //    false for every subject. The hoist must agree with the sweep, not
        //    invent a different answer.
        // ------------------------------------------------------------------
        resetFillet(subject);
        auto ghostGate = mgr.createLaw("ghost-gate", {&author});
        ghostGate->setActivation(Law::Activation::WhileTrue);
        ghostGate->setConditionModel(ConditionNode::all({
            ConditionNode::compare("@no-such-being.open", ConditionNode::Op::Gt,
                                   PropertyValue(0.0)),
            ConditionNode::compare("beacon", ConditionNode::Op::Gt, PropertyValue(0.0))}));
        ghostGate->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));
        mgr.tick();
        assert(nearf(filletOf(subject), 0.0) &&
               "a gate naming a being that is not in the world does not hold");

        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("gate_hoist_test: OK\n");
    return 0;
}
