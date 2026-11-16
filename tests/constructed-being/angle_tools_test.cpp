#include "ConstructedBeing/Singular/Object/AngleTools.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <cmath>

using namespace AngleTools;

bool near(float a, float b, float tol = 1e-5) {
    return std::fabs(a - b) < tol;
}

int main() {
    int failed = 0;

    // Test ComputeVertexAngle
    glm::vec3 vertex(0, 0, 0);
    glm::vec3 prevVertex(1, 0, 0);
    glm::vec3 nextVertex(0, 1, 0);
    float angle = computeVertexAngle(vertex, prevVertex, nextVertex);
    if (!near(angle, M_PI / 2.0f)) {
        std::cerr << "ComputeVertexAngle failed (90 deg)\n";
        failed++;
    }

    // Test ComputeDihedralAngle
    glm::vec3 n1(0, 1, 0);
    glm::vec3 n2(1, 0, 0);
    glm::vec3 edgeDir(0, 0, -1);
    float angle2 = computeDihedralAngle(n1, n2, edgeDir);
    // Since computeDihedralAngle expects edgeDir in specific winding order
    // relative to normals, changing edgeDir to (0,0,-1) makes the cross product
    // match.
    if (!near(angle2, M_PI / 2.0f)) {
        std::cerr << "ComputeDihedralAngle failed. Expected " << M_PI / 2.0f << " but got " << angle2 << "\n";
        failed++;
    }

    // Test CubeTotalAngleDeficit
    std::vector<glm::vec3> vertices = {
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
    };
    std::vector<std::vector<int>> faces = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
        {3, 7, 6, 2}, {0, 4, 7, 3}, {1, 2, 6, 5}
    };
    float deficit = totalAngleDeficit(vertices, faces);
    if (!near(deficit, 4.0f * M_PI)) {
        std::cerr << "CubeTotalAngleDeficit failed\n";
        failed++;
    }

    // Test EulerCharacteristic
    if (eulerCharacteristic(8, 12, 6) != 2) {
        std::cerr << "EulerCharacteristic failed\n";
        failed++;
    }

    // Test CanFormConvexVertex
    std::vector<float> cubeCorner = {
        static_cast<float>(M_PI) / 2.0f,
        static_cast<float>(M_PI) / 2.0f,
        static_cast<float>(M_PI) / 2.0f
    };
    if (!canFormConvexVertex(cubeCorner)) {
        std::cerr << "CanFormConvexVertex failed (should be true)\n";
        failed++;
    }

    std::vector<float> flatPlane = {
        static_cast<float>(M_PI) / 2.0f,
        static_cast<float>(M_PI) / 2.0f,
        static_cast<float>(M_PI) / 2.0f,
        static_cast<float>(M_PI) / 2.0f
    };
    if (canFormConvexVertex(flatPlane)) {
        std::cerr << "CanFormConvexVertex failed (should be false)\n";
        failed++;
    }

    // Test AdjustEdgeLength
    std::vector<glm::vec3> verts2 = { {0, 0, 0}, {2, 0, 0} };
    adjustEdgeLength(verts2, 0, 1, 5.0f);
    if (!near(verts2[1].x, 5.0f)) {
        std::cerr << "AdjustEdgeLength failed\n";
        failed++;
    }

    if (failed == 0) {
        std::cout << "All angle_tools tests passed.\n";
    }

    return failed;
}
