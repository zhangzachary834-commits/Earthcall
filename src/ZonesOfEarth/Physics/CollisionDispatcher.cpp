#include "CollisionDispatcher.hpp"

#include "ConstructedBeing/Object/Object/PolyhedronData.hpp"
#include "ConstructedBeing/Object/Geometry/ComplexShape.hpp"
#include "ConstructedBeing/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Object/Geometry/SmoothSurface.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace Physics {
namespace {

constexpr float EPS = 1e-5f;

struct SupportPoint {
    glm::vec3 minkowski{0.0f};
    glm::vec3 pointA{0.0f};
    glm::vec3 pointB{0.0f};
};

glm::vec3 objectCenter(const Object& object) {
    return glm::vec3(object.getTransform()[3]);
}

glm::vec3 safeNormalize(const glm::vec3& v, const glm::vec3& fallback = glm::vec3(0.0f, 1.0f, 0.0f)) {
    float len2 = glm::dot(v, v);
    if (len2 <= 1e-12f) return fallback;
    return v / std::sqrt(len2);
}

glm::vec3 orientFromBToA(const Object& a, const Object& b, const glm::vec3& normal) {
    glm::vec3 n = safeNormalize(normal);
    glm::vec3 centerDelta = objectCenter(a) - objectCenter(b);
    if (glm::dot(centerDelta, n) < 0.0f) n = -n;
    return n;
}

glm::vec3 transformNormalToWorld(const glm::mat4& transform, const glm::vec3& localNormal) {
    glm::mat3 linear(transform);
    glm::vec3 worldNormal = glm::transpose(glm::inverse(linear)) * localNormal;
    return safeNormalize(worldNormal);
}

SupportPoint support(const Object& a, const Object& b, const glm::vec3& dir) {
    glm::vec3 pa = a.getSupportPointWorld(dir);
    glm::vec3 pb = b.getSupportPointWorld(-dir);
    return SupportPoint{pa - pb, pa, pb};
}

bool sameDirection(const glm::vec3& a, const glm::vec3& b) {
    return glm::dot(a, b) > 0.0f;
}

glm::vec3 perpendicularTo(const glm::vec3& v) {
    glm::vec3 axis = std::abs(v.x) < 0.8f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 p = glm::cross(v, axis);
    if (glm::dot(p, p) <= 1e-12f) p = glm::cross(v, glm::vec3(0.0f, 0.0f, 1.0f));
    return safeNormalize(p, glm::vec3(1.0f, 0.0f, 0.0f));
}

bool handleSimplex(std::vector<SupportPoint>& simplex, glm::vec3& dir) {
    const SupportPoint& aPoint = simplex.back();
    glm::vec3 a = aPoint.minkowski;
    glm::vec3 ao = -a;

    if (simplex.size() == 2) {
        glm::vec3 b = simplex[0].minkowski;
        glm::vec3 ab = b - a;
        if (sameDirection(ab, ao)) {
            dir = glm::cross(glm::cross(ab, ao), ab);
            if (glm::dot(dir, dir) <= 1e-12f) dir = perpendicularTo(ab);
        } else {
            simplex = {aPoint};
            dir = ao;
        }
        return false;
    }

    if (simplex.size() == 3) {
        const SupportPoint& bPoint = simplex[1];
        const SupportPoint& cPoint = simplex[0];
        glm::vec3 b = bPoint.minkowski;
        glm::vec3 c = cPoint.minkowski;
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 abc = glm::cross(ab, ac);

        glm::vec3 acPerp = glm::cross(abc, ac);
        if (sameDirection(acPerp, ao)) {
            if (sameDirection(ac, ao)) {
                simplex = {cPoint, aPoint};
                dir = glm::cross(glm::cross(ac, ao), ac);
            } else {
                simplex = {bPoint, aPoint};
                glm::vec3 abDir = glm::cross(glm::cross(ab, ao), ab);
                dir = glm::dot(abDir, abDir) > 1e-12f ? abDir : perpendicularTo(ab);
            }
            return false;
        }

        glm::vec3 abPerp = glm::cross(ab, abc);
        if (sameDirection(abPerp, ao)) {
            simplex = {bPoint, aPoint};
            dir = glm::cross(glm::cross(ab, ao), ab);
            if (glm::dot(dir, dir) <= 1e-12f) dir = perpendicularTo(ab);
            return false;
        }

        if (sameDirection(abc, ao)) dir = abc;
        else {
            simplex = {bPoint, cPoint, aPoint};
            dir = -abc;
        }
        return false;
    }

    if (simplex.size() == 4) {
        const SupportPoint& bPoint = simplex[2];
        const SupportPoint& cPoint = simplex[1];
        const SupportPoint& dPoint = simplex[0];
        glm::vec3 b = bPoint.minkowski;
        glm::vec3 c = cPoint.minkowski;
        glm::vec3 d = dPoint.minkowski;

        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 ad = d - a;
        glm::vec3 abc = glm::cross(ab, ac);
        glm::vec3 acd = glm::cross(ac, ad);
        glm::vec3 adb = glm::cross(ad, ab);

        if (sameDirection(abc, ao)) {
            simplex = {cPoint, bPoint, aPoint};
            dir = abc;
            return false;
        }
        if (sameDirection(acd, ao)) {
            simplex = {dPoint, cPoint, aPoint};
            dir = acd;
            return false;
        }
        if (sameDirection(adb, ao)) {
            simplex = {bPoint, dPoint, aPoint};
            dir = adb;
            return false;
        }
        return true;
    }

    return false;
}

bool gjkIntersect(const Object& a, const Object& b, std::vector<SupportPoint>& simplex) {
    simplex.clear();
    glm::vec3 dir = objectCenter(a) - objectCenter(b);
    if (glm::dot(dir, dir) <= 1e-12f) dir = glm::vec3(1.0f, 0.0f, 0.0f);

    simplex.push_back(support(a, b, dir));
    dir = -simplex.back().minkowski;
    if (glm::dot(dir, dir) <= 1e-12f) dir = glm::vec3(1.0f, 0.0f, 0.0f);

    for (int iteration = 0; iteration < 32; ++iteration) {
        SupportPoint next = support(a, b, dir);
        if (glm::dot(next.minkowski, dir) <= 1e-6f) return false;
        simplex.push_back(next);
        if (handleSimplex(simplex, dir)) return true;
    }
    return false;
}

struct EpaFace {
    int a = 0;
    int b = 0;
    int c = 0;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
};

EpaFace makeFace(const std::vector<SupportPoint>& polytope, int ia, int ib, int ic) {
    EpaFace face;
    face.a = ia;
    face.b = ib;
    face.c = ic;

    glm::vec3 a = polytope[ia].minkowski;
    glm::vec3 b = polytope[ib].minkowski;
    glm::vec3 c = polytope[ic].minkowski;
    glm::vec3 normal = glm::cross(b - a, c - a);
    if (glm::dot(normal, normal) <= 1e-12f) {
        face.distance = std::numeric_limits<float>::max();
        return face;
    }

    normal = glm::normalize(normal);
    float distance = glm::dot(normal, a);
    if (distance < 0.0f) {
        std::swap(face.b, face.c);
        face.normal = -normal;
        face.distance = -distance;
    } else {
        face.normal = normal;
        face.distance = distance;
    }
    return face;
}

void addBorderEdge(std::vector<std::pair<int, int>>& edges, int a, int b) {
    for (auto it = edges.begin(); it != edges.end(); ++it) {
        if (it->first == b && it->second == a) {
            edges.erase(it);
            return;
        }
    }
    edges.emplace_back(a, b);
}

bool epaPenetration(const Object& a,
                    const Object& b,
                    const std::vector<SupportPoint>& simplex,
                    glm::vec3& outNormal,
                    float& outDepth) {
    if (simplex.size() < 4) return false;

    std::vector<SupportPoint> polytope = simplex;
    std::vector<EpaFace> faces;
    faces.push_back(makeFace(polytope, 0, 1, 2));
    faces.push_back(makeFace(polytope, 0, 3, 1));
    faces.push_back(makeFace(polytope, 0, 2, 3));
    faces.push_back(makeFace(polytope, 1, 3, 2));

    const float tolerance = 1e-4f;
    for (int iteration = 0; iteration < 64; ++iteration) {
        auto closestFaceIt = std::min_element(faces.begin(), faces.end(), [](const EpaFace& lhs, const EpaFace& rhs) {
            return lhs.distance < rhs.distance;
        });
        if (closestFaceIt == faces.end() || closestFaceIt->distance == std::numeric_limits<float>::max()) {
            return false;
        }

        EpaFace closestFace = *closestFaceIt;
        SupportPoint next = support(a, b, closestFace.normal);
        float supportDistance = glm::dot(next.minkowski, closestFace.normal);
        if (supportDistance - closestFace.distance <= tolerance) {
            outNormal = closestFace.normal;
            outDepth = supportDistance;
            return true;
        }

        int newIndex = static_cast<int>(polytope.size());
        polytope.push_back(next);

        std::vector<std::pair<int, int>> borderEdges;
        for (auto it = faces.begin(); it != faces.end();) {
            glm::vec3 facePoint = polytope[it->a].minkowski;
            if (glm::dot(it->normal, next.minkowski - facePoint) > tolerance) {
                addBorderEdge(borderEdges, it->a, it->b);
                addBorderEdge(borderEdges, it->b, it->c);
                addBorderEdge(borderEdges, it->c, it->a);
                it = faces.erase(it);
            } else {
                ++it;
            }
        }

        for (const auto& edge : borderEdges) faces.push_back(makeFace(polytope, edge.first, edge.second, newIndex));
    }

    return false;
}

const PolyhedronData& cubePolyhedron() {
    static const PolyhedronData cube = []() {
        PolyhedronData data;
        data.vertices = {
            {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}
        };
        data.faces = {
            {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
            {3, 7, 6, 2}, {1, 2, 6, 5}, {0, 4, 7, 3}
        };
        data.recomputeAll();
        return data;
    }();
    return cube;
}

const PolyhedronData& polyhedronBodyFor(const Object& object) {
    const PolyhedronData& data = object.getPolyhedronData();
    if (object.getGeometryType() == Object::GeometryType::Polyhedron &&
        !data.vertices.empty() &&
        !data.faces.empty()) {
        return data;
    }
    return cubePolyhedron();
}

std::vector<const PolyhedronData*> componentsFor(const PolyhedronData& data) {
    std::vector<const PolyhedronData*> parts;
    const auto& components = data.getConvexComponents();
    if (!data.getIsConvex() && !components.empty()) {
        parts.reserve(components.size());
        for (const auto& component : components) parts.push_back(&component);
    } else {
        parts.push_back(&data);
    }
    return parts;
}

std::vector<glm::vec3> toWorld(const glm::mat4& transform, const PolyhedronData& data) {
    std::vector<glm::vec3> world;
    world.reserve(data.vertices.size());
    for (const auto& v : data.vertices) world.push_back(glm::vec3(transform * glm::vec4(v, 1.0f)));
    return world;
}

void project(const std::vector<glm::vec3>& verts, const glm::vec3& axis, float& outMin, float& outMax) {
    outMin = std::numeric_limits<float>::max();
    outMax = -std::numeric_limits<float>::max();
    for (const auto& v : verts) {
        float d = glm::dot(v, axis);
        outMin = std::min(outMin, d);
        outMax = std::max(outMax, d);
    }
}

std::vector<glm::vec3> collectEdges(const std::vector<glm::vec3>& verts,
                                    const std::vector<std::vector<int>>& faces) {
    std::vector<glm::vec3> dirs;
    for (const auto& face : faces) {
        size_t n = face.size();
        for (size_t i = 0; i < n; ++i) dirs.push_back(verts[face[(i + 1) % n]] - verts[face[i]]);
    }
    return dirs;
}

glm::vec3 supportFromVerts(const std::vector<glm::vec3>& verts, const glm::vec3& dir) {
    if (verts.empty()) return glm::vec3(0.0f);
    float best = -std::numeric_limits<float>::max();
    glm::vec3 bestV = verts.front();
    for (const auto& v : verts) {
        float d = glm::dot(v, dir);
        if (d > best) {
            best = d;
            bestV = v;
        }
    }
    return bestV;
}

std::optional<CollisionResult> satContact(const Object& objectA,
                                          const PolyhedronData& a,
                                          const Object& objectB,
                                          const PolyhedronData& b) {
    if (a.vertices.empty() || a.faces.empty() || b.vertices.empty() || b.faces.empty()) return std::nullopt;

    const std::vector<glm::vec3> worldA = toWorld(objectA.getRaycastTransform(), a);
    const std::vector<glm::vec3> worldB = toWorld(objectB.getRaycastTransform(), b);
    float bestOverlap = std::numeric_limits<float>::max();
    glm::vec3 bestAxis(0.0f, 1.0f, 0.0f);

    auto testAxis = [&](const glm::vec3& rawAxis) {
        if (glm::dot(rawAxis, rawAxis) < EPS * EPS) return true;
        glm::vec3 axis = glm::normalize(rawAxis);
        float minA, maxA, minB, maxB;
        project(worldA, axis, minA, maxA);
        project(worldB, axis, minB, maxB);
        float overlap = std::min(maxA, maxB) - std::max(minA, minB);
        if (overlap < -EPS) return false;
        if (overlap < bestOverlap) {
            bestOverlap = overlap;
            bestAxis = axis;
        }
        return true;
    };

    for (const auto& face : a.faces) {
        if (face.size() >= 3 && !testAxis(PolyhedronData::computeNewellNormal(worldA, face))) return std::nullopt;
    }
    for (const auto& face : b.faces) {
        if (face.size() >= 3 && !testAxis(PolyhedronData::computeNewellNormal(worldB, face))) return std::nullopt;
    }

    const std::vector<glm::vec3> edgesA = collectEdges(worldA, a.faces);
    const std::vector<glm::vec3> edgesB = collectEdges(worldB, b.faces);
    for (const auto& eA : edgesA) {
        for (const auto& eB : edgesB) {
            if (!testAxis(glm::cross(eA, eB))) return std::nullopt;
        }
    }

    if (bestOverlap == std::numeric_limits<float>::max()) return std::nullopt;

    CollisionResult result;
    result.hit = true;
    result.normal = orientFromBToA(objectA, objectB, bestAxis);
    result.depth = std::max(0.0f, bestOverlap);
    glm::vec3 pointA = supportFromVerts(worldA, -result.normal);
    glm::vec3 pointB = supportFromVerts(worldB, result.normal);
    result.point = (pointA + pointB) * 0.5f;
    result.method = CollisionMethod::PolyhedronSAT;
    return result;
}

CollisionResult polyhedronSatCollision(const Object& a, const Object& b) {
    const PolyhedronData& bodyA = polyhedronBodyFor(a);
    const PolyhedronData& bodyB = polyhedronBodyFor(b);
    std::vector<const PolyhedronData*> partsA = componentsFor(bodyA);
    std::vector<const PolyhedronData*> partsB = componentsFor(bodyB);

    CollisionResult best;
    for (const PolyhedronData* partA : partsA) {
        for (const PolyhedronData* partB : partsB) {
            std::optional<CollisionResult> candidate = satContact(a, *partA, b, *partB);
            if (candidate && (!best.hit || candidate->depth > best.depth)) best = *candidate;
        }
    }
    return best;
}

std::optional<std::pair<float, glm::vec3>> signedValueAndNormal(const Object& object, const glm::vec3& worldPoint) {
    glm::mat4 transform = object.getRaycastTransform();
    glm::mat4 inv = glm::inverse(transform);
    glm::vec3 localPoint = glm::vec3(inv * glm::vec4(worldPoint, 1.0f));
    auto spatial = object.getSpatialKind();

    auto finiteDifference = [&](auto&& f) {
        const float e = 1e-3f;
        glm::vec3 g(f(localPoint + glm::vec3(e,0,0)) - f(localPoint - glm::vec3(e,0,0)),
                    f(localPoint + glm::vec3(0,e,0)) - f(localPoint - glm::vec3(0,e,0)),
                    f(localPoint + glm::vec3(0,0,e)) - f(localPoint - glm::vec3(0,0,e)));
        return transformNormalToWorld(transform, g);
    };

    if (spatial == Object::SpatialKind::Field) {
        auto f = [&](const glm::vec3& p) { return geom::evalSdf(object.getFieldData(), p); };
        return std::make_pair(f(localPoint), finiteDifference(f));
    }
    if (spatial == Object::SpatialKind::ComplexShape) {
        auto f = [&](const glm::vec3& p) { return geom::implicitComplex(object.getComplexData(), p); };
        return std::make_pair(f(localPoint), finiteDifference(f));
    }
    if (spatial == Object::SpatialKind::SmoothSurface) {
        auto f = [&](const glm::vec3& p) { return geom::implicitSmooth(object.getSmoothData(), p); };
        return std::make_pair(f(localPoint), finiteDifference(f));
    }
    if (spatial == Object::SpatialKind::Polyhedron) {
        glm::vec3 correction(0.0f);
        if (object.computePointPenetration(worldPoint, correction)) {
            float depth = glm::length(correction);
            return std::make_pair(-depth, safeNormalize(correction));
        }
    }

    return std::nullopt;
}

std::vector<glm::vec3> sampleWorldPoints(const Object& object) {
    std::vector<glm::vec3> points;
    glm::mat4 transform = object.getRaycastTransform();

    auto addLocal = [&](const glm::vec3& p) {
        points.push_back(glm::vec3(transform * glm::vec4(p, 1.0f)));
    };

    if (object.getSpatialKind() == Object::SpatialKind::Polyhedron) {
        const PolyhedronData& body = polyhedronBodyFor(object);
        size_t step = std::max<size_t>(1, body.vertices.size() / 96);
        for (size_t i = 0; i < body.vertices.size(); i += step) addLocal(body.vertices[i]);
    } else {
        const auto& cloud = object.getSupportCloud();
        size_t step = std::max<size_t>(1, cloud.size() / 96);
        for (size_t i = 0; i < cloud.size(); i += step) addLocal(cloud[i]);
    }

    points.push_back(objectCenter(object));
    const std::array<glm::vec3, 14> dirs = {{
        { 1, 0, 0}, {-1, 0, 0}, {0,  1, 0}, {0, -1, 0}, {0, 0,  1}, {0, 0, -1},
        { 1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {0,  1, 1}, {0, 1, -1}, {1, 0,  1},
        {-1, 0, 1}, { 1, 1, 1}
    }};
    for (const auto& dir : dirs) points.push_back(object.getSupportPointWorld(safeNormalize(dir)));
    return points;
}

// Cheap signed value only (no gradient), using a precomputed inverse transform.
// Returns false if the object kind has no implicit field / the point is outside a
// polyhedron. Used by the narrowphase scan pass where most probe points are NOT
// penetrating and the (6-eval) finite-difference gradient would be thrown away.
bool signedValueOnly(const Object& object, const glm::mat4& inv, const glm::vec3& worldPoint, float& out) {
    glm::vec3 localPoint = glm::vec3(inv * glm::vec4(worldPoint, 1.0f));
    switch (object.getSpatialKind()) {
        case Object::SpatialKind::Field:
            out = geom::evalSdf(object.getFieldData(), localPoint); return true;
        case Object::SpatialKind::ComplexShape:
            out = geom::implicitComplex(object.getComplexData(), localPoint); return true;
        case Object::SpatialKind::SmoothSurface:
            out = geom::implicitSmooth(object.getSmoothData(), localPoint); return true;
        case Object::SpatialKind::Polyhedron: {
            glm::vec3 correction(0.0f);
            if (object.computePointPenetration(worldPoint, correction)) { out = -glm::length(correction); return true; }
            return false;
        }
        default: return false;
    }
}

CollisionResult sdfProbeCollision(const Object& a, const Object& b) {
    // Two-phase to avoid computing the finite-difference gradient (6 SDF evals)
    // for every probe point: a cheap VALUE-only scan finds the single deepest-
    // penetrating point, then the gradient/normal is computed once for that winner.
    struct Best { bool hit=false; float depth=0.0f; glm::vec3 point{0.0f};
                  const Object* container=nullptr; bool pointBelongsToA=false; };
    Best best;

    auto scan = [&](const Object& probeOwner, const Object& container, bool pointBelongsToA) {
        const glm::mat4 inv = glm::inverse(container.getRaycastTransform()); // once per direction
        for (const auto& wp : sampleWorldPoints(probeOwner)) {
            float v;
            if (!signedValueOnly(container, inv, wp, v)) continue;
            if (v >= 0.0f) continue;                 // outside: no penetration, skip
            const float depth = -v;
            if (!best.hit || depth > best.depth) {
                best = { true, depth, wp, &container, pointBelongsToA };
            }
        }
    };
    scan(a, b, true);
    scan(b, a, false);

    CollisionResult result;
    if (!best.hit) return result;
    result.hit = true;
    result.depth = best.depth;
    result.point = best.point;
    result.method = CollisionMethod::SdfProbe;

    // Gradient computed exactly once, for the winning point only.
    glm::vec3 n(0.0f, 1.0f, 0.0f);
    if (auto value = signedValueAndNormal(*best.container, best.point)) n = value->second;
    result.normal = best.pointBelongsToA ? n : -n;
    result.normal = orientFromBToA(a, b, result.normal);
    return result;
}

void refineImplicitNormal(const Object& a, const Object& b, CollisionResult& result) {
    auto spatialA = a.getSpatialKind();
    auto spatialB = b.getSpatialKind();
    auto hasImplicit = [](Object::SpatialKind kind) {
        return kind == Object::SpatialKind::SmoothSurface ||
               kind == Object::SpatialKind::ComplexShape ||
               kind == Object::SpatialKind::Field;
    };

    if (hasImplicit(spatialB)) {
        if (auto value = signedValueAndNormal(b, result.point)) {
            result.normal = orientFromBToA(a, b, value->second);
        }
    } else if (hasImplicit(spatialA)) {
        if (auto value = signedValueAndNormal(a, result.point)) {
            result.normal = orientFromBToA(a, b, -value->second);
        }
    }
}

CollisionResult gjkEpaCollision(const Object& a, const Object& b) {
    std::vector<SupportPoint> simplex;
    glm::vec3 normal(0.0f);
    float depth = 0.0f;
    if (!gjkIntersect(a, b, simplex) || !epaPenetration(a, b, simplex, normal, depth) || depth <= 0.0f) {
        return {};
    }

    CollisionResult result;
    result.hit = true;
    result.normal = orientFromBToA(a, b, normal);
    result.depth = depth;
    glm::vec3 pointA = a.getSupportPointWorld(-result.normal);
    glm::vec3 pointB = b.getSupportPointWorld(result.normal);
    result.point = (pointA + pointB) * 0.5f;
    result.method = CollisionMethod::GjkEpa;
    refineImplicitNormal(a, b, result);
    return result;
}

} // namespace

CollisionResult dispatchCollision(const Object& a, const Object& b) {
    auto kindA = a.getSpatialKind();
    auto kindB = b.getSpatialKind();

    if (kindA == Object::SpatialKind::Patch || kindB == Object::SpatialKind::Patch) {
        CollisionResult result;
        result.method = CollisionMethod::PatchNonSolid;
        return result;
    }

    if (kindA == Object::SpatialKind::Polyhedron && kindB == Object::SpatialKind::Polyhedron) {
        return polyhedronSatCollision(a, b);
    }

    if (kindA == Object::SpatialKind::Field || kindB == Object::SpatialKind::Field) {
        return sdfProbeCollision(a, b);
    }

    if (a.isCollisionShapeConvex() && b.isCollisionShapeConvex()) {
        return gjkEpaCollision(a, b);
    }

    return sdfProbeCollision(a, b);
}

} // namespace Physics
