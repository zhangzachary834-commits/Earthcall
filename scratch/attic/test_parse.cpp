#include <iostream>
#include <fstream>
#include "src/json.hpp"
#include <string>

int main() {
    std::ifstream ifs("saves/games/ToolMigration.json");
    if (!ifs) {
        std::cerr << "Could not open file" << std::endl;
        return 1;
    }
    nlohmann::json j;
    ifs >> j;
    if (j.contains("authoredLaws")) {
        auto al = j["authoredLaws"];
        if (al.contains("triggers")) {
            std::cout << "Contains triggers! " << al["triggers"].dump() << std::endl;
        } else {
            std::cout << "Does not contain triggers!" << std::endl;
            for (auto it = al.begin(); it != al.end(); ++it) {
                std::cout << "Key: " << it.key() << std::endl;
            }
        }
    }
    return 0;
}
