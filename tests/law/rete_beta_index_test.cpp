// Rete beta index test - O(1) direct beta node lookups via findBeta(id)
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "rete_beta_index_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "rete_beta_index_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "rete_beta_index_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        ReteNetwork rete;

        // 1. Initial state check: findBeta for arbitrary IDs returns nullptr
        assert(rete.findBeta(0) == nullptr);
        assert(rete.findBeta(1) == nullptr);
        assert(rete.findBeta(9999) == nullptr);

        // 2. Add Alpha Nodes
        std::size_t alpha1 = rete.addAlphaNode("Type A", [](const FactPtr& f) {
            return f->type == "typeA";
        });
        std::size_t alpha2 = rete.addAlphaNode("Type B", [](const FactPtr& f) {
            return f->type == "typeB";
        });

        assert(alpha1 > 0);
        assert(alpha2 > 0);

        // 3. Add Beta Node joining Alpha 1 and Alpha 2
        std::size_t beta1 = rete.addBetaNode(
            "Beta Join A and B",
            false,  // leftIsBeta = false
            alpha1, // leftId = alpha1
            alpha2  // rightAlphaId = alpha2
        );

        // 4. Verify findBeta returns valid BetaNode pointer
        const ReteNetwork::BetaNode* constBeta = const_cast<const ReteNetwork&>(rete).findBeta(beta1);
        ReteNetwork::BetaNode* mutBeta = rete.findBeta(beta1);

        assert(constBeta != nullptr);
        assert(mutBeta != nullptr);
        assert(constBeta == mutBeta);
        assert(constBeta->id == beta1);
        assert(constBeta->leftId == alpha1);
        assert(constBeta->rightAlphaId == alpha2);
        assert(constBeta->description == "Beta Join A and B");

        // 5. Verify out-of-bounds or alpha IDs return nullptr for findBeta
        assert(rete.findBeta(alpha1) == nullptr);
        assert(rete.findBeta(alpha2) == nullptr);
        assert(rete.findBeta(beta1 + 100) == nullptr);

        // 6. Test fact propagation and beta memory inspection via findBeta
        auto factA = std::make_shared<ReteFact>();
        factA->type = "typeA";
        factA->subjectId = "entity1";
        rete.assertFact(factA);

        auto factB = std::make_shared<ReteFact>();
        factB->type = "typeB";
        factB->subjectId = "entity1";
        rete.assertFact(factB);

        // Check that beta node memory was populated via O(1) findBeta lookup
        ReteNetwork::BetaNode* betaRef = rete.findBeta(beta1);
        assert(betaRef != nullptr);
        assert(betaRef->memory.size() == 1);
        assert(betaRef->memory[0].facts.size() == 2);
        assert(betaRef->memory[0].facts[0]->subjectId == "entity1");
        assert(betaRef->memory[0].facts[1]->subjectId == "entity1");
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("rete_beta_index_test: ALL OK");
    return 0;
}
