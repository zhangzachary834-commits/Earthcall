#include "Core/Engine.hpp"
#include "Core/Game.hpp"

int main(int argc, char** argv) {
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