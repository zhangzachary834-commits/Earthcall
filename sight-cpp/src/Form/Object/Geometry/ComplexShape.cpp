#include "ComplexShape.hpp"

#include <algorithm>
#include <cmath>

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

ComplexShapeData roundedBox(float half, float fillet) {
    ComplexShapeData c;
    // 6 flat faces (full squares for now; fillet patches are added in the
    // fillet-rendering stage). Outward normals along ±X/±Y/±Z.
    const glm::vec3 normals[6] = {
        { 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}
    };
    for (const auto& n : normals) {
        SurfacePatch p;
        p.type = SurfacePatch::Type::Planar;
        p.planeNormal = n;
        // Build a square on the face plane.
        glm::vec3 t = (std::fabs(n.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 u = glm::normalize(glm::cross(n, t));
        glm::vec3 v = glm::normalize(glm::cross(n, u));
        glm::vec3 ctr = n * half;
        p.polygon = {
            ctr + (-u - v) * half, ctr + (u - v) * half,
            ctr + (u + v) * half,  ctr + (-u + v) * half
        };
        c.patches.push_back(p);
    }
    // 12 rims are Soft (rounded) edges. We record them as classified edges with
    // the fillet radius; the actual fillet patches are generated later. Pair the
    // faces that share each rim (axis-adjacent faces).
    auto addSoftEdge = [&](int a, int b) {
        ClassifiedEdge e; e.patchA = a; e.patchB = b;
        e.continuity = EdgeContinuity::Soft; e.filletRadius = fillet;
        c.edges.push_back(e);
    };
    // faces: 0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z. Each ± face is adjacent to the 4
    // faces of the other two axes.
    const int faceX[2] = {0, 1}, faceY[2] = {2, 3}, faceZ[2] = {4, 5};
    for (int xi = 0; xi < 2; ++xi) for (int yi = 0; yi < 2; ++yi) addSoftEdge(faceX[xi], faceY[yi]);
    for (int yi = 0; yi < 2; ++yi) for (int zi = 0; zi < 2; ++zi) addSoftEdge(faceY[yi], faceZ[zi]);
    for (int zi = 0; zi < 2; ++zi) for (int xi = 0; xi < 2; ++xi) addSoftEdge(faceZ[zi], faceX[xi]);
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
