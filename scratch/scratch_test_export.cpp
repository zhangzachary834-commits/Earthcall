#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CodecChannel.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawLoader.hpp"
#include <iostream>
#include <fstream>

using namespace Core;

int main() {
    Engine::instance().init(true); // Headless if supported, or just init
    ZoneManager::live()->activateZone("world-genesis");
    
    // Load laws
    Earthcall::Storage::LawLoader::loadAllLaws(*ZoneManager::live()->active());

    // Fire the export event
    ECA::Event ev;
    ev.type = "export-json";
    Universe::instance().trigger(ev, *ZoneManager::live()->active());

    // Check if saves/worlds/export.json exists
    std::ifstream f("saves/worlds/export.json");
    if (f.good()) {
        std::cout << "SUCCESS: Export file was created!\n";
        return 0;
    } else {
        std::cerr << "FAILED: Export file was not created.\n";
        return 1;
    }
}
