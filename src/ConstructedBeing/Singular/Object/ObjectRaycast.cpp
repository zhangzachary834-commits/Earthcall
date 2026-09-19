// Object — raycast / face picking subsystem (split from Object.cpp).
// raycastFace (per-geometry) + triangle-soup ray test helper.

#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include "Singularity/Screen/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Ray vs a triangle-soup tessellation (Möller–Trumbore); nearest hit.
// Slab test. Cheap enough to run before anything else, and it is what turns
// "the pointer is nowhere near this object" from full price into six compares.
static bool rayHitsBox(const glm::vec3& o, const glm::vec3& d,
                       const glm::vec3& lo, const glm::vec3& hi) {
    float tEnter = 0.0f, tExit = 1e30f;
    for (int a = 0; a < 3; ++a) {
        if (std::fabs(d[a]) < 1e-12f) {
            if (o[a] < lo[a] || o[a] > hi[a]) return false;
            continue;
        }
        const float inv = 1.0f / d[a];
        float t0 = (lo[a] - o[a]) * inv, t1 = (hi[a] - o[a]) * inv;
        if (t0 > t1) std::swap(t0, t1);
        tEnter = std::max(tEnter, t0);
        tExit  = std::min(tExit,  t1);
        if (tEnter > tExit) return false;
    }
    return tExit >= 0.0f;
}

// The linear scan the TriGrid replaced on the field and patch paths. Kept
// because it is the reference the grid is asserted against in
// tests/constructed-being/tri_grid_test.cpp: an index is only worth having if
// it returns what the exhaustive answer returns.
bool raycastTessMeshLinear(const geom::TessMesh& m, const glm::vec3& o, const glm::vec3& d, float& tHit) {
    float nearest = 1e9f; bool found = false;
    for (size_t i = 0; i + 2 < m.tris.size(); i += 3) {
        const glm::vec3& a = m.tris[i].pos;
        const glm::vec3& b = m.tris[i + 1].pos;
        const glm::vec3& c = m.tris[i + 2].pos;
        glm::vec3 e1 = b - a, e2 = c - a, pv = glm::cross(d, e2);
        float det = glm::dot(e1, pv);
        if (std::fabs(det) < 1e-8f) continue;
        float inv = 1.0f / det;
        glm::vec3 tv = o - a;
        float u = glm::dot(tv, pv) * inv;
        if (u < 0.0f || u > 1.0f) continue;
        glm::vec3 qv = glm::cross(tv, e1);
        float v = glm::dot(d, qv) * inv;
        if (v < 0.0f || u + v > 1.0f) continue;
        float t = glm::dot(e2, qv) * inv;
        if (t > 1e-4f && t < nearest) { nearest = t; found = true; }
    }
    if (found) tHit = nearest;
    return found;
}

bool Object::raycastFace(const glm::vec3& rayOriginWorld, const glm::vec3& rayDirWorld,
                         float& outT, int& outFaceIndex, glm::vec2& outUV) const {
    // 2D objects live in screen space; a 3D ray never hits them.
    // InteractionChannel handles them with a pixel AABB test instead.
    if (is2D()) return false;

    glm::mat4 inv = glm::inverse(getRaycastTransform());
    glm::vec3 oL = glm::vec3(inv * glm::vec4(rayOriginWorld, 1.0f));
    glm::vec3 localDir = glm::vec3(inv * glm::vec4(rayDirWorld, 0.0f));
    float dirLen = glm::length(localDir);
    if (dirLen < 1e-8f) return false;
    glm::vec3 dL = localDir / dirLen;

    // Topology-based geometry takes precedence over the legacy primitive switch.
    if (_hasField) {
        // Pick against the cached mesh — robust for any field (morph/boolean/
        // implicit), including non-SDF implicit expressions where sphere-tracing
        // would be unreliable.
        //
        // Reject against the field's own extent BEFORE forcing the lazy
        // tessellation. `pickSurface` asks every object in the Zone every frame
        // and has no broadphase of its own, so without this a Person standing in
        // the air over a terrain still pays to mesh and then walk it. The field
        // is only defined over ±_fieldExtent and the mesh cannot leave that box,
        // so missing the box is a miss — no approximation.
        if (!rayHitsBox(oL, dL, -_fieldExtent, _fieldExtent)) return false;
        rebuildFieldMesh();
        if (_fieldMeshGrid.raycast(_fieldMesh, oL, dL, outT)) {
            outFaceIndex = 0; outUV = glm::vec2(0.5f); return true;
        }
        return false;
    }
    if (_hasComplex) {
        return geom::raycastComplex(complexData, oL, dL, outT, outFaceIndex, outUV);
    }
    if (_hasSmooth) {
        glm::vec3 n;
        if (geom::raycastSmooth(smoothData, oL, dL, outT, n, outUV)) { outT /= dirLen; outFaceIndex = 0; return true; }
        return false;
    }
    if (_hasPatch) {
        // Pick against the cached patch tessellation — same approach as fields.
        if (_patchMeshGridDirty) { _patchMeshGrid.build(_patchMesh); _patchMeshGridDirty = false; }
        if (_patchMeshGrid.raycast(_patchMesh, oL, dL, outT)) {
            outFaceIndex = 0; outUV = glm::vec2(0.5f); return true;
        }
        return false;
    }

    auto intersectAABBUnitCube = [&](float& tHit, int& faceIndex, glm::vec2& uv) -> bool {
        float tMin = -1e9f, tMax = 1e9f; int axis = -1; int sign = 0;
        for (int a = 0; a < 3; ++a) {
            float o = oL[a], d = dL[a];
            float t1, t2;
            if (fabs(d) < 1e-6f) {
                if (o < -0.5f || o > 0.5f) return false;
                t1 = -1e9f; t2 = 1e9f;
            } else {
                t1 = (-0.5f - o) / d; t2 = (0.5f - o) / d;
            }
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tMin) { tMin = t1; axis = a; sign = (d > 0 ? -1 : 1); }
            if (t2 < tMax) tMax = t2;
            if (tMin > tMax) return false;
        }
        if (tMin <= 0 || tMin >= 1e8f) return false;
        tHit = tMin;
        faceIndex = axis * 2 + (sign > 0 ? 0 : 1);
        glm::vec3 pL = oL + dL * tMin;
        const float eps = 1e-4f;
        if (fabs(pL.x - 0.5f) < eps) { // +X face
            uv = glm::vec2(pL.y + 0.5f, pL.z + 0.5f);
        } else if (fabs(pL.x + 0.5f) < eps) { // -X face
            uv = glm::vec2(pL.z + 0.5f, pL.y + 0.5f);
        } else if (fabs(pL.y - 0.5f) < eps) { // +Y face
            uv = glm::vec2(pL.z + 0.5f, pL.x + 0.5f);
        } else if (fabs(pL.y + 0.5f) < eps) { // -Y face
            uv = glm::vec2(pL.x + 0.5f, pL.z + 0.5f);
        } else if (fabs(pL.z - 0.5f) < eps) { // +Z face
            uv = glm::vec2(pL.x + 0.5f, pL.y + 0.5f);
        } else { // -Z face
            uv = glm::vec2(pL.y + 0.5f, pL.x + 0.5f);
        }
        uv = glm::clamp(uv, glm::vec2(0.0f), glm::vec2(1.0f));
        return true;
    };

    auto intersectSphere = [&](float& tHit, glm::vec2& uv) -> bool {
        // Sphere centered at origin, radius 0.5, axis along Z (matches drawSpherePrimitive)
        float r = 0.5f;
        float b = glm::dot(oL, dL);
        float c = glm::dot(oL, oL) - r * r;
        float disc = b * b - c;
        if (disc < 0.0f) return false;
        float sqrtDisc = sqrtf(disc);
        float t1 = -b - sqrtDisc;
        float t2 = -b + sqrtDisc;
        float t = (t1 > 1e-6f) ? t1 : ((t2 > 1e-6f) ? t2 : -1.0f);
        if (t <= 0.0f) return false;
        tHit = t;
        glm::vec3 p = oL + dL * t;
        // GLU sphere texture uses longitude around Z-axis and latitude by Z coordinate
        float u = 0.5f + atan2f(p.y, p.x) / (2.0f * (float)M_PI);
        float v = 0.5f - asinf(glm::clamp(p.z / r, -1.0f, 1.0f)) / (float)M_PI;
        uv = glm::vec2(u, v);
        return true;
    };

    auto intersectCylinder = [&](float& tHit, int& faceIndex, glm::vec2& uv) -> bool {
        // Cylinder axis along Z, radius 0.5, centered at origin: z in [-0.5, 0.5]
        // (drawObject applies glTranslatef(0,0,-0.5) before drawing, so local space is [-0.5,0.5])
        const float r = 0.5f;
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);

        // Side: x^2 + y^2 = r^2, z in [-0.5, 0.5]
        float A = dL.x * dL.x + dL.y * dL.y;
        float B = 2.0f * (oL.x * dL.x + oL.y * dL.y);
        float C = oL.x * oL.x + oL.y * oL.y - r * r;
        if (A > 1e-6f) {
            float disc = B * B - 4.0f * A * C;
            if (disc >= 0.0f) {
                float s = sqrtf(disc);
                float tA = (-B - s) / (2.0f * A);
                float tB = (-B + s) / (2.0f * A);
                auto testT = [&](float t) {
                    if (t > 1e-6f) {
                        glm::vec3 p = oL + dL * t;
                        if (p.z >= -0.5f && p.z <= 0.5f) {
                            float u = 0.5f + atan2f(p.y, p.x) / (2.0f * (float)M_PI);
                            float v = glm::clamp(p.z + 0.5f, 0.0f, 1.0f);
                            if (t < bestT) { bestT = t; bestFace = 0; bestUV = glm::vec2(u, v); }
                        }
                    }
                };
                testT(tA); testT(tB);
            }
        }

        // Caps at z = -0.5 (bottom) and z = 0.5 (top), share one texture (face 1)
        if (fabs(dL.z) > 1e-6f) {
            for (int sgn = 0; sgn <= 1; ++sgn) {
                float zPlane = sgn == 0 ? -0.5f : 0.5f;
                float t = (zPlane - oL.z) / dL.z;
                if (t > 1e-6f) {
                    glm::vec3 p = oL + dL * t;
                    float r2 = p.x * p.x + p.y * p.y;
                    if (r2 <= r * r) {
                        float theta = atan2f(p.y, p.x);
                        float rr = sqrtf(r2) / r; // 0..1
                        float u = 0.5f + 0.5f * rr * cosf(theta);
                        float v = 0.5f + 0.5f * rr * sinf(theta);
                        if (t < bestT) { bestT = t; bestFace = 1; bestUV = glm::vec2(u, v); }
                    }
                }
            }
        }

        if (bestFace >= 0) { tHit = bestT; faceIndex = bestFace; uv = bestUV; return true; }
        return false;
    };

    auto intersectCone = [&](float& tHit, int& faceIndex, glm::vec2& uv) -> bool {
        // Cone axis +Z: base (radius 0.5) at z=-0.5, apex at z=+0.5, z in [-0.5, 0.5]
        // Implicit surface: x^2 + y^2 = k^2 * (0.5 - z)^2, k = 0.5
        // (drawObject applies glTranslatef(0,0,-0.5) before gluCylinder(0.5,0,1), so local space is [-0.5,0.5])
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);

        // Side: x^2 + y^2 = (0.5*(0.5-z))^2, z in [-0.5, 0.5]
        // Quadratic coefficients derived from substituting ray into surface equation:
        float k = 0.5f;
        float oz_off = 0.5f - oL.z;   // (0.5 - oz), distance of ray origin from apex in Z
        float A = dL.x * dL.x + dL.y * dL.y - (k * k) * dL.z * dL.z;
        float B = 2.0f * (oL.x * dL.x + oL.y * dL.y) + 2.0f * (k * k) * oz_off * dL.z;
        float C = oL.x * oL.x + oL.y * oL.y - (k * k) * oz_off * oz_off;
        if (fabs(A) > 1e-6f) {
            float disc = B * B - 4.0f * A * C;
            if (disc >= 0.0f) {
                float s = sqrtf(disc);
                float tA = (-B - s) / (2.0f * A);
                float tB = (-B + s) / (2.0f * A);
                auto testT = [&](float t) {
                    if (t > 1e-6f) {
                        glm::vec3 p = oL + dL * t;
                        if (p.z >= -0.5f && p.z <= 0.5f) {
                            float theta = atan2f(p.y, p.x);
                            float u = 0.5f + theta / (2.0f * (float)M_PI);
                            float v = glm::clamp(0.5f - p.z, 0.0f, 1.0f); // 0 at apex (z=+0.5), 1 at base (z=-0.5)
                            if (t < bestT) { bestT = t; bestFace = 0; bestUV = glm::vec2(u, v); }
                        }
                    }
                };
                testT(tA); testT(tB);
            }
        }

        // Base disc at z = -0.5, radius 0.5 (face 1)
        if (fabs(dL.z) > 1e-6f) {
            float t = (-0.5f - oL.z) / dL.z;
            if (t > 1e-6f) {
                glm::vec3 p = oL + dL * t;
                float r2 = p.x * p.x + p.y * p.y;
                if (r2 <= 0.25f) {
                    float theta = atan2f(p.y, p.x);
                    float rr = sqrtf(r2) / 0.5f;
                    float u = 0.5f + 0.5f * rr * cosf(theta);
                    float v = 0.5f + 0.5f * rr * sinf(theta);
                    if (t < bestT) { bestT = t; bestFace = 1; bestUV = glm::vec2(u, v); }
                }
            }
        }

        if (bestFace >= 0) { tHit = bestT; faceIndex = bestFace; uv = bestUV; return true; }
        return false;
    };

    auto pointInPolygon2D = [](const std::vector<glm::vec2>& poly, const glm::vec2& p) -> bool {
        bool c = false;
        size_t n = poly.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            const glm::vec2& pi = poly[i];
            const glm::vec2& pj = poly[j];
            if (((pi.y > p.y) != (pj.y > p.y)) &&
                (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y + 1e-12f) + pi.x))
                c = !c;
        }
        return c;
    };

    auto intersectPolyhedron = [&](float& tHit, int& faceIndex, glm::vec2& uv) -> bool {
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);
        if (polyhedronData.vertices.empty() || polyhedronData.faces.empty()) return false;
        for (size_t fi = 0; fi < polyhedronData.faces.size(); ++fi) {
            const auto& face = polyhedronData.faces[fi];
            if (face.size() < 3) continue;
            glm::vec3 v0 = polyhedronData.vertices[face[0]];
            glm::vec3 v1 = polyhedronData.vertices[face[1]];
            glm::vec3 v2 = polyhedronData.vertices[face[2]];
            glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            float denom = glm::dot(normal, dL);
            if (fabs(denom) < 1e-6f) continue;
            float t = glm::dot(v0 - oL, normal) / denom;
            if (t <= 1e-6f) continue;
            glm::vec3 p = oL + dL * t;

            // Build tangent space
            glm::vec3 tangent = glm::normalize(glm::cross(fabs(normal.y) < 0.99f ? glm::vec3(0,1,0) : glm::vec3(1,0,0), normal));
            glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

            // Project polygon to 2D
            std::vector<glm::vec2> poly2d; poly2d.reserve(face.size());
            float minU = 1e9f, maxU = -1e9f, minV = 1e9f, maxV = -1e9f;
            for (int idx : face) {
                glm::vec3 v = polyhedronData.vertices[idx];
                float u = glm::dot(v - v0, tangent);
                float vv = glm::dot(v - v0, bitangent);
                poly2d.emplace_back(u, vv);
                minU = std::min(minU, u); maxU = std::max(maxU, u);
                minV = std::min(minV, vv); maxV = std::max(maxV, vv);
            }

            glm::vec2 p2(glm::dot(p - v0, tangent), glm::dot(p - v0, bitangent));
            if (!pointInPolygon2D(poly2d, p2)) continue;

            // Normalize to [0,1]
            float du = std::max(1e-6f, maxU - minU);
            float dv = std::max(1e-6f, maxV - minV);
            glm::vec2 uvLocal((p2.x - minU) / du, (p2.y - minV) / dv);
            if (t < bestT) { bestT = t; bestFace = (int)fi; bestUV = glm::clamp(uvLocal, glm::vec2(0.0f), glm::vec2(1.0f)); }
        }
        if (bestFace >= 0) { tHit = bestT; faceIndex = bestFace; uv = bestUV; return true; }
        return false;
    };

    float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);
    bool hit = false;
    switch (_shapeKind) {
        case ShapeKind::Cube: {
            float t; int f; glm::vec2 uv;
            if (intersectAABBUnitCube(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
        case ShapeKind::Sphere: {
            float t; glm::vec2 uv;
            if (intersectSphere(t, uv)) { bestT = t; bestFace = 0; bestUV = uv; hit = true; }
            break;
        }
        case ShapeKind::Cylinder: {
            float t; int f; glm::vec2 uv;
            if (intersectCylinder(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
        case ShapeKind::Cone: {
            float t; int f; glm::vec2 uv;
            if (intersectCone(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
        case ShapeKind::Polyhedron: {
            float t; int f; glm::vec2 uv;
            if (intersectPolyhedron(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
    }

    if (hit) { outT = bestT / dirLen; outFaceIndex = bestFace; outUV = bestUV; return true; }
    return false;
}
