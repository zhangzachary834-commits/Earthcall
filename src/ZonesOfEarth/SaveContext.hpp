#pragma once

// The state a save/load pass needs from outside the Zone layer.
//
// ZoneManager's save/load methods used to take a `Core::Game*`, which pulled
// Singularity/Core/Game.hpp (and friendship into every private member of
// Game) down into ZonesOfEarth/. That is a layer inversion: ZonesOfEarth/
// must not depend on Singularity/Core/. SaveContext carries pointers to the
// handful of live Game members save/load actually touches, so a load can
// still write back into the real objects without ZoneManager knowing Game
// exists at all.

namespace Core { class Camera; }
class MouseHandler;
class Tool;
class Person;
class LawManager;
class Ourverse;

struct SaveContext {
    Core::Camera* camera       = nullptr;
    MouseHandler* mouseHandler = nullptr;
    float*        currentColor = nullptr;   // 3 floats
    Person*       person       = nullptr;
    LawManager*   lawManager   = nullptr;
    Ourverse*     ourverse     = nullptr;      // semantic Ourverse root
    double*       worldTime    = nullptr;
    bool          unpackForAuthoring = false;
};
