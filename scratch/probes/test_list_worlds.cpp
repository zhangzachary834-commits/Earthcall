#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
int main() {
    auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
    for (const auto& w : worlds) {
        std::cout << w.label << " (" << w.path << ")\n";
    }
    return 0;
}
