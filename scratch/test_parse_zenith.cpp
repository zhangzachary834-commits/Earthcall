#include <iostream>
#include <fstream>
#include "src/json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::ifstream f(argv[1]);
    try {
        json j;
        f >> j;
        std::cout << "Parsed " << j["world"]["name"].get<std::string>() << "\n";
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
