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
    std::cout << "PASS: Legacy fossil files Law.cpp.new and LawAuditLogger.cpp are absent.\n";
    return 0;
}
