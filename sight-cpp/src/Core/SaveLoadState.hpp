#pragma once

#include <string>
#include <vector>

namespace Core {

// UI/list state for the save-load workflow. The actual save/load logic still
// lives on Game because it touches the full game state (zones, player, world,
// physics, etc.).
struct SaveLoadState {
    std::vector<std::string> files;
    bool showLoadWindow = false;
    bool showSaveWindow = false;
    bool showManager    = false;
    char customName[256] = "";
};

} // namespace Core
