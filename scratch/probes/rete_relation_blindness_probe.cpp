// SCRATCH PROBE (temporary; delete after reading the result).
//
// Question: does a WhileTrue law with a Related condition see a relation that
// is FORMED AFTER the subject was first seeded into the Rete network?
//
// argv[1] == "pre"  -> relation exists before the first tick (control)
// argv[1] == "post" -> relation formed after the first tick (the suspect)

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

int main(int argc, char** argv) {
    const bool preExisting = (argc > 1 && std::strcmp(argv[1], "pre") == 0);

    if (!glfwInit()) { std::fprintf(stderr, "glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* w = glfwCreateWindow(64, 64, "probe", nullptr, nullptr);
    if (!w) { std::fprintf(stderr, "no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(w);

    int status = 0;
    {
        Object author, a, b;
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&author); beings.push_back(&a); beings.push_back(&b);
        });

        std::vector<std::shared_ptr<Relation>> graph;
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (auto& r : graph) out.push_back(r.get());
        });

        auto formBond = [&]() {
            auto r = std::make_shared<Relation>("bonded-to", a, b, /*directed=*/true);
            graph.push_back(r);
            // The engine's own announcement path, so the network hears it.
            ECA::Event echo;
            echo.type = "relation-formed";
            echo.subject = r.get();
            echo.timestamp = std::time(nullptr);
            Core::EventBus::instance().publish(echo);
        };

        LawManager mgr;
        mgr.connectToEventBus();

        auto law = mgr.createLaw("bonded-glow", {&author});
        law->setActivation(Law::Activation::WhileTrue);
        law->setConditionModel(ConditionNode::related("bonded-to"));
        law->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5)));
        law->addTarget(a);

        PropertyPath::parse("shape.fillet").setValue(a, PropertyValue(0.0f));

        if (preExisting) formBond();
        mgr.tick();
        if (!preExisting) formBond();
        mgr.tick();
        mgr.tick();

        PropertyValue v;
        double fillet = -1.0;
        PropertyPath::parse("shape.fillet").getValue(a, v);
        propertyValueToNumber(v, fillet);

        const bool fired = fillet > 0.4;
        std::printf("[%s] relation is %s live in the graph; law %s\n",
                    preExisting ? "pre " : "post",
                    Universe::instance().relations().empty() ? "NOT" : "",
                    fired ? "FIRED" : "stayed DEAF");
        std::printf("     shape.fillet = %.3f (expected 0.500 if the law saw the bond)\n", fillet);
        // Is the raw predicate true regardless of the network?
        ECA::Event probeEvent;
        const bool predicateTrue = ConditionNode::related("bonded-to").compile()(probeEvent, a);
        std::printf("     raw Related predicate against the live graph: %s\n",
                    predicateTrue ? "TRUE" : "false");
        if (predicateTrue && !fired) {
            std::printf("     >>> DEAF: the condition holds, the law did not fire.\n");
            status = 2;
        }
        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
    }
    glfwDestroyWindow(w);
    glfwTerminate();
    return status;
}
