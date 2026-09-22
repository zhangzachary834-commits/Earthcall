#include "Singularity/Execution/ExecutionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticIndexBridge.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <iostream>
#include <cassert>
#include <memory>

using namespace Earthcall;

void testOrchestrationFallback() {
    std::cout << "[Test 1] Execution Channel Orchestration\n";

    Execution::ExecutionChannel channel;
    Prophetic::Index coreIndex;
    auto bridge = std::make_unique<PropheticIndexBridge>(coreIndex);
    
    // Inject the bridge
    channel.setPropheticIndex(std::move(bridge));

    // Create a safe universe
    auto safeLaw = std::make_shared<Law>("safe-law");
    safeLaw->setActionModel(ActionNode::set("health", 100.0));
    std::vector<std::shared_ptr<Law>> laws = { safeLaw };

    // Warm caches
    coreIndex.rebuild(laws);
    channel.warmCaches(laws);

    // Because the universe is safe (Disjoint), the channel should promote to JIT Active
    assert(channel.isJITActive());
    std::cout << "  ✓ Channel promotes to JIT when structurally safe\n";

    // Simulate an opaque write (Spawn) breaking the universe structure
    auto unsafeLaw = std::make_shared<Law>("unsafe-law");
    unsafeLaw->setActionModel(ActionNode::spawn("enemy-concept"));
    laws.push_back(unsafeLaw);
    coreIndex.rebuild(laws);
    
    // As soon as we try to execute, the channel should detect the interference
    // and instantly fallback to the VM.
    Object target;
    channel.executeLaw(*safeLaw, target);

    assert(!channel.isJITActive());
    std::cout << "  ✓ Channel instantly falls back to VM on structural interference\n";
}

int main() {
    std::cout << "\n=== ExecutionChannel Orchestration Test Suite ===\n\n";

    testOrchestrationFallback();

    std::cout << "\n✓ All ExecutionChannel tests passed!\n\n";
    return 0;
}
