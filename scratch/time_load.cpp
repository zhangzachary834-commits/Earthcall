#include <iostream>
#include <chrono>
#include "Singularity/Storage/SaveSystem.hpp"

int main() {
    auto t0 = std::chrono::high_resolution_clock::now();
    auto j = SaveSystem::readSaveJsonFile("saves/worlds/chess_app.json");
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "readSaveJsonFile: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
    return 0;
}
