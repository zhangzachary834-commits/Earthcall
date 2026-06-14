#include "ComplexShape.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace geom {

static const float kPI = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Builders
// ---------------------------------------------------------------------------
static std::vector<glm::vec3> diskPolygon(float radius, float z, int segments) {
    std::vector<glm::vec3> poly;
    poly.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        float a = float(i) / segments * 2.0f * kPI;
        poly.emplace_back(radius * std::cos(a), radius * std::sin(a), z);
    }
    return poly;
}

static SurfacePatch planarDisk(float radius, float z, const glm::vec3& outwardN, int segments = 32) {
    SurfacePatch p;
    p.type = SurfacePatch::Type::Planar;
    p.polygon = diskPolygon(radius, z, segments);
    p.planeNormal = outwardN;
    return p;
}

static SurfacePatch smoothPatch(const SmoothSurfaceData& s) {
    SurfacePatch p;
    p.type = SurfacePatch::Type::Smooth;
    p.smooth = s;
    return p;
}

ComplexShapeData cappedCylinder(float r, float halfH) {
    ComplexShapeData c;
    c.patches.push_back(smoothPatch(makeCylinderSide(r, halfH)));     // 0: round side
    c.patches.push_back(planarDisk(r, +halfH, glm::vec3(0, 0, 1)));   // 1: top cap
    c.patches.push_back(planarDisk(r, -halfH, glm::vec3(0, 0, -1)));  // 2: bottom cap
    ClassifiedEdge top; top.patchA = 0; top.patchB = 1; top.continuity = EdgeContinuity::Hard;
    top.curve = diskPolygon(r, +halfH, 32);
    ClassifiedEdge bot; bot.patchA = 0; bot.patchB = 2; bot.continuity = EdgeContinuity::Hard;
    bot.curve = diskPolygon(r, -halfH, 32);
    c.edges.push_back(top);
    c.edges.push_back(bot);
    return c;
}

ComplexShapeData cappedCone(float r, float halfH) {
    ComplexShapeData c;
    c.patches.push_back(smoothPatch(makeConeSide(r, halfH)));         // 0: round side (apex +halfH)
    c.patches.push_back(planarDisk(r, -halfH, glm::vec3(0, 0, -1)));  // 1: base cap
    ClassifiedEdge base; base.patchA = 0; base.patchB = 1; base.continuity = EdgeContinuity::Hard;
    base.curve = diskPolygon(r, -halfH, 32);
    c.edges.push_back(base);
    return c;
}

// A quarter-cylinder fillet strip bridging two orthogonal faces along an edge.
// nA, nB are the (unit, orthogonal) outward face normals; the cylinder axis is
// nA×nB; `inset` is half − fillet (where the flat faces stop). Exact parametric
// surface: c + axis·t + r·(cos φ·nA + sin φ·nB), φ ∈ [0, π/2].
static SurfacePatch filletStrip(const glm::vec3& nA, const glm::vec3& nB,
                                float inset, float r, int seg = 8) {
    SurfacePatch p; p.type = SurfacePatch::Type::Mesh; p.curved = true;
    glm::vec3 axis = glm::normalize(glm::cross(nA, nB));
    glm::vec3 base = nA * inset + nB * inset; // centre line offset in the nA-nB plane
    auto P = [&](float phi, float t) {
        glm::vec3 dir = std::cos(phi) * nA + std::sin(phi) * nB;
        return base + axis * t + r * dir;
    };
    auto N = [&](float phi) { return std::cos(phi) * nA + std::sin(phi) * nB; };
    for (int i = 0; i < seg; ++i) {
        float phi0 = (float(i) / seg) * (kPI * 0.5f);
        float phi1 = (float(i + 1) / seg) * (kPI * 0.5f);
        glm::vec3 n0 = N(phi0), n1 = N(phi1);
        glm::vec3 a = P(phi0, -inset), b = P(phi0, inset);
        glm::vec3 cc = P(phi1, inset), d = P(phi1, -inset);
        auto V = [](const glm::vec3& pos, const glm::vec3& nor, glm::vec2 uv) {
            TessVertex v; v.pos = pos; v.normal = glm::normalize(nor); v.uv = uv; return v;
        };
        p.mesh.tris.push_back(V(a, n0, {0, 0})); p.mesh.tris.push_back(V(b, n0, {0, 1})); p.mesh.tris.push_back(V(cc, n1, {1, 1}));
        p.mesh.tris.push_back(V(a, n0, {0, 0})); p.mesh.tris.push_back(V(cc, n1, {1, 1})); p.mesh.tris.push_back(V(d, n1, {1, 0}));
    }
    return p;
}

// A sphere-octant corner cap tangent to three faces. s = corner signs (±1 each).
static SurfacePatch cornerOctant(const glm::vec3& s, float inset, float r, int seg = 6) {
    SurfacePatch p; p.type = SurfacePatch::Type::Mesh; p.curved = true;
    glm::vec3 centre(s.x * inset, s.y * inset, s.z * inset);
    // Spherical patch over the octant facing (s.x, s.y, s.z): map (u,v) in [0,1]²
    // to directions in the corner's sign-octant via its axis basis.
    glm::vec3 ex(s.x, 0, 0), ey(0, s.y, 0), ez(0, 0, s.z);
    auto P = [&](float u, float v) {
        float a = u * (kPI * 0.5f);   // sweep ex → ey
        float b = v * (kPI * 0.5f);   // tilt up toward ez
        glm::vec3 dir = std::cos(b) * (std::cos(a) * ex + std::sin(a) * ey) + std::sin(b) * ez;
        return std::make_pair(centre + r * dir, glm::normalize(dir));
    };
    for (int i = 0; i < seg; ++i) for (int j = 0; j < seg; ++j) {
        float u0 = float(i) / seg, u1 = float(i + 1) / seg;
        float v0 = float(j) / seg, v1 = float(j + 1) / seg;
        auto p00 = P(u0, v0), p10 = P(u1, v0), p11 = P(u1, v1), p01 = P(u0, v1);
        auto V = [](std::pair<glm::vec3, glm::vec3> pn, glm::vec2 uv) {
            TessVertex v; v.pos = pn.first; v.normal = pn.second; v.uv = uv; return v;
        };
        p.mesh.tris.push_back(V(p00, {0, 0})); p.mesh.tris.push_back(V(p10, {1, 0})); p.mesh.tris.push_back(V(p11, {1, 1}));
        p.mesh.tris.push_back(V(p00, {0, 0})); p.mesh.tris.push_back(V(p11, {1, 1})); p.mesh.tris.push_back(V(p01, {0, 1}));
    }
    return p;
}

ComplexShapeData roundedBox(float half, float fillet) {
    ComplexShapeData c;
    fillet = std::min(fillet, half * 0.95f);
    const float inset = half - fillet; // where flat faces stop / fillet centre offset

    // 6 flat faces, each inset by the fillet radius so the curved strips can fill
    // the rims. The face plane stays at ±half; only its in-plane extent shrinks.
    const glm::vec3 normals[6] = {
        { 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}
    };
    for (const auto& n : normals) {
        SurfacePatch p;
        p.type = SurfacePatch::Type::Planar;
        p.planeNormal = n;
        glm::vec3 t = (std::fabs(n.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 u = glm::normalize(glm::cross(n, t));
        glm::vec3 v = glm::normalize(glm::cross(n, u));
        glm::vec3 ctr = n * half;
        p.polygon = {
            ctr + (-u - v) * inset, ctr + (u - v) * inset,
            ctr + (u + v) * inset,  ctr + (-u + v) * inset
        };
        c.patches.push_back(p);
    }

    // 12 edge fillet strips (real curved patches). Record each as a Soft edge
    // between the two flat faces it bridges.
    struct EdgeDef { glm::vec3 nA, nB; int faceA, faceB; };
    const EdgeDef edges[12] = {
        {{1,0,0},{0,1,0},0,2}, {{1,0,0},{0,-1,0},0,3}, {{-1,0,0},{0,1,0},1,2}, {{-1,0,0},{0,-1,0},1,3},
        {{1,0,0},{0,0,1},0,4}, {{1,0,0},{0,0,-1},0,5}, {{-1,0,0},{0,0,1},1,4}, {{-1,0,0},{0,0,-1},1,5},
        {{0,1,0},{0,0,1},2,4}, {{0,1,0},{0,0,-1},2,5}, {{0,-1,0},{0,0,1},3,4}, {{0,-1,0},{0,0,-1},3,5},
    };
    for (const auto& e : edges) {
        int patchIdx = c.patchCount();
        c.patches.push_back(filletStrip(e.nA, e.nB, inset, fillet));
        ClassifiedEdge ce; ce.patchA = e.faceA; ce.patchB = patchIdx;
        ce.continuity = EdgeContinuity::Soft; ce.filletRadius = fillet;
        c.edges.push_back(ce);
        ClassifiedEdge ce2; ce2.patchA = e.faceB; ce2.patchB = patchIdx;
        ce2.continuity = EdgeContinuity::Soft; ce2.filletRadius = fillet;
        c.edges.push_back(ce2);
    }

    // 8 corner octant caps (real curved patches).
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                c.patches.push_back(cornerOctant(glm::vec3(sx, sy, sz), inset, fillet));

    return c;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------
static bool raycastPlanarPatch(const SurfacePatch& patch, const glm::vec3& o, const glm::vec3& d,
                               float& tHit, glm::vec2& uv) {
    const glm::vec3& n = patch.planeNormal;
    float denom = glm::dot(n, d);
    if (std::fabs(denom) < 1e-6f) return false;
    const glm::vec3& p0 = patch.polygon.empty() ? glm::vec3(0.0f) : patch.polygon[0];
    float t = glm::dot(n, p0 - o) / denom;
    if (t <= 1e-4f) return false;
    glm::vec3 hit = o + d * t;

    // Point-in-polygon in a 2D basis on the plane.
    glm::vec3 t1 = (std::fabs(n.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 ax = glm::normalize(glm::cross(n, t1));
    glm::vec3 ay = glm::normalize(glm::cross(n, ax));
    auto to2D = [&](const glm::vec3& p) { return glm::vec2(glm::dot(p - p0, ax), glm::dot(p - p0, ay)); };
    glm::vec2 h2 = to2D(hit);
    bool inside = false;
    glm::vec2 minB(1e9f), maxB(-1e9f);
    for (size_t i = 0, j = patch.polygon.size() - 1; i < patch.polygon.size(); j = i++) {
        glm::vec2 pi = to2D(patch.polygon[i]);
        glm::vec2 pj = to2D(patch.polygon[j]);
        minB = glm::min(minB, pi); maxB = glm::max(maxB, pi);
        if (((pi.y > h2.y) != (pj.y > h2.y)) &&
            (h2.x < (pj.x - pi.x) * (h2.y - pi.y) / (pj.y - pi.y) + pi.x)) {
            inside = !inside;
        }
    }
    if (!inside) return false;
    tHit = t;
    glm::vec2 span = glm::max(maxB - minB, glm::vec2(1e-4f));
    uv = (h2 - minB) / span;
    return true;
}

// Möller–Trumbore ray/triangle.
static bool rayTri(const glm::vec3& o, const glm::vec3& d,
                   const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t) {
    glm::vec3 e1 = b - a, e2 = c - a;
    glm::vec3 pv = glm::cross(d, e2);
    float det = glm::dot(e1, pv);
    if (std::fabs(det) < 1e-8f) return false;
    float inv = 1.0f / det;
    glm::vec3 tv = o - a;
    float u = glm::dot(tv, pv) * inv;
    if (u < 0.0f || u > 1.0f) return false;
    glm::vec3 qv = glm::cross(tv, e1);
    float v = glm::dot(d, qv) * inv;
    if (v < 0.0f || u + v > 1.0f) return false;
    t = glm::dot(e2, qv) * inv;
    return t > 1e-4f;
}

static bool raycastMeshPatch(const SurfacePatch& patch, const glm::vec3& o, const glm::vec3& d,
                             float& tHit, glm::vec2& uv) {
    float nearest = 1e9f; bool found = false;
    const auto& tris = patch.mesh.tris;
    for (size_t i = 0; i + 2 < tris.size(); i += 3) {
        float t;
        if (rayTri(o, d, tris[i].pos, tris[i + 1].pos, tris[i + 2].pos, t) && t < nearest) {
            nearest = t; uv = tris[i].uv; found = true;
        }
    }
    if (found) tHit = nearest;
    return found;
}

bool raycastComplex(const ComplexShapeData& c, const glm::vec3& o, const glm::vec3& d,
                    float& tHit, int& outFace, glm::vec2& uv) {
    float nearest = 1e9f;
    bool found = false;
    for (int i = 0; i < c.patchCount(); ++i) {
        const SurfacePatch& patch = c.patches[i];
        float t; glm::vec2 puv;
        bool hit = false;
        if (patch.type == SurfacePatch::Type::Planar) {
            hit = raycastPlanarPatch(patch, o, d, t, puv);
        } else if (patch.type == SurfacePatch::Type::Mesh) {
            hit = raycastMeshPatch(patch, o, d, t, puv);
        } else {
            glm::vec3 n;
            hit = raycastSmooth(patch.smooth, o, d, t, n, puv);
        }
        if (hit && t > 0.0f && t < nearest) {
            nearest = t; outFace = i; uv = puv; found = true;
        }
    }
    if (found) tHit = nearest;
    return found;
}

float implicitComplex(const ComplexShapeData& c, const glm::vec3& p) {
    // Convex solids only (capped cylinder/cone, box): the solid is the
    // intersection of the side quadric interior and the outward cap/face
    // half-spaces. Inside ⇔ every term ≤ 0, so return the max term.
    float v = -1e9f;
    for (const auto& patch : c.patches) {
        if (patch.type == SurfacePatch::Type::Smooth) {
            v = std::max(v, implicitSmooth(patch.smooth, p));
        } else if (!patch.polygon.empty()) {
            v = std::max(v, glm::dot(patch.planeNormal, p - patch.polygon[0]));
        }
    }
    return v;
}

// ---------------------------------------------------------------------------
// Tessellation
// ---------------------------------------------------------------------------
TessMesh tessellatePatch(const SurfacePatch& patch, int slices) {
    TessMesh m;
    if (patch.type == SurfacePatch::Type::Mesh) {
        return patch.mesh;
    }
    if (patch.type == SurfacePatch::Type::Smooth) {
        return tessellateSmooth(patch.smooth, slices, slices / 2 + 1);
    }
    if (patch.polygon.size() >= 3) {
        const glm::vec3& n = patch.planeNormal;
        const glm::vec3& c0 = patch.polygon[0];
        for (size_t i = 1; i + 1 < patch.polygon.size(); ++i) {
            TessVertex a, b, d;
            a.pos = c0;                 a.normal = n; a.uv = glm::vec2(0.5f, 0.5f);
            b.pos = patch.polygon[i];   b.normal = n; b.uv = glm::vec2(0.0f, 1.0f);
            d.pos = patch.polygon[i+1]; d.normal = n; d.uv = glm::vec2(1.0f, 1.0f);
            m.tris.push_back(a); m.tris.push_back(b); m.tris.push_back(d);
        }
    }
    return m;
}

TessMesh tessellateComplex(const ComplexShapeData& c, int slices) {
    TessMesh m;
    for (const auto& patch : c.patches) {
        TessMesh pm = tessellatePatch(patch, slices);
        m.tris.insert(m.tris.end(), pm.tris.begin(), pm.tris.end());
    }
    return m;
}

} // namespace geom
