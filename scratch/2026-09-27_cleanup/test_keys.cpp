#include <iostream>
#include <fstream>
#include <vector>
#include "src/json.hpp"

int main() {
    std::string path = "saves/zones/Chess/zone.ecform";
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
    nlohmann::json root = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
    
    for (auto it = root.begin(); it != root.end(); ++it) {
        std::cout << it.key() << std::endl;
    }
    return 0;
}
