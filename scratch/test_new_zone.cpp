#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CodecChannel.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawLoader.hpp"
#include <iostream>
#include <fstream>

using namespace Core;

int main() {
    Engine::instance().init(true); // Headless

    std::string zoneId = "ecgraph-test-" + std::to_string(std::time(nullptr));
    auto newZone = ZoneManager::live()->authorZone(zoneId, "", "empty");
    ZoneManager::live()->activateZone(zoneId);
    
    Earthcall::Storage::LawLoader::loadAllLaws(*ZoneManager::live()->active());

    // Give it a few ticks to let WhileTrue laws run!
    for (int i = 0; i < 100; i++) {
        Universe::instance().tick(0.016f);
    }
    
    auto& c = CodecChannel::instance();
    std::cout << "Array Empty? " << c.propArrayEmpty() << "\n";
    std::cout << "Array Size (approx): " << c.propJsonArray().size() << "\n";

    // See if any Singulars were created
    std::cout << "Zone Object Count: " << ZoneManager::live()->active()->objects().size() << "\n";
    
    return 0;
}
