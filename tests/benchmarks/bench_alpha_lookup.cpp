#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <random>
#include <iomanip>
#include <cmath>

struct DummyAlphaNode {
    std::size_t id;
    std::string description;
    std::vector<int> dummyData;
};

// Strategy 1: Linear Scan
const DummyAlphaNode* findLinear(const std::vector<DummyAlphaNode>& nodes, std::size_t id) {
    for (const auto& alpha : nodes) {
        if (alpha.id == id) return &alpha;
    }
    return nullptr;
}

// Strategy 2: std::unordered_map<std::size_t, std::size_t>
const DummyAlphaNode* findUnorderedMap(const std::vector<DummyAlphaNode>& nodes,
                                       const std::unordered_map<std::size_t, std::size_t>& indexMap,
                                       std::size_t id) {
    auto it = indexMap.find(id);
    if (it != indexMap.end() && it->second < nodes.size()) {
        return &nodes[it->second];
    }
    return nullptr;
}

// Strategy 3: Direct-Address Sparse Vector Index
const DummyAlphaNode* findVectorIndex(const std::vector<DummyAlphaNode>& nodes,
                                     const std::vector<std::size_t>& directVec,
                                     std::size_t id) {
    if (id < directVec.size()) {
        std::size_t idx = directVec[id];
        if (idx != static_cast<std::size_t>(-1) && idx < nodes.size()) {
            return &nodes[idx];
        }
    }
    return nullptr;
}

struct TimingStats {
    double medianNs;
    double p95Ns;
};

TimingStats computeStats(std::vector<double>& samples, std::size_t opsPerSample) {
    std::sort(samples.begin(), samples.end());
    std::size_t n = samples.size();
    double median = samples[n / 2] / opsPerSample;
    double p95 = samples[static_cast<std::size_t>(n * 0.95)] / opsPerSample;
    return {median, p95};
}

void benchmarkPatterns(std::size_t nodeCount, std::size_t rounds = 50, std::size_t opsPerRound = 100000) {
    std::cout << "\n======================================================\n";
    std::cout << "NETWORK SIZE: " << nodeCount << " AlphaNodes | Rounds: " << rounds << " x " << opsPerRound << " ops\n";
    std::cout << "======================================================\n";

    std::vector<DummyAlphaNode> nodes;
    nodes.reserve(nodeCount);
    std::unordered_map<std::size_t, std::size_t> mapIndex;
    std::vector<std::size_t> vecIndex;

    std::mt19937 rng(1337);
    std::size_t currentId = 1;

    for (std::size_t i = 0; i < nodeCount; ++i) {
        currentId += 1 + (rng() % 3); // sparse IDs (shared counter simulation)
        nodes.push_back({currentId, "alpha_" + std::to_string(currentId), {1, 2, 3}});
        mapIndex[currentId] = i;
        if (currentId >= vecIndex.size()) vecIndex.resize(currentId + 1, static_cast<std::size_t>(-1));
        vecIndex[currentId] = i;
    }

    // Dynamic pre-generated query streams to prevent compile-time loop optimization
    std::vector<std::size_t> beginningHits(opsPerRound, nodes[0].id);
    std::vector<std::size_t> endHits(opsPerRound, nodes.back().id);
    std::vector<std::size_t> misses(opsPerRound);
    for (std::size_t i = 0; i < opsPerRound; ++i) {
        misses[i] = currentId + 10 + (rng() % 10000);
    }

    std::vector<std::size_t> holeIds;
    for (std::size_t id = 1; id < vecIndex.size(); ++id) {
        if (vecIndex[id] == static_cast<std::size_t>(-1)) holeIds.push_back(id);
    }
    std::vector<std::size_t> inRangeHoles(opsPerRound);
    std::vector<std::size_t> mixedQueries(opsPerRound);
    for (std::size_t i = 0; i < opsPerRound; ++i) {
        inRangeHoles[i] = holeIds[i % holeIds.size()];
        switch (i % 3) {
            case 0: mixedQueries[i] = nodes[rng() % nodes.size()].id; break;
            case 1: mixedQueries[i] = inRangeHoles[i]; break;
            default: mixedQueries[i] = misses[i]; break;
        }
    }

    auto benchmarkStream = [&](const char* label,
                               const std::vector<std::size_t>& queries) {
        std::vector<double> linearSamples(rounds), mapSamples(rounds), vecSamples(rounds);
        for (std::size_t r = 0; r < rounds; ++r) {
            auto s1 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c1 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) {
                if (findLinear(nodes, queries[i])) c1++;
            }
            auto e1 = std::chrono::high_resolution_clock::now();
            linearSamples[r] =
                std::chrono::duration_cast<std::chrono::nanoseconds>(e1 - s1).count();

            auto s2 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c2 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) {
                if (findUnorderedMap(nodes, mapIndex, queries[i])) c2++;
            }
            auto e2 = std::chrono::high_resolution_clock::now();
            mapSamples[r] =
                std::chrono::duration_cast<std::chrono::nanoseconds>(e2 - s2).count();

            auto s3 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c3 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) {
                if (findVectorIndex(nodes, vecIndex, queries[i])) c3++;
            }
            auto e3 = std::chrono::high_resolution_clock::now();
            vecSamples[r] =
                std::chrono::duration_cast<std::chrono::nanoseconds>(e3 - s3).count();
        }
        auto st1 = computeStats(linearSamples, opsPerRound);
        auto st2 = computeStats(mapSamples, opsPerRound);
        auto st3 = computeStats(vecSamples, opsPerRound);
        std::cout << label << " Linear: " << std::fixed << std::setprecision(2)
                  << st1.medianNs << "ns (p95: " << st1.p95Ns
                  << ") | Map: " << st2.medianNs << "ns (p95: " << st2.p95Ns
                  << ") | DirectVec: " << st3.medianNs << "ns (p95: "
                  << st3.p95Ns << ")\n";
    };

    // Pattern A: Beginning hits
    {
        std::vector<double> linearSamples(rounds), mapSamples(rounds), vecSamples(rounds);

        for (std::size_t r = 0; r < rounds; ++r) {
            auto s1 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c1 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findLinear(nodes, beginningHits[i])) c1++; }
            auto e1 = std::chrono::high_resolution_clock::now();
            linearSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e1 - s1).count();

            auto s2 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c2 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findUnorderedMap(nodes, mapIndex, beginningHits[i])) c2++; }
            auto e2 = std::chrono::high_resolution_clock::now();
            mapSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e2 - s2).count();

            auto s3 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c3 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findVectorIndex(nodes, vecIndex, beginningHits[i])) c3++; }
            auto e3 = std::chrono::high_resolution_clock::now();
            vecSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e3 - s3).count();
        }

        auto st1 = computeStats(linearSamples, opsPerRound);
        auto st2 = computeStats(mapSamples, opsPerRound);
        auto st3 = computeStats(vecSamples, opsPerRound);
        std::cout << "[Hit Beginning] Linear: " << std::fixed << std::setprecision(2) << st1.medianNs << "ns (p95: " << st1.p95Ns
                  << ") | Map: " << st2.medianNs << "ns (p95: " << st2.p95Ns
                  << ") | DirectVec: " << st3.medianNs << "ns (p95: " << st3.p95Ns << ")\n";
    }

    // Pattern B: End hits
    {
        std::vector<double> linearSamples(rounds), mapSamples(rounds), vecSamples(rounds);

        for (std::size_t r = 0; r < rounds; ++r) {
            auto s1 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c1 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findLinear(nodes, endHits[i])) c1++; }
            auto e1 = std::chrono::high_resolution_clock::now();
            linearSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e1 - s1).count();

            auto s2 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c2 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findUnorderedMap(nodes, mapIndex, endHits[i])) c2++; }
            auto e2 = std::chrono::high_resolution_clock::now();
            mapSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e2 - s2).count();

            auto s3 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c3 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findVectorIndex(nodes, vecIndex, endHits[i])) c3++; }
            auto e3 = std::chrono::high_resolution_clock::now();
            vecSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e3 - s3).count();
        }

        auto st1 = computeStats(linearSamples, opsPerRound);
        auto st2 = computeStats(mapSamples, opsPerRound);
        auto st3 = computeStats(vecSamples, opsPerRound);
        std::cout << "[Hit End]       Linear: " << st1.medianNs << "ns (p95: " << st1.p95Ns
                  << ") | Map: " << st2.medianNs << "ns (p95: " << st2.p95Ns
                  << ") | DirectVec: " << st3.medianNs << "ns (p95: " << st3.p95Ns << ")\n";
    }

    // Pattern C: Dynamic Misses
    {
        std::vector<double> linearSamples(rounds), mapSamples(rounds), vecSamples(rounds);

        for (std::size_t r = 0; r < rounds; ++r) {
            auto s1 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c1 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findLinear(nodes, misses[i])) c1++; }
            auto e1 = std::chrono::high_resolution_clock::now();
            linearSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e1 - s1).count();

            auto s2 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c2 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findUnorderedMap(nodes, mapIndex, misses[i])) c2++; }
            auto e2 = std::chrono::high_resolution_clock::now();
            mapSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e2 - s2).count();

            auto s3 = std::chrono::high_resolution_clock::now();
            volatile std::size_t c3 = 0;
            for (std::size_t i = 0; i < opsPerRound; ++i) { if (findVectorIndex(nodes, vecIndex, misses[i])) c3++; }
            auto e3 = std::chrono::high_resolution_clock::now();
            vecSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(e3 - s3).count();
        }

        auto st1 = computeStats(linearSamples, opsPerRound);
        auto st2 = computeStats(mapSamples, opsPerRound);
        auto st3 = computeStats(vecSamples, opsPerRound);
        std::cout << "[Misses]        Linear: " << st1.medianNs << "ns (p95: " << st1.p95Ns
                  << ") | Map: " << st2.medianNs << "ns (p95: " << st2.p95Ns
                  << ") | DirectVec: " << st3.medianNs << "ns (p95: " << st3.p95Ns << ")\n";
    }

    benchmarkStream("[In-range holes]", inRangeHoles);
    benchmarkStream("[Mixed random]  ", mixedQueries);
}

void benchmarkChurnAndMemory() {
    std::cout << "\n======================================================\n";
    std::cout << "LIFECYCLE & MEMORY CHURN BENCHMARK (10,000 MONOTONIC NODES)\n";
    std::cout << "======================================================\n";

    std::unordered_map<std::size_t, std::size_t> mapIndex;
    for (std::size_t i = 1; i <= 100; ++i) {
        mapIndex[i * 100] = i;
    }

    std::size_t maxSimulatedId = 10000;

    // Memory overhead calculation
    std::size_t mapMemBytes = mapIndex.bucket_count() * sizeof(void*) + 100 * (sizeof(std::size_t)*2 + sizeof(void*));
    std::size_t vecMemBytes = maxSimulatedId * sizeof(std::size_t);

    std::cout << "Index memory accounting for 100 active nodes with max ID 10,000:\n";
    std::cout << "  - std::unordered_map memory footprint (buckets=" << mapIndex.bucket_count() << "): ~" << mapMemBytes << " bytes\n";
    std::cout << "  - std::vector<std::size_t> memory footprint: " << vecMemBytes << " bytes (~" << (vecMemBytes / 1024) << " KB)\n";
}

void benchmarkPruneRebuild() {
    std::cout << "\n======================================================\n";
    std::cout << "PRUNE INDEX REBUILD BENCHMARK (dropUnboundAlphaNodes)\n";
    std::cout << "======================================================\n";

    auto runPrune = [](std::size_t maxId, std::size_t liveAlphas) {
        std::vector<std::size_t> vecIndex(maxId + 1, static_cast<std::size_t>(-1));
        std::vector<std::size_t> liveIds;
        for (std::size_t i = 0; i < liveAlphas; ++i) {
            std::size_t id = (i * (maxId / liveAlphas)) + 1;
            liveIds.push_back(id);
            vecIndex[id] = i;
        }

        constexpr std::size_t rounds = 1000;
        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t r = 0; r < rounds; ++r) {
            std::fill(vecIndex.begin(), vecIndex.end(), static_cast<std::size_t>(-1));
            for (std::size_t i = 0; i < liveAlphas; ++i) {
                std::size_t id = liveIds[i];
                vecIndex[id] = i;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        double perOpUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / static_cast<double>(rounds);
        std::cout << "Max ID: " << std::setw(7) << maxId << " | Live Alphas: " << std::setw(3) << liveAlphas
                  << " -> " << std::fixed << std::setprecision(3) << perOpUs << " us/prune\n";
    };

    runPrune(1000, 50);
    runPrune(10000, 50);
    runPrune(100000, 50);
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "REPRODUCIBLE BENCHMARK: AlphaNode Lookup Strategies\n";
    std::cout << "Compiler: GCC " << __VERSION__ << " | build: -O3\n";
    std::cout << "======================================================\n";

    benchmarkPatterns(5);
    benchmarkPatterns(20);
    benchmarkPatterns(100);
    benchmarkPatterns(500);

    benchmarkChurnAndMemory();
    benchmarkPruneRebuild();

    return 0;
}
