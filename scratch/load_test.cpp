#include <iostream>
#include <fstream>
#include "json.hpp"

int main() {
    std::ifstream file("saves/worlds/chess.json");
    if (!file.is_open()) return 1;
    nlohmann::json j;
    file >> j;
    for (const auto& law : j["authoredLaws"]) {
        std::cout << law["name"] << std::endl;
    }
    return 0;
}
