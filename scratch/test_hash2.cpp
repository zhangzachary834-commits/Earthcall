#include <iostream>
#include <fstream>
#include <string>
#include "src/json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
    std::ifstream f("saves/zones/Chess/zone.ecform", std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    try {
        json j = json::from_msgpack(data);
        j = json::parse(j["MigrationRoot"].get<std::string>());
        for (const auto& m : j["materials"]) {
            if (m.value("name", "") == "chess.board") {
                if (m.contains("faceTextures")) {
                    std::string b64 = m["faceTextures"][2]["pixelsB64"].get<std::string>();
                    std::ofstream out("ecform_b64.txt");
                    out << b64;
                }
            }
        }
    } catch (...) {}
    return 0;
}
