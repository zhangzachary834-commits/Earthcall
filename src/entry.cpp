#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/Game.hpp"
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "HELLO FROM CPP MAIN!" << std::endl;
    using namespace Core;
    Engine& engine = Engine::instance();
    if (!engine.init(argc, argv)) {
        return -1;
    }

    Game game;
    if (!game.init()) {
        engine.shutdown();
        return -1;
    }

    engine.run(game);

    engine.shutdown();
    return 0;
} 