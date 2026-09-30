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
            std::cout << "MIGRATION ROOT FOUND\n";
            j = json::parse(j["MigrationRoot"].get<std::string>());
        }
        for (auto& el : j.items()) {
            std::cout << "Key: " << el.key() << "\n";
        }
    } catch (...) {}
    return 0;
}
