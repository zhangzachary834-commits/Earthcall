#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
int main() {
    auto j = SaveSystem::readSaveData("saves/worlds/basic_2d_button.json");
    if (j.is_null()) { std::cout << "failed to read\n"; }
    else { std::cout << "read ok\n"; }
    return 0;
}
