#pragma once

#include <vector>
#include <cstdint>
#include "Formation/Formations.hpp"
#include <GLFW/glfw3.h> // Include OpenGL headers for rendering
#include <OpenGL/glu.h> // Include OpenGL utilities
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Singular.hpp"
#include "Core/EventBus.hpp"
#include <unordered_map>
#include <string>

// Forward declaration to break circular dependency
class BodyPart;

// Forward declaration for Object hover events
struct ObjectHoverEvent;
struct ObjectHoverEnterEvent;
struct ObjectHoverExitEvent;

class Object : public Singular {

public:
    // Geometry type to allow different primitive shapes
    enum class GeometryType { Cube = 0, Sphere, Cylinder, Cone, Polyhedron };

    // Helper structures for polyhedron construction
    struct Edge {
        int v1, v2;
        Edge(int vertex1, int vertex2) : v1(std::min(vertex1, vertex2)), v2(std::max(vertex1, vertex2)) {}
        bool operator==(const Edge& other) const { return v1 == other.v1 && v2 == other.v2; }
    };
    
    struct Face {
        std::vector<int> vertices;
        std::vector<Edge> edges;
        
        Face(const std::vector<int>& verts) : vertices(verts) {
            // Create edges from vertices
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
    
    // Polyhedron data structure for arbitrary polyhedrons
    struct PolyhedronData {
        std::vector<glm::vec3> vertices;      // 3D vertices in local space
        std::vector<std::vector<int>> faces;  // Face definitions as vertex indices
        std::vector<glm::vec3> faceNormals;   // Pre-computed face normals
        std::vector<glm::vec2> faceUVs;       // UV coordinates for each face (simplified)
        
        // Convexity and topology information
        bool isConvex = true;                 // Whether the polyhedron is convex
        std::vector<bool> faceConvexity;      // Convexity of individual faces
        std::vector<float> faceAreas;         // Area of each face
        std::vector<float> vertexCurvatures;  // Curvature at each vertex
        
        // Constructor for common polyhedrons
        PolyhedronData() = default;
        
        // Create a regular polyhedron
        static PolyhedronData createRegularPolyhedron(int numFaces, float radius = 0.5f);
        
        // Create a custom polyhedron from vertices and faces
        static PolyhedronData createCustomPolyhedron(const std::vector<glm::vec3>& verts, 
                                                    const std::vector<std::vector<int>>& faceDefs);
        
        // Create concave polyhedron variants
        static PolyhedronData createConcavePolyhedron(int numFaces, float radius = 0.5f, float concavity = 0.3f);
        static PolyhedronData createStarPolyhedron(int numFaces, float radius = 0.5f, float spikeLength = 0.3f);
        static PolyhedronData createCraterPolyhedron(int numFaces, float radius = 0.5f, float craterDepth = 0.2f);
        
        // Generate UV coordinates for texturing
        void generateUVs();
        
        // Compute face normals with convexity awareness
        void computeNormals();
        
        // Analyze convexity and topology
        void analyzeConvexity();
        
        // Compute face areas
        void computeFaceAreas();
        
        // Compute vertex curvatures
        void computeVertexCurvatures();
        
        // Get number of faces
        int getFaceCount() const { return static_cast<int>(faces.size()); }
        
        // Get number of vertices
        int getVertexCount() const { return static_cast<int>(vertices.size()); }
        
        // Check if polyhedron is convex
        bool getIsConvex() const { return isConvex; }
        
        // Get convexity of specific face
        bool getFaceConvexity(int faceIndex) const { 
            return (faceIndex >= 0 && faceIndex < static_cast<int>(faceConvexity.size())) ? 
                   faceConvexity[faceIndex] : true; 
        }
        
        // Helper methods for construction
        void addFace(const std::vector<int>& faceVertices);
        bool validateTopology() const;

        // Refactor this into a 3D Form system. With handlers?
        // Ensure all faces are wound outward w.r.t. the polyhedron centroid (consistent winding)
        void ensureOutwardWinding();

        // Uniformly scale vertices so the furthest vertex from origin matches the requested radius
        void scaleToRadius(float radius);
    };

    std::string screenMode();

    // Get the dimensions of the object.
    int getDimensions();
    void setDimensions(int d);

    int getCorners();
    void setCorners(int c);

    int getFaces();
    void setFaces(int f);

    int getMassQuantity();
    void setMassQuantity(int m);

    int getElements();
    void setElements(int e);

    int getRelationships();
    void setRelationships(int r);

    int getComplexityLevel();
    void setComplexityLevel(int cl);

    // Levels of Truth
    int getPhysicalObject();
    void setPhysicalObject(int po);

    int getSymbolicObject();
    void setSymbolicObject(int so);

    // Unique identifier for the object.
    std::string getObjectID();
    void setObjectID(int oi);

    std::string getObjectType() const;
    void setObjectType(int ot);

    // Position in 2D/3D space. The anchor point. Replace with glm::vec3 if using GLM for better math operations.
    int getX(); // x coordinate
    void setX(int x);

    int getY(); // y coordinate
    void setY(int y);

    int getZ(); // z coordinate
    void setZ(int z);

private:
    // Private members can be added here if needed, such as properties for the object
    // e.g., position, rotation, scale, texture, etc.

    int corners;
    int faces;

    int massQuantity = 0;
    int elements = 0;

    bool isElement = true;

    int complexityLevel = 0;

    float dimensions = 3.0f;

    int relationships = 0;

    bool physicalObject = true;

    std::string objectType;
    std::string objectID;

    // Position in 3D space. The anchor point. Replace with glm::vec3 if using GLM for better math operations.
    float x, y, z;

    // Formations that this Object is a part of
    std::vector<Formations> highOFormations;

    // Formations Formations that are within this object
    std::vector<Formations> lowOFormations;

    BodyPart* part = nullptr;

    // The primitive shape this Object represents. Default is Cube for compatibility.
    GeometryType geometryType = GeometryType::Cube;
    
    // Polyhedron data for arbitrary polyhedrons
    PolyhedronData polyhedronData;

public:
    // LEGACY flat colours kept for save/load compatibility (first 6 faces)
    float faceColors[6][3] = {
        {1.f, 0.f, 0.f}, {1.f, 0.f, 0.f},
        {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
        {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}
    };

    // Animation System code

    /** 
     * Dynamic Interaction-State memory. Keeps a record of interactions, actions, and states, 
     * This enables us to to recusrively build them like a fractal. 
     * Perhaps a fibbonacci, in which we would have a Formation or Object that resembles the Golden Ratio.
     * It should be so that we can dynamically use processes to create new processes, so it can propogate forward with new meaning.
     * We should be able to reconstruct every state of the object, from start to finish. 
     * Use simple data structures like vectors or lists to store these states. Loading, simulating, and creating new states should feel like plugging simple numbers into an equation to produce soemthing beautiful.
     * 
     * However in real world implementatino, we have to ensure that this powerful ability works at the agency and respects the dignity of our people and communities. 
     * In other words, privacy, property, autonomy, relational obligations, rights, etc..
     * Always records, but must record in a form undiscoverable to anyone except the owner(s) of the object, or those who have been granted permission to reload and propogate it. Zach himself must not have access to other's recording without permission.
     * If its not owned by anyone, either it is claimed through a process, or if it is unclaimable it can be partially loaded.
     * 
     * **/

protected:
    glm::mat4 transform = glm::mat4(1.0f);

public:

    // Must upgrade to a more robust polygonic collision system to host collision zones beyond cubes
    struct CollisionZone {
        glm::vec3 corners[8]; // 8 corners of the cube in world space
        // For polyhedrons, we'll use the bounding box approach for now
        // TODO: Implement proper polyhedron collision detection
    };
    mutable CollisionZone collisionZone;

    // -----------------------------------------------------------------
    // Per-face texture painting support
    // -----------------------------------------------------------------
    struct FaceTexture {
        GLuint id = 0;                 // OpenGL texture id
        mutable std::vector<uint8_t> pixels;   // RGBA8 pixel buffer (size×size×4) - mutable for layer compositing
        int size = 64;                 // dimensions (square for now)

        // Advanced texture features
        std::vector<std::vector<uint8_t>> layers;  // Multiple texture layers
        std::vector<float> layerOpacities;         // Layer opacity values
        std::vector<int> blendModes;               // Blending modes per layer
        int activeLayer = 0;                       // Currently active layer
        bool useLayers = false;                    // Enable/disable layer system

        // Brush stroke history for undo/redo
        struct StrokePoint {
            glm::vec2 uv;
            float radius;
            float opacity;
            glm::vec3 color;
            float timestamp;
        };
        std::vector<std::vector<StrokePoint>> strokeHistory;  // Per-layer stroke history
        std::vector<std::vector<StrokePoint>> undoStack;      // Undo stack per layer

        void create(GLuint initColorRGBA = 0xFFFFFFFF) {
            pixels.resize(size * size * 4);
            for (int i = 0; i < size * size; ++i) {
                reinterpret_cast<uint32_t*>(pixels.data())[i] = initColorRGBA;
            }
            
            // Initialize layer system
            layers.clear();
            layerOpacities.clear();
            blendModes.clear();
            strokeHistory.clear();
            undoStack.clear();
            
            // Create base layer
            addLayer();
            
            if (id == 0) glGenTextures(1, &id);
            uploadToGPU();
        }

        void addLayer() {
            std::vector<uint8_t> newLayer(size * size * 4, 0);
            layers.push_back(newLayer);
            layerOpacities.push_back(1.0f);
            blendModes.push_back(0); // Normal blend mode
            strokeHistory.push_back(std::vector<StrokePoint>());
            undoStack.push_back(std::vector<StrokePoint>());
        }

        void deleteLayer(int layerIndex) {
            if (layerIndex >= 0 && layerIndex < static_cast<int>(layers.size()) && layers.size() > 1) {
                layers.erase(layers.begin() + layerIndex);
                layerOpacities.erase(layerOpacities.begin() + layerIndex);
                blendModes.erase(blendModes.begin() + layerIndex);
                strokeHistory.erase(strokeHistory.begin() + layerIndex);
                undoStack.erase(undoStack.begin() + layerIndex);
                if (activeLayer >= static_cast<int>(layers.size())) {
                    activeLayer = static_cast<int>(layers.size()) - 1;
                }
                updateWholeGPU();
            }
        }

        void setLayerOpacity(int layerIndex, float opacity) {
            if (layerIndex >= 0 && layerIndex < static_cast<int>(layerOpacities.size())) {
                layerOpacities[layerIndex] = std::clamp(opacity, 0.0f, 1.0f);
                updateWholeGPU();
            }
        }

        void setBlendMode(int layerIndex, int mode) {
            if (layerIndex >= 0 && layerIndex < static_cast<int>(blendModes.size())) {
                blendModes[layerIndex] = mode;
                updateWholeGPU();
            }
        }

        void uploadToGPU() const {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            
            // Try to generate mipmaps using function pointer approach
            typedef void (*GenerateMipmapFunc)(GLenum);
            GenerateMipmapFunc generateMipmap = reinterpret_cast<GenerateMipmapFunc>(glfwGetProcAddress("glGenerateMipmap"));
            if (generateMipmap) {
                generateMipmap(GL_TEXTURE_2D);
            } else {
                // Fallback for older OpenGL versions
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
        }

        void updateWholeGPU() const { 
            if (useLayers) {
                compositeLayers();
            }
            uploadToGPU(); 
        }

        void compositeLayers() const {
            // Clear composite buffer
            std::fill(pixels.begin(), pixels.end(), 0);
            
            // Composite all layers
            for (size_t i = 0; i < layers.size(); ++i) {
                if (layerOpacities[i] > 0.0f) {
                    blendLayer(i);
                }
            }
        }

        void blendLayer(int layerIndex) const {
            const std::vector<uint8_t>& layer = layers[layerIndex];
            float opacity = layerOpacities[layerIndex];
            int blendMode = blendModes[layerIndex];
            
            for (size_t i = 0; i < pixels.size(); i += 4) {
                glm::vec4 dst(pixels[i]/255.0f, pixels[i+1]/255.0f, pixels[i+2]/255.0f, pixels[i+3]/255.0f);
                glm::vec4 src(layer[i]/255.0f, layer[i+1]/255.0f, layer[i+2]/255.0f, layer[i+3]/255.0f);
                
                glm::vec4 result = blendPixels(src, dst, blendMode, opacity);
                
                pixels[i] = static_cast<uint8_t>(result.r * 255);
                pixels[i+1] = static_cast<uint8_t>(result.g * 255);
                pixels[i+2] = static_cast<uint8_t>(result.b * 255);
                pixels[i+3] = static_cast<uint8_t>(result.a * 255);
            }
        }

        glm::vec4 blendPixels(const glm::vec4& src, const glm::vec4& dst, int blendMode, float opacity) const {
            glm::vec4 result = src;
            
            switch (blendMode) {
                case 0: // Normal
                    result = src * opacity + dst * (1.0f - opacity);
                    break;
                case 1: // Multiply
                    result = glm::vec4(glm::vec3(src.x, src.y, src.z) * glm::vec3(dst.x, dst.y, dst.z), src.w) * opacity + dst * (1.0f - opacity);
                    break;
                case 2: // Screen
                    result = glm::vec4(1.0f - (1.0f - glm::vec3(src.x, src.y, src.z)) * (1.0f - glm::vec3(dst.x, dst.y, dst.z)), src.w) * opacity + dst * (1.0f - opacity);
                    break;
                case 3: // Overlay
                    result = glm::vec4(
                        dst.x < 0.5f ? 2.0f * src.x * dst.x : 1.0f - 2.0f * (1.0f - src.x) * (1.0f - dst.x),
                        dst.y < 0.5f ? 2.0f * src.y * dst.y : 1.0f - 2.0f * (1.0f - src.y) * (1.0f - dst.y),
                        dst.z < 0.5f ? 2.0f * src.z * dst.z : 1.0f - 2.0f * (1.0f - src.z) * (1.0f - dst.z),
                        src.w
                    ) * opacity + dst * (1.0f - opacity);
                    break;
                case 4: // Add
                    result = glm::vec4(glm::min(glm::vec3(src.x, src.y, src.z) + glm::vec3(dst.x, dst.y, dst.z), glm::vec3(1.0f)), src.w) * opacity + dst * (1.0f - opacity);
                    break;
                case 5: // Subtract
                    result = glm::vec4(glm::max(glm::vec3(src.x, src.y, src.z) - glm::vec3(dst.x, dst.y, dst.z), glm::vec3(0.0f)), src.w) * opacity + dst * (1.0f - opacity);
                    break;
            }
            
            return result;
        }

        void saveStrokeState() {
            if (activeLayer >= 0 && activeLayer < static_cast<int>(strokeHistory.size())) {
                // Save current layer state to undo stack
                undoStack[activeLayer] = strokeHistory[activeLayer];
            }
        }

        void undo() {
            if (activeLayer >= 0 && activeLayer < static_cast<int>(strokeHistory.size()) && 
                !undoStack[activeLayer].empty()) {
                strokeHistory[activeLayer] = undoStack[activeLayer];
                // Reapply strokes to layer
                std::fill(layers[activeLayer].begin(), layers[activeLayer].end(), 0);
                // TODO: Implement full stroke recreation from history
                updateWholeGPU();
            }
        }
    };

    // One texture per logical face (vector size determined by geometry type)
    std::vector<FaceTexture> faceTextures;

    // Initialise or reinitialise textures after geometry type set/changed
    void initFaceTextures();

    // Convenience: fill entire face with a colour (compatibility with old fill tool)
    void fillFaceColor(int faceIndex, float r, float g, float b);

    // Paint a circular dab onto a face at UV (0-1) with given radius (0-1)
    void paintFace(int faceIndex, const glm::vec2& uv, float r, float g, float b, float radius = 0.05f, float softness = 1.0f);

    // Advanced brush painting with pressure and dynamics
    void paintFaceAdvanced(int faceIndex, const glm::vec2& uv, float r, float g, float b, 
                          float radius = 0.05f, float softness = 1.0f, float opacity = 1.0f, 
                          float flow = 1.0f, int brushType = 0);

    // Paint stroke between two points with interpolation
    void paintStroke(int faceIndex, const glm::vec2& startUV, const glm::vec2& endUV, 
                     float r, float g, float b, float radius = 0.05f, float softness = 1.0f, 
                     float opacity = 1.0f, float spacing = 0.1f);

    // Smudge tool - blend existing colors
    void smudgeFace(int faceIndex, const glm::vec2& uv, float radius = 0.05f, float strength = 0.5f);

    // Clone tool - copy from source to destination
    void cloneFace(int faceIndex, const glm::vec2& destUV, const glm::vec2& sourceUV, 
                   float radius = 0.05f, float opacity = 1.0f);

    // Airbrush effect
    void airbrushFace(int faceIndex, const glm::vec2& uv, float r, float g, float b, 
                      float radius = 0.05f, float density = 0.5f, float opacity = 1.0f);

    // Layer management
    void addTextureLayer(int faceIndex);
    void deleteTextureLayer(int faceIndex, int layerIndex);
    void setActiveLayer(int faceIndex, int layerIndex);
    void setLayerOpacity(int faceIndex, int layerIndex, float opacity);
    void setBlendMode(int faceIndex, int layerIndex, int mode);

    // Undo/Redo
    void saveStrokeState(int faceIndex);
    void undoStroke(int faceIndex);
    void clearStrokeHistory(int faceIndex);

    // Older API remains but now delegates to fillFaceColor for backward compatibility
    void setFaceColor(int faceIndex, float r, float g, float b) { fillFaceColor(faceIndex, r, g, b); }

    void updateCollisionZone(const glm::mat4& transform) const;
    bool isPointInside(const glm::vec3& point) const;

    void drawCube() const;
    void drawPolyhedron() const;

    void drawObject() const;
    void drawHighlightOutline() const;

    void interactWith(Formations&);
    void onInteraction(Formations&);

    void drawSymbolicBody(); // or drawAsGeometry, drawPhysicalShell

    Object();
    Object(Object&&) = default;
    Object& operator=(Object&&) = default;
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    virtual void setTransform(const glm::mat4& t) { transform = t; }
    virtual glm::mat4 getTransform() const { return transform; }

    // Generalized ray-face intersection for painting across all geometry types.
    // Returns true if hit, along with distance t in world units, the face index, and UV in [0,1].
    bool raycastFace(const glm::vec3& rayOriginWorld, const glm::vec3& rayDirWorld,
                     float& outT, int& outFaceIndex, glm::vec2& outUV) const;

    // Hover detection methods
    bool isMouseHovering(const glm::vec2& mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int windowWidth, int windowHeight) const;
    bool isMouseHovering(const glm::vec3& worldMousePos) const;
    void updateHoverState(bool isHovering);
    bool getIsHovered() const { return _isHovered; }
    glm::vec3 getHoverPoint() const { return _hoverPoint; }

    // Setter / getter so tools can pick the shape
    void setGeometryType(GeometryType t) {
        geometryType = t;
        initFaceTextures();
    }
    GeometryType getGeometryType() const { return geometryType; }
    
    // Polyhedron-specific methods
    void setPolyhedronData(const PolyhedronData& data);
    const PolyhedronData& getPolyhedronData() const { return polyhedronData; }
    
    // Create common polyhedrons
    void createTetrahedron();
    void createOctahedron();
    void createDodecahedron();
    void createIcosahedron();
    void createCustomPolyhedron(const std::vector<glm::vec3>& vertices, 
                               const std::vector<std::vector<int>>& faces);

    virtual ~Object() = default;

    // Singular interface implementation
    std::string getIdentifier() const override { return objectID; }

    // --------------------------------------------------------------
    // Attributes and Tags for selection/filtering (for physics laws, etc.)
    // --------------------------------------------------------------
    void setAttribute(const std::string& key, const std::string& value);
    bool hasAttribute(const std::string& key) const;
    const std::string& getAttribute(const std::string& key) const; // empty string if missing
    const std::unordered_map<std::string, std::string>& getAttributes() const { return attributes; }
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::vector<std::string>& getTags() const { return tags; }

private:
    // Hover state tracking
    mutable bool _isHovered = false;
    mutable glm::vec3 _hoverPoint{0.0f, 0.0f, 0.0f};
    mutable bool _wasHoveredLastFrame = false;

    // Attributes and tags storage
    std::unordered_map<std::string, std::string> attributes;
    std::vector<std::string> tags;
};

struct StateSnapshot {
    float time;
    float x, y, z;
    std::string interactionSummary;
    std::vector<std::string> symbolicTags;
};