// Do identical conditions share a node, and what does it cost if they do not?
//
// o3's DEEP_CODEBASE_ANALYSIS_2026-09-07 claims: "Prophetic Rete is brilliant
// but has no alpha-node sharing or runtime profile hooks; memory will balloon."
// This measures the claim instead of arguing with it.
//
// Precision first, because the claim names the wrong subsystem: alpha nodes
// live in ReteNetwork (Law.hpp), not in PropheticRete.cpp, which is the
// ahead-of-time abstract interpreter. o3's own body text has this right
// ("Pass 4 uses classic Rete network"); the summary line does not.
//
// And sharing is not absent, it is PARTIAL. internTypeAlpha dedupes through
// _typeAlphaIndex, so fifty laws listening for "collision" really do share one
// predicate. addAlphaNode — the path every AUTHORED condition takes — appends
// unconditionally. So the gap is specifically: two laws whose condition text is
// identical compile two identical alpha nodes.
//
// What that costs is not the node. A node is a closure and a string. The cost
// is that every bound node keeps a vector of every fact it has matched, so L
// laws over F matching facts hold L x F fact references instead of F.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "alpha_sharing_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    const int kBeings = 200;
    std::printf("\nALPHA SHARING — %d beings, all carrying `beacon`\n", kBeings);
    std::printf("  %6s  %8s  %8s  %14s  %12s\n",
                "laws", "alphas", "betas", "fact refs held", "refs per law");

    std::vector<std::size_t> refs;
    for (int laws : {1, 2, 4, 8}) {
        std::vector<std::unique_ptr<Object>> owned;
        std::vector<Singular*> population;
        for (int i = 0; i < kBeings; ++i) {
            auto o = std::make_unique<Object>();
            o->setDynamicProperty("beacon", PropertyValue(1.0));
            population.push_back(o.get());
            owned.push_back(std::move(o));
        }
        Object author;
        Universe::instance().setProvider([&](std::vector<Singular*>& b) {
            for (Singular* s : population) b.push_back(s);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();
        for (int l = 0; l < laws; ++l) {
            // IDENTICAL condition text in every law. If authored alphas were
            // shared on (path, op, operand) these would collapse to one node.
            auto law = mgr.createLaw("same-" + std::to_string(l), {&author});
            law->setActivation(Law::Activation::WhileTrue);
            law->setConditionModel(ConditionNode::compare(
                "beacon", ConditionNode::Op::Gt, PropertyValue(0.0)));
            law->setActionModel(ActionNode::set("position.z", PropertyValue(1.0f)));
        }
        mgr.tick();
        mgr.tick();

        const std::size_t held = mgr.rete().nodeMemoryFootprint();
        refs.push_back(held);
        std::printf("  %6d  %8zu  %8zu  %14zu  %12.1f\n", laws,
                    mgr.rete().alphaNodeCount(), mgr.rete().betaNodeCount(),
                    held, static_cast<double>(held) / laws);
        Universe::instance().setProvider(nullptr);
    }

    std::printf("\n  If identical conditions shared a node, 'fact refs held' would be\n"
                "  FLAT across the rows. Growth is the duplication, and it is the\n"
                "  memory o3's note is about.\n\n");
    // THE GUARD. Identical condition text must collapse to one node, so the
    // fact references held must not grow with the number of laws stating it.
    // Before sharing this read 200 / 400 / 800 / 1600.
    assert(refs.size() == 4);
    for (std::size_t i = 1; i < refs.size(); ++i) {
        assert(refs[i] == refs[0] &&
               "identical conditions are no longer sharing a node — every law "
               "stating one is holding its own copy of the same match set");
    }

    // And the sharing must be by the WHOLE serialized leaf, not a subset of its
    // fields. These two differ only in operand vs operandPath — identical under
    // a (path, op, const) key, and emphatically not the same condition. chess
    // states both shapes on `chessColor` today, so this is a live collision and
    // not a hypothetical one.
    {
        const auto literal = ConditionNode::compare(
            "chessColor", ConditionNode::Op::Eq, PropertyValue(1.0));
        auto referent = ConditionNode::compare(
            "chessColor", ConditionNode::Op::Eq, PropertyValue(1.0));
        referent.operandPath = PropertyPath::parse("@state.chess.turn");
        assert(literal.toJson().dump() != referent.toJson().dump() &&
               "a condition comparing against a literal and one comparing "
               "against another being's property must not share a node");
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("alpha_sharing_test: OK\n");
    return 0;
}
