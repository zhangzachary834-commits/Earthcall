#include "PolyhedronData.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PolyhedronData PolyhedronData::createRegularPolyhedron(int numFaces, float radius) {
    PolyhedronData data;
    
    // Base scale for seed geometry. Final uniform radius is applied later by scaleToRadius().
    float scale = 0.5f;
    
    // Create vertices for regular polyhedrons
    switch (numFaces) {
        case 4: // Tetrahedron
            data.vertices = {
                {0.0f, scale, 0.0f},
                {-scale, -scale, scale},
                {scale, -scale, scale},
                {0.0f, -scale, -scale}
            };
            data.faces = {
                {0, 1, 2},
                {0, 2, 3},
                {0, 3, 1},
                {1, 3, 2}
            };
            break;
            
        case 6: // Cube (already handled by ShapeKind::Cube, but included for completeness)
            data.vertices = {
                {-scale, -scale, -scale}, {scale, -scale, -scale}, {scale, scale, -scale}, {-scale, scale, -scale},
                {-scale, -scale, scale}, {scale, -scale, scale}, {scale, scale, scale}, {-scale, scale, scale}
            };
            data.faces = {
                {0, 1, 2, 3}, // front
                {5, 4, 7, 6}, // back
                {4, 0, 3, 7}, // left
                {1, 5, 6, 2}, // right
                {3, 2, 6, 7}, // top
                {4, 5, 1, 0}  // bottom
            };
            break;
            
        case 8: // Octahedron
            data.vertices = {
                {0.0f, scale, 0.0f},   // top
                {0.0f, -scale, 0.0f},  // bottom
                {scale, 0.0f, 0.0f},   // right
                {-scale, 0.0f, 0.0f},  // left
                {0.0f, 0.0f, scale},   // front
                {0.0f, 0.0f, -scale}   // back
            };
            data.faces = {
                {0, 2, 4}, {0, 4, 3}, {0, 3, 5}, {0, 5, 2},  // top faces
                {1, 4, 2}, {1, 3, 4}, {1, 5, 3}, {1, 2, 5}   // bottom faces
            };
            break;
            
        case 12: // Dodecahedron
            {
                // Build a regular dodecahedron as the dual of a canonical icosahedron
                // Commenting out the previous experimental face-finding implementation for stability
                // (kept for reference)
                /*
                // BEGIN OLD DODECAHEDRON (experimental)
                // ... previous algorithmic approach removed for clarity ...
                // END OLD DODECAHEDRON
                */

                // 1) Build canonical icosahedron (unit-ish), then derive dual
                std::vector<glm::vec3> icoV;
                std::vector<std::vector<int>> icoF;
                {
                    const float PHI = (1.0f + sqrt(5.0f)) * 0.5f;
                    icoV = {
                        {-1,  PHI,  0}, { 1,  PHI,  0}, {-1, -PHI,  0}, { 1, -PHI,  0},
                        { 0, -1,  PHI}, { 0,  1,  PHI}, { 0, -1, -PHI}, { 0,  1, -PHI},
                        { PHI,  0, -1}, { PHI,  0,  1}, {-PHI,  0, -1}, {-PHI,  0,  1}
                    };
                    icoF = {
                        {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
                        {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
                        {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
                        {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1}
                    };
                }

                // Normalize icosahedron onto a sphere (preserve shape ratios)
                for (auto &v : icoV) {
                    if (glm::length(v) > 0.0f) v = glm::normalize(v);
                }

                // 2) Dodecahedron vertices are centroids of icosahedron faces
                data.vertices.clear();
                data.vertices.reserve(icoF.size());
                for (const auto &f : icoF) {
                    glm::vec3 c(0.0f);
                    for (int idx : f) c += icoV[idx];
                    c /= static_cast<float>(f.size());
                    data.vertices.push_back(c);
                }

                // 3) Dodecahedron faces: for each icosahedron vertex, the surrounding 5 faces
                // Collect faces incident to each icosahedron vertex
                std::vector<std::vector<int>> incidentFaces(icoV.size());
                for (int fi = 0; fi < static_cast<int>(icoF.size()); ++fi) {
                    for (int vi : icoF[fi]) incidentFaces[vi].push_back(fi);
                }

                data.faces.clear();
                data.faces.reserve(icoV.size()); // 12 faces
                for (int vi = 0; vi < static_cast<int>(icoV.size()); ++vi) {
                    const glm::vec3 &vertex = icoV[vi];

                    // Build local tangent basis at this icosahedron vertex
                    glm::vec3 n = glm::normalize(vertex);
                    glm::vec3 ref = (fabs(n.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
                    glm::vec3 t1 = glm::normalize(glm::cross(ref, n));
                    glm::vec3 t2 = glm::normalize(glm::cross(n, t1));

                    // Sort incident face centroids by angle around n
                    std::vector<std::pair<float,int>> angles;
                    angles.reserve(incidentFaces[vi].size());
                    for (int fIdx : incidentFaces[vi]) {
                        glm::vec3 c = data.vertices[fIdx];
                        glm::vec3 u = c - vertex;
                        float a = atan2f(glm::dot(u, t2), glm::dot(u, t1));
                        angles.emplace_back(a, fIdx);
                    }
                    std::sort(angles.begin(), angles.end(), [](auto &A, auto &B){ return A.first < B.first; });

                    // Face is the ordered list of dodeca vertices (which map 1:1 to icosa faces)
                    std::vector<int> face;
                    face.reserve(angles.size());
                    for (auto &p : angles) face.push_back(p.second);
                    data.faces.push_back(face);
                }
            }
            break;
            
        case 20: // Icosahedron
            {
                // Replace with canonical 12-vertex, 20-face icosahedron
                // Commenting out previous incorrect data for reference
                /*
                // BEGIN OLD ICOSAHEDRON (incorrect vertex/face counts)
                float phi = (1.0f + sqrt(5.0f)) / 2.0f;
                float invPhi = 1.0f / phi;
                data.vertices = { /* ... 14 entries ... */ /* };
                data.faces = { /* ... 28 faces ... */ /* };
                // END OLD ICOSAHEDRON
                */

                const float PHI = (1.0f + sqrt(5.0f)) * 0.5f;
                data.vertices = {
                    {-1,  PHI,  0}, { 1,  PHI,  0}, {-1, -PHI,  0}, { 1, -PHI,  0},
                    { 0, -1,  PHI}, { 0,  1,  PHI}, { 0, -1, -PHI}, { 0,  1, -PHI},
                    { PHI,  0, -1}, { PHI,  0,  1}, {-PHI,  0, -1}, {-PHI,  0,  1}
                };
                data.faces = {
                    {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
                    {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
                    {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
                    {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1}
                };

                // Normalize to sphere for consistent scaling
                for (auto &v : data.vertices) {
                    if (glm::length(v) > 0.0f) v = glm::normalize(v);
                }
            }
            break;
            
        default:
            // Create a simple pyramid as fallback
            data.vertices = {
                {0.0f, 0.5f, 0.0f},   // apex
                {-0.5f, -0.5f, -0.5f}, // base corners
                {0.5f, -0.5f, -0.5f},
                {0.5f, -0.5f, 0.5f},
                {-0.5f, -0.5f, 0.5f}
            };
            data.faces = {
                {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}, // triangular faces
                {1, 4, 3, 2} // base face
            };
            break;
    }
    
    // Ensure consistent outward winding and desired radius, then compute properties
    data.ensureOutwardWinding();
    data.scaleToRadius((radius > 0.0f) ? radius : 0.5f);
    data.ensureOutwardWinding();
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    data.rebuildConvexComponents();
    return data;
}

PolyhedronData PolyhedronData::createCustomPolyhedron(const std::vector<glm::vec3>& verts, 
                                                                      const std::vector<std::vector<int>>& faceDefs) {
    PolyhedronData data;
    data.vertices = verts;
    data.faces = faceDefs;
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    data.rebuildConvexComponents();
    return data;
}

PolyhedronData PolyhedronData::createConcavePolyhedron(int numFaces, float radius, float concavity) {
    PolyhedronData data;
    
    // Start with a regular polyhedron
    data = createRegularPolyhedron(numFaces, radius);

    // Apply concavity by alternating inward and slightly outward radial vertices.
    // Moving every vertex by the same factor only scales the convex hull; the
    // alternating rhythm creates real recesses for collision and ray tests.
    concavity = std::clamp(concavity, 0.0f, 0.9f);
    for (size_t i = 0; i < data.vertices.size(); ++i) {
        auto& vertex = data.vertices[i];
        glm::vec3 direction = glm::normalize(vertex);
        float distance = glm::length(vertex);
        float factor = (i % 2 == 0) ? (1.0f - concavity) : (1.0f + concavity * 0.15f);
        float newDistance = distance * factor;
        vertex = direction * newDistance;
    }
    
    // Recompute all properties
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    data.rebuildConvexComponents();
    
    return data;
}

PolyhedronData PolyhedronData::createStarPolyhedron(int numFaces, float radius, float spikeLength) {
    PolyhedronData data;
    
    // Start with a regular polyhedron
    data = createRegularPolyhedron(numFaces, radius);

    // Replace each face with a shallow outward pyramid. This is the first real
    // star foundation: the old version extended every vertex equally, which was
    // just another convex scale. Face spikes create concave valleys between them.
    std::vector<std::vector<int>> originalFaces = data.faces;
    std::vector<glm::vec3> originalNormals = data.faceNormals;
    data.faces.clear();
    spikeLength = std::max(0.0f, spikeLength);

    for (size_t faceIndex = 0; faceIndex < originalFaces.size(); ++faceIndex) {
        const auto& face = originalFaces[faceIndex];
        if (face.size() < 3) continue;

        glm::vec3 center(0.0f);
        for (int idx : face) center += data.vertices[idx];
        center /= static_cast<float>(face.size());

        glm::vec3 normal = (faceIndex < originalNormals.size())
            ? originalNormals[faceIndex]
            : computeNewellNormal(data.vertices, face);
        if (glm::dot(normal, normal) <= 1e-10f) normal = glm::normalize(center);

        int spikeIndex = static_cast<int>(data.vertices.size());
        data.vertices.push_back(center + glm::normalize(normal) * spikeLength);

        for (size_t i = 0; i < face.size(); ++i) {
            int a = face[i];
            int b = face[(i + 1) % face.size()];
            data.faces.push_back({a, b, spikeIndex});
        }
    }
    
    // Recompute all properties
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    data.rebuildConvexComponents();
    
    return data;
}

PolyhedronData PolyhedronData::createCraterPolyhedron(int numFaces, float radius, float craterDepth) {
    PolyhedronData data;
    
    // Start with a regular polyhedron
    data = createRegularPolyhedron(numFaces, radius);
    
    // Create craters by pushing some vertices inward
    for (size_t i = 0; i < data.vertices.size(); ++i) {
        auto& vertex = data.vertices[i];
        glm::vec3 direction = glm::normalize(vertex);
        float distance = glm::length(vertex);
        
        // Create craters on alternating vertices
        if (i % 2 == 0) {
            float newDistance = distance * (1.0f - craterDepth);
            vertex = direction * newDistance;
        }
    }
    
    // Recompute all properties
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    data.rebuildConvexComponents();
    
    return data;
}

void PolyhedronData::computeNormals() {
    faceNormals.clear();
    try { faceNormals.reserve(faces.size()); } catch (...) {}
    
    for (const auto& face : faces) {
        if (face.size() < 3) {
            faceNormals.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // default normal
            continue;
        }
        
        // Compute face normal using first three vertices
        glm::vec3 v0 = vertices[face[0]];
        glm::vec3 v1 = vertices[face[1]];
        glm::vec3 v2 = vertices[face[2]];
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        // For concave faces, ensure normal points outward
        if (face.size() > 3) {
            // Check if face is concave by testing center point
            glm::vec3 center(0.0f);
            for (int idx : face) {
                center += vertices[idx];
            }
            center /= static_cast<float>(face.size());
            
            // If center is "inside" the polyhedron, flip normal
            if (glm::dot(normal, center) < 0) {
                normal = -normal;
            }
        }
        
        faceNormals.push_back(normal);
    }
}

glm::vec3 PolyhedronData::computeNewellNormal(const std::vector<glm::vec3>& vertices, const std::vector<int>& face) {
    glm::vec3 n(0.0f);
    size_t m = face.size();
    if (m < 3) return glm::vec3(0,1,0);
    for (size_t i = 0; i < m; ++i) {
        const glm::vec3& current = vertices[face[i]];
        const glm::vec3& next = vertices[face[(i + 1) % m]];
        n.x += (current.y - next.y) * (current.z + next.z);
        n.y += (current.z - next.z) * (current.x + next.x);
        n.z += (current.x - next.x) * (current.y + next.y);
    }
    float len = glm::length(n);
    if (len > 1e-8f) n /= len; else n = glm::vec3(0,1,0);
    return n;
}

void PolyhedronData::ensureOutwardWinding() {
    if (vertices.empty() || faces.empty()) return;
    // Compute polyhedron centroid
    glm::vec3 centroid(0.0f);
    for (const auto& v : vertices) centroid += v;
    centroid /= static_cast<float>(vertices.size());

    for (auto& face : faces) {
        if (face.size() < 3) continue;
        glm::vec3 n = PolyhedronData::computeNewellNormal(vertices, face);
        glm::vec3 p0 = vertices[face[0]];
        // If normal points toward centroid, flip winding
        if (glm::dot(n, centroid - p0) > 0.0f) {
            std::reverse(face.begin(), face.end());
        }
    }
}

void PolyhedronData::scaleToRadius(float radius) {
    if (vertices.empty()) return;
    float maxR = 0.0f;
    for (const auto& v : vertices) maxR = std::max(maxR, glm::length(v));
    if (maxR < 1e-8f) return;
    float s = radius / maxR;
    for (auto& v : vertices) v *= s;
}

// ===========================================================================
//  Irregular polyhedron factory methods
// ===========================================================================

PolyhedronData PolyhedronData::createPrism(
    const std::vector<glm::vec2>& basePolygon, float height, float radius)
{
    PolyhedronData data;
    int n = static_cast<int>(basePolygon.size());
    if (n < 3) return data;

    float halfH = height * 0.5f;

    // Bottom vertices (y = -halfH), then top vertices (y = +halfH)
    for (int i = 0; i < n; ++i)
        data.vertices.push_back(glm::vec3(basePolygon[i].x, -halfH, basePolygon[i].y));
    for (int i = 0; i < n; ++i)
        data.vertices.push_back(glm::vec3(basePolygon[i].x, halfH, basePolygon[i].y));

    // Bottom face (reversed winding for outward normal pointing -Y)
    std::vector<int> bottom;
    for (int i = n - 1; i >= 0; --i) bottom.push_back(i);
    data.faces.push_back(bottom);

    // Top face
    std::vector<int> top;
    for (int i = 0; i < n; ++i) top.push_back(n + i);
    data.faces.push_back(top);

    // Side faces (quads connecting bottom edge to top edge)
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        data.faces.push_back({i, next, n + next, n + i});
    }

    data.ensureOutwardWinding();
    data.scaleToRadius(radius);
    data.recomputeAll();
    return data;
}

PolyhedronData PolyhedronData::createPrism(
    int sides, float radius, float height)
{
    std::vector<glm::vec2> base;
    sides = std::max(3, sides);
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / sides;
        base.emplace_back(std::cos(angle), std::sin(angle));
    }
    return createPrism(base, height, radius);
}

PolyhedronData PolyhedronData::createAntiprism(
    int n, float radius, float height)
{
    PolyhedronData data;
    if (n < 3) return data;

    float halfH = height * 0.5f;
    float twistAngle = static_cast<float>(M_PI) / n; // Half-step rotation

    // Bottom ring
    for (int i = 0; i < n; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / n;
        data.vertices.push_back(glm::vec3(std::cos(angle), -halfH, std::sin(angle)));
    }
    // Top ring (rotated by half a step)
    for (int i = 0; i < n; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / n + twistAngle;
        data.vertices.push_back(glm::vec3(std::cos(angle), halfH, std::sin(angle)));
    }

    // Bottom face
    std::vector<int> bottom;
    for (int i = n - 1; i >= 0; --i) bottom.push_back(i);
    data.faces.push_back(bottom);

    // Top face
    std::vector<int> top;
    for (int i = 0; i < n; ++i) top.push_back(n + i);
    data.faces.push_back(top);

    // Alternating triangles connecting the two rings
    for (int i = 0; i < n; ++i) {
        int botCur  = i;
        int botNext = (i + 1) % n;
        int topCur  = n + i;
        int topNext = n + (i + 1) % n;

        // Triangle pointing up
        data.faces.push_back({botCur, botNext, topCur});
        // Triangle pointing down
        data.faces.push_back({botNext, topNext, topCur});
    }

    data.ensureOutwardWinding();
    data.scaleToRadius(radius);
    data.recomputeAll();
    return data;
}

PolyhedronData PolyhedronData::createPyramid(
    const std::vector<glm::vec2>& basePolygon, float apexHeight, float radius)
{
    PolyhedronData data;
    int n = static_cast<int>(basePolygon.size());
    if (n < 3) return data;

    // Base vertices at y = 0
    for (int i = 0; i < n; ++i)
        data.vertices.push_back(glm::vec3(basePolygon[i].x, 0.0f, basePolygon[i].y));

    // Apex vertex
    int apexIdx = n;
    data.vertices.push_back(glm::vec3(0.0f, apexHeight, 0.0f));

    // Base face (reversed winding)
    std::vector<int> base;
    for (int i = n - 1; i >= 0; --i) base.push_back(i);
    data.faces.push_back(base);

    // Triangular side faces
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        data.faces.push_back({i, next, apexIdx});
    }

    data.ensureOutwardWinding();
    data.scaleToRadius(radius);
    data.recomputeAll();
    return data;
}

PolyhedronData PolyhedronData::createBipyramid(
    int n, float radius, float height)
{
    PolyhedronData data;
    if (n < 3) return data;

    float halfH = height * 0.5f;

    // Equatorial ring (the "waist" of the diamond)
    for (int i = 0; i < n; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / n;
        data.vertices.push_back(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
    }

    // Top apex
    int topIdx = n;
    data.vertices.push_back(glm::vec3(0.0f, halfH, 0.0f));

    // Bottom apex
    int botIdx = n + 1;
    data.vertices.push_back(glm::vec3(0.0f, -halfH, 0.0f));

    // Upper triangles
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        data.faces.push_back({i, next, topIdx});
    }

    // Lower triangles (reversed winding relative to upper)
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        data.faces.push_back({next, i, botIdx});
    }

    data.ensureOutwardWinding();
    data.scaleToRadius(radius);
    data.recomputeAll();
    return data;
}

PolyhedronData PolyhedronData::createFrustum(
    const std::vector<glm::vec2>& basePolygon, float height,
    float topScale, float radius)
{
    PolyhedronData data;
    int n = static_cast<int>(basePolygon.size());
    if (n < 3) return data;

    float halfH = height * 0.5f;

    // Bottom vertices
    for (int i = 0; i < n; ++i)
        data.vertices.push_back(glm::vec3(basePolygon[i].x, -halfH, basePolygon[i].y));

    // Top vertices (scaled down version of the base)
    for (int i = 0; i < n; ++i)
        data.vertices.push_back(glm::vec3(basePolygon[i].x * topScale, halfH,
                                           basePolygon[i].y * topScale));

    // Bottom face (reversed winding)
    std::vector<int> bottom;
    for (int i = n - 1; i >= 0; --i) bottom.push_back(i);
    data.faces.push_back(bottom);

    // Top face
    std::vector<int> top;
    for (int i = 0; i < n; ++i) top.push_back(n + i);
    data.faces.push_back(top);

    // Side faces (trapezoids connecting bottom to top)
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        data.faces.push_back({i, next, n + next, n + i});
    }

    data.ensureOutwardWinding();
    data.scaleToRadius(radius);
    data.recomputeAll();
    return data;
}


// ===========================================================================
//  Topological operations
// ===========================================================================

PolyhedronData PolyhedronData::truncate(
    const PolyhedronData& source, float amount)
{
    PolyhedronData result;
    if (source.vertices.empty() || source.faces.empty()) return result;

    amount = std::clamp(amount, 0.01f, 0.49f);

    // For each original edge, compute two new vertices by interpolating
    // "amount" from each endpoint toward the other
    struct EdgeKey {
        int a, b;
        EdgeKey(int i, int j) : a(std::min(i,j)), b(std::max(i,j)) {}
        bool operator==(const EdgeKey& o) const { return a == o.a && b == o.b; }
    };
    struct EKHash {
        size_t operator()(const EdgeKey& k) const {
            return std::hash<long long>()(static_cast<long long>(k.a) << 32 | k.b);
        }
    };

    // Map from (originalEdge, whichEndpoint) -> new vertex index
    // Key: (edge, endpoint) where endpoint is 0 (near a) or 1 (near b)
    std::unordered_map<EdgeKey, std::pair<int,int>, EKHash> edgeNewVerts;

    for (const auto& face : source.faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            int v0 = face[i];
            int v1 = face[(i + 1) % face.size()];
            EdgeKey ek(v0, v1);
            if (edgeNewVerts.find(ek) == edgeNewVerts.end()) {
                // Two new vertices along this edge
                glm::vec3 pA = glm::mix(source.vertices[ek.a], source.vertices[ek.b], amount);
                glm::vec3 pB = glm::mix(source.vertices[ek.b], source.vertices[ek.a], amount);
                int idxA = static_cast<int>(result.vertices.size());
                result.vertices.push_back(pA);
                int idxB = static_cast<int>(result.vertices.size());
                result.vertices.push_back(pB);
                edgeNewVerts[ek] = {idxA, idxB};
            }
        }
    }

    auto getNewIdx = [&](int originalV, int otherV) -> int {
        EdgeKey ek(originalV, otherV);
        auto it = edgeNewVerts.find(ek);
        if (it == edgeNewVerts.end()) return -1;
        return (originalV == ek.a) ? it->second.first : it->second.second;
    };

    // Truncated face: replace each original face with a new face using the
    // interpolated vertices
    for (const auto& face : source.faces) {
        std::vector<int> newFace;
        for (size_t i = 0; i < face.size(); ++i) {
            int prev = face[(i + face.size() - 1) % face.size()];
            int curr = face[i];
            int next = face[(i + 1) % face.size()];
            int fromPrev = getNewIdx(curr, prev);
            int toNext   = getNewIdx(curr, next);
            if (fromPrev >= 0) newFace.push_back(fromPrev);
            if (toNext >= 0)   newFace.push_back(toNext);
        }
        if (newFace.size() >= 3)
            result.faces.push_back(newFace);
    }

    // Vertex figures: for each original vertex, collect the surrounding
    // new vertices and form a new face
    std::unordered_map<int, std::vector<std::pair<int, int>>> vertexNeighborEdges;
    for (const auto& face : source.faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            int curr = face[i];
            int next = face[(i + 1) % face.size()];
            vertexNeighborEdges[curr].push_back({curr, next});
        }
    }

    for (const auto& kv : vertexNeighborEdges) {
        int origV = kv.first;
        const auto& neighbors = kv.second;

        // Collect new vertices around this original vertex
        std::vector<int> ring;
        for (const auto& edge : neighbors) {
            int newIdx = getNewIdx(edge.first, edge.second);
            if (newIdx >= 0) ring.push_back(newIdx);
        }

        if (ring.size() >= 3) {
            // Sort ring by angle around the original vertex direction
            glm::vec3 center(0.0f);
            for (int idx : ring) center += result.vertices[idx];
            center /= static_cast<float>(ring.size());

            glm::vec3 normal = glm::normalize(source.vertices[origV]);
            glm::vec3 ref = (std::fabs(normal.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, normal));
            glm::vec3 t2 = glm::normalize(glm::cross(normal, t1));

            std::vector<std::pair<float, int>> angleIdx;
            for (int idx : ring) {
                glm::vec3 d = result.vertices[idx] - center;
                float a = std::atan2(glm::dot(d, t2), glm::dot(d, t1));
                angleIdx.push_back({a, idx});
            }
            std::sort(angleIdx.begin(), angleIdx.end());

            std::vector<int> sortedRing;
            for (const auto& p : angleIdx) sortedRing.push_back(p.second);
            result.faces.push_back(sortedRing);
        }
    }

    result.ensureOutwardWinding();
    result.recomputeAll();
    return result;
}

PolyhedronData PolyhedronData::createDual(
    const PolyhedronData& source)
{
    PolyhedronData result;
    if (source.vertices.empty() || source.faces.empty()) return result;

    // Dual vertices = centroids of source faces
    result.vertices.reserve(source.faces.size());
    for (const auto& face : source.faces) {
        glm::vec3 centroid(0.0f);
        for (int idx : face) centroid += source.vertices[idx];
        centroid /= static_cast<float>(face.size());
        result.vertices.push_back(centroid);
    }

    // Dual faces: for each source vertex, collect indices of source faces
    // that contain it, ordered by angle around the vertex normal
    std::unordered_map<int, std::vector<int>> vertexToFaces;
    for (int fi = 0; fi < static_cast<int>(source.faces.size()); ++fi) {
        for (int vi : source.faces[fi]) {
            vertexToFaces[vi].push_back(fi);
        }
    }

    for (const auto& kv : vertexToFaces) {
        int vi = kv.first;
        const auto& incidentFaces = kv.second;
        if (incidentFaces.size() < 3) continue;

        // Sort faces by angle around the vertex normal
        glm::vec3 vn = glm::normalize(source.vertices[vi]);
        glm::vec3 ref = (std::fabs(vn.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        glm::vec3 t1 = glm::normalize(glm::cross(ref, vn));
        glm::vec3 t2 = glm::normalize(glm::cross(vn, t1));

        std::vector<std::pair<float, int>> angleIdx;
        for (int fi : incidentFaces) {
            glm::vec3 d = result.vertices[fi] - source.vertices[vi];
            float a = std::atan2(glm::dot(d, t2), glm::dot(d, t1));
            angleIdx.push_back({a, fi});
        }
        std::sort(angleIdx.begin(), angleIdx.end());

        std::vector<int> dualFace;
        for (const auto& p : angleIdx) dualFace.push_back(p.second);
        result.faces.push_back(dualFace);
    }

    result.ensureOutwardWinding();
    float maxR = 0.0f;
    for (const auto& v : source.vertices) maxR = std::max(maxR, glm::length(v));
    if (maxR > 1e-8f) result.scaleToRadius(maxR);
    result.recomputeAll();
    return result;
}


// ===========================================================================
//  Contour and angle analysis
// ===========================================================================

void PolyhedronData::classifyContours() {
    contourTypes.clear();
    contourCurvatures.clear();
    contourTypes.reserve(faces.size());
    contourCurvatures.reserve(faces.size());

    for (size_t fi = 0; fi < faces.size(); ++fi) {
        const auto& face = faces[fi];
        if (face.size() < 3) {
            contourTypes.push_back(ContourType::Flat);
            contourCurvatures.push_back({});
            continue;
        }

        // Check planarity: measure max distance of vertices from the plane
        // defined by the first three vertices
        glm::vec3 v0 = vertices[face[0]];
        glm::vec3 v1 = vertices[face[1]];
        glm::vec3 v2 = vertices[face[2]];
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        float nLen = glm::length(normal);
        if (nLen < 1e-8f) {
            contourTypes.push_back(ContourType::Flat);
            contourCurvatures.push_back({});
            continue;
        }
        normal /= nLen;

        float maxDist = 0.0f;
        for (size_t i = 3; i < face.size(); ++i) {
            float d = std::fabs(glm::dot(vertices[face[i]] - v0, normal));
            maxDist = std::max(maxDist, d);
        }

        // Threshold: if vertices deviate from the plane by more than a small
        // fraction of the face's extent, classify as Round
        float faceExtent = 0.0f;
        for (size_t i = 0; i < face.size(); ++i) {
            for (size_t j = i + 1; j < face.size(); ++j) {
                faceExtent = std::max(faceExtent,
                    glm::length(vertices[face[i]] - vertices[face[j]]));
            }
        }
        float threshold = faceExtent * 0.01f;

        if (maxDist > threshold && face.size() > 3) {
            contourTypes.push_back(ContourType::Round);
            // Estimate curvature from vertex deviation
            ContourCurvature cc;
            float avgDeviation = 0.0f;
            int deviationCount = 0;
            for (size_t i = 3; i < face.size(); ++i) {
                avgDeviation += std::fabs(glm::dot(vertices[face[i]] - v0, normal));
                deviationCount++;
            }
            if (deviationCount > 0) avgDeviation /= deviationCount;
            float radius = (avgDeviation > 1e-8f)
                ? (faceExtent * faceExtent) / (8.0f * avgDeviation) + avgDeviation / 2.0f
                : 0.0f;
            if (radius > 1e-8f) {
                cc.principalK1 = 1.0f / radius;
                cc.principalK2 = 1.0f / radius;
                cc.gaussianCurvature = cc.principalK1 * cc.principalK2;
                cc.meanCurvature = (cc.principalK1 + cc.principalK2) / 2.0f;
            }
            contourCurvatures.push_back(cc);
        } else {
            contourTypes.push_back(ContourType::Flat);
            contourCurvatures.push_back({});
        }
    }
}

std::unique_ptr<Contour> PolyhedronData::buildContour(int faceIndex) const {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faces.size())) return nullptr;

    bool isRound = (faceIndex < static_cast<int>(contourTypes.size()))
                   && contourTypes[faceIndex] == ContourType::Round;

    if (!isRound) {
        auto fc = std::make_unique<FlatContour>(faces[faceIndex], vertices);
        fc->id = faceIndex;
        return fc;
    }

    // For round contours, create a spherical approximation
    const auto& face = faces[faceIndex];
    glm::vec3 centroid(0.0f);
    for (int idx : face) centroid += vertices[idx];
    centroid /= static_cast<float>(face.size());

    float avgR = 0.0f;
    for (int idx : face) avgR += glm::length(vertices[idx] - centroid);
    avgR /= static_cast<float>(face.size());

    auto rc = std::make_unique<RoundContour>();
    rc->id = faceIndex;
    *rc = RoundContour::createSpherical(
        centroid, avgR, 0.0f, static_cast<float>(M_PI),
        0.0f, 2.0f * static_cast<float>(M_PI), face);
    return rc;
}

void PolyhedronData::computeAngleData() {
    if (faceNormals.empty()) computeNormals();
    dihedralAngles = AngleTools::computeDihedralAngles(vertices, faces, faceNormals);
    edgeInfos = AngleTools::computeAllEdgeInfo(vertices, faces, faceNormals);
}

void PolyhedronData::recomputeAll() {
    computeNormals();
    analyzeConvexity();
    computeFaceAreas();
    computeVertexCurvatures();
    generateUVs();
    classifyContours();
    computeAngleData();
    rebuildConvexComponents();
}

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD.h>

void PolyhedronData::rebuildConvexComponents() {
    convexComponents.clear();
    if (vertices.empty() || faces.empty()) return;
    if (isConvex) return;

    // Convert faces to triangles for V-HACD
    std::vector<uint32_t> triangles;
    for (const auto& face : faces) {
        if (face.size() < 3) continue;
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            triangles.push_back(static_cast<uint32_t>(face[0]));
            triangles.push_back(static_cast<uint32_t>(face[i]));
            triangles.push_back(static_cast<uint32_t>(face[i + 1]));
        }
    }

    if (triangles.empty()) return;

    VHACD::IVHACD* interfaceVHACD = VHACD::CreateVHACD();
    VHACD::IVHACD::Parameters params;
    params.m_maxConvexHulls = 32; // reasonable default
    params.m_resolution = 100000;
    params.m_asyncACD = false; // Synchronous

    bool res = interfaceVHACD->Compute(
        &vertices[0].x, static_cast<uint32_t>(vertices.size()),
        triangles.data(), static_cast<uint32_t>(triangles.size() / 3),
        params
    );

    if (res) {
        uint32_t numHulls = interfaceVHACD->GetNConvexHulls();
        for (uint32_t i = 0; i < numHulls; ++i) {
            VHACD::IVHACD::ConvexHull ch;
            if (interfaceVHACD->GetConvexHull(i, ch)) {
                PolyhedronData component;
                component.vertices.reserve(ch.m_points.size());
                for (const auto& pt : ch.m_points) {
                    component.vertices.emplace_back(static_cast<float>(pt.mX), static_cast<float>(pt.mY), static_cast<float>(pt.mZ));
                }

                component.faces.reserve(ch.m_triangles.size());
                for (const auto& tri : ch.m_triangles) {
                    component.faces.push_back({
                        static_cast<int>(tri.mI0),
                        static_cast<int>(tri.mI1),
                        static_cast<int>(tri.mI2)
                    });
                }

                component.ensureOutwardWinding();
                component.computeNormals();
                component.analyzeConvexity();
                component.computeFaceAreas();
                component.computeVertexCurvatures();
                component.generateUVs();
                component.classifyContours();
                component.computeAngleData();
                convexComponents.push_back(component);
            }
        }
    }

    interfaceVHACD->Clean();
    interfaceVHACD->Release();
}


void PolyhedronData::analyzeConvexity() {
    isConvex = true;
    faceConvexity.clear();
    try { faceConvexity.reserve(faces.size()); } catch (...) {}
    
    // Check if polyhedron is convex by testing if all vertices are on the same side of each face
    for (size_t faceIdx = 0; faceIdx < faces.size(); ++faceIdx) {
        const auto& face = faces[faceIdx];
        if (face.size() < 3) {
            faceConvexity.push_back(true);
            continue;
        }
        
        // Compute face normal
        glm::vec3 v0 = vertices[face[0]];
        glm::vec3 v1 = vertices[face[1]];
        glm::vec3 v2 = vertices[face[2]];
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        
        // Test all vertices against this face
        bool faceIsConvex = true;
        float firstSign = 0.0f;
        
        for (size_t vertIdx = 0; vertIdx < vertices.size(); ++vertIdx) {
            // Skip vertices that are part of this face
            bool isPartOfFace = false;
            for (int faceVert : face) {
                if (static_cast<int>(vertIdx) == faceVert) {
                    isPartOfFace = true;
                    break;
                }
            }
            if (isPartOfFace) continue;
            
            // Test if vertex is on the same side as the face normal
            glm::vec3 toVertex = vertices[vertIdx] - v0;
            float sign = glm::dot(normal, toVertex);
            
            if (firstSign == 0.0f) {
                firstSign = sign;
            } else if ((sign > 0) != (firstSign > 0)) {
                faceIsConvex = false;
                isConvex = false;
                break;
            }
        }
        
        faceConvexity.push_back(faceIsConvex);
    }
}

void PolyhedronData::computeFaceAreas() {
    faceAreas.clear();
    try { faceAreas.reserve(faces.size()); } catch (...) {}
    
    for (const auto& face : faces) {
        if (face.size() < 3) {
            faceAreas.push_back(0.0f);
            continue;
        }
        
        // Compute area using cross product method
        float area = 0.0f;
        glm::vec3 v0 = vertices[face[0]];
        
        for (size_t i = 1; i < face.size() - 1; ++i) {
            glm::vec3 v1 = vertices[face[i]];
            glm::vec3 v2 = vertices[face[i + 1]];
            
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            area += glm::length(glm::cross(edge1, edge2)) * 0.5f;
        }
        
        faceAreas.push_back(area);
    }
}

void PolyhedronData::computeVertexCurvatures() {
    vertexCurvatures.clear();
    vertexCurvatures.resize(vertices.size(), 0.0f);
    
    // Compute curvature at each vertex using angle deficit
    for (size_t vertIdx = 0; vertIdx < vertices.size(); ++vertIdx) {
        float angleSum = 0.0f;
        int faceCount = 0;
        
        // Find all faces that contain this vertex
        for (const auto& face : faces) {
            bool containsVertex = false;
            for (int faceVert : face) {
                if (static_cast<int>(vertIdx) == faceVert) {
                    containsVertex = true;
                    break;
                }
            }
            
            if (containsVertex && face.size() >= 3) {
                // Find the angle at this vertex in this face
                for (size_t i = 0; i < face.size(); ++i) {
                    if (static_cast<int>(vertIdx) == face[i]) {
                        int prev = face[(i - 1 + face.size()) % face.size()];
                        int next = face[(i + 1) % face.size()];
                        
                        glm::vec3 v = vertices[vertIdx];
                        glm::vec3 v1 = vertices[prev];
                        glm::vec3 v2 = vertices[next];
                        
                        glm::vec3 edge1 = glm::normalize(v1 - v);
                        glm::vec3 edge2 = glm::normalize(v2 - v);
                        
                        float angle = acos(glm::dot(edge1, edge2));
                        angleSum += angle;
                        faceCount++;
                        break;
                    }
                }
            }
        }
        
        // Curvature is 2π minus the sum of angles
        if (faceCount > 0) {
            vertexCurvatures[vertIdx] = 2.0f * M_PI - angleSum;
        }
    }
}

void PolyhedronData::generateUVs() {
    faceUVs.clear();
    try {
        faceUVs.reserve(faces.size());
    } catch (...) {
        // In case faces.size() is enormous due to corruption, clamp to avoid length_error
        faceUVs.reserve(std::min<size_t>(faces.size(), 100000));
    }
    
    for (size_t i = 0; i < faces.size(); ++i) {
        // Simple planar UV mapping for each face
        // This is a basic implementation - more sophisticated UV mapping could be added
        faceUVs.push_back(glm::vec2(0.0f, 0.0f)); // placeholder UV coordinates
    }
}

void PolyhedronData::addFace(const std::vector<int>& faceVertices) {
    faces.push_back(faceVertices);
    // Recompute normals and other properties
    computeNormals();
    analyzeConvexity();
    computeFaceAreas();
    computeVertexCurvatures();
    rebuildConvexComponents();
}

bool PolyhedronData::validateTopology() const {
    // Stronger closed-manifold validation
    if (faces.empty() || vertices.empty()) return false;

    // Edge usage map: undirected edge -> count
    struct EdgeKey { int a,b; bool operator==(const EdgeKey& o) const { return a==o.a && b==o.b; } }; 
    struct EdgeHasher { size_t operator()(const EdgeKey& k) const { return (static_cast<size_t>(k.a)<<32) ^ static_cast<size_t>(k.b); } };
    std::unordered_map<EdgeKey,int,EdgeHasher> edgeUse;

    auto addEdge = [&](int i, int j){
        EdgeKey k{std::min(i,j), std::max(i,j)};
        edgeUse[k]++;
    };

    for (const auto& f : faces) {
        if (f.size() < 3) return false;
        for (size_t i = 0; i < f.size(); ++i) {
            int v0 = f[i];
            int v1 = f[(i+1)%f.size()];
            if (v0 < 0 || v0 >= static_cast<int>(vertices.size())) return false;
            if (v1 < 0 || v1 >= static_cast<int>(vertices.size())) return false;
            addEdge(v0, v1);
        }
    }
    for (const auto& kv : edgeUse) {
        if (kv.second != 2) return false; // each undirected edge must be used exactly twice
    }

    // Connectivity: BFS over face-adjacency via shared edges
    std::vector<std::vector<Edge>> faceEdges;
    faceEdges.reserve(faces.size());
    for (const auto& f : faces) {
        std::vector<Edge> es; es.reserve(f.size());
        for (size_t i = 0; i < f.size(); ++i) es.emplace_back(f[i], f[(i+1)%f.size()]);
        faceEdges.push_back(std::move(es));
    }
    auto sharesEdge = [&](size_t i, size_t j){
        for (const auto& e1 : faceEdges[i])
            for (const auto& e2 : faceEdges[j])
                if (e1 == e2) return true;
        return false;
    };
    std::vector<char> vis(faces.size(), 0);
    std::vector<size_t> q; q.push_back(0); vis[0]=1;
    for (size_t qi=0; qi<q.size(); ++qi) {
        size_t u = q[qi];
        for (size_t v = 0; v < faces.size(); ++v) if (!vis[v] && sharesEdge(u,v)) { vis[v]=1; q.push_back(v);}    
    }
    for (char c : vis) if (!c) return false; // not connected
    return true;
}
