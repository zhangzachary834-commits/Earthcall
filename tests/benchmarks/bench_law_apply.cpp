#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    constexpr std::size_t kTargets = 500;
    constexpr std::size_t kLaws = 50;
    constexpr std::size_t kRounds = 50;

    std::vector<std::unique_ptr<Object>> targets;
    targets.reserve(kTargets);
    for (std::size_t i = 0; i < kTargets; ++i) {
        auto obj = std::make_unique<Object>();
        obj->setObjectID("obj_" + std::to_string(i));
        obj->setDynamicProperty("glow", PropertyValue(1.0));
        obj->setDynamicProperty("temperature", PropertyValue(20.0));
        targets.push_back(std::move(obj));
    }

    std::vector<std::unique_ptr<Object>> authors;
    authors.reserve(kLaws);

    std::vector<std::shared_ptr<Law>> laws;
    laws.reserve(kLaws);
    for (std::size_t i = 0; i < kLaws; ++i) {
        std::string lawId = "law_" + std::to_string(i);
        auto law = std::make_shared<Law>(lawId);
        law->setLawIdentifier(lawId);
        law->setEnabled(true);
        law->setActivation(Law::Activation::WhileTrue);

        auto author = std::make_unique<Object>();
        author->setObjectID("author_" + std::to_string(i));
        law->addAuthor(*author);
        authors.push_back(std::move(author));

        ActionNode setGlow = ActionNode::set("glow", PropertyValue(2.0));
        ActionNode setTemp = ActionNode::set("temperature", PropertyValue(25.0));
        law->setActionModel(ActionNode::sequence({setGlow, setTemp}));
        laws.push_back(std::move(law));
    }

    // Warmup
    for (const auto& law : laws) {
        for (const auto& target : targets) {
            law->applyToAfterConditions(*target);
        }
    }

    std::vector<double> samplesUs;
    samplesUs.reserve(kRounds);

    for (std::size_t round = 0; round < kRounds; ++round) {
        auto start = std::chrono::high_resolution_clock::now();
        std::size_t applied = 0;
        for (const auto& law : laws) {
            for (const auto& target : targets) {
                if (law->applyToAfterConditions(*target) == Law::ApplicationResult::Applied) {
                    applied++;
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        double durationUs = std::chrono::duration<double, std::micro>(end - start).count();
        samplesUs.push_back(durationUs);
    }

    std::sort(samplesUs.begin(), samplesUs.end());
    double medianUs = samplesUs[kRounds / 2];
    double p95Us = samplesUs[static_cast<std::size_t>(kRounds * 0.95)];

    std::cout << "BENCH_LAW_APPLY targets=" << kTargets << " laws=" << kLaws
              << " total_evals_per_round=" << (kTargets * kLaws) << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Median: " << medianUs << " us (p95: " << p95Us << " us)\n";

    return 0;
}
