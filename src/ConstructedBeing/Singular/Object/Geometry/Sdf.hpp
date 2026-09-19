#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "SmoothSurface.hpp" // TessMesh
#include "ComplexShape.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

// Geometry kernel — the SDF spine.
//
// A signed-distance expression tree. This is the unifying representation that
// lets shapes morph into each other, be combined (boolean/CSG), and be defined
// by their mathematical structure. It is deliberately plain *data* (not opaque
// lambdas) so it can be serialized, introspected, and edited from a math-mode
// editor as well as from direct manipulation.
namespace geom {

enum class SdfPrim {
    Sphere,     // dims.x = radius
    Box,        // dims = half-extents
    RoundBox,   // dims = half-extents, p0 = corner radius
    Ellipsoid,  // dims = semi-axes
    Cylinder,   // dims.x = radius, dims.y = half-height (axis Z)
    Cone,       // dims.x = base radius, dims.y = half-height (apex +Z)
    Torus,      // dims.x = major radius, dims.y = minor radius (axis Z)
    Expr,       // implicit f(x,y,z) = 0 from a compiled expression
    Convex      // convex polyhedron = max over face half-spaces (see `planes`)
};

// One instruction of a compiled implicit expression (postfix / RPN).
struct SdfToken {
    enum Kind { Num, X, Y, Z, Add, Sub, Mul, Div, Pow, Neg,
                Sin, Cos, Tan, Sqrt, Abs, Exp, Log } kind = Num;
    float num = 0.0f;
};

enum class SdfOp {
    Leaf,        // a primitive
    Morph,       // lerp(a, b, t)  — continuous transform between shapes
    Union,       // min(a, b)
    Intersect,   // max(a, b)
    Subtract,    // max(a, -b)
    SmoothUnion  // smooth min(a, b, t)
};

struct SdfNode {
    SdfOp op = SdfOp::Leaf;

    // Leaf payload.
    SdfPrim prim = SdfPrim::Sphere;
    glm::vec3 dims{0.5f};
    glm::vec3 offset{0.0f};   // local placement of this leaf (for CSG composition)
    float p0 = 0.0f;
    float p1 = 0.0f;

    // Implicit-expression payload (prim == Expr).
    std::string expr;               // source f(x,y,z); 0 iso-surface is the shape
    std::vector<SdfToken> rpn;      // compiled form (derived from expr)
    std::shared_ptr<OntoMath::MathNode> mathNode; // OntoMath algebraic AST
    std::shared_ptr<OntoMath::Piecewise> piecewise; // OntoMath piecewise function

    // Convex-polyhedron payload (prim == Convex): one plane per face, stored as
    // (n.x, n.y, n.z, d) with outward unit normal n and offset d (plane: n·x = d).
    // SDF = max_i (n_i·p - d_i): exact inside & on faces, a valid 1-Lipschitz bound
    // outside. Lowered from a flat-faced polyhedron's faces.
    std::vector<glm::vec4> planes;

    // Operator payload.
    float t = 0.5f;                 // blend for Morph / SmoothUnion
    // Children are held by shared_ptr so a sub-shape CAN be shared by several parents
    // (the Tree -> DAG -> Formation path; see SHAPE_FORMATION_DAG_PLAN.md). Until
    // sharing is an authored act, value semantics are preserved exactly: copying an
    // SdfNode deep-clones its children (below), so every `SdfNode x = other;` stays an
    // independent being. Sharing, when it lands, is explicit shared_ptr aliasing, never
    // an accidental copy.
    std::vector<std::shared_ptr<SdfNode>> children;  // 2 for binary ops

    SdfNode() = default;
    // Deep-clone copy (preserve value semantics — independent subtree).
    SdfNode(const SdfNode& o)
        : op(o.op), prim(o.prim), dims(o.dims), offset(o.offset), p0(o.p0), p1(o.p1),
          expr(o.expr), rpn(o.rpn),
          mathNode(o.mathNode ? std::make_shared<OntoMath::MathNode>(*o.mathNode) : nullptr),
          piecewise(o.piecewise ? std::make_shared<OntoMath::Piecewise>(*o.piecewise) : nullptr),
          planes(o.planes), t(o.t) {
        children.reserve(o.children.size());
        for (const auto& c : o.children)
            children.push_back(c ? std::make_shared<SdfNode>(*c) : nullptr);
    }
    SdfNode& operator=(const SdfNode& o) {
        if (this != &o) { SdfNode tmp(o); *this = std::move(tmp); }
        return *this;
    }
    SdfNode(SdfNode&&) = default;
    SdfNode& operator=(SdfNode&&) = default;

    std::shared_ptr<OntoMath::MathNode> toMathNode() const;

    static SdfNode leaf(SdfPrim prim, glm::vec3 dims, float p0 = 0.0f, float p1 = 0.0f) {
        SdfNode n; n.op = SdfOp::Leaf; n.prim = prim; n.dims = dims; n.p0 = p0; n.p1 = p1; return n;
    }
    static SdfNode binary(SdfOp op, const SdfNode& a, const SdfNode& b, float t = 0.5f) {
        SdfNode n; n.op = op; n.t = t;
        n.children.push_back(std::make_shared<SdfNode>(a));
        n.children.push_back(std::make_shared<SdfNode>(b));
        return n;
    }
    static SdfNode convex(std::vector<glm::vec4> planes) {
        SdfNode n; n.op = SdfOp::Leaf; n.prim = SdfPrim::Convex; n.planes = std::move(planes); return n;
    }
};

// Compile an implicit expression f(x,y,z) into RPN (empty on parse failure).
std::vector<SdfToken> compileExpr(const std::string& src);
// Build an implicit-surface leaf node from an expression.
SdfNode makeImplicit(const std::string& src);
SdfNode makeImplicit(std::shared_ptr<OntoMath::MathNode> node);
SdfNode makeImplicit(std::shared_ptr<OntoMath::Piecewise> pw);
// Named smooth surface as an SDF leaf — same mathematics the quadric
// names, so a marching backend does not draw a UV tessellation instead.
SdfNode sdfFromSmooth(const SmoothSurfaceData& s);
// Capped cylinder / cone (one round quadric + planar caps) as the matching
// SDF primitive. Mesh-fillet complexes (rounded box) return false — those
// are not a single leaf. The round side's UV tessellation is a drawing
// cache, not the shape.
bool sdfFromComplex(const ComplexShapeData& c, SdfNode& out);

// Evaluate the signed distance of the tree at a local-space point.
float evalSdf(const SdfNode& n, const glm::vec3& p);
// Evaluate the conservative interval of the tree over a local-space AABB.
OntoMath::Interval evalRange(const SdfNode& n, const glm::vec3& boxMin, const glm::vec3& boxMax);

// ---------------------------------------------------------------------------
// Min/max heightfield grid — GPU ray-DDA skip acceleration (rendering-
// optimization Phase C; see docs/plans/FRONTIER_MULTI_HUNDRED_FPS_SDF_ENGINE_
// EXPANSION_PLAN.md and its 2026-08-31 addendum). A grazing/horizon ray over a
// heightfield can skip any (x,z) cell whose conservative (hMin,hMax) bound
// cannot intersect the ray's height range there — this is the derivation of
// that bound, done once on the CPU when the field changes, not a shader guess.
// ---------------------------------------------------------------------------
struct HeightGrid {
    int dimX = 0, dimZ = 0;              // 0 means "not built / not eligible"
    std::vector<glm::vec2> cells;        // row-major z-major: idx = z*dimX+x -> (hMin, hMax)
};

// Decidable structural check: is `n` a single Expr leaf whose OntoMath tree is
// exactly Sub(A, h) with A the y-component of the ambient point AND h proved
// independent of ambient y -- literally f(p) = y - h(x,z)? Unknown structure
// refuses specialization and uses the generic implicit path. On a match, *outH
// borrows the h subtree (alive as long as n.mathNode is); on no match, *outH is
// left untouched and the caller must not build a grid for n.
bool isHeightfieldExpr(const SdfNode& n, const OntoMath::MathNode** outH);

// Build the grid: dimX x dimZ cells over [-halfExtent.x,halfExtent.x] x
// [-halfExtent.z,halfExtent.z]. Each cell samples h once at its center and
// widens by a slack derived from a compositional Lipschitz estimate of h
// (see Sdf.cpp) times the cell's worst-case radius. `isHeightfieldExpr` admits
// only h independent of y, but this lower-level builder remains conservative
// if called directly with a y-dependent expression. Returns an empty grid when h
// contains an operation the estimate cannot cover soundly: "no acceleration"
// is always a safe answer, an unsound tightened bound never is.
HeightGrid computeHeightGrid(const OntoMath::MathNode& h, const glm::vec3& halfExtent,
                             int dimX, int dimZ);
// Central-difference surface normal.
glm::vec3 sdfNormal(const SdfNode& n, const glm::vec3& p);
// Sphere-march a ray (origin outside) against the field. nrm normalised.
bool raycastSdf(const SdfNode& n, const glm::vec3& o, const glm::vec3& d,
                float& tHit, glm::vec3& nrm);
// Triangulate the iso-surface (f = 0) for GL via marching tetrahedra over a
// regular grid — handles arbitrary topology (holes, cavities) so boolean and
// implicit shapes render correctly. `extent` is the half-size of the sampled
// cube; `res` is grid cells per axis.
TessMesh tessellateSdf(const SdfNode& n, const glm::vec3& extent = glm::vec3(1.0f), const glm::ivec3& res = glm::ivec3(24));

} // namespace geom
