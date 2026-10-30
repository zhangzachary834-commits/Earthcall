// Direct-address beta lookup regression. Checks are active even with NDEBUG.
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "rete_beta_index_test:%d: %s failed\\n", __LINE__, #condition); \
        return 1; \
    } \
} while (false)

int main() {
    ReteNetwork rete;
    const ReteNetwork& readOnly = rete;

    CHECK(rete.findBeta(0) == nullptr);
    CHECK(readOnly.findBeta(1) == nullptr);
    CHECK(rete.findBeta(std::numeric_limits<std::size_t>::max()) == nullptr);

    const auto alphaA = rete.addAlphaNode("A", [](const FactPtr& f) { return f->type == "A"; });
    const auto alphaB = rete.addAlphaNode("B", [](const FactPtr& f) { return f->type == "B"; });
    const auto alphaC = rete.addAlphaNode("C", [](const FactPtr& f) { return f->type == "C"; });
    const auto betaAB = rete.addBetaNode("A and B", false, alphaA, alphaB);
    const auto betaABC = rete.addBetaNode("A and B and C", true, betaAB, alphaC);

    // Shared alpha/beta ID namespace: an alpha ID must never resolve as beta.
    CHECK(alphaA != alphaB && alphaB != alphaC);
    CHECK(betaAB != betaABC);
    CHECK(rete.findBeta(alphaA) == nullptr);
    CHECK(rete.findBeta(alphaB) == nullptr);
    CHECK(rete.findBeta(alphaC) == nullptr);
    CHECK(rete.findBeta(betaABC + 100) == nullptr);

    // Force beta vector reallocations and interleave a new alpha ID.
    std::vector<std::size_t> siblingBetas;
    for (int i = 0; i < 24; ++i) {
        siblingBetas.push_back(rete.addBetaNode("sibling", false, alphaA, alphaB));
    }
    const auto laterAlpha = rete.addAlphaNode("later", [](const FactPtr& f) {
        return f->type == "later";
    });
    CHECK(rete.findBeta(laterAlpha) == nullptr);
    CHECK(readOnly.findBeta(betaAB) != nullptr);
    CHECK(readOnly.findBeta(betaABC) != nullptr);
    CHECK(readOnly.findBeta(betaAB)->id == betaAB);
    CHECK(readOnly.findBeta(betaABC)->id == betaABC);
    CHECK(readOnly.findBeta(betaABC)->leftIsBeta);
    CHECK(readOnly.findBeta(betaABC)->leftId == betaAB);
    CHECK(readOnly.findBeta(betaABC)->rightAlphaId == alphaC);
    CHECK(readOnly.findBeta(betaAB) == rete.findBeta(betaAB));
    for (auto id : siblingBetas) {
        CHECK(rete.findBeta(id) != nullptr);
        CHECK(rete.findBeta(id)->id == id);
    }

    auto fact = [](const std::string& type) {
        auto f = std::make_shared<ReteFact>();
        f->type = type;
        f->subjectId = "entity1";
        return f;
    };
    rete.assertFact(fact("A"));
    rete.assertFact(fact("B"));
    rete.assertFact(fact("C"));

    const auto* ab = readOnly.findBeta(betaAB);
    const auto* abc = readOnly.findBeta(betaABC);
    CHECK(ab != nullptr && abc != nullptr);
    CHECK(ab->memory.size() == 1);
    CHECK(ab->memory[0].facts.size() == 2);
    CHECK(abc->memory.size() == 1);
    CHECK(abc->memory[0].facts.size() == 3);
    for (const auto& matched : abc->memory[0].facts) {
        CHECK(matched != nullptr && matched->subjectId == "entity1");
    }
    for (auto id : siblingBetas) {
        CHECK(readOnly.findBeta(id)->memory.size() == 1);
    }

    // Law binding must read the same beta backlog via the index.
    rete.bindLawToBeta("three-way-law", betaABC);
    CHECK(rete.agenda().size() == 1);
    CHECK(rete.agenda()[0].token.facts.size() == 3);
    rete.drainAgenda();

    // Retraction removes stale tokens without invalidating stable beta IDs.
    rete.retractFirst(1);
    CHECK(readOnly.findBeta(betaAB)->memory.empty());
    CHECK(readOnly.findBeta(betaABC)->memory.empty());
    for (auto id : siblingBetas) {
        CHECK(readOnly.findBeta(id)->memory.empty());
    }
    rete.clearFacts();
    CHECK(readOnly.findBeta(betaAB)->id == betaAB);
    CHECK(readOnly.findBeta(betaABC)->id == betaABC);

    std::puts("rete_beta_index_test: ALL OK");
    return 0;
}
