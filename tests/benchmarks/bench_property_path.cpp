#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    Object obj("test_obj");
    obj.setDynamicProperty("health", PropertyValue(100.0));
    obj.setDynamicProperty("position.x", PropertyValue(10.0));
    obj.setDynamicProperty("spatial.transform.scale", PropertyValue(1.5));

    PropertyPath pathHealth = PropertyPath::parse("health");
    PropertyPath pathPosX = PropertyPath::parse("position.x");
    PropertyPath pathScale = PropertyPath::parse("spatial.transform.scale");

    constexpr std::size_t rounds = 50;
    constexpr std::size_t opsPerRound = 100000;

    // Warmup
    PropertyValue out;
    for (int i = 0; i < 1000; ++i) {
        pathHealth.getValue(obj, out);
        pathPosX.getValue(obj, out);
        pathScale.getValue(obj, out);
    }

    std::vector<double> resolveSamples(rounds);
    std::vector<double> getValueSamples(rounds);

    for (std::size_t r = 0; r < rounds; ++r) {
        auto start1 = std::chrono::high_resolution_clock::now();
        volatile std::size_t dummy = 0;
        for (std::size_t i = 0; i < opsPerRound; ++i) {
            auto slot1 = pathHealth.resolve(obj);
            auto slot2 = pathPosX.resolve(obj);
            auto slot3 = pathScale.resolve(obj);
            if (slot1.dynamicSlot) dummy++;
            if (slot2.dynamicSlot) dummy++;
            if (slot3.dynamicSlot) dummy++;
        }
        auto end1 = std::chrono::high_resolution_clock::now();
        resolveSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count() / static_cast<double>(opsPerRound * 3);

        auto start2 = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < opsPerRound; ++i) {
            pathHealth.getValue(obj, out);
            pathPosX.getValue(obj, out);
            pathScale.getValue(obj, out);
        }
        auto end2 = std::chrono::high_resolution_clock::now();
        getValueSamples[r] = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count() / static_cast<double>(opsPerRound * 3);
    }

    std::sort(resolveSamples.begin(), resolveSamples.end());
    std::sort(getValueSamples.begin(), getValueSamples.end());

    std::cout << "======================================================\n";
    std::cout << "BENCHMARK: PropertyPath Dynamic Property Lookup\n";
    std::cout << "======================================================\n";
    std::cout << "resolve() ns/op median: " << std::fixed << std::setprecision(2) << resolveSamples[rounds / 2]
              << " ns (p95: " << resolveSamples[static_cast<std::size_t>(rounds * 0.95)] << " ns)\n";
    std::cout << "getValue() ns/op median: " << std::fixed << std::setprecision(2) << getValueSamples[rounds / 2]
              << " ns (p95: " << getValueSamples[static_cast<std::size_t>(rounds * 0.95)] << " ns)\n";

    return 0;
}
