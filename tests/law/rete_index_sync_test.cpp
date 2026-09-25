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

    std::cout << "rete_index_sync_test: PASS" << std::endl;
}

int main() {
    testAlphaIndexSynchronization();
    return 0;
}
