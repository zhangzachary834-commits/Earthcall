#include "support/test_harness.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>

struct ScratchSaveRoot {
    std::filesystem::path path;
    ScratchSaveRoot() {
        path = std::filesystem::temp_directory_path() /
            ("earthcall-probe-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path / "worlds");
        SaveSystem::setSaveRoot(path.string());
    }
    ~ScratchSaveRoot() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

int main() {
    const auto source = std::filesystem::absolute(
        TestSupport::resolveRealWorldPath("saves/worlds/synthesis_studio_living.json"));
    if (!std::filesystem::exists(source)) {
        std::cerr << "my_probe_test: world file missing: " << source << "\n";
        return 1;
    }

    ScratchSaveRoot scratch;
    const auto world = scratch.path / "worlds/synthesis_studio_living.json";
    std::filesystem::copy_file(source, world);

    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(world.string());
    
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
