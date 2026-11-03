#include <iostream>
#include <fstream>
#include <vector>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/zones/Chess/zone.ecform", std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
    nlohmann::json j1 = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
    
    std::ifstream in2("saves/zones/Chess/zone.json");
    nlohmann::json j2;
    in2 >> j2;
    
    std::string b1 = j1["materials"][0]["faceTextures"][2]["pixelsB64"].get<std::string>();
    std::string b2 = j2["materials"][0]["faceTextures"][2]["pixelsB64"].get<std::string>();
    
    std::cout << "Match: " << (b1 == b2 ? "YES" : "NO") << std::endl;
    return 0;
}
