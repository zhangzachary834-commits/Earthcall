#include <iostream>
#include <fstream>
#include "json.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

int main() {
    std::ifstream file("saves/worlds/chess.json");
    if (!file.is_open()) { std::cout << "no file\n"; return 1; }
    nlohmann::json j;
    file >> j;
    
    int successCount = 0;
    try {
        for (const auto& lj : j["authoredLaws"]["laws"]) {
            auto law = Law::fromJson(lj);
            successCount++;
        }
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Successfully parsed " << successCount << " laws!" << std::endl;
    return 0;
}
