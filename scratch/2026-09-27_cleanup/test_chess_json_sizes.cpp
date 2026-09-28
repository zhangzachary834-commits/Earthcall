#include <iostream>
#include <fstream>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/zones/Chess/zone.json");
    if (!in) {
        std::cout << "no zone.json" << std::endl;
        return 1;
    }
    nlohmann::json j;
    in >> j;
    for (auto& mat : j["materials"]) {
        if (mat["name"] == "chess.board") {
            for (auto& ft : mat["faceTextures"]) {
                std::cout << "Texture: " << (ft.contains("width") ? ft["width"].get<int>() : 0)
                          << "x" << (ft.contains("height") ? ft["height"].get<int>() : 0)
                          << " (size=" << (ft.contains("size") ? ft["size"].get<int>() : 0) << ")"
                          << " b64 len: " << (ft.contains("pixelsB64") ? ft["pixelsB64"].get<std::string>().size() : 0)
                          << std::endl;
            }
        }
    }
    return 0;
}
