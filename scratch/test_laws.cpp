#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <cstdio>
#include <chrono>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    ZoneManager::instance().resetToEmpty();
    Universe::instance().setClock(0.0, 0.0);
    
    std::string err;
    if (!SaveSystem::loadWorld(argv[1], err)) {
        printf("Failed to load: %s\n", err.c_str());
        return 1;
    }
    
    auto zone = ZoneManager::instance().activeZone();
    if (!zone) return 1;
    
    printf("Loaded world with %zu objects.\n", zone->getOwnedObjects().size());
    
    auto& lawManager = zone->lawManager();
    
    double t0 = glfwGetTime();
    lawManager.tick();
    double t1 = glfwGetTime();
    
    printf("One tick took: %.3f ms\n", (t1 - t0) * 1000.0);
    return 0;
}
