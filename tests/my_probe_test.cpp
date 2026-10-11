#include "support/test_harness.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

// The probe exercises a real authored world but must never mutate the
// repository's authored saves. Include dependent Zone/Person/Home/Law
// identities so a scratch-world-only boot cannot silently measure an empty
// simulated world instead.
struct ProbeSaveSandbox {
    std::filesystem::path scratchRoot;
    std::string priorSaveRoot;

    explicit ProbeSaveSandbox(const std::filesystem::path& realRoot)
        : priorSaveRoot(SaveSystem::saveRoot()) {
        namespace fs = std::filesystem;
        scratchRoot = fs::temp_directory_path() /
            ("earthcall-probe-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(scratchRoot);
        try {
            fs::copy(realRoot, scratchRoot,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            SaveSystem::setSaveRoot(scratchRoot.string());
        } catch (...) {
            std::error_code ec;
            fs::remove_all(scratchRoot, ec);
            throw;
        }
    }

    ~ProbeSaveSandbox() {
        SaveSystem::setSaveRoot(priorSaveRoot);
        std::error_code ec;
        std::filesystem::remove_all(scratchRoot, ec);
    }
};


int main() {
    namespace fs = std::filesystem;
    const fs::path source = fs::absolute(
        TestSupport::resolveRealWorldPath("saves/worlds/synthesis_studio_living.json"));
    if (!fs::is_regular_file(source)) {
        std::cerr << "my_probe_test: required authored fixture missing: " << source << '\\n';
        return 1;
    }

    // Prove that every save directory remains byte-identical after the test.
    const fs::path realRoot = source.parent_path().parent_path();
    const std::string before = TestSupport::hashDirectoryTree(realRoot);
    {
        ProbeSaveSandbox sandbox(realRoot);
        TestSupport::BootedEngineHarness harness;
        const std::string filename =
            (sandbox.scratchRoot / "worlds" / source.filename()).string();
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
    
    }
    if (TestSupport::hashDirectoryTree(realRoot) != before) {
        std::cerr << "my_probe_test: authored saves changed during probe!\\n";
        return 1;
    }
    return 0;
}
