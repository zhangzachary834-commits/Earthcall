#include "Singularity/Storage/SaveSystem.hpp"
#include "../support/test_save_helper.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    std::cout << "Running save integrity preservation test...\n";
    std::filesystem::path createdPath;
    {
        TempSaveRoot tempSaveRoot;
        nlohmann::json data = {{"integrity_check", true}};
        createdPath = SaveSystem::writeSaveData(data, "save_integrity_guard", SaveSystem::SaveType::CUSTOM);
        assert(!createdPath.empty());
        assert(createdPath.string().find("earthcall-test-save-") != std::string::npos);
        assert(std::filesystem::exists(createdPath));
    }
    assert(!std::filesystem::exists(createdPath));
    std::cout << "save_integrity_preservation_test: ALL OK\n";
    return 0;
}
