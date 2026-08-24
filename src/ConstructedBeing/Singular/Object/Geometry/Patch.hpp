#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "SmoothSurface.hpp" // TessMesh
#include "Singularity/OntoMath/ScalarForm.hpp"

// Geometry kernel — freeform control-net surfaces (Bezier patch).
//
// The "control-point side of the coin": a surface defined by a grid of control
// points you can drag (direct manipulation) whose degree + control coordinates
// ARE its polynomial coefficients (the algebraic side). A Bezier patch is the
// single-segment case of a NURBS surface (no internal knots, unit weights) — the
// honest first step toward full NURBS.
namespace geom {

struct BezierPatch {
    int du = 3;                    // degree in u  (control columns = du + 1)
    int dv = 3;                    // degree in v  (control rows    = dv + 1)
    std::vector<glm::vec3> ctrl;   // (du+1)*(dv+1), row-major: index = j*(du+1) + i

    int nu() const { return du + 1; }
    int nv() const { return dv + 1; }
    glm::vec3&       at(int i, int j)       { return ctrl[j * nu() + i]; }
    const glm::vec3& at(int i, int j) const { return ctrl[j * nu() + i]; }
    bool valid() const { return static_cast<int>(ctrl.size()) == nu() * nv() && nu() >= 2 && nv() >= 2; }
};

// Flat (du+1)x(dv+1) control grid in the XY plane, spanning [-size, size].
BezierPatch makeBezierGrid(int du = 3, int dv = 3, float size = 0.5f);

// Surface point / normal at parameter (u, v) in [0,1]^2 (Bernstein basis).
glm::vec3 evalBezier(const BezierPatch& p, float u, float v);
// The six partial derivatives of a patch's coordinate polynomials, taken ONCE.
//
// bezierNormal used to call patchToScalarForms and derivative() itself, so
// tessellating a patch re-expanded the whole surface into Bernstein-basis
// ScalarForms and re-differentiated it six times FOR EVERY VERTEX. Measured on
// one 4x4 patch at 24x24: 1530 ms against 1.66 ms for the finite-difference
// normal it replaced -- 920x, in the path that runs whenever a control point
// moves. The symbolic normal is the right answer; recomputing the symbols per
// vertex was the mistake.
struct PatchDerivatives {
    OntoMath::ScalarForm dxdu, dydu, dzdu;
    OntoMath::ScalarForm dxdv, dydv, dzdv;
};
PatchDerivatives patchDerivatives(const BezierPatch& p);

// Exact symbolic normal from derivatives already taken. Prefer this in loops.
glm::vec3 bezierNormal(const BezierPatch& p, const PatchDerivatives& d, float u, float v);
// Convenience for a single sample: takes the derivatives, then delegates.
glm::vec3 bezierNormal(const BezierPatch& p, float u, float v);

// Triangulate the patch for GL (local space).
TessMesh tessellateBezier(const BezierPatch& p, int resU = 24, int resV = 24);

// Degree elevation: add a control row/column without changing the surface
// (so the user gets more handles / higher polynomial order to sculpt with).
void elevateU(BezierPatch& p);
void elevateV(BezierPatch& p);

// --- The "same coin": geometry <-> OntoMath algebra -----------------------
// Convert Bezier patch to three multivariate ScalarForms in u and v.
struct PatchForms {
    OntoMath::ScalarForm x;
    OntoMath::ScalarForm y;
    OntoMath::ScalarForm z;
};
PatchForms patchToScalarForms(const BezierPatch& p);
BezierPatch scalarFormsToPatch(const PatchForms& forms, int du, int dv);

// Monomial (power) basis coefficients: index = l*(du+1) + k
std::vector<glm::vec3> patchToMonomial(const BezierPatch& p);
BezierPatch monomialToPatch(const std::vector<glm::vec3>& coeff, int du, int dv);

} // namespace geom
