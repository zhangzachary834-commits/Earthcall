#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>

int main() {
    ReteNetwork rete;

    // Create 50 independent Beta nodes joining 2 Alpha nodes
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

    constexpr int N = 200;
    constexpr int rounds = 20;

    double totalMs = 0.0;

    for (int r = 0; r < rounds; ++r) {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::string> factIds;
        factIds.reserve(N * 100);

        for (int i = 0; i < N; ++i) {
            for (int k = 0; k < 100; ++k) {
                auto f = std::make_shared<ReteFact>();
                f->type = "type_" + std::to_string(k);
                f->subjectId = "subj_" + std::to_string(i);
                factIds.push_back(rete.assertFact(f));
            }
        }

        for (const auto& fid : factIds) {
            rete.retractFact(fid);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double durationMs = std::chrono::duration<double, std::milli>(end - start).count();
        totalMs += durationMs;
    }

    double avgMs = totalMs / rounds;
    std::cout << "Beta propagation & retraction benchmark (50 beta nodes, " << N << " subjects across 100 types):\n";
    std::cout << "Average duration per round: " << std::fixed << std::setprecision(3) << avgMs << " ms\n";

    return 0;
}
