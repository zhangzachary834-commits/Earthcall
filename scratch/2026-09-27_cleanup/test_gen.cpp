#include <iostream>
#include <fstream>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/zones/Chess/zone.json");
    nlohmann::json j;
    in >> j;
    std::ofstream out("test_chess.ecform", std::ios::binary);
    nlohmann::json wrapper;
    wrapper["MigrationRoot"] = j.dump(-1);
    std::vector<uint8_t> bytes = nlohmann::json::to_msgpack(wrapper);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return 0;
}
