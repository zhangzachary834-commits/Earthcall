#include <iostream>
#include <fstream>
#include <string>
#include "src/json.hpp"

int main() {
    std::ifstream ifs("saves/games/ToolMigration.json");
    nlohmann::json j;
    ifs >> j;
    if (j.contains("authoredLaws")) {
        auto al = j["authoredLaws"];
        if (al.contains("triggers")) {
            for (auto it = al["triggers"].begin(); it != al["triggers"].end(); ++it) {
                std::cout << "Key: " << it.key() << std::endl;
                for (const auto& type : it.value()) {
                    std::cout << "  Type: " << type.get<std::string>() << std::endl;
                }
            }
        } else {
            std::cout << "Does not contain triggers!" << std::endl;
        }
    }
    return 0;
}
