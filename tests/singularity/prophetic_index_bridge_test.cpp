#include "ZonesOfEarth/AuthorsOfLaw/PropheticIndexBridge.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Core/StringId.hpp"
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>

using namespace Earthcall;

void testBridgeDisjointness() {
    std::cout << "[Test 1] Prophetic Index Bridge: Disjoint vs Intersects\n";

    Prophetic::Index coreIndex;
    PropheticIndexBridge bridge(coreIndex);

    // Create a safe law (only standard value mutation, no structural changes)
    auto safeLaw = std::make_shared<Law>("safe-law");
    safeLaw->setActionModel(ActionNode::set("health", 100.0));
    
    // Build index with safe law
    std::vector<std::shared_ptr<Law>> laws = { safeLaw };
    coreIndex.rebuild(laws);
    bridge.recalculatePossibilitySpace();

    // Query should return Disjoint (Safe to JIT)
    PropertyPath dummyPath = PropertyPath::parse("health");
    auto result = bridge.queryStructuralDisjointness(StringInterner::intern("archetype_a"), dummyPath);
    assert(result == Execution::PropheticIndex::Interference::Disjoint);

    std::cout << "  ✓ Safe laws prove Disjointness\n";

    // Create an unsafe law (opaque structural write, like Spawn)
    auto unsafeLaw = std::make_shared<Law>("unsafe-law");
    unsafeLaw->setActionModel(ActionNode::spawn("enemy-concept"));

    // Rebuild index with unsafe law
    laws.push_back(unsafeLaw);
    coreIndex.rebuild(laws);

    // Query should now return Intersects (Must fallback to VM)
    result = bridge.queryStructuralDisjointness(StringInterner::intern("archetype_a"), dummyPath);
    assert(result == Execution::PropheticIndex::Interference::Intersects);

    std::cout << "  ✓ Opaque structural mutations correctly force Interference\n";
}

int main() {
    std::cout << "\n=== PropheticIndexBridge Test Suite ===\n\n";

    testBridgeDisjointness();

    std::cout << "\n✓ All PropheticIndexBridge tests passed!\n\n";
    return 0;
}
