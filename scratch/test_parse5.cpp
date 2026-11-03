#include <iostream>
#include <fstream>
#include "src/json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::ifstream f(argv[1], std::ios::binary);
    if (!f.is_open()) return 1;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    try {
        json j = json::from_msgpack(data);
        if (j.contains("MigrationRoot")) {
            j = json::parse(j["MigrationRoot"].get<std::string>());
        }
        if (j.contains("world") && j["world"].contains("objects")) {
            for (const auto& obj : j["world"]["objects"]) {
                std::cout << "Shape: " << obj.value("shapeKind", "UNKNOWN") << " ID: " << obj.value("objectID", "") << "\n";
            }
        }
    } catch (...) {}
    return 0;
}
