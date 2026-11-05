#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main() {
    namespace fs = std::filesystem;
    const fs::path toolRelative = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp";
    fs::path root;
    bool found = false;
    for (const fs::path& candidate : {fs::path("."), fs::path(".."), fs::path("../..")}) {
        if (fs::is_regular_file(candidate / toolRelative)) {
            root = candidate;
            found = true;
            break;
        }
    }
    if (!found) {
        std::cerr << "FAIL: Could not locate Tool.cpp for fossil inspection\n";
        return 1;
    }

    for (const fs::path& fossil : {
             fs::path("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp.new"),
             fs::path("src/ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.cpp"),
             fs::path("src/Singularity/FirstMoverOntology/Legacy/DesignSystem.cpp"),
             fs::path("src/Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp")}) {
        if (fs::exists(root / fossil)) {
            std::cerr << "FAIL: Legacy fossil file exists: " << root / fossil << "\n";
            return 1;
        }
    }

    std::ifstream toolFile(root / toolRelative);
    if (!toolFile) {
        std::cerr << "FAIL: Could not open Tool.cpp at " << root / toolRelative << "\n";
        return 1;
    }
    const std::string content((std::istreambuf_iterator<char>(toolFile)),
                              std::istreambuf_iterator<char>());
    for (const char* fossil : {"eraseLegacyStrokeSegments", "deleteLegacyStrokesAt",
                                     "configureStrokeTool", "raycastCollisionAABB"}) {
        if (content.find(fossil) != std::string::npos) {
            std::cerr << "FAIL: Legacy stroke/raycast fossil stub exists: " << fossil << "\n";
            return 1;
        }
    }

    // Inspect Engine.hpp and CreatorConsoleState.hpp for legacy 2D tools fossil methods/members
    const fs::path engineHeaderRel = "src/Singularity/Core/Engine.hpp";
    std::ifstream engineFile(root / engineHeaderRel);
    if (!engineFile) {
        std::cerr << "FAIL: Could not open Engine.hpp at " << root / engineHeaderRel << "\n";
        return 1;
    }
    const std::string engineContent((std::istreambuf_iterator<char>(engineFile)),
                                    std::istreambuf_iterator<char>());
    if (engineContent.find("getUseLegacy2DTools") != std::string::npos ||
        engineContent.find("_useLegacy2DTools") != std::string::npos) {
        std::cerr << "FAIL: Engine.hpp contains legacy 2D tools fossil: getUseLegacy2DTools / _useLegacy2DTools\n";
        return 1;
    }

    const fs::path consoleHeaderRel = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp";
    std::ifstream consoleFile(root / consoleHeaderRel);
    if (!consoleFile) {
        std::cerr << "FAIL: Could not open CreatorConsoleState.hpp at " << root / consoleHeaderRel << "\n";
        return 1;
    }
    const std::string consoleContent((std::istreambuf_iterator<char>(consoleFile)),
                                     std::istreambuf_iterator<char>());
    if (consoleContent.find("useLegacy2DTools") != std::string::npos) {
        std::cerr << "FAIL: CreatorConsoleState.hpp contains legacy 2D tools fossil: useLegacy2DTools\n";
        return 1;
    }

    std::cout << "PASS: Legacy fossil files and stubs are absent.\n";
    return 0;
}
