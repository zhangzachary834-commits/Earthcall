#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::path fossilPath = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp.new";
    if (std::filesystem::exists(fossilPath)) {
        std::cerr << "FAIL: Legacy fossil file exists: " << fossilPath << "\n";
        return 1;
    }
    std::cout << "PASS: Legacy fossil file Law.cpp.new is absent.\n";
    return 0;
}
