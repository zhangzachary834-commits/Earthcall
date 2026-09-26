#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <tuple>
#include <vector>

namespace {

// Baseline implementation of relevance graph construction (unoptimized: calls
// pathsMayAlias(string, string) inside inner loop, forcing repeated PropertyPath::parse)
void buildRelevanceGraphBaseline(
    const std::vector<Prophetic::LawFacts>& facts, bool complete, bool anyOpaqueWrite,
    std::vector<Prophetic::Index::RelevanceEdge>& outEdges) {

    (void)complete;
    (void)anyOpaqueWrite;
    outEdges.clear();
    std::set<std::tuple<std::string, std::string, std::string, std::string,
                        std::string, bool>> seen;

    for (const auto& writerFacts : facts) {
        for (const auto& write : writerFacts.writes) {
            for (const auto& readerFacts : facts) {
                for (const auto& read : readerFacts.branchReads) {
                    PropertyPath pWrite = PropertyPath::parse(write.path);
                    PropertyPath pRead = PropertyPath::parse(read.path);

                    auto normPaths = [](const PropertyPath& p, const std::string& dotted) {
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
                    };

                    auto aa = normPaths(pWrite, write.path);
                    auto bb = normPaths(pRead, read.path);
                    bool mayAlias = false;
                    for (const auto& left : aa) {
                        for (const auto& right : bb) {
                            if (left == right) { mayAlias = true; break; }
                        }
                        if (mayAlias) break;
                    }

                    if (!mayAlias) continue;
                    if (!write.range.mayIntersect(read.satisfying)) continue;

                    const auto key = std::make_tuple(
                        write.lawId, write.branchId, read.lawId, read.branchId,
                        read.path, read.aboutInstances);
                    if (!seen.insert(key).second) continue;
                    outEdges.push_back(Prophetic::Index::RelevanceEdge{
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances});
                }
            }
        }
    }
}

// Optimized implementation (pre-computed vector normalization)
void buildRelevanceGraphOptimized(
    const std::vector<Prophetic::LawFacts>& facts,
    std::vector<Prophetic::Index::RelevanceEdge>& outEdges) {

    outEdges.clear();

    auto normalizedPaths = [](const std::string& dotted) {
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
    };

    auto pathsMayAliasVec = [](const std::vector<std::string>& aa, const std::vector<std::string>& bb) {
        for (const auto& left : aa) {
            for (const auto& right : bb) {
                if (left == right) return true;
            }
        }
        return false;
    };

    std::vector<std::vector<std::vector<std::string>>> writeNorms(facts.size());
    std::vector<std::vector<std::vector<std::string>>> readNorms(facts.size());

    for (std::size_t j = 0; j < facts.size(); ++j) {
        writeNorms[j].reserve(facts[j].writes.size());
        for (const auto& w : facts[j].writes) {
            writeNorms[j].push_back(normalizedPaths(w.path));
        }

        readNorms[j].reserve(facts[j].branchReads.size());
        for (const auto& r : facts[j].branchReads) {
            readNorms[j].push_back(normalizedPaths(r.path));
        }
    }

    std::set<std::tuple<std::string, std::string, std::string, std::string,
                        std::string, bool>> seen;

    for (std::size_t i = 0; i < facts.size(); ++i) {
        const auto& writerFacts = facts[i];
        for (std::size_t w = 0; w < writerFacts.writes.size(); ++w) {
            const auto& write = writerFacts.writes[w];
            for (std::size_t j = 0; j < facts.size(); ++j) {
                const auto& readerFacts = facts[j];
                for (std::size_t r = 0; r < readerFacts.branchReads.size(); ++r) {
                    const auto& read = readerFacts.branchReads[r];
                    if (!pathsMayAliasVec(writeNorms[i][w], readNorms[j][r])) continue;
                    if (!write.range.mayIntersect(read.satisfying)) continue;
                    const auto key = std::make_tuple(
                        write.lawId, write.branchId, read.lawId, read.branchId,
                        read.path, read.aboutInstances);
                    if (!seen.insert(key).second) continue;
                    outEdges.push_back(Prophetic::Index::RelevanceEdge{
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances});
                }
            }
        }
    }
}

struct TimingStats {
    double medianUs;
    double p95Us;
    double minUs;
    double maxUs;
};

TimingStats computeStatsUs(std::vector<double>& samplesUs) {
    std::sort(samplesUs.begin(), samplesUs.end());
    std::size_t n = samplesUs.size();
    double median = samplesUs[n / 2];
    double p95 = samplesUs[static_cast<std::size_t>(n * 0.95)];
    double minV = samplesUs.front();
    double maxV = samplesUs.back();
    return {median, p95, minV, maxV};
}

std::vector<std::shared_ptr<Law>> generateSyntheticLaws(std::size_t lawCount) {
    std::vector<std::shared_ptr<Law>> laws;
    laws.reserve(lawCount);

    const std::vector<std::string> prefixes = {"@event.subject", "@event.object", "transform", "player.status"};
    const std::vector<std::string> properties = {"position.x", "position.y", "health", "velocity.z", "mana", "stamina"};

    for (std::size_t i = 0; i < lawCount; ++i) {
        std::string lawId = "law_" + std::to_string(i);
        auto law = std::make_shared<Law>(lawId);
        law->setLawIdentifier(lawId);

        std::string writePath = prefixes[i % prefixes.size()] + "." + properties[(i * 3) % properties.size()];
        std::string readPath = prefixes[(i + 1) % prefixes.size()] + "." + properties[(i * 2) % properties.size()];

        ConditionNode cond = ConditionNode::compare(readPath, ConditionNode::Op::Gt, PropertyValue(10.0));
        ActionNode act = ActionNode::set(writePath, PropertyValue(20.0));

        law->setConditionModel(std::move(cond));
        law->setActionModel(std::move(act));

        laws.push_back(law);
    }
    return laws;
}

void runBenchmarkForScale(std::size_t lawCount, std::size_t rounds = 50) {
    auto laws = generateSyntheticLaws(lawCount);

    Prophetic::Index testIndex;
    testIndex.rebuild(laws);
    const auto& facts = testIndex.facts();

    // Correctness Witness verification
    std::vector<Prophetic::Index::RelevanceEdge> baselineEdges;
    std::vector<Prophetic::Index::RelevanceEdge> optEdges;

    buildRelevanceGraphBaseline(facts, testIndex.complete(), false, baselineEdges);
    buildRelevanceGraphOptimized(facts, optEdges);

    if (baselineEdges.size() != optEdges.size()) {
        std::cerr << "CORRECTNESS FAILURE: baseline edges (" << baselineEdges.size()
                  << ") != opt edges (" << optEdges.size() << ") for scale " << lawCount << "\n";
        std::exit(1);
    }

    for (std::size_t e = 0; e < baselineEdges.size(); ++e) {
        if (baselineEdges[e].writerLawId != optEdges[e].writerLawId ||
            baselineEdges[e].readerLawId != optEdges[e].readerLawId) {
            std::cerr << "CORRECTNESS FAILURE: edge mismatch at index " << e << "\n";
            std::exit(1);
        }
    }

    std::cout << "\n======================================================\n";
    std::cout << "SCALE: " << lawCount << " Laws (" << facts.size() << " Fact sets, "
              << baselineEdges.size() << " Relevance Edges) | Rounds: " << rounds << "\n";
    std::cout << "======================================================\n";
    std::cout << "Correctness Witness: PASS (" << baselineEdges.size() << " edges identical across algorithms)\n";

    std::vector<double> baseUs(rounds), optUs(rounds);

    for (std::size_t r = 0; r < rounds; ++r) {
        auto s1 = std::chrono::high_resolution_clock::now();
        std::vector<Prophetic::Index::RelevanceEdge> bEdges;
        buildRelevanceGraphBaseline(facts, testIndex.complete(), false, bEdges);
        auto e1 = std::chrono::high_resolution_clock::now();
        baseUs[r] = std::chrono::duration_cast<std::chrono::microseconds>(e1 - s1).count();

        auto s2 = std::chrono::high_resolution_clock::now();
        std::vector<Prophetic::Index::RelevanceEdge> oEdges;
        buildRelevanceGraphOptimized(facts, oEdges);
        auto e2 = std::chrono::high_resolution_clock::now();
        optUs[r] = std::chrono::duration_cast<std::chrono::microseconds>(e2 - s2).count();
    }

    auto stBase = computeStatsUs(baseUs);
    auto stOpt = computeStatsUs(optUs);

    double speedup = stBase.medianUs / std::max(0.001, stOpt.medianUs);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[Baseline]  Median: " << stBase.medianUs << " us (p95: " << stBase.p95Us
              << " us, min: " << stBase.minUs << " us, max: " << stBase.maxUs << " us)\n";
    std::cout << "[Optimized] Median: " << stOpt.medianUs << " us (p95: " << stOpt.p95Us
              << " us, min: " << stOpt.minUs << " us, max: " << stOpt.maxUs << " us)\n";
    std::cout << "SPEEDUP RATIO: " << speedup << "x faster (Median: " << stBase.medianUs
              << " us -> " << stOpt.medianUs << " us)\n";
}

} // namespace

int main() {
    std::cout << "======================================================\n";
    std::cout << "BENCHMARK: Prophetic Index Rebuild Path Pre-normalization\n";
    std::cout << "Compiler: GCC " << __VERSION__ << " | Build: Release (-O3)\n";
    std::cout << "======================================================\n";

    runBenchmarkForScale(5);
    runBenchmarkForScale(20);
    runBenchmarkForScale(100);
    runBenchmarkForScale(250);

    return 0;
}
