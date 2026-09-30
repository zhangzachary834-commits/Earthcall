#include <iostream>
#include <fstream>
#include "src/json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    try {
        json j = json::from_msgpack(data);
        j = json::parse(j["MigrationRoot"].get<std::string>());
        for (const auto& obj : j["world"]["objects"]) {
            if (obj.value("materialId", "") == "material.chess.board") {
                std::cout << "Board Object ID: " << obj.value("objectID", "") << "\n";
            }
        }
    } catch (...) {}
    return 0;
}
