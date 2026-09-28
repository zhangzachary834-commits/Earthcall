#include <iostream>
#include <fstream>
#include <vector>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/zones/Chess/zone.ecform", std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
    nlohmann::json j = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
    
    for (const auto& mat : j["materials"]) {
        if (mat["name"] == "chess.board") {
            for (const auto& ft : mat["faceTextures"]) {
                std::cout << ft["width"] << "x" << ft["height"] << " base64 length: " << ft["pixelsB64"].get<std::string>().length() << std::endl;
            }
        }
    }
    return 0;
}
