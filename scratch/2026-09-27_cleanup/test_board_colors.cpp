#include <iostream>
#include <fstream>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/zones/Chess/zone.json");
    if (!in) return 1;
    nlohmann::json j;
    in >> j;
    if (j.contains("objects")) {
        for (auto& obj : j["objects"]) {
            if (obj["objectID"] == "object.chess.board") {
                std::cout << obj["faceColors"].dump(2) << std::endl;
            }
        }
    }
    return 0;
}
