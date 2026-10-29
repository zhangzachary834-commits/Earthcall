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


void testBetaIndexPropagationAndRetraction() {
    ReteNetwork rete;
    const auto alphaA = rete.addAlphaNode("A", [](const FactPtr& f) { return f->type == "A"; });
    const auto alphaB = rete.addAlphaNode("B", [](const FactPtr& f) { return f->type == "B"; });
    const auto alphaC = rete.addAlphaNode("C", [](const FactPtr& f) { return f->type == "C"; });
    const auto betaAB = rete.addBetaNode("A and B", false, alphaA, alphaB);
    const auto betaABC = rete.addBetaNode("AB and C", true, betaAB, alphaC);

    // Alpha and beta ids share one namespace; alpha holes must not alias a beta.
    assert(rete.findBeta(alphaA) == nullptr);
    assert(rete.findBeta(alphaB) == nullptr);
    assert(rete.findBeta(alphaC) == nullptr);
    assert(rete.findBeta(betaABC + 1) == nullptr);
    assert(rete.findBeta(betaAB) && rete.findBeta(betaAB)->id == betaAB);
    assert(rete.findBeta(betaABC) && rete.findBeta(betaABC)->id == betaABC);

    rete.bindLawToBeta("three-clause-law", betaABC);
    auto fact = [](const std::string& type, const std::string& subject) {
        auto f = std::make_shared<ReteFact>();
        f->type = type;
        f->subjectId = subject;
        return f;
    };
    rete.assertFact(fact("A", "same"));
    const auto idB = rete.assertFact(fact("B", "same"));
    rete.assertFact(fact("C", "other"));
    assert(rete.findBeta(betaAB)->memory.size() == 1);
    assert(rete.findBeta(betaABC)->memory.empty());
    const auto idC = rete.assertFact(fact("C", "same"));
    assert(rete.findBeta(betaABC)->memory.size() == 1);
    const auto agenda = rete.drainAgenda();
    assert(agenda.size() == 1 && agenda[0].token.facts.size() == 3);

    // Both the direct beta and its downstream beta must retract their tokens.
    assert(rete.retractFact(idB));
    assert(rete.findBeta(betaAB)->memory.empty());
    assert(rete.findBeta(betaABC)->memory.empty());
    assert(rete.drainAgenda().empty());

    // The index remains valid after retraction and a new propagation cycle.
    rete.assertFact(fact("B", "same"));
    assert(rete.findBeta(betaAB)->memory.size() == 1);
    assert(rete.findBeta(betaABC)->memory.size() == 1);
    assert(rete.retractFact(idC));
    assert(rete.findBeta(betaABC)->memory.empty());
    std::cout << "rete beta index propagation/retraction: PASS" << std::endl;
}

int main() {
    testAlphaIndexSynchronization();
    testBetaIndexPropagationAndRetraction();
    return 0;
}
