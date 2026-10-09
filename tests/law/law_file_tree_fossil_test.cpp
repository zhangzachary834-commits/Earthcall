#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    std::filesystem::path fossilPath1 = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp.new";
    if (std::filesystem::exists(fossilPath1)) {
        std::cerr << "FAIL: Legacy fossil file exists: " << fossilPath1 << "\n";
        return 1;
    }
    std::filesystem::path fossilPath2 = "src/ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.cpp";
    if (std::filesystem::exists(fossilPath2)) {
        std::cerr << "FAIL: Legacy fossil file exists: " << fossilPath2 << "\n";
        return 1;
    }

    std::filesystem::path toolFilePath = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp";
    if (!std::filesystem::exists(toolFilePath)) {
        toolFilePath = "../src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp";
    }
    if (!std::filesystem::exists(toolFilePath)) {
        toolFilePath = "../../src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp";
    }
    if (!std::filesystem::exists(toolFilePath)) {
        std::cerr << "FAIL: Could not locate Tool.cpp for fossil inspection\n";
        return 1;
    }

    std::ifstream toolFile(toolFilePath);
    if (!toolFile) {
        std::cerr << "FAIL: Could not open Tool.cpp at " << toolFilePath << "\n";
        return 1;
    }

    std::string content((std::istreambuf_iterator<char>(toolFile)), std::istreambuf_iterator<char>());
    if (content.find("eraseLegacyStrokeSegments") != std::string::npos ||
        content.find("deleteLegacyStrokesAt") != std::string::npos ||
        content.find("configureStrokeTool") != std::string::npos ||
        content.find("raycastCollisionAABB") != std::string::npos) {
        std::cerr << "FAIL: Legacy stroke/raycast fossil stubs exist in Tool.cpp\n";
        return 1;
    }

    std::cout << "PASS: Legacy fossil files and stubs are absent.\n";
    return 0;
}
