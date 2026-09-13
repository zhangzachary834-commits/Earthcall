// Regression witness for terminal-backed OnBecomeTrue semantics.
//
// The disconnected sweep path already had coverage in continuous_law_test.
// This test connects the LawManager so a plain local Compare compiles to a
// Rete terminal, then proves the edge contract on that fast path:
// false -> true fires once, persistent true stays silent, false re-arms, and
// the next false -> true fires exactly once again.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

double filletOf(Object& obj) {
    PropertyValue v;
    if (PropertyPath::parse("shape.fillet").getValue(obj, v) !=
        PropertyPath::PathResult::Ok) {
        return -1.0;
    }
    double out = -1.0;
    propertyValueToNumber(v, out);
    return out;
}

bool near(double a, double b, double eps = 1e-5) {
    return std::fabs(a - b) < eps;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "on_become_true_rete_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "on_become_true_rete_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "on_become_true_rete_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object subject;
        subject.setPosition(glm::vec3(0.0f, 2.0f, 0.0f));
        assert(PropertyPath::parse("shape.fillet").setValue(
                   subject, PropertyValue(0.0f)) == PropertyPath::PathResult::Ok);

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&subject);
        });

        LawManager mgr;
        mgr.connectToEventBus();

        auto law = mgr.createLaw("rete-edge-once", {&author});
        law->setActivation(Law::Activation::OnBecomeTrue);
        law->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        law->setActionModel(ActionNode::add("shape.fillet", 0.1));
        law->addTarget(subject);

        // Seed a FALSE state and let Rete compile/backfill before the edge.
        mgr.tick();
        assert(near(filletOf(subject), 0.0));

        // FALSE -> TRUE: exactly one firing.
        subject.setPosition(glm::vec3(0.0f, -1.0f, 0.0f));
        mgr.tick();
        assert(near(filletOf(subject), 0.1));

        // Persistent TRUE is not another edge. This is the regression: the
        // terminal fast path used to compute newlyTrue, then ignore it and
        // apply to every currently matching subject anyway.
        mgr.tick();
        mgr.tick();
        assert(near(filletOf(subject), 0.1));

        // TRUE -> FALSE re-arms without firing.
        subject.setPosition(glm::vec3(0.0f, 2.0f, 0.0f));
        mgr.tick();
        assert(near(filletOf(subject), 0.1));

        // The next FALSE -> TRUE edge fires once more.
        subject.setPosition(glm::vec3(0.0f, -1.0f, 0.0f));
        mgr.tick();
        assert(near(filletOf(subject), 0.2));
        mgr.tick();
        assert(near(filletOf(subject), 0.2));

        Universe::instance().setProvider({});
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("on_become_true_rete_test: ok");
    return 0;
}
