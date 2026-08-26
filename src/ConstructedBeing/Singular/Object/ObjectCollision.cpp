// Object — collision subsystem (split from Object.cpp).
// Collision zone/AABB, support cloud + support points, point penetration, isPointInside/isTouching.

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

namespace {
glm::vec3 transformNormalToWorld(const glm::mat4& transform, const glm::vec3& localNormal) {
    glm::mat3 linear(transform);
    glm::vec3 worldNormal = glm::transpose(glm::inverse(linear)) * localNormal;
    float len = glm::length(worldNormal);
    if (len <= 1e-6f) return glm::vec3(0.0f, 1.0f, 0.0f);
    return worldNormal / len;
}

glm::vec3 closestPointOnTriangle(const glm::vec3& p,
                                 const glm::vec3& a,
                                 const glm::vec3& b,
                                 const glm::vec3& c) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;

    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return a + v * ab;
    }

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return a + w * ac;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        glm::vec3 bc = c - b;
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + w * bc;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

bool rayIntersectsTriangle(const glm::vec3& origin,
                           const glm::vec3& dir,
                           const glm::vec3& a,
                           const glm::vec3& b,
                           const glm::vec3& c,
                           float& outT) {
    const float EPS = 1e-6f;
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 pvec = glm::cross(dir, ac);
    float det = glm::dot(ab, pvec);
    if (std::abs(det) <= EPS) return false;

    float invDet = 1.0f / det;
    glm::vec3 tvec = origin - a;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < EPS || u > 1.0f - EPS) return false;

    glm::vec3 qvec = glm::cross(tvec, ab);
    float v = glm::dot(dir, qvec) * invDet;
    if (v < EPS || u + v > 1.0f - EPS) return false;

    float t = glm::dot(ac, qvec) * invDet;
    if (t <= EPS) return false;
    outT = t;
    return true;
}
} // namespace

void Object::updateCollisionZone(const glm::mat4& transform) const {
    glm::vec3 minCorner = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxCorner = glm::vec3(-std::numeric_limits<float>::max());
    bool hasBounds = false;

    if (_shapeKind == ShapeKind::Polyhedron && !polyhedronData.vertices.empty()) {
        for (const auto& vertex : polyhedronData.vertices) {
            glm::vec4 world = transform * glm::vec4(vertex, 1.0f);
            glm::vec3 worldVertex = glm::vec3(world);
            minCorner = glm::min(minCorner, worldVertex);
            maxCorner = glm::max(maxCorner, worldVertex);
        }
        hasBounds = true;
    } else if (!_supportCloud.empty()) {
        for (const auto& vertex : _supportCloud) {
            glm::vec4 world = transform * glm::vec4(vertex, 1.0f);
            glm::vec3 worldVertex = glm::vec3(world);
            minCorner = glm::min(minCorner, worldVertex);
            maxCorner = glm::max(maxCorner, worldVertex);
        }
        hasBounds = true;
    }
    
    if (!hasBounds) {
        // Fallback for completely empty shapes (legacy behavior)
        glm::vec3 localCorners[8] = {
            {-0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f}
        };
        for (int i = 0; i < 8; ++i) {
            glm::vec4 world = transform * glm::vec4(localCorners[i], 1.0f);
            collisionZone.corners[i] = glm::vec3(world);
        }
        return;
    }

    // Create bounding box corners from min/max
    collisionZone.corners[0] = glm::vec3(minCorner.x, minCorner.y, minCorner.z);
    collisionZone.corners[1] = glm::vec3(maxCorner.x, minCorner.y, minCorner.z);
    collisionZone.corners[2] = glm::vec3(maxCorner.x, maxCorner.y, minCorner.z);
    collisionZone.corners[3] = glm::vec3(minCorner.x, maxCorner.y, minCorner.z);
    collisionZone.corners[4] = glm::vec3(minCorner.x, minCorner.y, maxCorner.z);
    collisionZone.corners[5] = glm::vec3(maxCorner.x, minCorner.y, maxCorner.z);
    collisionZone.corners[6] = glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z);
    collisionZone.corners[7] = glm::vec3(minCorner.x, maxCorner.y, maxCorner.z);
}

// Rebuilds everything derived from the shape: the cached render tessellations and
// the GJK support cloud. Called from every geometry mutation point, so the draw
namespace {
    bool compareSmooth(const geom::SmoothSurfaceData& a, const geom::SmoothSurfaceData& b) {
        if (a.closed != b.closed) return a.closed < b.closed;
        if (a.orientable != b.orientable) return a.orientable < b.orientable;
        if (a.hasBoundary != b.hasBoundary) return a.hasBoundary < b.hasBoundary;
        if (a.isVolume != b.isVolume) return a.isVolume < b.isVolume;
        if (a.model != b.model) return a.model < b.model;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (a.Q[i][j] != b.Q[i][j]) return a.Q[i][j] < b.Q[i][j];
            }
        }
        if (a.form != b.form) return a.form < b.form;
        if (a.pkind != b.pkind) return a.pkind < b.pkind;
        if (a.axes.x != b.axes.x) return a.axes.x < b.axes.x;
        if (a.axes.y != b.axes.y) return a.axes.y < b.axes.y;
        if (a.axes.z != b.axes.z) return a.axes.z < b.axes.z;
        if (a.zTrim.x != b.zTrim.x) return a.zTrim.x < b.zTrim.x;
        if (a.zTrim.y != b.zTrim.y) return a.zTrim.y < b.zTrim.y;
        if (a.params.size() != b.params.size()) return a.params.size() < b.params.size();
        for (size_t i = 0; i < a.params.size(); ++i) {
            if (a.params[i] != b.params[i]) return a.params[i] < b.params[i];
        }
        return false;
    }
    
    struct SmoothDataLess {
        bool operator()(const geom::SmoothSurfaceData& a, const geom::SmoothSurfaceData& b) const {
            return compareSmooth(a, b);
        }
    };
    
    std::map<geom::SmoothSurfaceData, std::shared_ptr<geom::TessMesh>, SmoothDataLess> s_smoothCache;
}

// Decimate to a capped, well-spread subset. The support cloud is argmax-scanned
// (O(cloud)) up to ~14x per object per collision pair; keeping the full marching-tet
// path never has to tessellate. Collision keeps its own coarser tessellation —
// the cloud is decimated to `maxPts` anyway, and matching render resolution here
// would change which points GJK sees for no benefit.
void Object::rebuildGeometryCaches() {
    _supportCloud.clear();
    _smoothMesh.reset();
    _complexMeshes.clear();
    _patchMesh = geom::TessMesh{};

    geom::TessMesh m; // collision source — decimated into _supportCloud below
    if (_hasField) {
        _fieldMesh = geom::tessellateSdf(fieldData, _fieldExtent);
        m = _fieldMesh; // render and collision share it: marching tets are too costly to repeat
    }
    else if (_hasComplex) {
        geom::SdfNode asField;
        if (geom::sdfFromComplex(complexData, asField)) {
            // Display mesh of the SDF primitive (capped cylinder/cone), not
            // a 24-slice UV side plus N-gon caps. Collision still uses the
            // coarser cloud below.
            const float ext = std::max(_shapeParams.r, _shapeParams.halfH) + 0.25f;
            _complexMeshes.push_back(geom::tessellateSdf(asField, std::max(ext, 0.6f), 32));
            m = _complexMeshes.front();
        } else {
            _complexMeshes.reserve(complexData.patches.size());
            for (const auto& patch : complexData.patches)
                _complexMeshes.push_back(geom::tessellatePatch(patch));
            m = geom::tessellateComplex(complexData, 16);
        }
    }
    else if (_hasSmooth) {
        auto it = s_smoothCache.find(smoothData);
        if (it != s_smoothCache.end()) {
            _smoothMesh = it->second;
        } else {
            _smoothMesh = std::make_shared<geom::TessMesh>(geom::tessellateSmooth(smoothData));
            s_smoothCache[smoothData] = _smoothMesh;
        }
        m = geom::tessellateSmooth(smoothData, 16, 10);
    }
    else if (_hasPatch) {
        _patchMesh = geom::tessellateBezier(patchData);
        m = _patchMesh; // render and collision share the one tessellation
    }
    else return;
    // Decimate to a capped, well-spread subset. The support cloud is argmax-scanned
    // (O(cloud)) up to ~14x per object per collision pair; keeping the full marching-tet
    // vertex set (tens of thousands of points) makes the narrowphase O(n*cloud) and was
    // measured at ~9 ms per field-field pair. A strided subset gives the same support
    // directions at a fraction of the cost (smooth surfaces use the analytic support fn).
    const size_t maxPts = 256;
    const size_t step = std::max<size_t>(1, m.tris.size() / maxPts);
    _supportCloud.reserve(m.tris.size() / step + 1);
    for (size_t i = 0; i < m.tris.size(); i += step) _supportCloud.push_back(m.tris[i].pos);
}

glm::vec3 Object::getLocalSupportPoint(const glm::vec3& localDirection) const {
    glm::vec3 dir = localDirection;
    if (glm::dot(dir, dir) <= 1e-12f) dir = glm::vec3(1.0f, 0.0f, 0.0f);

    // Topology model: analytic ellipsoid/sphere support, else cached cloud.
    if (_hasSmooth) {
        bool ok = false;
        glm::vec3 sp = geom::supportPoint(smoothData, dir, ok);
        if (ok) return sp;
    }
    if ((_hasSmooth || _hasComplex || _hasField || _hasPatch) && !_supportCloud.empty()) {
        float best = -std::numeric_limits<float>::max();
        glm::vec3 bestV = _supportCloud[0];
        for (const auto& v : _supportCloud) {
            float d = glm::dot(v, dir);
            if (d > best) { best = d; bestV = v; }
        }
        return bestV;
    }

    switch (_shapeKind) {
        case ShapeKind::Cube:
            return glm::vec3(dir.x >= 0.0f ? 0.5f : -0.5f,
                             dir.y >= 0.0f ? 0.5f : -0.5f,
                             dir.z >= 0.0f ? 0.5f : -0.5f);
        case ShapeKind::Sphere: {
            glm::vec3 n = glm::normalize(dir);
            return n * 0.5f;
        }
        case ShapeKind::Cylinder: {
            glm::vec2 radial(dir.x, dir.y);
            float radialLen = glm::length(radial);
            glm::vec3 support(0.0f, 0.0f, dir.z >= 0.0f ? 0.5f : -0.5f);
            if (radialLen > 1e-6f) {
                glm::vec2 radialDir = radial / radialLen;
                support.x = radialDir.x * 0.5f;
                support.y = radialDir.y * 0.5f;
            }
            return support;
        }
        case ShapeKind::Cone: {
            glm::vec2 radial(dir.x, dir.y);
            float radialLen = glm::length(radial);
            float apexScore = 0.5f * dir.z;
            float baseScore = 0.5f * radialLen - 0.5f * dir.z;
            if (apexScore >= baseScore) {
                return glm::vec3(0.0f, 0.0f, 0.5f);
            }

            glm::vec3 support(0.0f, 0.0f, -0.5f);
            if (radialLen > 1e-6f) {
                glm::vec2 radialDir = radial / radialLen;
                support.x = radialDir.x * 0.5f;
                support.y = radialDir.y * 0.5f;
            }
            return support;
        }
        case ShapeKind::Polyhedron: {
            if (polyhedronData.vertices.empty()) {
                return glm::vec3(dir.x >= 0.0f ? 0.5f : -0.5f,
                                 dir.y >= 0.0f ? 0.5f : -0.5f,
                                 dir.z >= 0.0f ? 0.5f : -0.5f);
            }

            float bestDot = -std::numeric_limits<float>::max();
            glm::vec3 bestVertex = polyhedronData.vertices.front();
            for (const auto& vertex : polyhedronData.vertices) {
                float candidate = glm::dot(vertex, dir);
                if (candidate > bestDot) {
                    bestDot = candidate;
                    bestVertex = vertex;
                }
            }
            return bestVertex;
        }
    }

    return glm::vec3(0.0f);
}

glm::vec3 Object::getSupportPointWorld(const glm::vec3& worldDirection) const {
    glm::mat4 transform = getRaycastTransform();
    glm::mat3 linear(transform);
    glm::vec3 localDirection = glm::transpose(linear) * worldDirection;
    glm::vec3 localSupport = getLocalSupportPoint(localDirection);
    return glm::vec3(transform * glm::vec4(localSupport, 1.0f));
}

bool Object::isCollisionShapeConvex() const {
    if (_hasField)   return false; // SDF expressions (morph/boolean) may be non-convex
    if (_hasSmooth)  return geom::isConvex(smoothData);
    if (_hasComplex) return true; // capped cylinder/cone and rounded box are convex
    if (_shapeKind != ShapeKind::Polyhedron) return true;
    return polyhedronData.getIsConvex();
}

bool Object::computeLocalPointPenetration(const glm::vec3& localPoint,
                                          glm::vec3& outSurfacePoint,
                                          glm::vec3& outLocalNormal) const {
    // Topology-based geometry: inside when the implicit value is negative; push
    // the point out along the surface gradient.
    if (_hasSmooth || _hasComplex || _hasField) {
        auto f = [&](const glm::vec3& p) {
            if (_hasField)   return geom::evalSdf(fieldData, p);
            if (_hasComplex) return geom::implicitComplex(complexData, p);
            return geom::implicitSmooth(smoothData, p);
        };
        float val = f(localPoint);
        if (val >= 0.0f) return false; // outside
        // Numeric gradient of the implicit field = outward normal direction.
        const float e = 1e-3f;
        glm::vec3 g(f(localPoint + glm::vec3(e,0,0)) - f(localPoint - glm::vec3(e,0,0)),
                    f(localPoint + glm::vec3(0,e,0)) - f(localPoint - glm::vec3(0,e,0)),
                    f(localPoint + glm::vec3(0,0,e)) - f(localPoint - glm::vec3(0,0,e)));
        float glen = glm::length(g);
        outLocalNormal = (glen > 1e-8f) ? g / glen : glm::vec3(0.0f, 1.0f, 0.0f);
        outSurfacePoint = localPoint; // approximate: project handled by caller's correction
        return true;
    }

    switch (_shapeKind) {
        case ShapeKind::Cube: {
            if (std::abs(localPoint.x) > 0.5f || std::abs(localPoint.y) > 0.5f || std::abs(localPoint.z) > 0.5f) {
                return false;
            }

            float dx = 0.5f - std::abs(localPoint.x);
            float dy = 0.5f - std::abs(localPoint.y);
            float dz = 0.5f - std::abs(localPoint.z);

            outSurfacePoint = localPoint;
            if (dx <= dy && dx <= dz) {
                outLocalNormal = glm::vec3(localPoint.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
                outSurfacePoint.x = 0.5f * outLocalNormal.x;
            } else if (dy <= dx && dy <= dz) {
                outLocalNormal = glm::vec3(0.0f, localPoint.y >= 0.0f ? 1.0f : -1.0f, 0.0f);
                outSurfacePoint.y = 0.5f * outLocalNormal.y;
            } else {
                outLocalNormal = glm::vec3(0.0f, 0.0f, localPoint.z >= 0.0f ? 1.0f : -1.0f);
                outSurfacePoint.z = 0.5f * outLocalNormal.z;
            }
            return true;
        }
        case ShapeKind::Sphere: {
            float len = glm::length(localPoint);
            if (len > 0.5f) return false;

            if (len > 1e-6f) {
                outLocalNormal = localPoint / len;
                outSurfacePoint = outLocalNormal * 0.5f;
            } else {
                outLocalNormal = glm::vec3(1.0f, 0.0f, 0.0f);
                outSurfacePoint = outLocalNormal * 0.5f;
            }
            return true;
        }
        case ShapeKind::Cylinder: {
            glm::vec2 radial(localPoint.x, localPoint.y);
            float radialLen = glm::length(radial);
            if (radialLen > 0.5f || std::abs(localPoint.z) > 0.5f) return false;

            float sideDepth = 0.5f - radialLen;
            float capDepth = 0.5f - std::abs(localPoint.z);
            if (sideDepth <= capDepth) {
                glm::vec2 radialDir = radialLen > 1e-6f ? radial / radialLen : glm::vec2(1.0f, 0.0f);
                outLocalNormal = glm::vec3(radialDir.x, radialDir.y, 0.0f);
                outSurfacePoint = glm::vec3(radialDir.x * 0.5f, radialDir.y * 0.5f, localPoint.z);
            } else {
                outLocalNormal = glm::vec3(0.0f, 0.0f, localPoint.z >= 0.0f ? 1.0f : -1.0f);
                outSurfacePoint = glm::vec3(localPoint.x, localPoint.y, 0.5f * outLocalNormal.z);
            }
            return true;
        }
        case ShapeKind::Cone: {
            float h = localPoint.z + 0.5f;
            if (h < 0.0f || h > 1.0f) return false;

            glm::vec2 radial(localPoint.x, localPoint.y);
            float radialLen = glm::length(radial);
            float maxRadius = 0.5f * (1.0f - h);
            if (radialLen > maxRadius) return false;

            glm::vec2 q(radialLen, h);
            glm::vec2 baseEdge(0.5f, 0.0f);
            glm::vec2 apex(0.0f, 1.0f);
            glm::vec2 side = apex - baseEdge;
            float sideT = glm::dot(q - baseEdge, side) / glm::dot(side, side);
            sideT = std::clamp(sideT, 0.0f, 1.0f);
            glm::vec2 projected = baseEdge + side * sideT;

            glm::vec3 sideSurface(0.0f, 0.0f, projected.y - 0.5f);
            if (projected.x > 1e-6f && radialLen > 1e-6f) {
                glm::vec2 radialDir = radial / radialLen;
                sideSurface.x = radialDir.x * projected.x;
                sideSurface.y = radialDir.y * projected.x;
            }

            glm::vec3 baseSurface(localPoint.x, localPoint.y, -0.5f);
            float baseDist2 = glm::dot(baseSurface - localPoint, baseSurface - localPoint);
            float sideDist2 = glm::dot(sideSurface - localPoint, sideSurface - localPoint);
            if (baseDist2 <= sideDist2) {
                outSurfacePoint = baseSurface;
                outLocalNormal = glm::vec3(0.0f, 0.0f, -1.0f);
            } else {
                outSurfacePoint = sideSurface;
                glm::vec3 delta = localPoint - sideSurface;
                if (glm::dot(delta, delta) > 1e-12f) {
                    outLocalNormal = glm::normalize(delta);
                } else {
                    outLocalNormal = glm::normalize(glm::vec3(localPoint.x, localPoint.y, 0.5f));
                }
            }
            return true;
        }
        case ShapeKind::Polyhedron: {
            if (polyhedronData.vertices.empty() || polyhedronData.faces.empty()) return false;

            glm::vec3 closestPoint(0.0f);
            glm::vec3 closestNormal(0.0f, 1.0f, 0.0f);
            float closestDist2 = std::numeric_limits<float>::max();
            int intersections = 0;
            const glm::vec3 rayDir = glm::normalize(glm::vec3(1.0f, 0.371f, 0.529f));

            bool convex = polyhedronData.getIsConvex();
            bool insideConvex = convex;
            float minPlaneDepth = std::numeric_limits<float>::max();
            glm::vec3 bestPlaneNormal(0.0f, 1.0f, 0.0f);
            glm::vec3 bestPlaneSurface(0.0f);

            for (size_t faceIndex = 0; faceIndex < polyhedronData.faces.size(); ++faceIndex) {
                const auto& face = polyhedronData.faces[faceIndex];
                if (face.size() < 3) continue;

                glm::vec3 faceNormal = (faceIndex < polyhedronData.faceNormals.size())
                    ? polyhedronData.faceNormals[faceIndex]
                    : PolyhedronData::computeNewellNormal(polyhedronData.vertices, face);
                glm::vec3 v0 = polyhedronData.vertices[face[0]];

                if (convex) {
                    float planeDistance = glm::dot(faceNormal, localPoint - v0);
                    if (planeDistance > 1e-5f) {
                        insideConvex = false;
                    } else if (-planeDistance < minPlaneDepth) {
                        minPlaneDepth = -planeDistance;
                        bestPlaneNormal = faceNormal;
                        bestPlaneSurface = localPoint - planeDistance * faceNormal;
                    }
                }

                for (size_t i = 1; i + 1 < face.size(); ++i) {
                    glm::vec3 a = polyhedronData.vertices[face[0]];
                    glm::vec3 b = polyhedronData.vertices[face[i]];
                    glm::vec3 c = polyhedronData.vertices[face[i + 1]];

                    glm::vec3 candidate = closestPointOnTriangle(localPoint, a, b, c);
                    glm::vec3 delta = candidate - localPoint;
                    float dist2 = glm::dot(delta, delta);
                    if (dist2 < closestDist2) {
                        closestDist2 = dist2;
                        closestPoint = candidate;
                        closestNormal = faceNormal;
                    }

                    float hitT = 0.0f;
                    if (rayIntersectsTriangle(localPoint, rayDir, a, b, c, hitT)) {
                        ++intersections;
                    }
                }
            }

            bool inside = convex ? insideConvex : ((intersections % 2) == 1);
            if (!inside) return false;

            if (convex && minPlaneDepth < std::numeric_limits<float>::max()) {
                outSurfacePoint = bestPlaneSurface;
                outLocalNormal = bestPlaneNormal;
                return true;
            }

            outSurfacePoint = closestPoint;
            glm::vec3 delta = localPoint - closestPoint;
            if (glm::dot(delta, delta) > 1e-12f) {
                outLocalNormal = glm::normalize(delta);
            } else {
                outLocalNormal = closestNormal;
            }
            return true;
        }
    }

    return false;
}

bool Object::computePointPenetration(const glm::vec3& point, glm::vec3& outCorrection) const {
    glm::vec3 minCorner = collisionZone.corners[0];
    glm::vec3 maxCorner = collisionZone.corners[0];
    for (int i = 1; i < 8; ++i) {
        minCorner = glm::min(minCorner, collisionZone.corners[i]);
        maxCorner = glm::max(maxCorner, collisionZone.corners[i]);
    }
    if (point.x < minCorner.x || point.x > maxCorner.x ||
        point.y < minCorner.y || point.y > maxCorner.y ||
        point.z < minCorner.z || point.z > maxCorner.z) {
        return false;
    }

    glm::mat4 collisionTransform = getRaycastTransform();
    glm::mat4 inv = glm::inverse(collisionTransform);
    glm::vec3 localPoint = glm::vec3(inv * glm::vec4(point, 1.0f));

    glm::vec3 localSurface(0.0f);
    glm::vec3 localNormal(0.0f, 1.0f, 0.0f);
    if (!computeLocalPointPenetration(localPoint, localSurface, localNormal)) {
        return false;
    }

    glm::vec3 worldSurface = glm::vec3(collisionTransform * glm::vec4(localSurface, 1.0f));
    glm::vec3 worldNormal = transformNormalToWorld(collisionTransform, localNormal);
    outCorrection = (worldSurface - point) + worldNormal * 0.001f;
    if (glm::dot(outCorrection, outCorrection) <= 1e-12f) {
        outCorrection = worldNormal * 0.001f;
    }
    return true;
}

bool Object::isPointInside(const glm::vec3& point) const {
    glm::vec3 correction(0.0f);
    return computePointPenetration(point, correction);
}

bool Object::isTouching(const Object& other) const {
    if (!collisionZone.isTouching(other.collisionZone)) {
        return false; // AABB broad-phase rejection
    }

    constexpr float EPS = 1e-5f;

    if (_shapeKind != ShapeKind::Polyhedron ||
        other._shapeKind != ShapeKind::Polyhedron ||
        polyhedronData.vertices.empty() || polyhedronData.faces.empty() ||
        other.polyhedronData.vertices.empty() || other.polyhedronData.faces.empty()) {
        return false;
    }

    auto toWorld = [](const glm::mat4& m, const std::vector<glm::vec3>& local) {
        std::vector<glm::vec3> world;
        world.reserve(local.size());
        for (const auto& v : local) {
            world.push_back(glm::vec3(m * glm::vec4(v, 1.0f)));
        }
        return world;
    };

    auto project = [](const std::vector<glm::vec3>& verts, const glm::vec3& axis,
                      float& outMin, float& outMax) {
        outMin = std::numeric_limits<float>::max();
        outMax = -std::numeric_limits<float>::max();
        for (const auto& v : verts) {
            float d = glm::dot(v, axis);
            outMin = std::min(outMin, d);
            outMax = std::max(outMax, d);
        }
    };

    auto isSeparating = [&](const std::vector<glm::vec3>& worldA, const std::vector<glm::vec3>& worldB, const glm::vec3& axis) {
        if (glm::dot(axis, axis) < EPS * EPS) return false;
        float minA, maxA, minB, maxB;
        project(worldA, axis, minA, maxA);
        project(worldB, axis, minB, maxB);
        return (maxA < minB - EPS) || (maxB < minA - EPS);
    };

    auto checkConvexPair = [&](const PolyhedronData& polyA, const glm::mat4& transformA,
                               const PolyhedronData& polyB, const glm::mat4& transformB) {
        std::vector<glm::vec3> worldA = toWorld(transformA, polyA.vertices);
        std::vector<glm::vec3> worldB = toWorld(transformB, polyB.vertices);

        for (const auto& face : polyA.faces) {
            if (face.size() < 3) continue;
            if (isSeparating(worldA, worldB, PolyhedronData::computeNewellNormal(worldA, face))) return false;
        }
        for (const auto& face : polyB.faces) {
            if (face.size() < 3) continue;
            if (isSeparating(worldA, worldB, PolyhedronData::computeNewellNormal(worldB, face))) return false;
        }

        auto collectEdges = [](const std::vector<glm::vec3>& verts,
                               const std::vector<std::vector<int>>& faces) {
            std::vector<glm::vec3> dirs;
            for (const auto& face : faces) {
                size_t n = face.size();
                for (size_t i = 0; i < n; ++i) {
                    dirs.push_back(verts[face[(i + 1) % n]] - verts[face[i]]);
                }
            }
            return dirs;
        };
        std::vector<glm::vec3> edgesA = collectEdges(worldA, polyA.faces);
        std::vector<glm::vec3> edgesB = collectEdges(worldB, polyB.faces);
        for (const auto& eA : edgesA) {
            for (const auto& eB : edgesB) {
                glm::vec3 axis = glm::cross(eA, eB);
                if (glm::length(axis) > EPS) {
                    if (isSeparating(worldA, worldB, glm::normalize(axis))) return false;
                }
            }
        }
        return true;
    };

    // If both have V-HACD components, test all pairs
    const std::vector<PolyhedronData>& partsA = polyhedronData.convexComponents.empty() ? 
        std::vector<PolyhedronData>{polyhedronData} : polyhedronData.convexComponents;
    const std::vector<PolyhedronData>& partsB = other.polyhedronData.convexComponents.empty() ? 
        std::vector<PolyhedronData>{other.polyhedronData} : other.polyhedronData.convexComponents;

    for (const auto& partA : partsA) {
        for (const auto& partB : partsB) {
            if (checkConvexPair(partA, transform, partB, other.transform)) {
                return true; // Any overlapping pair means the objects touch
            }
        }
    }

    return false;
}
