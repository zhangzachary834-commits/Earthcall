#include "Singularity/Storage/SaveSystem.hpp"
#include "../support/test_save_helper.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    namespace fs = std::filesystem;

    std::cout << "Running save integrity preservation test...\n";

    fs::path sandboxPath;
    fs::path writtenPath;
    {
        TempSaveRoot sandbox;
        sandboxPath = sandbox.path;
        assert(SaveSystem::rootPath() == sandboxPath);

        const nlohmann::json data = {{"integrity_check", true}};
        writtenPath = SaveSystem::writeSaveData(
            data, "save_integrity_guard", SaveSystem::SaveType::CUSTOM);

        // Assert the actual output location, not merely a substring that could
        // also occur in a path outside the isolated save root.
        const fs::path expectedPath =
            sandboxPath / "custom" / "save_integrity_guard.ecsave";
        assert(!writtenPath.empty());
        assert(writtenPath == fs::absolute(expectedPath));
        assert(fs::exists(writtenPath));
        assert(fs::file_size(writtenPath) > 0);
    }

    // TempSaveRoot must remove the isolated files and restore the default root.
    assert(!fs::exists(sandboxPath));
    assert(!fs::exists(writtenPath));
    assert(SaveSystem::saveRoot().empty());

    std::cout << "save_integrity_preservation_test: ALL OK\n";
    return 0;
}
