#include "Singularity/Storage/SaveSystem.hpp"
#include "../support/test_save_helper.hpp"

#include <stdexcept>
#include <filesystem>
#include <iostream>

int main() {
    namespace fs = std::filesystem;

    std::cout << "Running save integrity preservation test...\n";

    // Keep these checks active in Release builds (NDEBUG disables assert).
    const auto require = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };

    fs::path sandboxPath;
    fs::path writtenPath;
    {
        TempSaveRoot sandbox;
        sandboxPath = sandbox.path;
        require(SaveSystem::rootPath() == sandboxPath, "save root not redirected to sandbox");

        const nlohmann::json data = {{"integrity_check", true}};
        writtenPath = SaveSystem::writeSaveData(
            data, "save_integrity_guard", SaveSystem::SaveType::CUSTOM);

        // Assert the actual output location, not merely a substring that could
        // also occur in a path outside the isolated save root.
        const fs::path expectedPath =
            sandboxPath / "custom" / "save_integrity_guard.ecsave";
        require(!writtenPath.empty(), "save writer returned an empty path");
        require(writtenPath == fs::absolute(expectedPath), "save escaped expected sandbox path");
        require(fs::exists(writtenPath), "expected save file not written");
        require(fs::file_size(writtenPath) > 0, "written save file is empty");
    }

    // TempSaveRoot must remove the isolated files and restore the default root.
    require(!fs::exists(sandboxPath), "temporary save root was not removed");
    require(!fs::exists(writtenPath), "temporary save file survived cleanup");
    require(SaveSystem::saveRoot().empty(), "default save root not restored");

    std::cout << "save_integrity_preservation_test: ALL OK\n";
    return 0;
}
