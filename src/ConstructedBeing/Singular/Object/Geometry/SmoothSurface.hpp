#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Singularity/OntoMath/ScalarForm.hpp"

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
#include <atomic>
inline uint64_t nextTessMeshId() {
    static std::atomic<uint64_t> s_id{1};
    return s_id.fetch_add(1, std::memory_order_relaxed);
}

struct TessMesh {
    std::vector<TessVertex> tris;
    uint64_t id = nextTessMeshId();
    // Bumped by a writer that mutates `tris` IN PLACE on an already-cached mesh
    // (a future sculpt/deform tool, say). Every current writer instead replaces
    // the whole TessMesh by value (`_smoothMesh = geom::tessellateSmooth(...)`),
    // which already gets a fresh `id` for free — this field exists so an
    // in-place mutator has somewhere honest to say "I changed", the way
    // FaceTexture staleness was already hit once for want of exactly this.
    uint64_t revision = 0;
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

    OntoMath::ScalarForm toScalarForm() const;
};

// --- Quadric algebra -------------------------------------------------------
namespace Quadric {
    glm::mat4 sphere(float r);
    glm::mat4 ellipsoid(float a, float b, float c);
    glm::mat4 cylinder(float r);    // infinite along z; trim via zTrim
    glm::mat4 cone(float k);        // x² + y² = k² z²  (apex at origin)
    glm::mat4 paraboloid(float a);  // a(x² + y²) − z = 0
    glm::mat4 translate(const glm::mat4& Q, const glm::vec3& t); // shift the quadric

    // OntoMath algebraic conversions
    OntoMath::ScalarForm toScalarForm(const glm::mat4& Q);
    glm::mat4 fromScalarForm(const OntoMath::ScalarForm& form);

    // (o + t·d)ᵀ Q (o + t·d) = 0. Returns true on real roots, with t0 ≤ t1.
    bool raycast(const glm::mat4& Q, const glm::vec3& o, const glm::vec3& d, float& t0, float& t1);
    glm::vec3 gradient(const glm::mat4& Q, const glm::vec3& p); // unnormalised surface normal

    // --- Rung 3: OntoMath is the source of truth; the matrix is its evaluation.
    //
    // `gradient` and `raycast` above are NOT a second mathematics. They are the
    // closed-form evaluation of exactly the ScalarForm `toScalarForm(Q)` names:
    // ∇(pᵀQp) = 2Qp is the analytic derivative of that polynomial, and raycast's
    // A/B/C are the coefficients of f(o + t·d) as a polynomial in t. The pair
    // below computes both the long way, straight off ScalarForm::derivative, and
    // geometry_ontomath_test holds the fast path to agreeing with them for every
    // quadric the factories make.
    //
    // They are the REFERENCE, not the hot path, and deliberately so. Rung 2 put
    // symbolic re-expansion inside a per-vertex loop and cost 920x; raycast runs
    // per ray per frame. What the rung actually requires is that the mathematics
    // be authored and legible in OntoMath -- "a channel reads OntoMath; it never
    // decides what the thing is" (ONTOMATH_FRAMEWORK.md §1) -- not that every
    // evaluation walk an AST.
    glm::vec3 gradientFromForm(const OntoMath::ScalarForm& f, const glm::vec3& p);
    // Coefficients of f(o + t·d) = A t² + B t + C, taken symbolically.
    bool raycastCoefficientsFromForm(const OntoMath::ScalarForm& f, const glm::vec3& o,
                                     const glm::vec3& d, double& A, double& B, double& C);
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
