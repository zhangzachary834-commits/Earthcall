#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "ConstructedBeing/Object/Object.hpp"

class Zone;
class ZoneManager;
class Person;
class Ourverse;

namespace Rendering {

    enum class CreatorSection {
        Paint,
        Create3D,
        Character,
        World,
        Assets,
        Relations,
        Zones
    };

    enum class Mode3D {
        None,
        Selection,
        Rotation,
        Morph,
        Combine,
        Clay,
        Pottery,
        FacePaint,
        FaceBrush,
        BrushCreate
    };

    struct CreatorConsoleState {
        CreatorSection currentSection = CreatorSection::Create3D;
        Mode3D current3DMode = Mode3D::Selection;
        Object::ShapeKind currentShapeKind = Object::ShapeKind::Sphere;
        bool wireframe = false;
        glm::vec3 createColor = glm::vec3(0.8f, 0.8f, 0.8f);
        bool characterDesignLocked = false;
        bool cursorToolsOpen = false;
    };

    // Global state instance for the Creator Console
    CreatorConsoleState& getCreatorConsoleState();

    // Main window rendering function
    void renderCreatorConsoleWindow(bool* open, Person* player, Object* selected, ZoneManager& zoneMgr);

    // Hardcoded preview rendering for First Mover 3D tools
    void renderCreatorConsole3DPreviews(Person* player, Object* selected);

} // namespace Rendering
