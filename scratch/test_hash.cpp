#include <iostream>
#include <fstream>
#include <string>
#include <functional>
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
                    std::cout << "zone.ecform Face 2 length: " << b64.length() << "\n";
                    std::cout << "zone.ecform Face 2 hash: " << std::hash<std::string>{}(b64) << "\n";
                }
            }
        }
    } catch (...) {}
    return 0;
}
