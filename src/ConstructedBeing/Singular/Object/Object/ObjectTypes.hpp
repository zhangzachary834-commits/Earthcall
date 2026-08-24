#pragma once

// ============================================================================
// ObjectTypes.hpp - Type definitions for the Object class
// 
// This header contains all the enum types and data structures used by Object,
// allowing code that only needs these types to include this instead of the
// full Object.hpp.
// ============================================================================

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations for types used in the type definitions
namespace geom {
    struct SmoothSurfaceData;
    struct ComplexShapeData;
    struct SdfNode;
    class FieldNode;
    struct TessMesh;
    struct BezierPatch;
}

struct PolyhedronData;

namespace ObjectTypes {

// ============================================================================
// Geometry Types
// ============================================================================


// category; ShapeKind is just which parameterization. Serialized as an int,
// so this enum is APPEND-ONLY.
enum class ShapeKind {
    Cube = 0, Polyhedron = 1, Sphere = 2, Cylinder = 3, Cone = 4,  // legacy-aligned
    Ellipsoid = 5, Ovoid = 6, Paraboloid = 7, Torus = 8, RoundedBox = 9,
    Field = 10, // SDF expression (morph / boolean / implicit)
    Patch = 11, // Bezier control-net surface
    Shape2D = 12,
    Text2D = 13
};

// Per-shape parameters (defaults match the geom factory defaults so an
// unparameterized setShape reproduces current behavior). Persisted so
// parameterized shapes round-trip through save/load.
struct ShapeParams {
    float r = 0.5f;        // sphere/ellipsoid-x, cylinder/cone radius
    float ry = 0.32f;      // ellipsoid y semi-axis
    float rz = 0.5f;       // ellipsoid z semi-axis
    float halfH = 0.5f;    // cylinder/cone half-height
    float majorR = 0.35f;  // torus major radius
    float minorR = 0.15f;  // torus minor radius
    float paraboloidA = 2.0f;   // paraboloid steepness
    float ovoidAsym = 0.25f;  // ovoid taper
    float fillet = 0.12f;      // rounded-box fillet radius
    float width2D = 100.0f;    // 2D shape width
    float height2D = 100.0f;   // 2D shape height
    // Note: For std::string or complex types, PropertyBridge doesn't directly
    // support pointers to members easily, so we typically use attributes/tags.
};

// The fundamental category of the object. Named primitives are merely
// parameterizations inside a category, never the identity itself.
enum class SpatialKind { Polyhedron, SmoothSurface, ComplexShape, Field, Patch };

// ============================================================================
// State Snapshot for Object (used for interaction recording)
// ============================================================================

struct StateSnapshot {
    float time;
    float x, y, z;
    std::string interactionSummary;
    std::vector<std::string> symbolicTags;
};

} // namespace ObjectTypes
