#include <filesystem>
#include <iostream>

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
    std::filesystem::path fossilPath3 = "src/Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp";
    if (std::filesystem::exists(fossilPath3)) {
        std::cerr << "FAIL: Legacy fossil file exists: " << fossilPath3 << "\n";
        return 1;
    }
    std::filesystem::path fossilPath4 = "src/Singularity/FirstMoverOntology/Legacy/DesignSystem.cpp";
    if (std::filesystem::exists(fossilPath4)) {
        std::cerr << "FAIL: Legacy fossil file exists: " << fossilPath4 << "\n";
        return 1;
    }
    std::cout << "PASS: Legacy fossil files Law.cpp.new, LawAuditLogger.cpp, and DesignSystem are absent.\n";
    return 0;
}
