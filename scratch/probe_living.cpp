#include "ZonesOfEarth/AuthorsOfLaw/LawManager.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "ConstructedBeing/Universe.hpp"
#include <iostream>
#include <chrono>

using namespace Earthcall;

int main() {
    Universe::instance().clear();
    ZoneManager zones;
    LawManager lawManager;
    double worldTime = 0.0;
    
    std::string filename = "saves/worlds/synthesis_studio_living.json";
    if (!loadWorld(filename, zones, lawManager)) {
        std::cerr << "Failed to load " << filename << std::endl;
        return 1;
    }
    
    auto active = zones.zones()[zones.currentIndex()];
    std::cout << "Loaded " << active->getOwnedObjects().size() << " objects" << std::endl;
    
    for (int i=0; i<3; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        lawManager.tick();
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "Tick " << i << " took " << ms << " ms" << std::endl;
        
        const auto& timing = lawManager.lastTickTiming();
        std::cout << "  Rete Sync:       " << timing.syncMs << " ms" << std::endl;
        std::cout << "  Seed State:      " << timing.seedMs << " ms" << std::endl;
        std::cout << "  Eval + Sweep:    " << timing.evalMs << " ms" << std::endl;
        std::cout << "  Drive Sessions:  " << timing.driveMs << " ms" << std::endl;
        std::cout << "  Reap Unmade:     " << timing.reapMs << " ms" << std::endl;
    }
    
    return 0;
}
