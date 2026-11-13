#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// CMake's tests/*.cpp glob registers this file as an ordinary CTest.
// Keep the CI witness bounded; long-running throughput experiments belong
// outside the default test suite and require an explicit baseline comparison.
int main() {
    ReteNetwork rete;

    std::vector<std::size_t> alphas;
    for (int i = 0; i < 100; ++i) {
        alphas.push_back(rete.addAlphaNode("alpha_" + std::to_string(i), [i](const FactPtr& f) {
            return f->type == "type_" + std::to_string(i);
        }));
    }

    std::vector<std::size_t> betas;
    for (int i = 0; i < 50; ++i) {
        betas.push_back(rete.addBetaNode("beta_" + std::to_string(i), false, alphas[i], alphas[i + 50],
            [](const ReteToken& t, const FactPtr& f) {
                return t.facts[0]->subjectId == f->subjectId;
            }));
    }

    // Alpha and beta IDs share one namespace. Every beta must remain
    // discoverable after _betaNodes has grown and reallocated.
    for (const auto alphaId : alphas) {
        if (rete.findBeta(alphaId) != nullptr) {
            std::cerr << "Alpha ID unexpectedly resolved to a beta node\n";
            return 1;
        }
    }
    for (const auto betaId : betas) {
        const auto* beta = rete.findBeta(betaId);
        if (!beta || beta->id != betaId) {
            std::cerr << "Beta direct index failed after node growth\n";
            return 1;
        }
    }

    constexpr int subjects = 8;
    constexpr int rounds = 2;
    double totalMs = 0.0;

    for (int r = 0; r < rounds; ++r) {
        const auto start = std::chrono::steady_clock::now();
        std::vector<std::string> factIds;
        factIds.reserve(subjects * 100);

        for (int i = 0; i < subjects; ++i) {
            for (int k = 0; k < 100; ++k) {
                auto fact = std::make_shared<ReteFact>();
                fact->type = "type_" + std::to_string(k);
                fact->subjectId = "subj_" + std::to_string(i);
                factIds.push_back(rete.assertFact(fact));
            }
        }

        for (const auto betaId : betas) {
            const auto* beta = rete.findBeta(betaId);
            if (!beta || beta->memory.size() != subjects) {
                std::cerr << "Beta join propagation produced incorrect token count\n";
                return 1;
            }
        }

        for (const auto& factId : factIds) {
            rete.retractFact(factId);
        }

        for (const auto betaId : betas) {
            const auto* beta = rete.findBeta(betaId);
            if (!beta || !beta->memory.empty()) {
                std::cerr << "Beta retraction left stale tokens\n";
                return 1;
            }
        }

        const auto end = std::chrono::steady_clock::now();
        totalMs += std::chrono::duration<double, std::milli>(end - start).count();
    }

    std::cout << "bench_beta_lookup: PASS (50 beta nodes, "
              << subjects << " subjects, " << rounds << " rounds); "
              << std::fixed << std::setprecision(3)
              << totalMs / rounds << " ms/round (informational, not a speedup claim)\n";
    return 0;
}
