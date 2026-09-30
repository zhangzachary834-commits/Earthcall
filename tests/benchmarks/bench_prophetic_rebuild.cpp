#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace {

using Edge = Prophetic::Index::RelevanceEdge;
using EdgeKey = std::tuple<std::string, std::string, std::string, std::string,
                           std::string, bool>;

std::vector<std::string> normalizedPathsLocal(const std::string& dotted) {
    PropertyPath p = PropertyPath::parse(dotted);
    std::vector<std::string> out;
    out.push_back(dotted);
    if (p.segments.empty() || p.segments[0].empty() || p.segments[0][0] != '@') return out;
    for (std::size_t drop = 1; drop < p.segments.size(); ++drop) {
        std::string tail;
        for (std::size_t i = drop; i < p.segments.size(); ++i) {
            if (i > drop) tail += ".";
            tail += p.segments[i];
        }
        out.push_back(tail);
    }
    return out;
}

EdgeKey edgeKey(const Edge& edge) {
    return std::make_tuple(
        edge.writerLawId, edge.writerBranchId,
        edge.readerLawId, edge.readerBranchId,
        edge.path, edge.aboutInstances);
}

bool sameEdges(const std::vector<Edge>& a, const std::vector<Edge>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (edgeKey(a[i]) != edgeKey(b[i])) return false;
    }
    return true;
}

void buildRelevanceGraphBaseline(
    const std::vector<Prophetic::LawFacts>& facts,
    std::vector<Edge>& outEdges) {

    outEdges.clear();
    std::set<EdgeKey> seen;

    for (const auto& writerFacts : facts) {
        for (const auto& write : writerFacts.writes) {
            for (const auto& readerFacts : facts) {
                for (const auto& read : readerFacts.branchReads) {
                    const auto aa = normalizedPathsLocal(write.path);
                    const auto bb = normalizedPathsLocal(read.path);

                    bool mayAlias = false;
                    for (const auto& left : aa) {
                        for (const auto& right : bb) {
                            if (left == right) {
                                mayAlias = true;
                                break;
                            }
                        }
                        if (mayAlias) break;
                    }

                    if (!mayAlias) continue;
                    if (!write.range.mayIntersect(read.satisfying)) continue;

                    const EdgeKey key = std::make_tuple(
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances);
                    if (!seen.insert(key).second) continue;

                    outEdges.push_back(Edge{
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances});
                }
            }
        }
    }
}

void buildRelevanceGraphOptimized(
    const std::vector<Prophetic::LawFacts>& facts,
    std::vector<Edge>& outEdges) {

    outEdges.clear();

    std::vector<std::vector<std::vector<std::string>>> writeNorms(facts.size());
    std::vector<std::vector<std::vector<std::string>>> readNorms(facts.size());

    for (std::size_t i = 0; i < facts.size(); ++i) {
        writeNorms[i].reserve(facts[i].writes.size());
        for (const auto& write : facts[i].writes) {
            writeNorms[i].push_back(normalizedPathsLocal(write.path));
        }

        readNorms[i].reserve(facts[i].branchReads.size());
        for (const auto& read : facts[i].branchReads) {
            readNorms[i].push_back(normalizedPathsLocal(read.path));
        }
    }

    std::set<EdgeKey> seen;

    for (std::size_t i = 0; i < facts.size(); ++i) {
        const auto& writerFacts = facts[i];
        for (std::size_t w = 0; w < writerFacts.writes.size(); ++w) {
            const auto& write = writerFacts.writes[w];

            for (std::size_t j = 0; j < facts.size(); ++j) {
                const auto& readerFacts = facts[j];
                for (std::size_t r = 0; r < readerFacts.branchReads.size(); ++r) {
                    const auto& read = readerFacts.branchReads[r];

                    bool mayAlias = false;
                    for (const auto& left : writeNorms[i][w]) {
                        for (const auto& right : readNorms[j][r]) {
                            if (left == right) {
                                mayAlias = true;
                                break;
                            }
                        }
                        if (mayAlias) break;
                    }

                    if (!mayAlias) continue;
                    if (!write.range.mayIntersect(read.satisfying)) continue;

                    const EdgeKey key = std::make_tuple(
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances);
                    if (!seen.insert(key).second) continue;

                    outEdges.push_back(Edge{
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances});
                }
            }
        }
    }
}

struct TimingStats {
    double medianUs = 0.0;
    double p95Us = 0.0;
    double minUs = 0.0;
    double maxUs = 0.0;
};

TimingStats computeStatsUs(std::vector<double> samplesUs) {
    std::sort(samplesUs.begin(), samplesUs.end());
    const std::size_t n = samplesUs.size();
    const std::size_t p95Index =
        std::min(n - 1, static_cast<std::size_t>((n - 1) * 0.95));
    return {
        samplesUs[n / 2],
        samplesUs[p95Index],
        samplesUs.front(),
        samplesUs.back()
    };
}

std::vector<std::shared_ptr<Law>> generateSyntheticLaws(std::size_t lawCount) {
    std::vector<std::shared_ptr<Law>> laws;
    laws.reserve(lawCount);

    const std::vector<std::string> prefixes = {
        "@event.subject", "@event.object", "transform", "player.status"
    };
    const std::vector<std::string> properties = {
        "position.x", "position.y", "health", "velocity.z", "mana", "stamina"
    };

    for (std::size_t i = 0; i < lawCount; ++i) {
        const std::string lawId = "law_" + std::to_string(i);
        auto law = std::make_shared<Law>(lawId);
        law->setLawIdentifier(lawId);

        const std::string writePath =
            prefixes[i % prefixes.size()] + "." +
            properties[(i * 3) % properties.size()];
        const std::string readPath =
            prefixes[(i + 1) % prefixes.size()] + "." +
            properties[(i * 2) % properties.size()];

        law->setConditionModel(ConditionNode::compare(
            readPath, ConditionNode::Op::Gt, PropertyValue(10.0)));
        law->setActionModel(ActionNode::set(
            writePath, PropertyValue(20.0)));

        laws.push_back(std::move(law));
    }

    return laws;
}

template <typename Fn>
double timeUs(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

void runBenchmarkForScale(std::size_t lawCount, std::size_t rounds = 50) {
    auto laws = generateSyntheticLaws(lawCount);

    Prophetic::Index productionWitness;
    productionWitness.rebuild(laws);
    const auto facts = productionWitness.facts();

    std::vector<Edge> baselineEdges;
    std::vector<Edge> optimizedEdges;
    buildRelevanceGraphBaseline(facts, baselineEdges);
    buildRelevanceGraphOptimized(facts, optimizedEdges);

    if (!sameEdges(baselineEdges, optimizedEdges)) {
        std::cerr << "CORRECTNESS FAILURE: baseline/optimized full edge tuple mismatch at scale "
                  << lawCount << "\n";
        std::exit(1);
    }
    if (!sameEdges(baselineEdges, productionWitness.relevanceEdges())) {
        std::cerr << "CORRECTNESS FAILURE: benchmark/production full edge tuple mismatch at scale "
                  << lawCount << "\n";
        std::exit(1);
    }
    if (!productionWitness.complete() || !productionWitness.relevanceComplete()) {
        std::cerr << "CORRECTNESS FAILURE: synthetic benchmark unexpectedly incomplete at scale "
                  << lawCount << "\n";
        std::exit(1);
    }

    // Warm up the allocator/code paths before collecting timings.
    for (int i = 0; i < 3; ++i) {
        std::vector<Edge> scratch;
        buildRelevanceGraphBaseline(facts, scratch);
        buildRelevanceGraphOptimized(facts, scratch);
        Prophetic::Index warm;
        warm.rebuild(laws);
    }

    std::vector<double> baselinePhaseUs;
    std::vector<double> optimizedPhaseUs;
    std::vector<double> productionRebuildUs;
    baselinePhaseUs.reserve(rounds);
    optimizedPhaseUs.reserve(rounds);
    productionRebuildUs.reserve(rounds);

    for (std::size_t round = 0; round < rounds; ++round) {
        baselinePhaseUs.push_back(timeUs([&] {
            std::vector<Edge> edges;
            buildRelevanceGraphBaseline(facts, edges);
        }));

        optimizedPhaseUs.push_back(timeUs([&] {
            std::vector<Edge> edges;
            buildRelevanceGraphOptimized(facts, edges);
        }));

        productionRebuildUs.push_back(timeUs([&] {
            Prophetic::Index index;
            index.rebuild(laws);
        }));
    }

    const TimingStats baseline = computeStatsUs(baselinePhaseUs);
    const TimingStats optimized = computeStatsUs(optimizedPhaseUs);
    const TimingStats production = computeStatsUs(productionRebuildUs);
    const double phaseSpeedup =
        baseline.medianUs / std::max(0.001, optimized.medianUs);

    std::cout << "\n======================================================\n";
    std::cout << "SCALE: " << lawCount
              << " Laws (" << facts.size()
              << " Fact sets, " << baselineEdges.size()
              << " Relevance Edges) | Rounds: " << rounds << "\n";
    std::cout << "======================================================\n";
    std::cout << "Correctness Witness: PASS (full RelevanceEdge tuple + "
                 "production parity + relevanceComplete)\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[Pairwise baseline]  Median: " << baseline.medianUs
              << " us (p95: " << baseline.p95Us
              << ", min: " << baseline.minUs
              << ", max: " << baseline.maxUs << ")\n";
    std::cout << "[Pairwise optimized] Median: " << optimized.medianUs
              << " us (p95: " << optimized.p95Us
              << ", min: " << optimized.minUs
              << ", max: " << optimized.maxUs << ")\n";
    std::cout << "[Production Index::rebuild] Median: " << production.medianUs
              << " us (p95: " << production.p95Us
              << ", min: " << production.minUs
              << ", max: " << production.maxUs << ")\n";
    std::cout << "PAIRWISE_PHASE_SPEEDUP scale=" << lawCount
              << " ratio=" << phaseSpeedup
              << " baseline_median_us=" << baseline.medianUs
              << " optimized_median_us=" << optimized.medianUs
              << " baseline_p95_us=" << baseline.p95Us
              << " optimized_p95_us=" << optimized.p95Us << "\n";
    std::cout << "PRODUCTION_REBUILD scale=" << lawCount
              << " median_us=" << production.medianUs
              << " p95_us=" << production.p95Us
              << " min_us=" << production.minUs
              << " max_us=" << production.maxUs << "\n";
}

} // namespace

int main() {
    std::cout << "======================================================\n";
    std::cout << "BENCHMARK: Prophetic relevance-graph path pre-normalization\n";
    std::cout << "Compiler: " << __VERSION__ << " | CMake build type: Release\n";
    std::cout << "The pairwise helpers isolate the changed phase; "
                 "PRODUCTION_REBUILD times the real Index::rebuild implementation "
                 "compiled at this commit.\n";
    std::cout << "======================================================\n";

    runBenchmarkForScale(5);
    runBenchmarkForScale(20);
    runBenchmarkForScale(100);
    runBenchmarkForScale(250);
    return 0;
}
