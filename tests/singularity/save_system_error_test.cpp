#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

int main() {
    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_savesystem_error";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    int checks = 0;
    int failures = 0;

    auto check = [&](bool condition, const std::string& desc) {
        checks++;
        if (!condition) {
            failures++;
            std::cerr << "  FAILED: " << desc << "\n";
        } else {
            std::cout << "  ok: " << desc << "\n";
        }
    };

    // Test 1: JSON parse error in readSaveData (fallback to plain JSON)
    std::string badJsonPath = (sandbox / "worlds" / "bad.json").string();
    std::ofstream out(badJsonPath);
    out << "{ \"malformed\": true, }";
    out.close();

    nlohmann::json parsed = SaveSystem::readSaveData(badJsonPath);
    check(parsed.empty(), "Malformed JSON returns empty json object and is caught by try/catch");

    // Test 2: MSGPack decompression exception in readSaveData (.ecsave)
    std::string badEcsavePath = (sandbox / "worlds" / "bad.ecsave").string();
    std::ofstream eout(badEcsavePath, std::ios::binary);
    size_t smallSize = 10;
    eout.write(reinterpret_cast<const char*>(&smallSize), sizeof(smallSize));
    eout << "Some payload";
    eout.close();

    nlohmann::json parsedEcsave = SaveSystem::readSaveData(badEcsavePath);
    check(parsedEcsave.empty(), "Malformed .ecsave msgpack returns empty json object and is caught by try/catch");

    // Test 3: readMatterData returns the raw bytes (not throwing on malformed matter)
    std::string badMatterPath = (sandbox / "worlds" / "bad.ecmatter").string();
    std::ofstream mout(badMatterPath, std::ios::binary);
    mout << "Not compressed data at all";
    mout.close();

    std::vector<uint8_t> parsedMatter = SaveSystem::readMatterData(badMatterPath);
    check(!parsedMatter.empty(), "Malformed .ecmatter returns raw uncompressed bytes as expected");

    // Cleanup
    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << checks - failures << "/" << checks << " checks passed\n";
    if (failures > 0) {
        std::cout << "save_system_error_test: FAILED\n";
        return 1;
    }
    std::cout << "save_system_error_test: ALL OK\n";
    return 0;
}
