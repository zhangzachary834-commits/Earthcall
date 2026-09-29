#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <cassert>

namespace {

using Clock = std::chrono::high_resolution_clock;

void runBenchmarkForScale(std::size_t entityCount, std::size_t relationsPerEntity, std::size_t queryRounds = 1000) {
    std::vector<std::unique_ptr<Object>> objects;
    objects.reserve(entityCount);
    for (std::size_t i = 0; i < entityCount; ++i) {
        auto obj = std::make_unique<Object>();
        obj->setObjectID("entity_" + std::to_string(i));
        objects.push_back(std::move(obj));
    }

    RelationManager mgr;
    std::mt19937 rng(42);

    for (std::size_t i = 0; i < entityCount; ++i) {
        for (std::size_t k = 0; k < relationsPerEntity; ++k) {
            std::size_t targetIdx = (i + 1 + (rng() % (entityCount - 1))) % entityCount;
            std::string type = (k % 2 == 0) ? "instance-of" : "connected-to";
            auto rel = std::make_shared<Relation>(type, *objects[i], *objects[targetIdx], true);
            mgr.add(rel);
        }
    }

    const std::size_t totalRelations = mgr.getAll().size();

    // Query 1: getRelationsOf(Singular&)
    auto start1 = Clock::now();
    std::size_t count1 = 0;
    for (std::size_t r = 0; r < queryRounds; ++r) {
        const auto& targetObj = *objects[r % entityCount];
        auto res = mgr.getRelationsOf(targetObj);
        count1 += res.size();
    }
    auto end1 = Clock::now();
    double timeUs1 = std::chrono::duration<double, std::micro>(end1 - start1).count() / queryRounds;

    // Query 2: getRelationsOf(std::string)
    auto start2 = Clock::now();
    std::size_t count2 = 0;
    for (std::size_t r = 0; r < queryRounds; ++r) {
        const std::string& id = objects[r % entityCount]->getIdentifier();
        auto res = mgr.getRelationsOf(id);
        count2 += res.size();
    }
    auto end2 = Clock::now();
    double timeUs2 = std::chrono::duration<double, std::micro>(end2 - start2).count() / queryRounds;

    // Query 3: getRelationsBetween(Singular&, Singular&)
    auto start3 = Clock::now();
    std::size_t count3 = 0;
    for (std::size_t r = 0; r < queryRounds; ++r) {
        const auto& a = *objects[r % entityCount];
        const auto& b = *objects[(r + 1) % entityCount];
        auto res = mgr.getRelationsBetween(a, b);
        count3 += res.size();
    }
    auto end3 = Clock::now();
    double timeUs3 = std::chrono::duration<double, std::micro>(end3 - start3).count() / queryRounds;

    // Query 4: findAdjacentEntities(std::string, std::string)
    auto start4 = Clock::now();
    std::size_t count4 = 0;
    for (std::size_t r = 0; r < queryRounds; ++r) {
        const std::string& id = objects[r % entityCount]->getIdentifier();
        auto res = mgr.findAdjacentEntities(id, "instance-of");
        count4 += res.size();
    }
    auto end4 = Clock::now();
    double timeUs4 = std::chrono::duration<double, std::micro>(end4 - start4).count() / queryRounds;

    std::cout << "SCALE: " << entityCount << " entities, " << totalRelations << " total relations | " << queryRounds << " query ops\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  getRelationsOf(Singular&)  : " << timeUs1 << " us/op (total count=" << count1 << ")\n";
    std::cout << "  getRelationsOf(string)     : " << timeUs2 << " us/op (total count=" << count2 << ")\n";
    std::cout << "  getRelationsBetween(Singular&): " << timeUs3 << " us/op (total count=" << count3 << ")\n";
    std::cout << "  findAdjacentEntities(string): " << timeUs4 << " us/op (total count=" << count4 << ")\n";
}

} // namespace

int main() {
    std::cout << "======================================================\n";
    std::cout << "BENCHMARK: RelationManager query methods\n";
    std::cout << "======================================================\n";

    runBenchmarkForScale(100, 5, 2000);
    runBenchmarkForScale(500, 5, 2000);
    runBenchmarkForScale(1000, 10, 2000);

    return 0;
}
