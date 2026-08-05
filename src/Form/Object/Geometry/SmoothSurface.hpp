#pragma once

#include <glm/glm.hpp>
#include <vector>

// Geometry kernel — pure smooth surfaces (manifolds).
//
// A smooth surface's mathematical truth is either an implicit *quadric*
// (pᵀ Q p = 0 in local space) for everything algebraically expressible
// (sphere, ellipsoid, cylinder-side, cone-side, paraboloid, hyperboloid), or a
// named *parametric* kind for shapes a quadric cannot express (torus, and the
// non-orientable exotics later). The quadric is the fine-grained knob: nudging
// Q morphs the surface continuously (sphere ↔ ellipsoid ↔ paraboloid) without
// changing its identity. Topology flags carry the doc's classification.
namespace geom {

struct TessVertex {
    glm::vec3 pos{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
};
// Triangle soup: every 3 consecutive verts form one triangle (local space).
struct TessMesh {
    std::vector<TessVertex> tris;
};

struct SmoothSurfaceData {
    // Topology classification (the engine can report these).
    bool closed      = true;   // no boundary
    bool orientable  = true;
    bool hasBoundary = false;
    bool isVolume    = false;  // solid vs thin shell

    enum class Model { Quadric, Parametric };
    Model model = Model::Quadric;

    // --- Quadric model: pᵀ Q p = 0 ---
    glm::mat4 Q{1.0f};
    enum class QuadricForm { Sphere, Ellipsoid, CylinderSide, ConeSide, Paraboloid };
    QuadricForm form = QuadricForm::Sphere;

    // --- Parametric model ---
    enum class ParametricKind { Torus, Ovoid, Mobius, Klein, ProjectivePlane };
    ParametricKind pkind = ParametricKind::Torus;

    // Shared shape params (local space, ~[-0.5, 0.5]).
    glm::vec3 axes{0.5f};          // semi-axes (sphere/ellipsoid); axes.x = radius (cyl/cone)
    glm::vec2 zTrim{-0.5f, 0.5f};  // z bounds for open quadrics (cyl/cone/paraboloid side)
    std::vector<float> params;     // torus {majorR, minorR}; ovoid {r, asym}; ...
};

// --- Quadric algebra -------------------------------------------------------
namespace Quadric {
    glm::mat4 sphere(float r);
    glm::mat4 ellipsoid(float a, float b, float c);
    glm::mat4 cylinder(float r);    // infinite along z; trim via zTrim
    glm::mat4 cone(float k);        // x² + y² = k² z²  (apex at origin)
    glm::mat4 paraboloid(float a);  // a(x² + y²) − z = 0
    glm::mat4 translate(const glm::mat4& Q, const glm::vec3& t); // shift the quadric

    // (o + t·d)ᵀ Q (o + t·d) = 0. Returns true on real roots, with t0 ≤ t1.
    bool raycast(const glm::mat4& Q, const glm::vec3& o, const glm::vec3& d, float& t0, float& t1);
    glm::vec3 gradient(const glm::mat4& Q, const glm::vec3& p); // unnormalised surface normal
}

// --- Factories (fill a full SmoothSurfaceData) -----------------------------
SmoothSurfaceData makeSphere(float r = 0.5f);
SmoothSurfaceData makeEllipsoid(float a, float b, float c);
SmoothSurfaceData makeCylinderSide(float r = 0.5f, float halfH = 0.5f);
SmoothSurfaceData makeConeSide(float r = 0.5f, float halfH = 0.5f); // apex at +halfH
SmoothSurfaceData makeParaboloid(float a = 2.0f, float halfH = 0.5f);
SmoothSurfaceData makeTorus(float majorR = 0.35f, float minorR = 0.15f);
SmoothSurfaceData makeOvoid(float r = 0.5f, float asym = 0.25f);

// --- Queries ---------------------------------------------------------------
// Ray-surface intersection honouring trim & form. nrm normalised, uv in [0,1].
bool raycastSmooth(const SmoothSurfaceData& s, const glm::vec3& o, const glm::vec3& d,
                   float& tHit, glm::vec3& nrm, glm::vec2& uv);
// Signed implicit value, negative inside (meaningful for closed volumes).
float implicitSmooth(const SmoothSurfaceData& s, const glm::vec3& p);
// Whether the surface bounds a convex volume (everything but the torus here).
bool isConvex(const SmoothSurfaceData& s);
// Farthest local-space surface point along `dir` (GJK support); analytic for
// sphere/ellipsoid, empty-return signalled via `ok=false` for shapes that need
// the caller's vertex-cloud fallback.
glm::vec3 supportPoint(const SmoothSurfaceData& s, const glm::vec3& dir, bool& ok);
// Triangulate for GL (local space).
TessMesh tessellateSmooth(const SmoothSurfaceData& s, int slices = 24, int stacks = 16);

} // namespace geom
