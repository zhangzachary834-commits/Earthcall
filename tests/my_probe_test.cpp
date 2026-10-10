#include "support/test_harness.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>\n#include <unordered_map>

struct Scratch {
    std::filesystem::path path;
    ~Scratch() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

int main() {
    const auto source = TestSupport::resolveRealWorldPath("saves/worlds/synthesis_studio_living.json");
    if (!std::filesystem::exists(source)) {
        std::cerr << "my_probe_test: source world not found: " << source << "\n";
        return 1;
    }

    const nlohmann::json sourceJson = SaveSystem::readSaveData(source);
    // Verify the identities of the authored Laws, not the total runtime count:
    // the engine legitimately installs additional First-Mover Laws.
    std::unordered_map<std::string, std::string> expectedAuthoredLaws;
    if (sourceJson.contains("authoredLaws")) {
        const auto& authored = sourceJson["authoredLaws"];
        const nlohmann::json* entries = nullptr;
        if (authored.is_array()) entries = &authored;
        else if (authored.is_object() && authored.contains("laws") && authored["laws"].is_array())
            entries = &authored["laws"];
        if (entries) {
            for (const auto& entry : *entries) {
                if (!entry.is_object() || !entry.contains("id") || !entry["id"].is_string() ||
                    !entry.contains("name") || !entry["name"].is_string()) {
                    std::cerr << "my_probe_test: authored Law fixture lacks an id/name\\n";
                    return 1;
                }
                const std::string id = entry["id"].get<std::string>();
                const std::string name = entry["name"].get<std::string>();
                if (!expectedAuthoredLaws.emplace(id, name).second) {
                    std::cerr << "my_probe_test: duplicate authored Law id: " << id << "\\n";
                    return 1;
                }
            }
        }
    }
    if (expectedAuthoredLaws.empty()) {
        std::cerr << "my_probe_test: fixture contains no authored Laws\\n";
        return 1;
    }

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall-probe-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directories(scratch.path / "worlds");
    const auto world = scratch.path / "worlds/living.json";
    std::filesystem::copy_file(source, world);

    SaveSystem::setSaveRoot(scratch.path.string());
    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(world.string());

    const auto scratchLog = scratch.path / "earthcall-io.log";
    if (!std::filesystem::exists(scratchLog)) {
        std::cerr << "my_probe_test: ZoneManager I/O log escaped configured save root\n";
        return 1;
    }
    for (const auto& law : harness.lawManager.getAll()) {
        if (!law) continue;
        const auto found = expectedAuthoredLaws.find(law->getIdentifier());
        if (found == expectedAuthoredLaws.end()) continue;
        if (law->name() != found->second) {
            std::cerr << "my_probe_test: authored Law name mismatch for "
                      << law->getIdentifier() << "\\n";
            return 1;
        }
        expectedAuthoredLaws.erase(found);
    }
    if (!expectedAuthoredLaws.empty()) {
        std::cerr << "my_probe_test: missing authored Law "
                  << expectedAuthoredLaws.begin()->first << "\\n";
        return 1;
    }

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
