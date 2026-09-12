#include <iostream>
#include <vector>
#include "json.hpp"
int main() {
    std::vector<uint8_t> data = {10, 0, 0, 0, 0, 0, 0, 0, 'S', 'o', 'm', 'e', ' ', 'p', 'a', 'y', 'l', 'o', 'a', 'd'};
    try {
        nlohmann::json::from_msgpack(data);
        std::cout << "Did not throw\n";
    } catch(const std::exception& e) {
        std::cout << "Threw: " << e.what() << "\n";
    }
    return 0;
}
