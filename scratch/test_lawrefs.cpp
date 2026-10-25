#include <iostream>
#include <fstream>
#include <vector>
#include "src/json.hpp"

int main() {
    std::string path = "saves/zones/Cathedral of the Living Logos/zone.ecform";
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
    nlohmann::json root = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
    
    if (root.contains("lawRefs")) {
        std::cout << root["lawRefs"].dump(2) << std::endl;
    } else {
        std::cout << "No lawRefs" << std::endl;
    }
    return 0;
}
