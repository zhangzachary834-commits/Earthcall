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
             fs::path("src/ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.cpp")}) {
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

    const fs::path loggerHeaderRelative = "src/Singularity/Core/Logger.hpp";
    std::ifstream loggerHeaderFile(root / loggerHeaderRelative);
    if (!loggerHeaderFile) {
        std::cerr << "FAIL: Could not open Logger.hpp at " << root / loggerHeaderRelative << "\n";
        return 1;
    }
    const std::string loggerContent((std::istreambuf_iterator<char>(loggerHeaderFile)),
                                    std::istreambuf_iterator<char>());
    for (const char* fossil : {"_legacyLawLogFile", "_legacyLawJsonlFile"}) {
        if (loggerContent.find(fossil) != std::string::npos) {
            std::cerr << "FAIL: Legacy law audit mirror fossil exists in Logger.hpp: " << fossil << "\n";
            return 1;
        }
    }

    std::cout << "PASS: Legacy fossil files and stubs are absent.\n";
    return 0;
}
