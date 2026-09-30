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
                std::cout << ft["size"] << " base64 length: " << ft["pixelsB64"].get<std::string>().length() << std::endl;
            }
        }
    }
    return 0;
}
