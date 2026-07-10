// Continuous-law + condition-calculus milestone test.
//
// The ECA loop is edge-triggered: conditions are checked at the discrete
// moments events fire. This test proves the level-triggered side — laws whose
// condition phase monitors the program at all times (WhileTrue /
// OnBecomeTrue over the Universe of beings) — and the full condition
// calculus: All/Any/Not (&&/||/! with tree-nesting as parentheses), IsKind
// (runtime instanceof), Identity (one specific being), and ForAny/ForAll
// quantifiers with exceptions.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Form/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

double filletOf(Object& obj) {
    PropertyValue v;
    if (!PropertyPath::parse("shape.fillet").getValue(obj, v)) return -1.0;
    double out = -1.0;
    propertyValueToNumber(v, out);
    return out;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "continuous_law_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "continuous_law_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "continuous_law_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object a, b;                       // the world's population
        a.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        b.setPosition(glm::vec3(3.0f, 5.0f, 0.0f));

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&a);
            beings.push_back(&b);
        });

        LawManager mgr;

        // ------------------------------------------------------------------
        // 1. WhileTrue: no event ever fires, yet the law keeps watch.
        // ------------------------------------------------------------------
        auto floorLaw = mgr.createLaw("ground-rest", {&author});
        floorLaw->setActivation(Law::Activation::WhileTrue);
        floorLaw->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        floorLaw->setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));

        a.setPosition(glm::vec3(0.0f, -5.0f, 0.0f));      // sinks: nobody announces it
        auto records = mgr.tick();                        // NO events published
        assert(!records.empty());
        assert(nearf(a.getPosition().y, 0.0f));           // caught by watching

        a.setPosition(glm::vec3(0.0f, -2.0f, 0.0f));      // sinks again
        mgr.tick();
        assert(nearf(a.getPosition().y, 0.0f));           // fires each time it holds

        // Retire the floor law: continuous laws run in registration order
        // within a tick, and a live floor would clamp b before the laws
        // below ever see it sunk (the interaction is correct — but the next
        // sections need to observe sinking).
        floorLaw->setEnabled(false);

        // ------------------------------------------------------------------
        // 2. OnBecomeTrue: fires once at the false->true edge, then re-arms.
        // ------------------------------------------------------------------
        auto onsetLaw = mgr.createLaw("mark-the-fall", {&author});
        onsetLaw->setActivation(Law::Activation::OnBecomeTrue);
        onsetLaw->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        onsetLaw->setActionModel(ActionNode::add("shape.fillet", 0.1));
        onsetLaw->addTarget(b);                           // watches ONE being

        // fillet defaults to 0.12 — zero it so the onset increments read clean.
        assert(PropertyPath::parse("shape.fillet").setValue(b, PropertyValue(0.0f)));

        b.setPosition(glm::vec3(3.0f, -1.0f, 0.0f));
        mgr.tick();
        assert(nearf(static_cast<float>(filletOf(b)), 0.1f));   // onset: fired once
        mgr.tick();
        mgr.tick();
        assert(nearf(static_cast<float>(filletOf(b)), 0.1f));   // still true: no refire

        b.setPosition(glm::vec3(3.0f, 2.0f, 0.0f));       // condition releases
        mgr.tick();                                       // re-arms
        b.setPosition(glm::vec3(3.0f, -1.0f, 0.0f));      // second onset
        mgr.tick();
        assert(nearf(static_cast<float>(filletOf(b)), 0.2f));   // fired once more

        // ------------------------------------------------------------------
        // 3. The condition calculus, compiled and evaluated directly.
        // ------------------------------------------------------------------
        ECA::Event probe;
        probe.type = "test";
        Law someLaw("witness");

        // instanceof: an Object is an Object; a Law is BOTH (extra-spatial
        // Object) — honest C++ semantics; BeingKind::Law is the precise check.
        assert(ConditionNode::isKind(ConditionNode::BeingKind::Object).compile()(probe, a));
        assert(!ConditionNode::isKind(ConditionNode::BeingKind::Law).compile()(probe, a));
        assert(ConditionNode::isKind(ConditionNode::BeingKind::Law).compile()(probe, someLaw));
        assert(ConditionNode::isKind(ConditionNode::BeingKind::Object).compile()(probe, someLaw));

        // Identity: this one specific being and no other.
        assert(ConditionNode::identity(a.getIdentifier()).compile()(probe, a));
        assert(!ConditionNode::identity(a.getIdentifier()).compile()(probe, b));

        // Parenthetical grouping: (y > 1 || y < -1) && !(y > 100).
        auto grouped = ConditionNode::all({
            ConditionNode::any({
                ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(1.0)),
                ConditionNode::compare("position.y", ConditionNode::Op::Lt, PropertyValue(-1.0))}),
            ConditionNode::negate(
                ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(100.0)))});
        b.setPosition(glm::vec3(3.0f, 2.0f, 0.0f));
        assert(grouped.compile()(probe, b));              // 2 > 1, not > 100
        b.setPosition(glm::vec3(3.0f, 0.5f, 0.0f));
        assert(!grouped.compile()(probe, b));             // inside (-1, 1)

        // ------------------------------------------------------------------
        // 4. Quantifiers over the Universe, with exceptions.
        // ------------------------------------------------------------------
        a.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        b.setPosition(glm::vec3(3.0f, -2.0f, 0.0f));      // b is sunk

        auto anySunk = ConditionNode::forAny(
            ConditionNode::BeingKind::Object,
            ConditionNode::compare("position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        assert(anySunk.compile()(probe, author));         // some object is sunk

        auto allAbove = ConditionNode::forAll(
            ConditionNode::BeingKind::Object,
            ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(0.0)));
        assert(!allAbove.compile()(probe, author));       // b violates it

        // "...with possible exceptions": exempt b, and ALL holds.
        auto allAboveExceptB = ConditionNode::forAll(
            ConditionNode::BeingKind::Object,
            ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(0.0)),
            {b.getIdentifier()});
        assert(allAboveExceptB.compile()(probe, author));

        // A continuous alarm law: gild the witness beacon while ANY object
        // is sunk — the classic "if ANY instance" law, running with no events.
        Object beacon;
        auto alarm = mgr.createLaw("alarm-while-any-sunk", {&author});
        alarm->setActivation(Law::Activation::WhileTrue);
        alarm->setConditionModel(anySunk);
        alarm->setActionModel(ActionNode::set("shape.fillet", PropertyValue(1.0)));
        alarm->addTarget(beacon);

        mgr.tick();
        assert(nearf(static_cast<float>(filletOf(beacon)), 1.0f));   // b is sunk -> alarm

        // ------------------------------------------------------------------
        // 5. Activation and the calculus survive serialization.
        // ------------------------------------------------------------------
        auto reborn = Law::fromJson(alarm->toJson());
        assert(reborn->activation() == Law::Activation::WhileTrue);
        assert(reborn->conditionModel()->kind == ConditionNode::Kind::ForAny);
        reborn->addAuthor(author);
        Object beacon2;
        reborn->addTarget(beacon2);
        assert(reborn->applyTo(beacon2) == Law::ApplicationResult::Applied);
        assert(nearf(static_cast<float>(filletOf(beacon2)), 1.0f));

        Universe::instance().setProvider({});             // leave no dangling refs
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("continuous_law_test: ALL OK");
    return 0;
}
