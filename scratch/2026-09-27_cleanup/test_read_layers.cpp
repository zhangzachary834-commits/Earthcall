#include <iostream>
#include <fstream>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/zones/Chess/zone.json");
    nlohmann::json j;
    in >> j;
    
    for (const auto& mat : j["materials"]) {
        if (mat["name"] == "chess.board") {
            for (const auto& ft : mat["faceTextures"]) {
                std::cout << "useLayers: " << (ft.contains("useLayers") ? ft["useLayers"].get<bool>() : false) << std::endl;
            }
        }
    }
    return 0;
}
