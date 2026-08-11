#include "AngleTools.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AngleTools {

// ---- Helpers --------------------------------------------------------------

struct EdgeKey {
    int a, b;
    EdgeKey(int i, int j) : a(std::min(i,j)), b(std::max(i,j)) {}
    bool operator==(const EdgeKey& o) const { return a == o.a && b == o.b; }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& k) const {
        return std::hash<long long>()(static_cast<long long>(k.a) << 32 | k.b);
    }
};

// Build a map from each undirected edge to the indices of faces that use it
static std::unordered_map<EdgeKey, std::vector<int>, EdgeKeyHash>
buildEdgeToFaceMap(const std::vector<std::vector<int>>& faces) {
    std::unordered_map<EdgeKey, std::vector<int>, EdgeKeyHash> map;
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        const auto& face = faces[fi];
        for (size_t i = 0; i < face.size(); ++i) {
            EdgeKey ek(face[i], face[(i + 1) % face.size()]);
            map[ek].push_back(fi);
        }
    }
    return map;
}


// ---- Computation ----------------------------------------------------------

float computeVertexAngle(const glm::vec3& vertex,
                         const glm::vec3& prevVertex,
                         const glm::vec3& nextVertex) {
    glm::vec3 e1 = prevVertex - vertex;
    glm::vec3 e2 = nextVertex - vertex;
    float len1 = glm::length(e1);
    float len2 = glm::length(e2);
    if (len1 < 1e-8f || len2 < 1e-8f) return 0.0f;
    float cosA = glm::clamp(glm::dot(e1 / len1, e2 / len2), -1.0f, 1.0f);
    return std::acos(cosA);
}

float computeDihedralAngle(const glm::vec3& normal1,
                           const glm::vec3& normal2,
                           const glm::vec3& edgeDirection) {
    // The dihedral angle is the angle between the normals, measured in the
    // plane perpendicular to the edge.
    // We use atan2 to get a signed angle in [0, 2*pi).
    float cosA = glm::clamp(glm::dot(normal1, normal2), -1.0f, 1.0f);
    glm::vec3 crossN = glm::cross(normal1, normal2);
    float sinA = glm::dot(crossN, glm::normalize(edgeDirection));
    float angle = std::atan2(sinA, cosA);
    // Convert to the interior dihedral angle (the angle inside the solid)
    return static_cast<float>(M_PI) - angle;
}

std::vector<DihedralAngle> computeDihedralAngles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    const std::vector<glm::vec3>& faceNormals)
{
    std::vector<DihedralAngle> result;
    auto edgeFaces = buildEdgeToFaceMap(faces);

    for (const auto& kv : edgeFaces) {
        const auto& faceList = kv.second;
        if (faceList.size() != 2) continue; // Only interior edges

        int f1 = faceList[0];
        int f2 = faceList[1];
        if (f1 >= static_cast<int>(faceNormals.size()) ||
            f2 >= static_cast<int>(faceNormals.size())) continue;

        glm::vec3 edgeDir = vertices[kv.first.b] - vertices[kv.first.a];

        DihedralAngle da;
        da.face1 = f1;
        da.face2 = f2;
        da.edgeVertex1 = kv.first.a;
        da.edgeVertex2 = kv.first.b;
        da.angleRadians = computeDihedralAngle(faceNormals[f1], faceNormals[f2], edgeDir);
        result.push_back(da);
    }
    return result;
}

std::vector<VertexAngle> computeAllVertexAngles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces)
{
    std::vector<VertexAngle> result;
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        const auto& face = faces[fi];
        if (face.size() < 3) continue;
        for (size_t i = 0; i < face.size(); ++i) {
            int prev = face[(i + face.size() - 1) % face.size()];
            int curr = face[i];
            int next = face[(i + 1) % face.size()];

            VertexAngle va;
            va.faceIndex = fi;
            va.vertexIndex = curr;
            va.prevVertex = prev;
            va.nextVertex = next;
            va.angleRadians = computeVertexAngle(vertices[curr], vertices[prev], vertices[next]);
            result.push_back(va);
        }
    }
    return result;
}

std::vector<SolidAngle> computeSolidAngles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces)
{
    std::vector<SolidAngle> result;
    result.reserve(vertices.size());

    for (int vi = 0; vi < static_cast<int>(vertices.size()); ++vi) {
        float angleSum = 0.0f;
        int faceCount = 0;

        for (const auto& face : faces) {
            for (size_t i = 0; i < face.size(); ++i) {
                if (face[i] == vi) {
                    int prev = face[(i + face.size() - 1) % face.size()];
                    int next = face[(i + 1) % face.size()];
                    angleSum += computeVertexAngle(vertices[vi], vertices[prev], vertices[next]);
                    faceCount++;
                    break;
                }
            }
        }

        SolidAngle sa;
        sa.vertexIndex = vi;
        sa.angleDeficit = 2.0f * static_cast<float>(M_PI) - angleSum;
        // Approximate solid angle via angle deficit (exact for convex vertices)
        sa.steradians = sa.angleDeficit;
        result.push_back(sa);
    }
    return result;
}

std::vector<EdgeInfo> computeAllEdgeInfo(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    const std::vector<glm::vec3>& faceNormals)
{
    std::vector<EdgeInfo> result;
    auto edgeFaces = buildEdgeToFaceMap(faces);

    for (const auto& kv : edgeFaces) {
        EdgeInfo ei;
        ei.v1 = kv.first.a;
        ei.v2 = kv.first.b;
        ei.length = glm::length(vertices[ei.v2] - vertices[ei.v1]);

        const auto& faceList = kv.second;
        if (faceList.size() == 2) {
            int f1 = faceList[0];
            int f2 = faceList[1];
            if (f1 < static_cast<int>(faceNormals.size()) &&
                f2 < static_cast<int>(faceNormals.size()))
            {
                glm::vec3 edgeDir = vertices[ei.v2] - vertices[ei.v1];
                DihedralAngle da;
                da.face1 = f1;
                da.face2 = f2;
                da.edgeVertex1 = ei.v1;
                da.edgeVertex2 = ei.v2;
                da.angleRadians = computeDihedralAngle(faceNormals[f1], faceNormals[f2], edgeDir);
                ei.dihedralAngles.push_back(da);
            }
        }
        result.push_back(ei);
    }
    return result;
}

float vertexAngleSum(const std::vector<glm::vec3>& vertices,
                     const std::vector<std::vector<int>>& faces,
                     int vertexIndex)
{
    float sum = 0.0f;
    for (const auto& face : faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            if (face[i] == vertexIndex) {
                int prev = face[(i + face.size() - 1) % face.size()];
                int next = face[(i + 1) % face.size()];
                sum += computeVertexAngle(vertices[vertexIndex], vertices[prev], vertices[next]);
                break;
            }
        }
    }
    return sum;
}


// ---- Manipulation ---------------------------------------------------------

void rotateFaceAroundEdge(
    std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    int faceIndex,
    int edgeV1, int edgeV2,
    float angleDeltaRadians)
{
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faces.size())) return;
    const auto& face = faces[faceIndex];

    // Collect vertex indices that are NOT on the shared edge
    std::unordered_set<int> edgeVerts;
    edgeVerts.insert(edgeV1);
    edgeVerts.insert(edgeV2);

    // Rotation axis: direction along the shared edge
    glm::vec3 edgeDir = glm::normalize(vertices[edgeV2] - vertices[edgeV1]);
    glm::vec3 pivot = vertices[edgeV1];

    for (int vi : face) {
        if (edgeVerts.count(vi)) continue;
        // Rotate this vertex around the edge
        glm::vec3 rel = vertices[vi] - pivot;
        // Rodrigues' rotation formula
        float cosA = std::cos(angleDeltaRadians);
        float sinA = std::sin(angleDeltaRadians);
        glm::vec3 rotated = rel * cosA
                           + glm::cross(edgeDir, rel) * sinA
                           + edgeDir * glm::dot(edgeDir, rel) * (1.0f - cosA);
        vertices[vi] = pivot + rotated;
    }
}

void adjustVertexAngle(
    std::vector<glm::vec3>& vertices,
    const std::vector<int>& face,
    int vertexPositionInFace,
    float targetAngleRadians)
{
    if (face.size() < 3) return;
    int n = static_cast<int>(face.size());
    int vi = face[vertexPositionInFace];
    int prevI = face[(vertexPositionInFace + n - 1) % n];
    int nextI = face[(vertexPositionInFace + 1) % n];

    float currentAngle = computeVertexAngle(vertices[vi], vertices[prevI], vertices[nextI]);
    float delta = targetAngleRadians - currentAngle;
    if (std::fabs(delta) < 1e-6f) return;

    // Rotate the vertex around the midpoint of the two edge directions
    // to open/close the angle
    glm::vec3 e1 = glm::normalize(vertices[prevI] - vertices[vi]);
    glm::vec3 e2 = glm::normalize(vertices[nextI] - vertices[vi]);
    glm::vec3 bisector = glm::normalize(e1 + e2);
    glm::vec3 faceNormal = glm::normalize(glm::cross(e1, e2));

    // Move the vertex along the bisector to achieve the target angle
    float dist1 = glm::length(vertices[prevI] - vertices[vi]);
    float dist2 = glm::length(vertices[nextI] - vertices[vi]);
    float avgDist = (dist1 + dist2) * 0.5f;

    // Compute new vertex position: keep the same distance from the midpoint
    // of the two neighbors, but adjust the angle
    glm::vec3 midNeighbor = (vertices[prevI] + vertices[nextI]) * 0.5f;
    float halfAngle = targetAngleRadians * 0.5f;

    // New position: move along negative bisector direction
    glm::vec3 newPos = midNeighbor - bisector * (avgDist * std::cos(halfAngle));
    vertices[vi] = newPos;
}

void adjustEdgeLength(
    std::vector<glm::vec3>& vertices,
    int v1, int v2,
    float targetLength)
{
    glm::vec3 dir = vertices[v2] - vertices[v1];
    float currentLength = glm::length(dir);
    if (currentLength < 1e-8f) return;
    dir /= currentLength;

    // Move v2 so the edge reaches targetLength, keeping v1 fixed
    vertices[v2] = vertices[v1] + dir * targetLength;
}


// ---- Validation -----------------------------------------------------------

bool canFormConvexVertex(const std::vector<float>& angles) {
    float sum = 0.0f;
    for (float a : angles) sum += a;
    return sum < 2.0f * static_cast<float>(M_PI);
}

int eulerCharacteristic(int vertexCount, int edgeCount, int faceCount) {
    return vertexCount - edgeCount + faceCount;
}

float totalAngleDeficit(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces)
{
    float total = 0.0f;
    for (int vi = 0; vi < static_cast<int>(vertices.size()); ++vi) {
        float angleSum = vertexAngleSum(vertices, faces, vi);
        total += 2.0f * static_cast<float>(M_PI) - angleSum;
    }
    return total;
}

} // namespace AngleTools
