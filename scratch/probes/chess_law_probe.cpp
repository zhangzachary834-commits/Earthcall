#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawManager.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    try {
        std::cout << "Loading save: " << argv[1] << std::endl;
        
        Core::Camera camera;
        MouseHandler mouseHandler;
        Person player;
        LawManager lawManager;
        float currentColor[3] = {1.0f, 1.0f, 1.0f};
        double worldTime = 0.0;
        
        SaveContext ctx;
        ctx.camera = &camera;
        ctx.mouseHandler = &mouseHandler;
        ctx.currentColor = currentColor;
        ctx.player = &player;
        ctx.lawManager = &lawManager;
        ctx.worldTime = &worldTime;
        ctx.unpackForAuthoring = false;
        
        ZoneManager zones;
        zones.loadState(argv[1], ctx);
        
        std::cout << "SUCCESS! World parsed and loaded into ZoneManager!" << std::endl;
    } catch(const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
