#include <iostream>
#include <vector>
#include <string>
#include "src/json.hpp"

int main() {
    nlohmann::json j;
    j["base64"] = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";
    std::vector<uint8_t> bytes = nlohmann::json::to_msgpack(j);
    nlohmann::json j2 = nlohmann::json::from_msgpack(bytes);
    std::cout << j2["base64"].get<std::string>() << std::endl;
    return 0;
}
