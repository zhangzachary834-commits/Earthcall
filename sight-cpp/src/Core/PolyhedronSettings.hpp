#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Form/Object/Object.hpp"

namespace Core {

struct PolyhedronSettings {
    // Base primitive selection (used by the brush)
    Object::GeometryType primitive = Object::GeometryType::Cube;

    // Regular polyhedron
    int currentType = 4; // default tetrahedron

    // Custom polyhedron
    bool useCustom = false;
    std::vector<glm::vec3> customVertices;
    std::vector<std::vector<int>> customFaces;
    int customVertexCount = 4;
    int customFaceCount   = 4;

    // Convex/concave variants (0=Regular, 1=Concave, 2=Star, 3=Crater)
    int   concaveType    = 0;
    float concavityAmount = 0.3f;
    float spikeLength    = 0.3f;
    float craterDepth    = 0.2f;

    // Irregular (0=None/Regular, 1=Prism, 2=Antiprism, 3=Pyramid, 4=Bipyramid, 5=Frustum)
    int   irregularType      = 0;
    int   irregularBaseSides = 5;
    float irregularHeight    = 1.0f;
    float frustumTopScale    = 0.5f;

    // Modifiers (applied after base shape)
    bool  applyTruncation   = false;
    float truncationAmount  = 0.3f;
    bool  applyDual         = false;

    // Build a PolyhedronData from current settings (regular or irregular + modifiers).
    PolyhedronData build() const;

    // Generate the custom vertices/faces from customVertexCount / customFaceCount.
    void generateCustom();
};

} // namespace Core
