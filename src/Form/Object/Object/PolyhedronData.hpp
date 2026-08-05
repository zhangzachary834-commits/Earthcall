#pragma once

#include <algorithm>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../AngleTools.hpp"
#include "../Contour.hpp"

// Polyhedron geometry container and shape-algebra factory.
// Lifted out of Object so polygon math has its own home.
struct PolyhedronData {

    // Helper structures for polyhedron construction
    struct Edge {
        int v1, v2;
        Edge(int vertex1, int vertex2)
            : v1(std::min(vertex1, vertex2)), v2(std::max(vertex1, vertex2)) {}
        bool operator==(const Edge& other) const { return v1 == other.v1 && v2 == other.v2; }
    };

    struct Face {
        std::vector<int> vertices;
        std::vector<Edge> edges;

        Face(const std::vector<int>& verts) : vertices(verts) {
            for (size_t i = 0; i < vertices.size(); ++i) {
                int next = (i + 1) % vertices.size();
                edges.emplace_back(vertices[i], vertices[next]);
            }
        }

        bool sharesEdgeWith(const Face& other) const {
            for (const auto& edge1 : edges) {
                for (const auto& edge2 : other.edges) {
                    if (edge1 == edge2) return true;
                }
            }
            return false;
        }
    };

    std::vector<glm::vec3> vertices;
    std::vector<std::vector<int>> faces;
    std::vector<glm::vec3> faceNormals;
    std::vector<glm::vec2> faceUVs;

    bool isConvex = true;
    std::vector<bool> faceConvexity;
    std::vector<float> faceAreas;
    std::vector<float> vertexCurvatures;

    enum class ContourType { Flat, Round };
    std::vector<ContourType> contourTypes;

    struct ContourCurvature {
        float gaussianCurvature = 0.0f;
        float meanCurvature     = 0.0f;
        float principalK1       = 0.0f;
        float principalK2       = 0.0f;
    };
    std::vector<ContourCurvature> contourCurvatures;

    std::vector<AngleTools::DihedralAngle> dihedralAngles;
    std::vector<AngleTools::EdgeInfo>      edgeInfos;
    std::vector<PolyhedronData> convexComponents;

    PolyhedronData() = default;

    static PolyhedronData createRegularPolyhedron(int numFaces, float radius = 0.5f);
    static PolyhedronData createCustomPolyhedron(const std::vector<glm::vec3>& verts,
                                                 const std::vector<std::vector<int>>& faceDefs);

    static PolyhedronData createConcavePolyhedron(int numFaces, float radius = 0.5f, float concavity = 0.3f);
    static PolyhedronData createStarPolyhedron(int numFaces, float radius = 0.5f, float spikeLength = 0.3f);
    static PolyhedronData createCraterPolyhedron(int numFaces, float radius = 0.5f, float craterDepth = 0.2f);

    static PolyhedronData createPrism(
        const std::vector<glm::vec2>& basePolygon, float height, float radius = 0.5f);
    static PolyhedronData createAntiprism(int n, float radius = 0.5f, float height = 1.0f);
    static PolyhedronData createPyramid(
        const std::vector<glm::vec2>& basePolygon, float apexHeight, float radius = 0.5f);
    static PolyhedronData createBipyramid(int n, float radius = 0.5f, float height = 1.0f);
    static PolyhedronData createFrustum(
        const std::vector<glm::vec2>& basePolygon, float height,
        float topScale = 0.5f, float radius = 0.5f);

    static PolyhedronData truncate(const PolyhedronData& source, float amount = 0.3f);
    static PolyhedronData createDual(const PolyhedronData& source);

    void generateUVs();
    void computeNormals();
    void analyzeConvexity();
    void computeFaceAreas();
    void computeVertexCurvatures();

    int getFaceCount() const { return static_cast<int>(faces.size()); }
    int getVertexCount() const { return static_cast<int>(vertices.size()); }
    bool getIsConvex() const { return isConvex; }
    const std::vector<PolyhedronData>& getConvexComponents() const { return convexComponents; }
    bool getFaceConvexity(int faceIndex) const {
        return (faceIndex >= 0 && faceIndex < static_cast<int>(faceConvexity.size()))
               ? faceConvexity[faceIndex] : true;
    }

    void addFace(const std::vector<int>& faceVertices);
    bool validateTopology() const;

    // Newell's method to compute a stable polygon normal from indexed vertices.
    static glm::vec3 computeNewellNormal(const std::vector<glm::vec3>& vertices,
                                         const std::vector<int>& face);

    void ensureOutwardWinding();
    void scaleToRadius(float radius);

    void classifyContours();
    std::unique_ptr<Contour> buildContour(int faceIndex) const;
    void computeAngleData();
    void rebuildConvexComponents();
    void recomputeAll();
};
