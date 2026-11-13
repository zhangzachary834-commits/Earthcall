#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void testAlphaIndexSynchronization() {
    ReteNetwork rete;

    // Create 3 alpha nodes: id1=1, id2=2, id3=3
    std::size_t id1 = rete.addAlphaNode("alpha1", [](const FactPtr&) { return true; });
    std::size_t id2 = rete.addAlphaNode("alpha2", [](const FactPtr&) { return true; });
    std::size_t id3 = rete.addAlphaNode("alpha3", [](const FactPtr&) { return true; });

    assert(id1 == 1);
    assert(id2 == 2);
    assert(id3 == 3);

    assert(rete.isAlphaNode(id1));
    assert(rete.isAlphaNode(id2));
    assert(rete.isAlphaNode(id3));

    // Bind law1 to alpha1 and alpha3
    std::string law1 = "test-law-1";
    rete.bindLawToAlpha(law1, id1);
    rete.bindLawToAlpha(law1, id3);

    // Bind law2 to alpha3 only, so alpha3 survives when law1 is unbound
    std::string law2 = "test-law-2";
    rete.bindLawToAlpha(law2, id3);

    // Unbind law1. alpha1 and alpha2 have no bindings left and no beta reading them.
    // They will be pruned by dropUnboundAlphaNodes(). alpha3 remains bound to law2!
    rete.unbindLaw(law1);

    // alpha1 and alpha2 are dropped
    assert(!rete.isAlphaNode(id1));
    assert(!rete.isAlphaNode(id2));

    // alpha3 survives compaction and still resolves correctly via _alphaIndexById!
    assert(rete.isAlphaNode(id3));

    // Create new alpha node after pruning
    std::size_t id4 = rete.addAlphaNode("alpha4", [](const FactPtr&) { return true; });
    assert(id4 == 4);
    assert(rete.isAlphaNode(id4));
    assert(rete.isAlphaNode(id3));
    assert(!rete.isAlphaNode(id1));
    assert(!rete.isAlphaNode(id2));

    // Create a Beta node joining id3 and id4
    std::size_t beta1 = rete.addBetaNode("beta1", false, id3, id4);
    assert(beta1 == 5);
    assert(rete.isBetaNode(beta1));
    assert(!rete.isAlphaNode(beta1));
    assert(rete.findBeta(beta1) != nullptr);
    assert(rete.findBeta(beta1)->id == beta1);
    assert(rete.findBeta(999) == nullptr);

    std::cout << "rete_index_sync_test: PASS" << std::endl;
}


void testBetaIndexAcrossGrowthAndRetraction() {
    ReteNetwork rete;
    const auto left = rete.addAlphaNode("left", [](const FactPtr& f) { return f->type == "left"; });
    const auto right = rete.addAlphaNode("right", [](const FactPtr& f) { return f->type == "right"; });
    const auto joined = rete.addBetaNode("joined", false, left, right);
    const auto chained = rete.addBetaNode("chained", true, joined, right);

    // Reallocation of the beta vector must never invalidate id-to-index mappings.
    for (int i = 0; i < 64; ++i) {
        const auto id = rete.addBetaNode("extra", false, left, right);
        assert(rete.findBeta(id) && rete.findBeta(id)->id == id);
    }
    const ReteNetwork& readonly = rete;
    assert(readonly.findBeta(joined) && readonly.findBeta(joined)->id == joined);
    assert(readonly.findBeta(chained) && readonly.findBeta(chained)->leftId == joined);
    assert(readonly.findBeta(left) == nullptr);
    assert(readonly.findBeta(right) == nullptr);
    assert(readonly.findBeta(999999) == nullptr);

    rete.bindLawToBeta("chained-law", chained);
    auto leftFact = std::make_shared<ReteFact>();
    leftFact->type = "left";
    leftFact->subjectId = "shared";
    auto rightFact = std::make_shared<ReteFact>();
    rightFact->type = "right";
    rightFact->subjectId = "shared";

    const auto leftFactId = rete.assertFact(leftFact);
    const auto rightFactId = rete.assertFact(rightFact);
    assert(!leftFactId.empty() && !rightFactId.empty());
    assert(rete.findBeta(joined)->memory.size() == 1);
    assert(rete.findBeta(chained)->memory.size() == 1);
    assert(rete.agenda().size() == 1);

    // Retraction uses the same index, and must remove dependent beta tokens
    // and their pending law activation without changing the stable node ids.
    assert(rete.retractFact(rightFactId));
    assert(rete.findBeta(joined)->memory.empty());
    assert(rete.findBeta(chained)->memory.empty());
    assert(rete.agenda().empty());
    assert(rete.findBeta(chained)->id == chained);
}

int main() {
    testAlphaIndexSynchronization();
    testBetaIndexAcrossGrowthAndRetraction();
    return 0;
}
