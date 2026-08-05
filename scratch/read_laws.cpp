#include "Util/SaveSystem.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    nlohmann::json j = SaveSystem::readSaveData(argv[1]);
    if (j.contains("authoredLaws") && j["authoredLaws"].contains("laws")) {
        std::cout << j["authoredLaws"]["laws"].dump(2) << std::endl;
    }
    return 0;
}
