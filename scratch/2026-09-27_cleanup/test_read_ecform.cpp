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
        std::cout << "Material: " << mat["name"].get<std::string>() << std::endl;
        if (mat.contains("faceTextures")) {
            std::cout << "  Has faceTextures array of size " << mat["faceTextures"].size() << std::endl;
        } else {
            std::cout << "  NO faceTextures!" << std::endl;
        }
    }
    return 0;
}
