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

    const fs::path engineRelative = "src/Singularity/Core/Engine.hpp";
    std::ifstream engineFile(root / engineRelative);
    if (!engineFile) {
        std::cerr << "FAIL: Could not open Engine.hpp at " << root / engineRelative << "\n";
        return 1;
    }
    const std::string engineContent((std::istreambuf_iterator<char>(engineFile)),
                                    std::istreambuf_iterator<char>());
    for (const char* fossil : {"getUseLegacy2DTools", "_useLegacy2DTools"}) {
        if (engineContent.find(fossil) != std::string::npos) {
            std::cerr << "FAIL: Legacy 2D tools fossil method/member exists: " << fossil << "\n";
            return 1;
        }
    }

    const fs::path consoleRelative =
        "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp";
    std::ifstream consoleFile(root / consoleRelative);
    if (!consoleFile) {
        std::cerr << "FAIL: Could not open CreatorConsoleState.hpp at " << root / consoleRelative << "\n";
        return 1;
    }
    const std::string consoleContent((std::istreambuf_iterator<char>(consoleFile)),
                                     std::istreambuf_iterator<char>());
    if (consoleContent.find("useLegacy2DTools") != std::string::npos) {
        std::cerr << "FAIL: Legacy 2D tools console state field still exists\n";
        return 1;
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

    std::cout << "PASS: Legacy fossil files and stubs are absent.\n";
    return 0;
}
