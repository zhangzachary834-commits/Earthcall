#include "Object.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include "Rendering/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Event structures for Object hover events
struct ObjectHoverEvent {
    const Object& object;
    glm::vec3 hoverPoint;
    glm::vec2 screenPosition;
    std::time_t timestamp;
    
    ObjectHoverEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), hoverPoint(point), screenPosition(screen), timestamp(std::time(nullptr)) {}
};

struct ObjectHoverEnterEvent {
    const Object& object;
    glm::vec3 hoverPoint;
    glm::vec2 screenPosition;
    std::time_t timestamp;
    
    ObjectHoverEnterEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), hoverPoint(point), screenPosition(screen), timestamp(std::time(nullptr)) {}
};

struct ObjectHoverExitEvent {
    const Object& object;
    glm::vec3 lastHoverPoint;
    glm::vec2 lastScreenPosition;
    std::time_t timestamp;
    
    ObjectHoverExitEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), lastHoverPoint(point), lastScreenPosition(screen), timestamp(std::time(nullptr)) {}
};

// Utility: number of logical faces for each geometry type
// Need to implement a more ground-up approach: 
// Getter and setter method for number of sides under a unified Polygonal framework 
// instead of strict shape types
static int numFacesForGeometry(Object::GeometryType t) {
    switch (t) {
        case Object::GeometryType::Cube:      return 6;
        case Object::GeometryType::Sphere:    return 1; // treat entire sphere as one face for now
        case Object::GeometryType::Cylinder:  return 2; // caps+side (simplified)
        case Object::GeometryType::Cone:      return 2; // base + side
        case Object::GeometryType::Polyhedron: return 0; // Will be determined by polyhedronData
    }
    return 1;
}

// Implementation of PolyhedronData methods
Object::PolyhedronData Object::PolyhedronData::createRegularPolyhedron(int numFaces, float radius) {
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
            
        case 6: // Cube (already handled by GeometryType::Cube, but included for completeness)
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
    return data;
}

Object::PolyhedronData Object::PolyhedronData::createCustomPolyhedron(const std::vector<glm::vec3>& verts, 
                                                                      const std::vector<std::vector<int>>& faceDefs) {
    PolyhedronData data;
    data.vertices = verts;
    data.faces = faceDefs;
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    return data;
}

Object::PolyhedronData Object::PolyhedronData::createConcavePolyhedron(int numFaces, float radius, float concavity) {
    PolyhedronData data;
    
    // Start with a regular polyhedron
    data = createRegularPolyhedron(numFaces, radius);
    
    // Apply concavity by pushing vertices inward
    for (auto& vertex : data.vertices) {
        glm::vec3 direction = glm::normalize(vertex);
        float distance = glm::length(vertex);
        
        // Push vertex inward based on concavity
        float newDistance = distance * (1.0f - concavity);
        vertex = direction * newDistance;
    }
    
    // Recompute all properties
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    
    return data;
}

Object::PolyhedronData Object::PolyhedronData::createStarPolyhedron(int numFaces, float radius, float spikeLength) {
    PolyhedronData data;
    
    // Start with a regular polyhedron
    data = createRegularPolyhedron(numFaces, radius);
    
    // Create spikes by extending vertices outward
    for (auto& vertex : data.vertices) {
        glm::vec3 direction = glm::normalize(vertex);
        float distance = glm::length(vertex);
        
        // Extend vertex outward to create spike
        float newDistance = distance + spikeLength;
        vertex = direction * newDistance;
    }
    
    // Recompute all properties
    data.computeNormals();
    data.analyzeConvexity();
    data.computeFaceAreas();
    data.computeVertexCurvatures();
    data.generateUVs();
    
    return data;
}

Object::PolyhedronData Object::PolyhedronData::createCraterPolyhedron(int numFaces, float radius, float craterDepth) {
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
    
    return data;
}

void Object::PolyhedronData::computeNormals() {
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

// Newell's method to compute stable polygon normal
static glm::vec3 computeNewellNormal(const std::vector<glm::vec3>& vertices, const std::vector<int>& face) {
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

void Object::PolyhedronData::ensureOutwardWinding() {
    if (vertices.empty() || faces.empty()) return;
    // Compute polyhedron centroid
    glm::vec3 centroid(0.0f);
    for (const auto& v : vertices) centroid += v;
    centroid /= static_cast<float>(vertices.size());

    for (auto& face : faces) {
        if (face.size() < 3) continue;
        glm::vec3 n = computeNewellNormal(vertices, face);
        glm::vec3 p0 = vertices[face[0]];
        // If normal points toward centroid, flip winding
        if (glm::dot(n, centroid - p0) > 0.0f) {
            std::reverse(face.begin(), face.end());
        }
    }
}

void Object::PolyhedronData::scaleToRadius(float radius) {
    if (vertices.empty()) return;
    float maxR = 0.0f;
    for (const auto& v : vertices) maxR = std::max(maxR, glm::length(v));
    if (maxR < 1e-8f) return;
    float s = radius / maxR;
    for (auto& v : vertices) v *= s;
}

void Object::PolyhedronData::analyzeConvexity() {
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

void Object::PolyhedronData::computeFaceAreas() {
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

void Object::PolyhedronData::computeVertexCurvatures() {
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

void Object::PolyhedronData::generateUVs() {
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

void Object::PolyhedronData::addFace(const std::vector<int>& faceVertices) {
    faces.push_back(faceVertices);
    // Recompute normals and other properties
    computeNormals();
    analyzeConvexity();
    computeFaceAreas();
    computeVertexCurvatures();
}

bool Object::PolyhedronData::validateTopology() const {
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

int Object::getDimensions() {
    return dimensions;
}

void Object::setDimensions(int d) {
    dimensions = d;
}

int Object::getCorners() {
    if (geometryType == GeometryType::Polyhedron) {
        return polyhedronData.getVertexCount();
    }
    return corners;
}

void Object::setCorners(int c) {
    corners = c;
    // For polyhedrons, this could trigger a regeneration of the polyhedron
    // For now, we'll just store the value for compatibility
}

int Object::getFaces() {
    if (geometryType == GeometryType::Polyhedron) {
        return polyhedronData.getFaceCount();
    }
    return faces;
}

void Object::setFaces(int f) {
    faces = f;
    // For polyhedrons, this could trigger a regeneration of the polyhedron
    // For now, we'll just store the value for compatibility
}

int Object::getMassQuantity() {
    return massQuantity;
}

void Object::setMassQuantity(int m) {
    massQuantity = m;
}

int Object::getElements() {
    return elements;
}

void Object::setElements(int e) {
    elements = e;
}

int Object::getRelationships() {
    return relationships;
}

void Object::setRelationships(int r) {
    relationships = r;
}

int Object::getComplexityLevel() {
    return complexityLevel;
}

void Object::setComplexityLevel(int cl) {
    complexityLevel = cl;
}

int Object::getPhysicalObject() {
    return physicalObject ? 1 : 0;
}

void Object::setPhysicalObject(int po) {
    physicalObject = (po != 0);
}

int Object::getSymbolicObject() {
    return physicalObject ? 0 : 1; // Inverse of physical object
}

void Object::setSymbolicObject(int so) {
    physicalObject = (so == 0); // Inverse of symbolic object
}

std::string Object::getObjectID() {
    return objectID;
}

void Object::setObjectID(int oi) {
    objectID = std::to_string(oi);
}

std::string Object::getObjectType() const {
    return objectType;
}

void Object::setObjectType(int ot) {
    objectType = std::to_string(ot);
}

int Object::getX() {
    return x;
}

void Object::setX(int x) {
    this->x = x;
}

std::string Object::screenMode() {

    if (dimensions == 2.0f) {
        return "2D";
    } else if (dimensions == 3.0f) {
        return "3D";
    } else {
        return "Unknown";
    }

}

void Object::initFaceTextures() {
    int n;
    if (geometryType == GeometryType::Polyhedron) {
        n = polyhedronData.getFaceCount();
    } else {
        n = numFacesForGeometry(geometryType);
    }
    faceTextures.resize(n);

    // Default colour per face similar to previous defaults (RGB)
    static const float defaultCols[6][3] = {
        {1.f,0.f,0.f}, {1.f,0.f,0.f}, {0.f,1.f,0.f}, {0.f,1.f,0.f}, {0.f,0.f,1.f}, {0.f,0.f,1.f}
    };

    for (int i = 0; i < n; ++i) {
        const float* c = defaultCols[i % 6];
        uint8_t r = static_cast<uint8_t>(c[0] * 255);
        uint8_t g = static_cast<uint8_t>(c[1] * 255);
        uint8_t b = static_cast<uint8_t>(c[2] * 255);
        uint8_t a = 255;
        uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
        faceTextures[i].create(rgba);
    }
}

void Object::fillFaceColor(int faceIndex, float r, float g, float b) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    FaceTexture& tex = faceTextures[faceIndex];
    uint8_t R = static_cast<uint8_t>(std::clamp(r, 0.f, 1.f) * 255);
    uint8_t G = static_cast<uint8_t>(std::clamp(g, 0.f, 1.f) * 255);
    uint8_t B = static_cast<uint8_t>(std::clamp(b, 0.f, 1.f) * 255);
    uint8_t A = 255;
    for (size_t i = 0; i < tex.pixels.size(); i += 4) {
        tex.pixels[i] = R;
        tex.pixels[i+1] = G;
        tex.pixels[i+2] = B;
        tex.pixels[i+3] = A;
    }
    tex.updateWholeGPU();

    if(faceIndex>=0 && faceIndex<6){
        faceColors[faceIndex][0] = r;
        faceColors[faceIndex][1] = g;
        faceColors[faceIndex][2] = b;
    }
}

void Object::paintFace(int faceIndex, const glm::vec2& uv, float r, float g, float b, float radius, float softness) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    FaceTexture& tex = faceTextures[faceIndex];
    int size = tex.size;
    int cx = static_cast<int>(uv.x * size);
    // int cy = static_cast<int>((1.0f - uv.y) * size); // flip Y so UV origin at bottom-left
    int cy = static_cast<int>(uv.y * size);
    int radPx = static_cast<int>(radius * size);

    // Debug: print pixel coordinates
    printf("UV (%.2f,%.2f) -> Pixel (%d,%d) size=%d\n", uv.x, uv.y, cx, cy, size);

    uint8_t R = static_cast<uint8_t>(std::clamp(r, 0.f, 1.f) * 255);
    uint8_t G = static_cast<uint8_t>(std::clamp(g, 0.f, 1.f) * 255);
    uint8_t B = static_cast<uint8_t>(std::clamp(b, 0.f, 1.f) * 255);

    int x0 = std::max(0, cx - radPx);
    int x1 = std::min(size - 1, cx + radPx);
    int y0 = std::max(0, cy - radPx);
    int y1 = std::min(size - 1, cy + radPx);
    int radSq = radPx * radPx;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            int distSq = dx*dx + dy*dy;
            if (distSq <= radSq) {
                float t = 1.0f;
                if(softness < 0.99f){
                    float distNorm = std::sqrt(static_cast<float>(distSq)) / static_cast<float>(radPx);
                    t = std::clamp(1.0f - distNorm, 0.0f, 1.0f);
                    t = std::pow(t, 1.0f / std::max(0.001f, softness));
                }
                size_t idx = (y * size + x) * 4;
                uint8_t* dst = &tex.pixels[idx];
                float inv = 1.0f - t;
                dst[0] = static_cast<uint8_t>(dst[0]*inv + R*t);
                dst[1] = static_cast<uint8_t>(dst[1]*inv + G*t);
                dst[2] = static_cast<uint8_t>(dst[2]*inv + B*t);
                dst[3] = 255;
            }
        }
    }
    tex.updateWholeGPU();
}

void Object::paintFaceAdvanced(int faceIndex, const glm::vec2& uv, float r, float g, float b, 
                              float radius, float softness, float opacity, float flow, int brushType) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    FaceTexture& tex = faceTextures[faceIndex];
    
    // Save stroke state for undo
    tex.saveStrokeState();
    
    // Add stroke point to history
    if (tex.activeLayer >= 0 && tex.activeLayer < static_cast<int>(tex.strokeHistory.size())) {
        FaceTexture::StrokePoint point;
        point.uv = uv;
        point.radius = radius;
        point.opacity = opacity;
        point.color = glm::vec3(r, g, b);
        point.timestamp = static_cast<float>(glfwGetTime());
        tex.strokeHistory[tex.activeLayer].push_back(point);
    }
    
    int size = tex.size;
    int cx = static_cast<int>(uv.x * size);
    // int cy = static_cast<int>((1.0f - uv.y) * size);
    int cy = static_cast<int>(uv.y * size);
    int radPx = static_cast<int>(radius * size);
    
    uint8_t R = static_cast<uint8_t>(std::clamp(r, 0.f, 1.f) * 255);
    uint8_t G = static_cast<uint8_t>(std::clamp(g, 0.f, 1.f) * 255);
    uint8_t B = static_cast<uint8_t>(std::clamp(b, 0.f, 1.f) * 255);
    
    int x0 = std::max(0, cx - radPx);
    int x1 = std::min(size - 1, cx + radPx);
    int y0 = std::max(0, cy - radPx);
    int y1 = std::min(size - 1, cy + radPx);
    int radSq = radPx * radPx;
    
    std::vector<uint8_t>& targetBuffer = tex.useLayers ? tex.layers[tex.activeLayer] : tex.pixels;
    
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            int distSq = dx*dx + dy*dy;
            if (distSq <= radSq) {
                float t = 1.0f;
                if(softness < 0.99f){
                    float distNorm = std::sqrt(static_cast<float>(distSq)) / static_cast<float>(radPx);
                    t = std::clamp(1.0f - distNorm, 0.0f, 1.0f);
                    t = std::pow(t, 1.0f / std::max(0.001f, softness));
                }
                
                // Apply brush type effects
                switch (brushType) {
                    case 1: // Airbrush
                        t *= (0.5f + 0.5f * (static_cast<float>(rand()) / RAND_MAX));
                        break;
                    case 2: // Chalk
                        t *= (0.3f + 0.7f * (static_cast<float>(rand()) / RAND_MAX));
                        break;
                    case 3: // Spray
                        if (static_cast<float>(rand()) / RAND_MAX > 0.7f) {
                            t *= 0.3f;
                        }
                        break;
                }
                
                t *= opacity * flow;
                size_t idx = (y * size + x) * 4;
                uint8_t* dst = &targetBuffer[idx];
                float inv = 1.0f - t;
                dst[0] = static_cast<uint8_t>(dst[0]*inv + R*t);
                dst[1] = static_cast<uint8_t>(dst[1]*inv + G*t);
                dst[2] = static_cast<uint8_t>(dst[2]*inv + B*t);
                dst[3] = 255;
            }
        }
    }
    tex.updateWholeGPU();
}

void Object::paintStroke(int faceIndex, const glm::vec2& startUV, const glm::vec2& endUV, 
                         float r, float g, float b, float radius, float softness, 
                         float opacity, float spacing) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    
    float distance = glm::length(endUV - startUV);
    int steps = static_cast<int>(distance / spacing) + 1;
    
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        glm::vec2 uv = glm::mix(startUV, endUV, t);
        paintFaceAdvanced(faceIndex, uv, r, g, b, radius, softness, opacity, 1.0f, 0);
    }
}

void Object::smudgeFace(int faceIndex, const glm::vec2& uv, float radius, float strength) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    FaceTexture& tex = faceTextures[faceIndex];
    
    int size = tex.size;
    int cx = static_cast<int>(uv.x * size);
    // int cy = static_cast<int>((1.0f - uv.y) * size);
    int cy = static_cast<int>(uv.y * size);
    int radPx = static_cast<int>(radius * size);
    
    int x0 = std::max(0, cx - radPx);
    int x1 = std::min(size - 1, cx + radPx);
    int y0 = std::max(0, cy - radPx);
    int y1 = std::min(size - 1, cy + radPx);
    int radSq = radPx * radPx;
    
    std::vector<uint8_t>& targetBuffer = tex.useLayers ? tex.layers[tex.activeLayer] : tex.pixels;
    
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            int distSq = dx*dx + dy*dy;
            if (distSq <= radSq) {
                float t = 1.0f - std::sqrt(static_cast<float>(distSq)) / static_cast<float>(radPx);
                t = std::clamp(t, 0.0f, 1.0f) * strength;
                
                size_t idx = (y * size + x) * 4;
                uint8_t* dst = &targetBuffer[idx];
                
                // Blend with neighboring pixels
                glm::vec3 avgColor(0.0f);
                int samples = 0;
                for (int sy = -1; sy <= 1; ++sy) {
                    for (int sx = -1; sx <= 1; ++sx) {
                        int nx = x + sx;
                        int ny = y + sy;
                        if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                            size_t nidx = (ny * size + nx) * 4;
                            avgColor += glm::vec3(targetBuffer[nidx]/255.0f, 
                                                 targetBuffer[nidx+1]/255.0f, 
                                                 targetBuffer[nidx+2]/255.0f);
                            samples++;
                        }
                    }
                }
                if (samples > 0) {
                    avgColor /= static_cast<float>(samples);
                    dst[0] = static_cast<uint8_t>((dst[0]*(1.0f-t) + avgColor.r*255*t));
                    dst[1] = static_cast<uint8_t>((dst[1]*(1.0f-t) + avgColor.g*255*t));
                    dst[2] = static_cast<uint8_t>((dst[2]*(1.0f-t) + avgColor.b*255*t));
                }
            }
        }
    }
    tex.updateWholeGPU();
}

void Object::cloneFace(int faceIndex, const glm::vec2& destUV, const glm::vec2& sourceUV, 
                       float radius, float opacity) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    FaceTexture& tex = faceTextures[faceIndex];
    
    int size = tex.size;
    int destX = static_cast<int>(destUV.x * size);
    // int destY = static_cast<int>((1.0f - destUV.y) * size);
    int destY = static_cast<int>(destUV.y * size);
    int sourceX = static_cast<int>(sourceUV.x * size);
    // int sourceY = static_cast<int>((1.0f - sourceUV.y) * size);
    int sourceY = static_cast<int>(sourceUV.y * size);
    int radPx = static_cast<int>(radius * size);
    
    int x0 = std::max(0, destX - radPx);
    int x1 = std::min(size - 1, destX + radPx);
    int y0 = std::max(0, destY - radPx);
    int y1 = std::min(size - 1, destY + radPx);
    int radSq = radPx * radPx;
    
    std::vector<uint8_t>& targetBuffer = tex.useLayers ? tex.layers[tex.activeLayer] : tex.pixels;
    
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - destX;
            int dy = y - destY;
            int distSq = dx*dx + dy*dy;
            if (distSq <= radSq) {
                float t = 1.0f - std::sqrt(static_cast<float>(distSq)) / static_cast<float>(radPx);
                t = std::clamp(t, 0.0f, 1.0f) * opacity;
                
                int sx = sourceX + dx;
                int sy = sourceY + dy;
                if (sx >= 0 && sx < size && sy >= 0 && sy < size) {
                    size_t destIdx = (y * size + x) * 4;
                    size_t sourceIdx = (sy * size + sx) * 4;
                    uint8_t* dst = &targetBuffer[destIdx];
                    uint8_t* src = &targetBuffer[sourceIdx];
                    
                    dst[0] = static_cast<uint8_t>(dst[0]*(1.0f-t) + src[0]*t);
                    dst[1] = static_cast<uint8_t>(dst[1]*(1.0f-t) + src[1]*t);
                    dst[2] = static_cast<uint8_t>(dst[2]*(1.0f-t) + src[2]*t);
                    dst[3] = 255;
                }
            }
        }
    }
    tex.updateWholeGPU();
}

void Object::airbrushFace(int faceIndex, const glm::vec2& uv, float r, float g, float b, 
                          float radius, float density, float opacity) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(faceTextures.size())) return;
    FaceTexture& tex = faceTextures[faceIndex];
    
    int size = tex.size;
    int cx = static_cast<int>(uv.x * size);
    // int cy = static_cast<int>((1.0f - uv.y) * size);
    int cy = static_cast<int>(uv.y * size);
    int radPx = static_cast<int>(radius * size);
    
    uint8_t R = static_cast<uint8_t>(std::clamp(r, 0.f, 1.f) * 255);
    uint8_t G = static_cast<uint8_t>(std::clamp(g, 0.f, 1.f) * 255);
    uint8_t B = static_cast<uint8_t>(std::clamp(b, 0.f, 1.f) * 255);
    
    std::vector<uint8_t>& targetBuffer = tex.useLayers ? tex.layers[tex.activeLayer] : tex.pixels;
    
    // Generate multiple particles for airbrush effect
    int particles = static_cast<int>(density * 50.0f);
    for (int p = 0; p < particles; ++p) {
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
        float dist = static_cast<float>(rand()) / RAND_MAX * radPx;
        int x = cx + static_cast<int>(cos(angle) * dist);
        int y = cy + static_cast<int>(sin(angle) * dist);
        
        if (x >= 0 && x < size && y >= 0 && y < size) {
            float t = opacity * (1.0f - static_cast<float>(rand()) / RAND_MAX * 0.5f);
            size_t idx = (y * size + x) * 4;
            uint8_t* dst = &targetBuffer[idx];
            float inv = 1.0f - t;
            dst[0] = static_cast<uint8_t>(dst[0]*inv + R*t);
            dst[1] = static_cast<uint8_t>(dst[1]*inv + G*t);
            dst[2] = static_cast<uint8_t>(dst[2]*inv + B*t);
            dst[3] = 255;
        }
    }
    tex.updateWholeGPU();
}

// Layer management methods
void Object::addTextureLayer(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].addLayer();
        faceTextures[faceIndex].useLayers = true;
    }
}

void Object::deleteTextureLayer(int faceIndex, int layerIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].deleteLayer(layerIndex);
    }
}

void Object::setActiveLayer(int faceIndex, int layerIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].activeLayer = layerIndex;
    }
}

void Object::setLayerOpacity(int faceIndex, int layerIndex, float opacity) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].setLayerOpacity(layerIndex, opacity);
    }
}

void Object::setBlendMode(int faceIndex, int layerIndex, int mode) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].setBlendMode(layerIndex, mode);
    }
}

// Undo/Redo methods
void Object::saveStrokeState(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].saveStrokeState();
    }
}

void Object::undoStroke(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        faceTextures[faceIndex].undo();
    }
}

void Object::clearStrokeHistory(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(faceTextures.size())) {
        FaceTexture& tex = faceTextures[faceIndex];
        for (auto& history : tex.strokeHistory) {
            history.clear();
        }
        for (auto& stack : tex.undoStack) {
            stack.clear();
        }
    }
}

// ---------------------------------------------------------------------
// Modified drawCube to bind per-face texture
// ---------------------------------------------------------------------

void Object::drawCube() const {
    static const struct { GLfloat nx, ny, nz; GLfloat vx[4][3]; } faceData[6] = {
        { 1,0,0,  { {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {0.5,0.5,0.5}, {0.5,-0.5,0.5} } }, // +X
        {-1,0,0,  { {-0.5,-0.5,-0.5}, {-0.5,-0.5,0.5}, {-0.5,0.5,0.5}, {-0.5,0.5,-0.5} } }, // -X
        { 0,1,0,  { {-0.5,0.5,-0.5}, {-0.5,0.5,0.5}, {0.5,0.5,0.5}, {0.5,0.5,-0.5} } }, // +Y
        { 0,-1,0, { {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,-0.5,0.5}, {-0.5,-0.5,0.5} } }, // -Y
        { 0,0,1,  { {-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5} } }, // +Z
        { 0,0,-1, { {-0.5,-0.5,-0.5}, {-0.5,0.5,-0.5}, {0.5,0.5,-0.5}, {0.5,-0.5,-0.5} } }  // -Z
    };

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f,1.0f,1.0f);
    for (int f = 0; f < 6 && f < static_cast<int>(faceTextures.size()); ++f) {
        const FaceTexture& tex = faceTextures[f];
        glBindTexture(GL_TEXTURE_2D, tex.id);
    glBegin(GL_QUADS);
        glNormal3f(faceData[f].nx, faceData[f].ny, faceData[f].nz);
        glTexCoord2f(0,0); glVertex3fv(faceData[f].vx[0]);
        glTexCoord2f(1,0); glVertex3fv(faceData[f].vx[1]);
        glTexCoord2f(1,1); glVertex3fv(faceData[f].vx[2]);
        glTexCoord2f(0,1); glVertex3fv(faceData[f].vx[3]);
    glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}

// Helper to draw a smooth shaded sphere
static void drawSpherePrimitive() {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);
    gluSphere(quad, 0.5f, 16, 16);
    gluDeleteQuadric(quad);
}

// Helper to draw a cylinder primitive of height 1 (centered at origin)
static void drawCylinderPrimitive(float topRadius) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);
    gluCylinder(quad, 0.5f, topRadius, 1.0f, 16, 4);
    gluDeleteQuadric(quad);
}

void Object::drawObject() const {
    switch (geometryType) {
        case GeometryType::Cube:
            drawCube();
            break;
        case GeometryType::Sphere:
        {
            glEnable(GL_TEXTURE_2D);
            if (!faceTextures.empty()) {
                glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
            }
            glColor3f(1.0f, 1.0f, 1.0f);
            drawSpherePrimitive();
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case GeometryType::Cylinder:
        {
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
            // Center cylinder along Z in [-0.5, 0.5]
            glTranslatef(0.0f, 0.0f, -0.5f);

            // Draw side
            if (faceTextures.size() >= 1) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
            drawCylinderPrimitive(0.5f);

            // Draw caps with second face texture if available
            GLUquadric* disk = gluNewQuadric();
            gluQuadricTexture(disk, GL_TRUE);
            if (faceTextures.size() >= 2) glBindTexture(GL_TEXTURE_2D, faceTextures[1].id);
            // Bottom cap at z = 0 (world z = -0.5) - outward normal should be -Z
            glPushMatrix();
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f); // flip to face -Z
            gluDisk(disk, 0.0f, 0.5f, 32, 1);
            glPopMatrix();
            // Top cap at z = 1 (world z = +0.5) - outward normal +Z
            glPushMatrix();
            glTranslatef(0.0f, 0.0f, 1.0f);
            gluDisk(disk, 0.0f, 0.5f, 32, 1);
            glPopMatrix();
            gluDeleteQuadric(disk);

            glPopMatrix();
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case GeometryType::Cone:
        {
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
            // Center cone along Z in [-0.5, 0.5] (base at -0.5, apex at +0.5)
            glTranslatef(0.0f, 0.0f, -0.5f);

            // Draw side
            if (faceTextures.size() >= 1) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
            drawCylinderPrimitive(0.0f); // top radius 0 = cone

            // Draw base disk with second face texture if available
            if (faceTextures.size() >= 2) glBindTexture(GL_TEXTURE_2D, faceTextures[1].id);
            GLUquadric* disk = gluNewQuadric();
            gluQuadricTexture(disk, GL_TRUE);
            glPushMatrix();
            // Base is at local z=0 (world z = -0.5). Outward normal should be -Z → flip the disk.
            // This fixes the cap appearing on the wrong side.
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            gluDisk(disk, 0.0f, 0.5f, 32, 1);
            glPopMatrix();
            gluDeleteQuadric(disk);

            glPopMatrix();
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case GeometryType::Polyhedron:
            drawPolyhedron();
            break;
    }
}

// Render a glowing outline around the object's collision zone using multiple scaled passes
void Object::drawHighlightOutline() const {
    using Rendering::HighlightSystem;
    bool sel = HighlightSystem::isSelected(this);
    bool cand = HighlightSystem::isLawCandidate(this);
    if (!sel && !cand) return;

    // Choose color: yellow for selection, red for law-candidate
    glm::vec3 color = sel ? glm::vec3(1.0f, 0.9f, 0.2f) : glm::vec3(1.0f, 0.2f, 0.2f);

    // Draw 3-4 inflated shells of collision AABB as wireframes for a soft glow effect
    // Tailored to the object shape via its collisionZone corners (AABB). For more complex shapes, this can be extended.
    glm::vec3 minCorner = collisionZone.corners[0];
    glm::vec3 maxCorner = collisionZone.corners[0];
    for (int i = 1; i < 8; ++i) {
        minCorner = glm::min(minCorner, collisionZone.corners[i]);
        maxCorner = glm::max(maxCorner, collisionZone.corners[i]);
    }

    // Compute center and half-extent
    glm::vec3 center = (minCorner + maxCorner) * 0.5f;
    glm::vec3 half   = (maxCorner - minCorner) * 0.5f;

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Multiple passes for thickness + soft glow
    const int passes = 4;
    for (int p = 0; p < passes; ++p) {
        float t = (float)(p+1) / (float)passes;
        float inflate = 0.02f * (p+1); // expand a bit each pass
        float alpha = 0.5f * (1.0f - (float)p / (float)passes); // fade outer passes
        glm::vec3 ext = half + glm::vec3(inflate);
        glm::vec3 c = color * (0.8f + 0.2f * (1.0f - t));
        glColor4f(c.r, c.g, c.b, alpha);
        glLineWidth(2.5f + p * 1.2f);

        // Draw 12 edges of the inflated AABB wireframe
        auto drawEdge = [&](const glm::vec3& a, const glm::vec3& b){
            glBegin(GL_LINES);
            glVertex3f(a.x, a.y, a.z);
            glVertex3f(b.x, b.y, b.z);
            glEnd();
        };
        glm::vec3 v[8];
        v[0] = center + glm::vec3(-ext.x, -ext.y, -ext.z);
        v[1] = center + glm::vec3( ext.x, -ext.y, -ext.z);
        v[2] = center + glm::vec3( ext.x,  ext.y, -ext.z);
        v[3] = center + glm::vec3(-ext.x,  ext.y, -ext.z);
        v[4] = center + glm::vec3(-ext.x, -ext.y,  ext.z);
        v[5] = center + glm::vec3( ext.x, -ext.y,  ext.z);
        v[6] = center + glm::vec3( ext.x,  ext.y,  ext.z);
        v[7] = center + glm::vec3(-ext.x,  ext.y,  ext.z);
        // bottom rectangle
        drawEdge(v[0], v[1]); drawEdge(v[1], v[2]); drawEdge(v[2], v[3]); drawEdge(v[3], v[0]);
        // top rectangle
        drawEdge(v[4], v[5]); drawEdge(v[5], v[6]); drawEdge(v[6], v[7]); drawEdge(v[7], v[4]);
        // verticals
        drawEdge(v[0], v[4]); drawEdge(v[1], v[5]); drawEdge(v[2], v[6]); drawEdge(v[3], v[7]);
    }
    glPopAttrib();
}

bool Object::raycastFace(const glm::vec3& rayOriginWorld, const glm::vec3& rayDirWorld,
                         float& outT, int& outFaceIndex, glm::vec2& outUV) const {
    // Transform ray to local space
    glm::mat4 inv = glm::inverse(getTransform());
    glm::vec3 oL = glm::vec3(inv * glm::vec4(rayOriginWorld, 1.0f));
    glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(rayDirWorld, 0.0f)));

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
        // Cylinder axis along Z, radius 0.5, height z in [0,1] (matching drawCylinderPrimitive offset by -0.5)
        const float r = 0.5f;
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);

        // Side: x^2 + y^2 = r^2, z in [0,1]
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
                        if (p.z >= 0.0f && p.z <= 1.0f) {
                            float u = 0.5f + atan2f(p.y, p.x) / (2.0f * (float)M_PI);
                            float v = glm::clamp(p.z, 0.0f, 1.0f);
                            if (t < bestT) { bestT = t; bestFace = 0; bestUV = glm::vec2(u, v); }
                        }
                    }
                };
                testT(tA); testT(tB);
            }
        }

        // Caps at z = 0 and z = 1 share one texture (face 1)
        if (fabs(dL.z) > 1e-6f) {
            for (int sgn = 0; sgn <= 1; ++sgn) {
                float zPlane = sgn == 0 ? 0.0f : 1.0f;
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
        // Cone axis +Z, height 1, base at z=0, apex at z=1, base radius 0.5
        // Matches drawCylinderPrimitive(0.0f) with pre-translate -0.5
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);

        // Side: for cone along +Z, implicit: x^2 + y^2 = (k z)^2 with k = r/h = 0.5/1 = 0.5, and 0<=z<=1
        float k = 0.5f;
        float A = dL.x * dL.x + dL.y * dL.y - (k * k) * dL.z * dL.z;
        float B = 2.0f * (oL.x * dL.x + oL.y * dL.y - (k * k) * oL.z * dL.z);
        float C = oL.x * oL.x + oL.y * oL.y - (k * k) * oL.z * oL.z;
        if (fabs(A) > 1e-6f) {
            float disc = B * B - 4.0f * A * C;
            if (disc >= 0.0f) {
                float s = sqrtf(disc);
                float tA = (-B - s) / (2.0f * A);
                float tB = (-B + s) / (2.0f * A);
                auto testT = [&](float t) {
                    if (t > 1e-6f) {
                        glm::vec3 p = oL + dL * t;
                        if (p.z >= 0.0f && p.z <= 1.0f) {
                            float theta = atan2f(p.y, p.x);
                            float u = 0.5f + theta / (2.0f * (float)M_PI);
                            float v = glm::clamp(p.z, 0.0f, 1.0f); // 0 at base, 1 at tip
                            if (t < bestT) { bestT = t; bestFace = 0; bestUV = glm::vec2(u, v); }
                        }
                    }
                };
                testT(tA); testT(tB);
            }
        }

        // Base disc at z = 0, radius 0.5 (face 1)
        if (fabs(dL.z) > 1e-6f) {
            float t = (0.0f - oL.z) / dL.z;
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
    switch (geometryType) {
        case GeometryType::Cube: {
            float t; int f; glm::vec2 uv;
            if (intersectAABBUnitCube(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
        case GeometryType::Sphere: {
            float t; glm::vec2 uv;
            if (intersectSphere(t, uv)) { bestT = t; bestFace = 0; bestUV = uv; hit = true; }
            break;
        }
        case GeometryType::Cylinder: {
            float t; int f; glm::vec2 uv;
            if (intersectCylinder(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
        case GeometryType::Cone: {
            float t; int f; glm::vec2 uv;
            if (intersectCone(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
        case GeometryType::Polyhedron: {
            float t; int f; glm::vec2 uv;
            if (intersectPolyhedron(t, f, uv)) { bestT = t; bestFace = f; bestUV = uv; hit = true; }
            break;
        }
    }

    if (hit) { outT = bestT; outFaceIndex = bestFace; outUV = bestUV; return true; }
    return false;
}

void Object::updateCollisionZone(const glm::mat4& transform) const {
    if (geometryType == GeometryType::Polyhedron && !polyhedronData.vertices.empty()) {
        // For polyhedrons, compute bounding box from all vertices
        glm::vec3 minCorner = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxCorner = glm::vec3(-std::numeric_limits<float>::max());
        
        for (const auto& vertex : polyhedronData.vertices) {
            glm::vec4 world = transform * glm::vec4(vertex, 1.0f);
            glm::vec3 worldVertex = glm::vec3(world);
            minCorner = glm::min(minCorner, worldVertex);
            maxCorner = glm::max(maxCorner, worldVertex);
        }
        
        // Create bounding box corners
        collisionZone.corners[0] = glm::vec3(minCorner.x, minCorner.y, minCorner.z);
        collisionZone.corners[1] = glm::vec3(maxCorner.x, minCorner.y, minCorner.z);
        collisionZone.corners[2] = glm::vec3(maxCorner.x, maxCorner.y, minCorner.z);
        collisionZone.corners[3] = glm::vec3(minCorner.x, maxCorner.y, minCorner.z);
        collisionZone.corners[4] = glm::vec3(minCorner.x, minCorner.y, maxCorner.z);
        collisionZone.corners[5] = glm::vec3(maxCorner.x, minCorner.y, maxCorner.z);
        collisionZone.corners[6] = glm::vec3(maxCorner.x, maxCorner.y, maxCorner.z);
        collisionZone.corners[7] = glm::vec3(minCorner.x, maxCorner.y, maxCorner.z);
    } else {
        // Local-space corners of a unit cube centered at origin (legacy behavior)
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
    }
}

bool Object::isPointInside(const glm::vec3& point) const {
    // Compute AABB from corners
    glm::vec3 minCorner = collisionZone.corners[0];
    glm::vec3 maxCorner = collisionZone.corners[0];
    for (int i = 1; i < 8; ++i) {
        minCorner = glm::min(minCorner, collisionZone.corners[i]);
        maxCorner = glm::max(maxCorner, collisionZone.corners[i]);
    }
    return (point.x >= minCorner.x && point.x <= maxCorner.x &&
            point.y >= minCorner.y && point.y <= maxCorner.y &&
            point.z >= minCorner.z && point.z <= maxCorner.z);
}

void Object::drawPolyhedron() const {
    if (polyhedronData.vertices.empty() || polyhedronData.faces.empty()) {
        return; // No polyhedron data to draw
    }
    
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Draw each face of the polyhedron
    for (size_t faceIndex = 0; faceIndex < polyhedronData.faces.size(); ++faceIndex) {
        const auto& face = polyhedronData.faces[faceIndex];
        if (face.size() < 3) continue; // Skip invalid faces
        
        // Bind texture for this face if available
        if (faceIndex < faceTextures.size()) {
            const FaceTexture& tex = faceTextures[faceIndex];
            glBindTexture(GL_TEXTURE_2D, tex.id);
        }
        
        // Compute per-face tangent space and UVs consistent with raycast mapping, and use Newell normal
        glm::vec3 v0 = polyhedronData.vertices[face[0]];
        glm::vec3 normal = computeNewellNormal(polyhedronData.vertices, face);
        glm::vec3 tangent = glm::normalize(glm::cross(fabs(normal.y) < 0.99f ? glm::vec3(0,1,0) : glm::vec3(1,0,0), normal));
        glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

        float minU = 1e9f, maxU = -1e9f, minV = 1e9f, maxV = -1e9f;
        std::vector<glm::vec2> projected;
        projected.reserve(face.size());
        for (int idx : face) {
            const glm::vec3& v = polyhedronData.vertices[idx];
            float u = glm::dot(v - v0, tangent);
            float vv = glm::dot(v - v0, bitangent);
            projected.emplace_back(u, vv);
            minU = std::min(minU, u); maxU = std::max(maxU, u);
            minV = std::min(minV, vv); maxV = std::max(maxV, vv);
        }

        // Triangulate face with a fan around centroid to avoid GL_POLYGON issues
        glm::vec3 centroid(0.0f);
        for (int idx : face) centroid += polyhedronData.vertices[idx];
        centroid /= static_cast<float>(face.size());
        float du = std::max(1e-6f, maxU - minU);
        float dv = std::max(1e-6f, maxV - minV);
        float cU = (glm::dot(centroid - v0, tangent) - minU) / du;
        float cV = (glm::dot(centroid - v0, bitangent) - minV) / dv;

        glBegin(GL_TRIANGLES);
        glNormal3f(normal.x, normal.y, normal.z);
        for (size_t i = 0; i < face.size(); ++i) {
            size_t i0 = i;
            size_t i1 = (i + 1) % face.size();
            int vi0 = face[i0];
            int vi1 = face[i1];
            if (vi0 < 0 || vi0 >= static_cast<int>(polyhedronData.vertices.size())) continue;
            if (vi1 < 0 || vi1 >= static_cast<int>(polyhedronData.vertices.size())) continue;
            const glm::vec3& p0 = polyhedronData.vertices[vi0];
            const glm::vec3& p1 = polyhedronData.vertices[vi1];

            glm::vec2 proj0 = projected[i0];
            glm::vec2 proj1 = projected[i1];
            float u0 = (proj0.x - minU) / du; float v0uv = (proj0.y - minV) / dv;
            float u1 = (proj1.x - minU) / du; float v1uv = (proj1.y - minV) / dv;

            glTexCoord2f(cU, cV); glVertex3f(centroid.x, centroid.y, centroid.z);
            glTexCoord2f(u0, v0uv); glVertex3f(p0.x, p0.y, p0.z);
            glTexCoord2f(u1, v1uv); glVertex3f(p1.x, p1.y, p1.z);
        }
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Polyhedron-specific methods
void Object::setPolyhedronData(const PolyhedronData& data) {
    polyhedronData = data;
    if (geometryType == GeometryType::Polyhedron) {
        initFaceTextures();
    }
}

void Object::createTetrahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(4);
    initFaceTextures();
}

void Object::createOctahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(8);
    initFaceTextures();
}

void Object::createDodecahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(12);
    initFaceTextures();
}

void Object::createIcosahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(20);
    initFaceTextures();
}

void Object::createCustomPolyhedron(const std::vector<glm::vec3>& vertices, 
                                   const std::vector<std::vector<int>>& faces) {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createCustomPolyhedron(vertices, faces);
    initFaceTextures();
}

Object::Object() {
    initFaceTextures();
}

// Hover detection method implementations
bool Object::isMouseHovering(const glm::vec2& mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int windowWidth, int windowHeight) const {
    // Convert screen coordinates to world coordinates
    glm::vec4 screenPos(mousePos.x, mousePos.y, 0.0f, 1.0f);
    
    // Convert to normalized device coordinates
    screenPos.x = (screenPos.x / windowWidth) * 2.0f - 1.0f;
    screenPos.y = (screenPos.y / windowHeight) * 2.0f - 1.0f;
    screenPos.y = -screenPos.y; // Flip Y coordinate
    
    // Create ray from camera through mouse position
    glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);
    glm::vec4 worldPos = invVP * screenPos;
    worldPos /= worldPos.w;
    
    glm::vec3 rayOrigin = glm::vec3(invVP * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::vec3 rayDirection = glm::normalize(glm::vec3(worldPos) - rayOrigin);
    
    // Check intersection with object's collision zone
    return isMouseHovering(rayOrigin + rayDirection * 10.0f); // Check at reasonable distance
}

bool Object::isMouseHovering(const glm::vec3& worldMousePos) const {
    // Use the existing collision detection system
    return isPointInside(worldMousePos);
}

void Object::updateHoverState(bool isHovering) {
    bool wasHovered = _wasHoveredLastFrame;
    _wasHoveredLastFrame = _isHovered;
    _isHovered = isHovering;
    
    // Trigger events based on hover state changes
    if (isHovering && !wasHovered) {
        // Mouse entered the object
        ObjectHoverEnterEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    } else if (!isHovering && wasHovered) {
        // Mouse exited the object
        ObjectHoverExitEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    } else if (isHovering) {
        // Mouse is hovering over the object
        ObjectHoverEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    }
}

// --------------------------------------------------------------
// Attributes/Tags implementation
// --------------------------------------------------------------
void Object::setAttribute(const std::string& key, const std::string& value) {
    attributes[key] = value;
}

bool Object::hasAttribute(const std::string& key) const {
    return attributes.find(key) != attributes.end();
}

const std::string& Object::getAttribute(const std::string& key) const {
    static const std::string empty;
    auto it = attributes.find(key);
    return it == attributes.end() ? empty : it->second;
}

void Object::addTag(const std::string& tag) {
    if (!hasTag(tag)) tags.push_back(tag);
}

void Object::removeTag(const std::string& tag) {
    tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

bool Object::hasTag(const std::string& tag) const {
    for (const auto& t : tags) if (t == tag) return true;
    return false;
}