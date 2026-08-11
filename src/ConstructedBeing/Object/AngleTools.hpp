#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// AngleTools -- utilities for measuring and manipulating angles, edges, and
// geometric relationships within polyhedra and general 3D shapes.
//
// These operate on raw vertex / face data so they stay decoupled (independent)
// from the Object class hierarchy.
// ---------------------------------------------------------------------------

namespace AngleTools {

// ---- Data types -----------------------------------------------------------

// Dihedral angle: the angle between two adjacent faces measured along
// the edge they share (like the angle of an open book).
struct DihedralAngle {
    int face1;
    int face2;
    int edgeVertex1;
    int edgeVertex2;
    float angleRadians;

    float angleDegrees() const { return angleRadians * 180.0f / static_cast<float>(M_PI); }
};

// Vertex angle: the interior angle at one corner of a single face
// (the angle between the two edges meeting at that corner).
struct VertexAngle {
    int faceIndex;
    int vertexIndex;
    int prevVertex;
    int nextVertex;
    float angleRadians;

    float angleDegrees() const { return angleRadians * 180.0f / static_cast<float>(M_PI); }
};

// Solid angle: the "cone of directions" subtended at a vertex by all
// surrounding faces.  Measured in steradians (like square-radians).
struct SolidAngle {
    int vertexIndex;
    float steradians;      // The solid angle value
    float angleDeficit;    // 2*pi minus the sum of face angles at this vertex
                           // (related to Gaussian curvature via Descartes' theorem)
};

// Edge descriptor with length and associated dihedral angles.
struct EdgeInfo {
    int v1, v2;
    float length;
    std::vector<DihedralAngle> dihedralAngles;
};


// ---- Computation ----------------------------------------------------------

// Compute all dihedral angles of a polyhedron (one per shared edge pair)
std::vector<DihedralAngle> computeDihedralAngles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    const std::vector<glm::vec3>& faceNormals);

// Compute the dihedral angle between two faces given their normals and
// the direction of their shared edge
float computeDihedralAngle(
    const glm::vec3& normal1,
    const glm::vec3& normal2,
    const glm::vec3& edgeDirection);

// Compute every vertex angle in every face
std::vector<VertexAngle> computeAllVertexAngles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces);

// Compute the interior angle at one corner of a triangle/polygon
float computeVertexAngle(
    const glm::vec3& vertex,
    const glm::vec3& prevVertex,
    const glm::vec3& nextVertex);

// Compute solid angles at all vertices
std::vector<SolidAngle> computeSolidAngles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces);

// Compute complete edge information (length + dihedral) for every edge
std::vector<EdgeInfo> computeAllEdgeInfo(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    const std::vector<glm::vec3>& faceNormals);

// Sum of all face angles meeting at a vertex (the "angle sum")
float vertexAngleSum(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    int vertexIndex);


// ---- Manipulation ---------------------------------------------------------

// Rotate all vertices of a face (that aren't on the shared edge) around
// the edge defined by (edgeV1, edgeV2) by angleDeltaRadians.
void rotateFaceAroundEdge(
    std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces,
    int faceIndex,
    int edgeV1, int edgeV2,
    float angleDeltaRadians);

// Move a single vertex within a face so that its interior angle becomes
// targetAngleRadians.  The two neighboring vertices in the face stay fixed.
void adjustVertexAngle(
    std::vector<glm::vec3>& vertices,
    const std::vector<int>& face,
    int vertexPositionInFace,
    float targetAngleRadians);

// Move one endpoint of an edge so the edge reaches targetLength while
// keeping the other endpoint fixed.
void adjustEdgeLength(
    std::vector<glm::vec3>& vertices,
    int v1, int v2,
    float targetLength);


// ---- Validation -----------------------------------------------------------

// Can the given set of face angles close up into a convex vertex?
// (The sum must be strictly less than 2*pi.)
bool canFormConvexVertex(const std::vector<float>& angles);

// Euler characteristic: V - E + F.  Should be 2 for any closed convex
// polyhedron (a famous topological invariant).
int eulerCharacteristic(int vertexCount, int edgeCount, int faceCount);

// Total angle deficit across all vertices (should equal 4*pi for a closed
// convex polyhedron by Descartes' theorem -- one of the coolest results
// in discrete differential geometry).
float totalAngleDeficit(
    const std::vector<glm::vec3>& vertices,
    const std::vector<std::vector<int>>& faces);

} // namespace AngleTools
