#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "SmoothSurface.hpp"

// Geometry kernel — complex / mixed-smoothness shapes.
//
// A complex shape has at least one round face AND at least one edge. It is
// modelled fundamentally as a set of real surface *patches* (each flat or
// curved) joined at *classified edges*. This is NOT a polyhedron that got
// visually rounded: a rounded box is genuine flat patches + genuine curved
// fillet patches, and the soft seams between them are real edges separating a
// zero-curvature face from a nonzero-curvature face.
namespace geom {

// Continuity across an edge (per the design doc):
//   Hard     — G0: position matches, normal jumps (a crease, e.g. a cube edge)
//   Soft     — G1: normal continuous, curvature jumps (a rounded/filleted edge)
//   Fairness — G2: curvature continuous, its rate jumps (class-A seam)
enum class EdgeContinuity { Hard, Soft, Fairness };

enum class EdgeCurveModel { Unknown, Polyline, LineSegment, Circle };
enum class ComplexConstituentKind { Patch, Edge };

struct ComplexConstituentRef {
    ComplexConstituentKind kind = ComplexConstituentKind::Patch;
    int id = 0;
};

struct SurfacePatch {
    int id = 0; // stable constituent id for laws/tools; vector index may change
    enum class Type { Planar, Smooth, Mesh };
    Type type = Type::Planar;

    // Planar patch: a flat face given by a CCW boundary polygon (local space).
    std::vector<glm::vec3> polygon;
    glm::vec3 planeNormal{0.0f, 1.0f, 0.0f};

    // Smooth patch: a (possibly trimmed) smooth surface.
    SmoothSurfaceData smooth;

    // Mesh patch: explicit curved geometry (e.g. a fillet strip or corner
    // octant) with real per-vertex normals. Still a genuine curved face — it is
    // NOT a smoothed polyhedron face. `curved` marks it as nonzero-curvature.
    TessMesh mesh;
    bool curved = false;
};

struct ClassifiedEdge {
    int id = 0; // stable constituent id for laws/tools; vector index may change

    // Current adjacency still stores patch vector indices for fast local access.
    // patchAId / patchBId preserve the stable graph identity for law targets.
    int patchA = -1;
    int patchB = -1;
    int patchAId = 0;
    int patchBId = 0;
    EdgeContinuity continuity = EdgeContinuity::Hard;
    float filletRadius = 0.0f;          // for Soft edges (rendered in a later stage)

    EdgeCurveModel curveModel = EdgeCurveModel::Unknown;
    glm::vec3 circleCenter{0.0f};
    glm::vec3 circleNormal{0.0f, 0.0f, 1.0f};
    float circleRadius = 0.0f;
    std::vector<glm::vec3> curve;       // edge curve as a local-space polyline
};

struct ComplexShapeData {
    std::vector<SurfacePatch> patches;
    std::vector<ClassifiedEdge> edges;

    int patchCount() const { return static_cast<int>(patches.size()); }

    int addPatch(const SurfacePatch& patch);
    int addEdge(const ClassifiedEdge& edge);
    SurfacePatch* findPatch(int id);
    const SurfacePatch* findPatch(int id) const;
    ClassifiedEdge* findEdge(int id);
    const ClassifiedEdge* findEdge(int id) const;
    bool validateTopology(std::string* reason = nullptr) const;

private:
    int nextPatchId = 1;
    int nextEdgeId = 1;
};

// --- Builders (local space, ~[-0.5, 0.5]) ----------------------------------
// A capped cylinder: round side (Smooth) + 2 flat disk caps (Planar), joined
// at 2 Hard circular edges.
ComplexShapeData cappedCylinder(float r = 0.5f, float halfH = 0.5f);
// A capped cone: round side (Smooth) + 1 flat base disk (Planar), 1 Hard edge.
ComplexShapeData cappedCone(float r = 0.5f, float halfH = 0.5f);
// A rounded box: 6 flat faces (Planar) with their 12 rims classified as Soft
// edges (filletRadius set). Fillet patches themselves are generated in the
// fillet-rendering stage; here the faces + classified edges are the truth.
ComplexShapeData roundedBox(float half = 0.5f, float fillet = 0.1f);

// --- Queries ---------------------------------------------------------------
// Ray vs complex shape: nearest patch. outFace = patch index, uv in [0,1].
bool raycastComplex(const ComplexShapeData& c, const glm::vec3& o, const glm::vec3& d,
                    float& tHit, int& outFace, glm::vec2& uv);
// Signed implicit value, negative inside (best-effort union of patches' volumes).
float implicitComplex(const ComplexShapeData& c, const glm::vec3& p);
// Triangulate a single patch for GL (local space).
TessMesh tessellatePatch(const SurfacePatch& patch, int slices = 24);
// Triangulate every patch for GL (local space).
TessMesh tessellateComplex(const ComplexShapeData& c, int slices = 24);

} // namespace geom
