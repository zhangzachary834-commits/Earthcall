#include <iostream>
#include <fstream>
#include <vector>
#include "src/json.hpp"

int main() {
    std::string path = "saves/zones/Chess/zone.ecform";
    std::ifstream in(path, std::ios::binary);
    if (!in) return 1;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    
    nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
    nlohmann::json root = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
    
    bool fixed = false;
    for (auto& obj : root["world"]["objects"]) {
        if (obj["objectID"] == "object.chess.board") {
            obj["textureResolution"] = 1024;
            fixed = true;
        }
    }
    
    wrapper["MigrationRoot"] = root.dump();
    std::vector<uint8_t> outBytes = nlohmann::json::to_msgpack(wrapper);
    
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(outBytes.data()), outBytes.size());
    std::cout << (fixed ? "Fixed!" : "Not found") << std::endl;
    return 0;
}
