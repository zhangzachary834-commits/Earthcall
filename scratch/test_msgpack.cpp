#include "src/json.hpp"
#include <iostream>
#include <vector>

int main() {
    nlohmann::json wrapper = nlohmann::json::object();
    wrapper["MigrationRoot"] = "{\"hello\":\"world\"}";
    std::vector<uint8_t> outBytes = nlohmann::json::to_msgpack(wrapper);
    for(auto b : outBytes) {
        printf("%02x ", (int)b);
    }
    printf("\n");
}
