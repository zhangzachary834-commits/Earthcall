// Rung 10B — scene-level secondary-surface BVH tribunal.
//
// This is deliberately a ZERO-PIXEL-AUTHORITY witness.  Rung 10A established
// the semantic reference chain:
//
//   secondary ray -> actual Object -> Object Material -> bounded provenance
//
// The missing prerequisite was an execution road cheaper than asking every
// Object in the scene to raycast for every secondary interaction.  This test
// keeps the exhaustive Object::raycastFace scan as the exact oracle and asks a
// conservative BVH only to DISCOVER candidate Objects.  A BVH leaf never
// fabricates geometry: the candidate still answers through Object::raycastFace.
//
// If parity, boundedness, stale clearing, or local refit fail, the BVH earns no
// authority.  Nothing in production rendering consumes this test artifact.

#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

enum class QueryStatus {
    BudgetExhausted,
    Miss,
    Hit,
};

struct QueryStats {
    uint64_t sceneQueries = 0;
    uint64_t aabbTests = 0;
    uint64_t nodesVisited = 0;
    uint64_t exactObjectRaycasts = 0;
};

struct SecondaryHit {
    QueryStatus status = QueryStatus::Miss;
    Object* object = nullptr;
    std::string objectId;
    std::string materialId;
    float t = 0.0f;
    int face = -1;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    uint32_t bounceIndex = 0;
    uint32_t remainingBounceBudget = 0;
};

struct Bounds {
    glm::vec3 lo{0.0f};
    glm::vec3 hi{0.0f};
};

Bounds objectBounds(const Object& object) {
    Bounds out{object.collisionZone.corners[0],
               object.collisionZone.corners[0]};
    for (int i = 1; i < 8; ++i) {
        out.lo = glm::min(out.lo, object.collisionZone.corners[i]);
        out.hi = glm::max(out.hi, object.collisionZone.corners[i]);
    }
    return out;
}

Bounds unite(const Bounds& a, const Bounds& b) {
    return Bounds{glm::min(a.lo, b.lo), glm::max(a.hi, b.hi)};
}

glm::vec3 centroid(const Bounds& b) {
    return 0.5f * (b.lo + b.hi);
}

bool rayBounds(const Bounds& b,
               const glm::vec3& origin,
               const glm::vec3& direction,
               float& outEnter,
               float& outExit) {
    float enter = 0.0f;
    float exit = std::numeric_limits<float>::infinity();
    for (int axis = 0; axis < 3; ++axis) {
        if (std::fabs(direction[axis]) < 1e-12f) {
            if (origin[axis] < b.lo[axis] || origin[axis] > b.hi[axis])
                return false;
            continue;
        }
        const float inv = 1.0f / direction[axis];
        float t0 = (b.lo[axis] - origin[axis]) * inv;
        float t1 = (b.hi[axis] - origin[axis]) * inv;
        if (t0 > t1) std::swap(t0, t1);
        enter = std::max(enter, t0);
        exit = std::min(exit, t1);
        if (enter > exit) return false;
    }
    if (exit <= 0.0f) return false;
    outEnter = enter;
    outExit = exit;
    return true;
}

// Mirror the Rung-10A / pickSurface normal consequence.  Cubes have exact face
// normals.  Non-cubes keep the existing reference approximation; this tribunal
// deliberately uses cubes so normal parity is exact rather than aspirational.
glm::vec3 referenceNormal(const Object& object,
                          int face,
                          const glm::vec3& point,
                          const glm::vec3& rayDirection) {
    const glm::mat4 xf = object.getRaycastTransform();
    if (object.getShapeKind() == Object::ShapeKind::Cube && face >= 0) {
        const int axis = face / 2;
        const int sign = (face % 2 == 0) ? 1 : -1;
        glm::vec3 local(0.0f);
        local[axis] = static_cast<float>(sign);
        return glm::normalize(glm::vec3(xf * glm::vec4(local, 0.0f)));
    }
    const glm::vec3 centre =
        glm::vec3(xf * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    const glm::vec3 n = point - centre;
    return glm::dot(n, n) > 1e-12f ? glm::normalize(n) : -rayDirection;
}

void fillHit(SecondaryHit& out,
             Object* object,
             float t,
             int face,
             const glm::vec3& origin,
             const glm::vec3& direction,
             uint32_t bounceIndex,
             uint32_t remainingBudget) {
    out.status = QueryStatus::Hit;
    out.object = object;
    out.objectId = object->getIdentifier();
    out.materialId = object->materialId();
    out.t = t;
    out.face = face;
    out.point = origin + direction * t;
    out.normal = referenceNormal(*object, face, out.point, direction);
    out.bounceIndex = bounceIndex;
    out.remainingBounceBudget = remainingBudget;
}

SecondaryHit exhaustiveOracle(const std::vector<Object*>& objects,
                              const glm::vec3& origin,
                              const glm::vec3& direction,
                              uint32_t bounceIndex,
                              uint32_t bounceBudget,
                              QueryStats& stats) {
    SecondaryHit out;
    out.bounceIndex = bounceIndex;
    out.remainingBounceBudget = bounceBudget;

    // Rung-10 boundedness law: zero budget means ZERO scene queries.
    if (bounceBudget == 0) {
        out.status = QueryStatus::BudgetExhausted;
        return out;
    }

    ++stats.sceneQueries;
    float nearest = std::numeric_limits<float>::infinity();
    Object* winner = nullptr;
    int winnerFace = -1;
    for (Object* object : objects) {
        if (!object || object->is2D()) continue;
        ++stats.exactObjectRaycasts;
        float t = 0.0f;
        int face = -1;
        glm::vec2 uv(0.0f);
        if (object->raycastFace(origin, direction, t, face, uv) &&
            t > 0.0f && t < nearest) {
            nearest = t;
            winner = object;
            winnerFace = face;
        }
    }

    if (!winner) {
        out.status = QueryStatus::Miss;
        out.object = nullptr;
        out.objectId.clear();
        out.materialId.clear();
        out.remainingBounceBudget = bounceBudget - 1;
        return out;
    }

    fillHit(out, winner, nearest, winnerFace, origin, direction,
            bounceIndex, bounceBudget - 1);
    return out;
}

class SceneSurfaceBvhProbe {
public:
    struct Node {
        Bounds bounds;
        int left = -1;
        int right = -1;
        int parent = -1;
        Object* leaf = nullptr;

        bool isLeaf() const { return leaf != nullptr; }
    };

    struct RepairStats {
        uint64_t leavesRefit = 0;
        uint64_t ancestorNodesRefit = 0;
    };

    void build(const std::vector<Object*>& source) {
        _nodes.clear();
        _leafByObject.clear();
        _builds += 1;

        std::vector<Object*> objects;
        objects.reserve(source.size());
        for (Object* object : source) {
            if (object && !object->is2D()) objects.push_back(object);
        }
        if (objects.empty()) {
            _root = -1;
            return;
        }
        _root = buildRange(objects, 0, objects.size(), -1);
    }

    SecondaryHit query(const glm::vec3& origin,
                       const glm::vec3& direction,
                       uint32_t bounceIndex,
                       uint32_t bounceBudget,
                       QueryStats& stats) const {
        SecondaryHit out;
        out.bounceIndex = bounceIndex;
        out.remainingBounceBudget = bounceBudget;

        if (bounceBudget == 0) {
            out.status = QueryStatus::BudgetExhausted;
            return out;
        }

        ++stats.sceneQueries;
        if (_root < 0) {
            out.status = QueryStatus::Miss;
            out.remainingBounceBudget = bounceBudget - 1;
            return out;
        }

        struct StackEntry {
            int node = -1;
            float enter = 0.0f;
        };

        std::vector<StackEntry> stack;
        stack.reserve(64);

        float rootEnter = 0.0f;
        float rootExit = 0.0f;
        ++stats.aabbTests;
        if (!rayBounds(_nodes[_root].bounds, origin, direction,
                       rootEnter, rootExit)) {
            out.status = QueryStatus::Miss;
            out.remainingBounceBudget = bounceBudget - 1;
            return out;
        }
        stack.push_back(StackEntry{_root, rootEnter});

        float nearest = std::numeric_limits<float>::infinity();
        Object* winner = nullptr;
        int winnerFace = -1;

        while (!stack.empty()) {
            const StackEntry current = stack.back();
            stack.pop_back();
            if (current.enter > nearest) continue;

            const Node& node = _nodes[current.node];
            ++stats.nodesVisited;

            if (node.isLeaf()) {
                ++stats.exactObjectRaycasts;
                float t = 0.0f;
                int face = -1;
                glm::vec2 uv(0.0f);
                if (node.leaf->raycastFace(origin, direction, t, face, uv) &&
                    t > 0.0f && t < nearest) {
                    nearest = t;
                    winner = node.leaf;
                    winnerFace = face;
                }
                continue;
            }

            float leftEnter = 0.0f, leftExit = 0.0f;
            float rightEnter = 0.0f, rightExit = 0.0f;
            bool hitLeft = false, hitRight = false;

            if (node.left >= 0) {
                ++stats.aabbTests;
                hitLeft = rayBounds(_nodes[node.left].bounds, origin, direction,
                                    leftEnter, leftExit) &&
                          leftEnter <= nearest;
            }
            if (node.right >= 0) {
                ++stats.aabbTests;
                hitRight = rayBounds(_nodes[node.right].bounds, origin, direction,
                                     rightEnter, rightExit) &&
                           rightEnter <= nearest;
            }

            // LIFO stack: push farther first so the nearer child is evaluated
            // first and can tighten 'nearest' for later pruning.
            if (hitLeft && hitRight) {
                if (leftEnter <= rightEnter) {
                    stack.push_back(StackEntry{node.right, rightEnter});
                    stack.push_back(StackEntry{node.left, leftEnter});
                } else {
                    stack.push_back(StackEntry{node.left, leftEnter});
                    stack.push_back(StackEntry{node.right, rightEnter});
                }
            } else if (hitLeft) {
                stack.push_back(StackEntry{node.left, leftEnter});
            } else if (hitRight) {
                stack.push_back(StackEntry{node.right, rightEnter});
            }
        }

        if (!winner) {
            out.status = QueryStatus::Miss;
            out.object = nullptr;
            out.objectId.clear();
            out.materialId.clear();
            out.remainingBounceBudget = bounceBudget - 1;
            return out;
        }

        fillHit(out, winner, nearest, winnerFace, origin, direction,
                bounceIndex, bounceBudget - 1);
        return out;
    }

    RepairStats refit(Object* object) {
        RepairStats stats;
        auto it = _leafByObject.find(object);
        if (it == _leafByObject.end()) return stats;

        int nodeIndex = it->second;
        _nodes[nodeIndex].bounds = objectBounds(*object);
        ++stats.leavesRefit;

        int parent = _nodes[nodeIndex].parent;
        while (parent >= 0) {
            Node& n = _nodes[parent];
            assert(n.left >= 0 && n.right >= 0);
            n.bounds = unite(_nodes[n.left].bounds, _nodes[n.right].bounds);
            ++stats.ancestorNodesRefit;
            parent = n.parent;
        }

        ++_refits;
        return stats;
    }

    size_t nodeCount() const { return _nodes.size(); }
    uint64_t builds() const { return _builds; }
    uint64_t refits() const { return _refits; }

private:
    int buildRange(std::vector<Object*>& objects,
                   size_t begin,
                   size_t end,
                   int parent) {
        assert(begin < end);

        const int index = static_cast<int>(_nodes.size());
        _nodes.push_back(Node{});
        _nodes[index].parent = parent;

        Bounds bounds = objectBounds(*objects[begin]);
        Bounds centroidBounds{centroid(bounds), centroid(bounds)};
        for (size_t i = begin + 1; i < end; ++i) {
            const Bounds b = objectBounds(*objects[i]);
            bounds = unite(bounds, b);
            const glm::vec3 c = centroid(b);
            centroidBounds.lo = glm::min(centroidBounds.lo, c);
            centroidBounds.hi = glm::max(centroidBounds.hi, c);
        }
        _nodes[index].bounds = bounds;

        const size_t count = end - begin;
        if (count == 1) {
            _nodes[index].leaf = objects[begin];
            _leafByObject[objects[begin]] = index;
            return index;
        }

        const glm::vec3 span = centroidBounds.hi - centroidBounds.lo;
        int axis = 0;
        if (span.y > span.x) axis = 1;
        if (span.z > span[axis]) axis = 2;

        const size_t mid = begin + count / 2;
        std::nth_element(
            objects.begin() + static_cast<std::ptrdiff_t>(begin),
            objects.begin() + static_cast<std::ptrdiff_t>(mid),
            objects.begin() + static_cast<std::ptrdiff_t>(end),
            [axis](Object* a, Object* b) {
                return centroid(objectBounds(*a))[axis] <
                       centroid(objectBounds(*b))[axis];
            });

        const int left = buildRange(objects, begin, mid, index);
        const int right = buildRange(objects, mid, end, index);
        _nodes[index].left = left;
        _nodes[index].right = right;
        _nodes[index].bounds = unite(_nodes[left].bounds, _nodes[right].bounds);
        return index;
    }

    std::vector<Node> _nodes;
    std::unordered_map<Object*, int> _leafByObject;
    int _root = -1;
    uint64_t _builds = 0;
    uint64_t _refits = 0;
};

bool nearlyEqual(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

bool nearlyEqual(const glm::vec3& a,
                 const glm::vec3& b,
                 float eps = 1e-4f) {
    return glm::length(a - b) <= eps;
}

void assertSameConsequence(const SecondaryHit& exact,
                           const SecondaryHit& accelerated) {
    assert(exact.status == accelerated.status);
    assert(exact.bounceIndex == accelerated.bounceIndex);
    assert(exact.remainingBounceBudget ==
           accelerated.remainingBounceBudget);

    if (exact.status != QueryStatus::Hit) {
        assert(accelerated.object == nullptr);
        assert(accelerated.objectId.empty());
        assert(accelerated.materialId.empty());
        return;
    }

    assert(exact.object == accelerated.object);
    assert(exact.objectId == accelerated.objectId);
    assert(exact.materialId == accelerated.materialId);
    assert(exact.face == accelerated.face);
    assert(nearlyEqual(exact.t, accelerated.t));
    assert(nearlyEqual(exact.point, accelerated.point));
    assert(nearlyEqual(exact.normal, accelerated.normal));
}

glm::mat4 translated(float x, float y, float z) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}

} // namespace

int main() {
    // A deliberately hostile scene for the old query shape: one true receiver
    // sits on the central ray while hundreds of unrelated Objects occupy broad
    // spatially separated regions.  The exhaustive oracle still asks every
    // Object.  A useful scene road should not.
    std::vector<std::unique_ptr<Object>> owned;
    std::vector<Object*> scene;
    owned.reserve(257);
    scene.reserve(257);

    auto addCube = [&](const std::string& id,
                       const std::string& material,
                       const glm::vec3& position) -> Object* {
        auto object = std::make_unique<Object>(id);
        object->setShapeKind(Object::ShapeKind::Cube);
        object->setMaterialId(material);
        object->setTransform(
            translated(position.x, position.y, position.z));
        Object* ptr = object.get();
        scene.push_back(ptr);
        owned.push_back(std::move(object));
        return ptr;
    };

    Object* receiver =
        addCube("rung10b.receiver", "material.rung10b.ivory",
                glm::vec3(0.0f, 0.0f, 10.0f));

    // 256 distractors, deliberately kept away from the central ray while
    // spanning a large enough volume to make scene-level discovery meaningful.
    for (int i = 0; i < 256; ++i) {
        const int side = (i & 1) ? 1 : -1;
        const float x = side * (8.0f + static_cast<float>(i % 16) * 1.5f);
        const float y =
            -12.0f + static_cast<float>((i / 16) % 16) * 1.6f;
        const float z = 2.0f + static_cast<float>(i % 23) * 1.1f;
        addCube("rung10b.distractor." + std::to_string(i),
                "material.rung10b.distractor",
                glm::vec3(x, y, z));
    }

    SceneSurfaceBvhProbe bvh;
    bvh.build(scene);
    assert(bvh.builds() == 1);
    assert(bvh.nodeCount() == scene.size() * 2 - 1);

    const glm::vec3 origin(0.0f, 0.0f, -20.0f);
    const glm::vec3 centralDir(0.0f, 0.0f, 1.0f);

    // BOUNDEDNESS: budget zero performs no scene query, no AABB work, and no
    // Object raycast on either road.
    QueryStats zeroExactStats;
    QueryStats zeroBvhStats;
    const SecondaryHit zeroExact =
        exhaustiveOracle(scene, origin, centralDir, 0, 0, zeroExactStats);
    const SecondaryHit zeroBvh =
        bvh.query(origin, centralDir, 0, 0, zeroBvhStats);
    assert(zeroExact.status == QueryStatus::BudgetExhausted);
    assert(zeroBvh.status == QueryStatus::BudgetExhausted);
    assert(zeroExactStats.sceneQueries == 0 &&
           zeroExactStats.exactObjectRaycasts == 0);
    assert(zeroBvhStats.sceneQueries == 0 &&
           zeroBvhStats.aabbTests == 0 &&
           zeroBvhStats.nodesVisited == 0 &&
           zeroBvhStats.exactObjectRaycasts == 0);

    // PRIMARY TRIBUNAL: exact consequence parity plus a meaningful candidate
    // reduction.  The index may perform cheap AABB work, but it must avoid
    // invoking expensive exact Object raycasts for unrelated beings.
    QueryStats exactStats;
    QueryStats bvhStats;
    const SecondaryHit exact =
        exhaustiveOracle(scene, origin, centralDir, 0, 1, exactStats);
    const SecondaryHit accelerated =
        bvh.query(origin, centralDir, 0, 1, bvhStats);
    assertSameConsequence(exact, accelerated);
    assert(exact.status == QueryStatus::Hit);
    assert(exact.object == receiver);
    assert(exact.materialId == "material.rung10b.ivory");
    assert(exactStats.exactObjectRaycasts == scene.size());
    assert(bvhStats.exactObjectRaycasts <= 4);
    assert(bvhStats.exactObjectRaycasts * 32 <
           exactStats.exactObjectRaycasts);

    // PARITY SWEEP: target a deterministic sample of distractor centroids.
    // BVH broad-phase may choose any traversal order; the nearest exact Object
    // and its Material consequence must still be identical.
    uint64_t sweepExactRaycasts = 0;
    uint64_t sweepBvhRaycasts = 0;
    uint64_t sweepAabbTests = 0;
    for (size_t i = 1; i < scene.size(); i += 17) {
        const glm::vec3 target = scene[i]->getPosition();
        const glm::vec3 dir = glm::normalize(target - origin);
        QueryStats oracleStats;
        QueryStats indexStats;
        const SecondaryHit oracle =
            exhaustiveOracle(scene, origin, dir, 1, 2, oracleStats);
        const SecondaryHit indexed =
            bvh.query(origin, dir, 1, 2, indexStats);
        assertSameConsequence(oracle, indexed);
        sweepExactRaycasts += oracleStats.exactObjectRaycasts;
        sweepBvhRaycasts += indexStats.exactObjectRaycasts;
        sweepAabbTests += indexStats.aabbTests;
    }
    assert(sweepBvhRaycasts < sweepExactRaycasts);

    // MATERIAL / GEOMETRY SEPARATION: changing only the receiving Material
    // changes the consequence without touching or refitting the geometry index.
    const uint64_t refitsBeforeMaterial = bvh.refits();
    receiver->setMaterialId("material.rung10b.gold");
    QueryStats materialExactStats;
    QueryStats materialBvhStats;
    const SecondaryHit materialExact =
        exhaustiveOracle(scene, origin, centralDir, 2, 2,
                         materialExactStats);
    const SecondaryHit materialBvh =
        bvh.query(origin, centralDir, 2, 2, materialBvhStats);
    assertSameConsequence(materialExact, materialBvh);
    assert(materialBvh.materialId == "material.rung10b.gold");
    assert(bvh.refits() == refitsBeforeMaterial &&
           "Material edit incorrectly dirtied geometry discovery");

    // LOCAL GEOMETRY REPAIR: moving one Object updates that leaf and its
    // ancestor chain, not the whole scene artifact.
    const float oldDistance = materialBvh.t;
    receiver->setTransform(translated(0.0f, 0.0f, 14.0f));
    const auto repair = bvh.refit(receiver);
    assert(repair.leavesRefit == 1);
    assert(repair.ancestorNodesRefit > 0);
    assert(repair.leavesRefit + repair.ancestorNodesRefit <
           bvh.nodeCount() / 4);
    assert(bvh.builds() == 1 &&
           "local transform edit rebuilt the whole scene BVH");

    QueryStats movedExactStats;
    QueryStats movedBvhStats;
    const SecondaryHit movedExact =
        exhaustiveOracle(scene, origin, centralDir, 3, 2, movedExactStats);
    const SecondaryHit movedBvh =
        bvh.query(origin, centralDir, 3, 2, movedBvhStats);
    assertSameConsequence(movedExact, movedBvh);
    assert(movedBvh.object == receiver);
    assert(movedBvh.t > oldDistance + 3.5f);

    // MISS / STALE CLEARING: a miss after prior hits carries no old Object or
    // Material identity.
    const glm::vec3 missOrigin(-100.0f, -100.0f, -100.0f);
    const glm::vec3 missDir(-1.0f, 0.0f, 0.0f);
    QueryStats missExactStats;
    QueryStats missBvhStats;
    const SecondaryHit missExact =
        exhaustiveOracle(scene, missOrigin, missDir, 4, 1, missExactStats);
    const SecondaryHit missBvh =
        bvh.query(missOrigin, missDir, 4, 1, missBvhStats);
    assertSameConsequence(missExact, missBvh);
    assert(missBvh.status == QueryStatus::Miss);
    assert(missBvh.object == nullptr);
    assert(missBvh.objectId.empty());
    assert(missBvh.materialId.empty());

    std::printf(
        "RUNG10B_SECONDARY_BVH "
        "parity=1 budget_zero_queries=0 objects=%zu nodes=%zu "
        "linear_exact_raycasts=%llu bvh_exact_raycasts=%llu "
        "bvh_aabb_tests=%llu bvh_nodes_visited=%llu "
        "candidate_reduction_x=%.2f "
        "sweep_linear_exact_raycasts=%llu sweep_bvh_exact_raycasts=%llu "
        "sweep_aabb_tests=%llu "
        "material_edit_refits=0 local_refit_leaf=%llu "
        "local_refit_ancestors=%llu builds=%llu refits=%llu "
        "pixel_authority=0\n",
        scene.size(), bvh.nodeCount(),
        static_cast<unsigned long long>(exactStats.exactObjectRaycasts),
        static_cast<unsigned long long>(bvhStats.exactObjectRaycasts),
        static_cast<unsigned long long>(bvhStats.aabbTests),
        static_cast<unsigned long long>(bvhStats.nodesVisited),
        bvhStats.exactObjectRaycasts > 0
            ? static_cast<double>(exactStats.exactObjectRaycasts) /
                  static_cast<double>(bvhStats.exactObjectRaycasts)
            : 0.0,
        static_cast<unsigned long long>(sweepExactRaycasts),
        static_cast<unsigned long long>(sweepBvhRaycasts),
        static_cast<unsigned long long>(sweepAabbTests),
        static_cast<unsigned long long>(repair.leavesRefit),
        static_cast<unsigned long long>(repair.ancestorNodesRefit),
        static_cast<unsigned long long>(bvh.builds()),
        static_cast<unsigned long long>(bvh.refits()));

    return 0;
}
