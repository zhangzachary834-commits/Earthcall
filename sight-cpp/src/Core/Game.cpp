// Game.cpp – Constructor, destructor, and small accessors.
// All substantial method implementations live in the Game*.cpp split files:
//   GameInit.cpp        – init(), registerCallbacks(), GLFW callbacks
//   GameUpdate.cpp      – update()
//   GameRender.cpp      – render()
//   GameToolbar.cpp     – renderCreatorToolbar()
//   GamePolyhedron.cpp  – buildCurrentPolyhedron(), _generateCustomPolyhedron()
//   GameSaveLoad.cpp    – save / load / shutdown / save-UI dialogs

#include "Game.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cstdio>

extern ZoneManager mgr;

namespace Core {

Game::Game()
    : _elementalToolHandler(&mgr) {}

Game::~Game() {
    printf("[Shutdown] Game dtor\n");

    // Cleanup Advanced Face Paint System
    AdvancedFacePaint::cleanupAdvancedPainter();
}

// ---- Simple accessors ------------------------------------------------

bool Game::getAdvanced2DBrush() const {
    return _useAdvanced2DBrush;
}

void Game::setAdvanced2DBrush(bool value) {
    _useAdvanced2DBrush = value;
}

bool Game::getMouseLeftPressedLast() const {
    return _mouseLeftPressedLast;
}

void Game::setMouseLeftPressedLast(bool value) {
    _mouseLeftPressedLast = value;
}

void Game::setPlacementMode(BrushPlacementMode mode) {
    _placementMode = mode;
}

} // namespace Core
