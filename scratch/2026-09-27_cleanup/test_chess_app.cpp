#include <iostream>
#include <fstream>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/worlds/chess_app.json");
    nlohmann::json j;
    in >> j;
    
    for (auto& zj : j["zones"]) {
        for (auto& mat : zj["materials"]) {
            if (mat["name"] == "chess.board") {
                std::cout << "Original width: " << mat["faceTextures"][2]["width"] << std::endl;
                std::cout << "Original length: " << mat["faceTextures"][2]["pixelsB64"].get<std::string>().size() << std::endl;
            }
        }
    }
    return 0;
}
