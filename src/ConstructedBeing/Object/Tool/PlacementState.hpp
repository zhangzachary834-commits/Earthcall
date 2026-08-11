#pragma once

#include <glm/glm.hpp>

namespace Core {

enum class BrushPlacementMode {
    InFront = 0,
    ManualDistance,
    CursorSnap
};

struct PlacementState {
    BrushPlacementMode mode = BrushPlacementMode::InFront;
    BrushPlacementMode prevMode = BrushPlacementMode::InFront;

    glm::vec3 manualOffset {0.0f, 0.0f, 2.0f}; // x (right), y (up), z (forward)

    bool      anchorValid = false;
    glm::vec3 anchorPos;
    glm::vec3 anchorRight;
    glm::vec3 anchorUp;
    glm::vec3 anchorForward;
};

} // namespace Core
