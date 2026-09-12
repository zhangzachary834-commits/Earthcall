#include "support/test_harness.hpp"
#include <iostream>
#include <chrono>

int main() {
    TestSupport::BootedEngineHarness harness;
    std::string filename = "saves/worlds/synthesis_studio_living.json";
    harness.loadWorld(filename);
    
    // warm up
    for (int i=0; i<5; ++i) harness.lawManager.tick();

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i=0; i<10; ++i) {
        harness.lawManager.tick();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count() / 10.0;
    std::cout << "Average tick took " << ms << " ms" << std::endl;
    
    const auto& timing = harness.lawManager.lastTickTiming();
    std::cout << "  Rete Sync:       " << timing.syncMs << " ms" << std::endl;
    std::cout << "  Seed State:      " << timing.seedMs << " ms" << std::endl;
    std::cout << "  Eval + Sweep:    " << timing.evalMs << " ms" << std::endl;
    std::cout << "  Drive Sessions:  " << timing.driveMs << " ms" << std::endl;
    std::cout << "  Reap Unmade:     " << timing.reapMs << " ms" << std::endl;
    
    return 0;
}
