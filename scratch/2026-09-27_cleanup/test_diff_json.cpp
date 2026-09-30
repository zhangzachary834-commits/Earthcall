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
    
    // Remove pixelsB64 from both for cleaner diff
    for (auto& mat : j1["materials"]) {
        for (auto& ft : mat["faceTextures"]) {
            ft.erase("pixelsB64");
        }
    }
    for (auto& mat : j2["materials"]) {
        for (auto& ft : mat["faceTextures"]) {
            ft.erase("pixelsB64");
        }
    }
    
    std::ofstream out1("chess_ecform_no_b64.json");
    out1 << j1.dump(2);
    
    std::ofstream out2("chess_json_no_b64.json");
    out2 << j2.dump(2);
    
    return 0;
}
