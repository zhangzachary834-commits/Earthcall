#pragma once

#include <glm/glm.hpp>

class Object;

namespace Core {

struct CloneToolState {
    bool active = false;
    glm::vec2 sourceUV {0.0f, 0.0f};
    glm::vec2 offset   {0.0f, 0.0f};
};

struct StrokeTracking {
    glm::vec2 lastBrushUV {-1.0f, -1.0f};
    float lastBrushTime = 0.0f;
    int lastBrushFace = -1;
    Object* lastBrushObject = nullptr;
};

} // namespace Core
