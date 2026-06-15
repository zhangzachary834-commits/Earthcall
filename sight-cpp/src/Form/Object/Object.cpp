#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
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
    if (_hasPatch)   return 1;
    if (_hasField)   return 1;
    if (_hasComplex) return complexData.patchCount();
    if (_hasSmooth)  return 1;
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
    if (_hasPatch) {
        n = 1;
    } else if (_hasField) {
        n = 1;
    } else if (_hasComplex) {
        n = complexData.patchCount();
    } else if (_hasSmooth) {
        n = 1;
    } else if (geometryType == GeometryType::Polyhedron) {
        n = polyhedronData.getFaceCount();
    } else {
        n = numFacesForGeometry(geometryType);
    }
    if (n < 1) n = 1;
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

// Ray vs a triangle-soup tessellation (Möller–Trumbore); nearest hit.
static bool raycastTessMesh(const geom::TessMesh& m, const glm::vec3& o, const glm::vec3& d, float& tHit) {
    float nearest = 1e9f; bool found = false;
    for (size_t i = 0; i + 2 < m.tris.size(); i += 3) {
        const glm::vec3& a = m.tris[i].pos;
        const glm::vec3& b = m.tris[i + 1].pos;
        const glm::vec3& c = m.tris[i + 2].pos;
        glm::vec3 e1 = b - a, e2 = c - a, pv = glm::cross(d, e2);
        float det = glm::dot(e1, pv);
        if (std::fabs(det) < 1e-8f) continue;
        float inv = 1.0f / det;
        glm::vec3 tv = o - a;
        float u = glm::dot(tv, pv) * inv;
        if (u < 0.0f || u > 1.0f) continue;
        glm::vec3 qv = glm::cross(tv, e1);
        float v = glm::dot(d, qv) * inv;
        if (v < 0.0f || u + v > 1.0f) continue;
        float t = glm::dot(e2, qv) * inv;
        if (t > 1e-4f && t < nearest) { nearest = t; found = true; }
    }
    if (found) tHit = nearest;
    return found;
}

// Render a triangle-soup tessellation in immediate mode (legacy GL path).
static void drawTessMesh(const geom::TessMesh& m) {
    if (m.tris.empty()) return;
    // Client-side vertex arrays: one draw call instead of ~3 GL calls per vertex.
    // TessVertex is interleaved {pos(3), normal(3), uv(2)}, so the strided
    // pointers all walk the same buffer. This is the difference between a handful
    // of GL calls and hundreds of thousands per frame for marching-tet / patch
    // meshes — the cause of the multi-object lag.
    const geom::TessVertex* base = m.tris.data();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(geom::TessVertex), &base->pos);
    glNormalPointer(GL_FLOAT, sizeof(geom::TessVertex), &base->normal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(geom::TessVertex), &base->uv);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m.tris.size()));
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Object::drawSmoothModel() const {
    glEnable(GL_TEXTURE_2D);
    if (!faceTextures.empty()) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTessMesh(geom::tessellateSmooth(smoothData));
    glDisable(GL_TEXTURE_2D);
}

void Object::drawComplexModel() const {
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    // Each patch is a real face — draw it with its own face texture so the
    // round side and the flat caps can be painted independently.
    for (int i = 0; i < complexData.patchCount(); ++i) {
        if (i < static_cast<int>(faceTextures.size())) {
            glBindTexture(GL_TEXTURE_2D, faceTextures[i].id);
        }
        drawTessMesh(geom::tessellatePatch(complexData.patches[i]));
    }
    glDisable(GL_TEXTURE_2D);
}

void Object::drawFieldModel() const {
    glEnable(GL_TEXTURE_2D);
    if (!faceTextures.empty()) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTessMesh(_fieldMesh); // cached: SDF tessellation is expensive, rebuilt on change
    glDisable(GL_TEXTURE_2D);
}

void Object::drawPatchModel() const {
    glEnable(GL_TEXTURE_2D);
    if (!faceTextures.empty()) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
    glColor3f(1.0f, 1.0f, 1.0f);
    // An open control-net surface has two visible sides — light both.
    GLint twoSide = 0; glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &twoSide);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    drawTessMesh(_patchMesh);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, twoSide);
    glDisable(GL_TEXTURE_2D);
}

void Object::drawObject() const {
    if (_hasPatch)   { drawPatchModel();   return; }
    if (_hasField)   { drawFieldModel();   return; }
    if (_hasComplex) { drawComplexModel(); return; }
    if (_hasSmooth)  { drawSmoothModel();  return; }
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
    // IMPORTANT: collisionZone.corners are stored in world space, but this function is typically called
    // after the object's model transform has already been applied via glMultMatrixf(...).
    // If we draw world-space corners here, they get transformed again (double-transform), causing the outline
    // to appear offset from the object. Convert to local space first.
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);

    // Trace the ACTUAL shape (local space — the model transform is already applied
    // by the caller), not an AABB. Flat-faced shapes get a crisp edge wireframe;
    // curved / field shapes get a translucent additive glow shell hugging them.
    auto drawWireframe = [&](const std::vector<std::pair<glm::vec3, glm::vec3>>& edges) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (int p = 0; p < 2; ++p) {
            glColor4f(color.r, color.g, color.b, 0.65f - 0.3f * p);
            glLineWidth(2.0f + 2.5f * p);
            glBegin(GL_LINES);
            for (const auto& e : edges) {
                glVertex3f(e.first.x, e.first.y, e.first.z);
                glVertex3f(e.second.x, e.second.y, e.second.z);
            }
            glEnd();
        }
    };
    auto drawShell = [&](const geom::TessMesh& m) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
        glDepthMask(GL_FALSE);
        for (int p = 0; p < 2; ++p) {
            float s = 1.0f + 0.02f * (p + 1);
            glColor4f(color.r, color.g, color.b, 0.16f - 0.06f * p);
            glBegin(GL_TRIANGLES);
            for (const auto& v : m.tris) { glm::vec3 q = v.pos * s; glVertex3f(q.x, q.y, q.z); }
            glEnd();
        }
        glDepthMask(GL_TRUE);
    };

    if (geometryType == GeometryType::Polyhedron && !polyhedronData.vertices.empty()) {
        std::vector<std::pair<glm::vec3, glm::vec3>> edges;
        for (const auto& face : polyhedronData.faces)
            for (size_t k = 0; k < face.size(); ++k)
                edges.emplace_back(polyhedronData.vertices[face[k]],
                                   polyhedronData.vertices[face[(k + 1) % face.size()]]);
        drawWireframe(edges);
    } else if (_hasField) {
        drawShell(_fieldMesh);
    } else if (_hasSmooth) {
        drawShell(geom::tessellateSmooth(smoothData, 20, 12));
    } else if (_hasComplex) {
        drawShell(geom::tessellateComplex(complexData, 16));
    } else {
        // Legacy unit cube wireframe.
        const float h = 0.5f;
        glm::vec3 v[8] = {
            {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
            {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}
        };
        std::vector<std::pair<glm::vec3, glm::vec3>> edges = {
            {v[0],v[1]},{v[1],v[2]},{v[2],v[3]},{v[3],v[0]},
            {v[4],v[5]},{v[5],v[6]},{v[6],v[7]},{v[7],v[4]},
            {v[0],v[4]},{v[1],v[5]},{v[2],v[6]},{v[3],v[7]}
        };
        drawWireframe(edges);
    }
    glPopAttrib();
}

bool Object::raycastFace(const glm::vec3& rayOriginWorld, const glm::vec3& rayDirWorld,
                         float& outT, int& outFaceIndex, glm::vec2& outUV) const {
    // Transform ray to local space
    glm::mat4 inv = glm::inverse(getRaycastTransform());
    glm::vec3 oL = glm::vec3(inv * glm::vec4(rayOriginWorld, 1.0f));
    glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(rayDirWorld, 0.0f)));

    // Topology-based geometry takes precedence over the legacy primitive switch.
    if (_hasPatch) {
        if (raycastTessMesh(_patchMesh, oL, dL, outT)) { outFaceIndex = 0; outUV = glm::vec2(0.5f); return true; }
        return false;
    }
    if (_hasField) {
        // Pick against the cached mesh — robust for any field (morph/boolean/
        // implicit), including non-SDF implicit expressions where sphere-tracing
        // would be unreliable.
        if (raycastTessMesh(_fieldMesh, oL, dL, outT)) { outFaceIndex = 0; outUV = glm::vec2(0.5f); return true; }
        return false;
    }
    if (_hasComplex) {
        return geom::raycastComplex(complexData, oL, dL, outT, outFaceIndex, outUV);
    }
    if (_hasSmooth) {
        glm::vec3 n;
        if (geom::raycastSmooth(smoothData, oL, dL, outT, n, outUV)) { outFaceIndex = 0; return true; }
        return false;
    }

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
        // Cylinder axis along Z, radius 0.5, centered at origin: z in [-0.5, 0.5]
        // (drawObject applies glTranslatef(0,0,-0.5) before drawing, so local space is [-0.5,0.5])
        const float r = 0.5f;
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);

        // Side: x^2 + y^2 = r^2, z in [-0.5, 0.5]
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
                        if (p.z >= -0.5f && p.z <= 0.5f) {
                            float u = 0.5f + atan2f(p.y, p.x) / (2.0f * (float)M_PI);
                            float v = glm::clamp(p.z + 0.5f, 0.0f, 1.0f);
                            if (t < bestT) { bestT = t; bestFace = 0; bestUV = glm::vec2(u, v); }
                        }
                    }
                };
                testT(tA); testT(tB);
            }
        }

        // Caps at z = -0.5 (bottom) and z = 0.5 (top), share one texture (face 1)
        if (fabs(dL.z) > 1e-6f) {
            for (int sgn = 0; sgn <= 1; ++sgn) {
                float zPlane = sgn == 0 ? -0.5f : 0.5f;
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
        // Cone axis +Z: base (radius 0.5) at z=-0.5, apex at z=+0.5, z in [-0.5, 0.5]
        // Implicit surface: x^2 + y^2 = k^2 * (0.5 - z)^2, k = 0.5
        // (drawObject applies glTranslatef(0,0,-0.5) before gluCylinder(0.5,0,1), so local space is [-0.5,0.5])
        float bestT = 1e9f; int bestFace = -1; glm::vec2 bestUV(0.0f);

        // Side: x^2 + y^2 = (0.5*(0.5-z))^2, z in [-0.5, 0.5]
        // Quadratic coefficients derived from substituting ray into surface equation:
        float k = 0.5f;
        float oz_off = 0.5f - oL.z;   // (0.5 - oz), distance of ray origin from apex in Z
        float A = dL.x * dL.x + dL.y * dL.y - (k * k) * dL.z * dL.z;
        float B = 2.0f * (oL.x * dL.x + oL.y * dL.y) + 2.0f * (k * k) * oz_off * dL.z;
        float C = oL.x * oL.x + oL.y * oL.y - (k * k) * oz_off * oz_off;
        if (fabs(A) > 1e-6f) {
            float disc = B * B - 4.0f * A * C;
            if (disc >= 0.0f) {
                float s = sqrtf(disc);
                float tA = (-B - s) / (2.0f * A);
                float tB = (-B + s) / (2.0f * A);
                auto testT = [&](float t) {
                    if (t > 1e-6f) {
                        glm::vec3 p = oL + dL * t;
                        if (p.z >= -0.5f && p.z <= 0.5f) {
                            float theta = atan2f(p.y, p.x);
                            float u = 0.5f + theta / (2.0f * (float)M_PI);
                            float v = glm::clamp(0.5f - p.z, 0.0f, 1.0f); // 0 at apex (z=+0.5), 1 at base (z=-0.5)
                            if (t < bestT) { bestT = t; bestFace = 0; bestUV = glm::vec2(u, v); }
                        }
                    }
                };
                testT(tA); testT(tB);
            }
        }

        // Base disc at z = -0.5, radius 0.5 (face 1)
        if (fabs(dL.z) > 1e-6f) {
            float t = (-0.5f - oL.z) / dL.z;
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

namespace {
glm::vec3 extractScaleFromTransform(const glm::mat4& transform) {
    glm::vec3 scale(glm::length(glm::vec3(transform[0])),
                    glm::length(glm::vec3(transform[1])),
                    glm::length(glm::vec3(transform[2])));
    if (scale.x <= 1e-6f) scale.x = 1.0f;
    if (scale.y <= 1e-6f) scale.y = 1.0f;
    if (scale.z <= 1e-6f) scale.z = 1.0f;
    return scale;
}

glm::vec3 extractRotationDegreesFromTransform(const glm::mat4& transform) {
    glm::vec3 scale = extractScaleFromTransform(transform);
    glm::mat3 rotationBasis;
    rotationBasis[0] = glm::vec3(transform[0]) / scale.x;
    rotationBasis[1] = glm::vec3(transform[1]) / scale.y;
    rotationBasis[2] = glm::vec3(transform[2]) / scale.z;

    if (glm::determinant(rotationBasis) < 0.0f) {
        rotationBasis[0] = -rotationBasis[0];
    }

    glm::quat rotation = glm::normalize(glm::quat_cast(rotationBasis));
    return glm::degrees(glm::eulerAngles(rotation));
}

float wrapDegrees(float degrees) {
    float wrapped = std::fmod(degrees, 360.0f);
    if (wrapped > 180.0f) wrapped -= 360.0f;
    if (wrapped < -180.0f) wrapped += 360.0f;
    return wrapped;
}

float shortestAngleDelta(float current, float target) {
    return wrapDegrees(target - current);
}
} // namespace

void Object::updateCollisionZone(const glm::mat4& transform) const {
    if (getSpatialKind() == SpatialKind::Polyhedron &&
        geometryType == GeometryType::Polyhedron &&
        !polyhedronData.vertices.empty()) {
        // Custom polyhedra compute their broadphase box from the authored vertices.
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
    } else if (getSpatialKind() != SpatialKind::Polyhedron && !_supportCloud.empty()) {
        // [A/B TEST — temporarily the OLD O(cloud) behavior to confirm cost]
        glm::vec3 minCorner = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxCorner = glm::vec3(-std::numeric_limits<float>::max());
        for (const auto& vertex : _supportCloud) {
            glm::vec3 worldVertex = glm::vec3(transform * glm::vec4(vertex, 1.0f));
            minCorner = glm::min(minCorner, worldVertex);
            maxCorner = glm::max(maxCorner, worldVertex);
        }
        glm::vec3 lo = minCorner, hi = maxCorner;
        glm::vec3 localCorners[8] = {
            {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
            {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}
        };
        for (int i = 0; i < 8; ++i) collisionZone.corners[i] = localCorners[i];
    } else {
        // Unit cube fallback: cube objects and topology models without a mesh cloud.
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

void Object::rebuildSupportCloud() {
    _supportCloud.clear();
    geom::TessMesh m;
    if (_hasPatch) {
        _patchMesh = geom::tessellateBezier(patchData);
        m = _patchMesh;
    }
    else if (_hasField) {
        _fieldMesh = geom::tessellateSdf(fieldData, _fieldExtent);
        m = _fieldMesh;
    }
    else if (_hasComplex) m = geom::tessellateComplex(complexData, 16);
    else if (_hasSmooth)  m = geom::tessellateSmooth(smoothData, 16, 10);
    else { _localMin = glm::vec3(-0.5f); _localMax = glm::vec3(0.5f); return; }
    _supportCloud.reserve(m.tris.size());
    glm::vec3 lo(std::numeric_limits<float>::max()), hi(-std::numeric_limits<float>::max());
    for (const auto& v : m.tris) {
        _supportCloud.push_back(v.pos);
        lo = glm::min(lo, v.pos);
        hi = glm::max(hi, v.pos);
    }
    if (m.tris.empty()) { lo = glm::vec3(-0.5f); hi = glm::vec3(0.5f); }
    _localMin = lo;
    _localMax = hi;
}

glm::vec3 Object::getLocalSupportPoint(const glm::vec3& localDirection) const {
    glm::vec3 dir = localDirection;
    if (glm::dot(dir, dir) <= 1e-12f) dir = glm::vec3(1.0f, 0.0f, 0.0f);

    auto supportFromCloud = [&]() -> std::optional<glm::vec3> {
        if (_supportCloud.empty()) return std::nullopt;
        float best = -std::numeric_limits<float>::max();
        glm::vec3 bestVertex = _supportCloud[0];
        for (const auto& vertex : _supportCloud) {
            float candidate = glm::dot(vertex, dir);
            if (candidate > best) {
                best = candidate;
                bestVertex = vertex;
            }
        }
        return bestVertex;
    };

    switch (getSpatialKind()) {
        case SpatialKind::SmoothSurface: {
            bool ok = false;
            glm::vec3 sp = geom::supportPoint(smoothData, dir, ok);
            if (ok) return sp;
            if (auto cloudSupport = supportFromCloud()) return *cloudSupport;
            break;
        }
        case SpatialKind::ComplexShape:
        case SpatialKind::Field:
        case SpatialKind::Patch: {
            if (auto cloudSupport = supportFromCloud()) return *cloudSupport;
            break;
        }
        case SpatialKind::Polyhedron: {
            switch (geometryType) {
                case GeometryType::Polyhedron: {
                    if (polyhedronData.vertices.empty()) break;

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
                case GeometryType::Cube:
                default:
                    return glm::vec3(dir.x >= 0.0f ? 0.5f : -0.5f,
                                     dir.y >= 0.0f ? 0.5f : -0.5f,
                                     dir.z >= 0.0f ? 0.5f : -0.5f);
            }
            break;
        }
    }

    return glm::vec3(dir.x >= 0.0f ? 0.5f : -0.5f,
                     dir.y >= 0.0f ? 0.5f : -0.5f,
                     dir.z >= 0.0f ? 0.5f : -0.5f);
}

glm::vec3 Object::getSupportPointWorld(const glm::vec3& worldDirection) const {
    glm::mat4 transform = getRaycastTransform();
    glm::mat3 linear(transform);
    glm::vec3 localDirection = glm::transpose(linear) * worldDirection;
    glm::vec3 localSupport = getLocalSupportPoint(localDirection);
    return glm::vec3(transform * glm::vec4(localSupport, 1.0f));
}

bool Object::isCollisionShapeConvex() const {
    switch (getSpatialKind()) {
        case SpatialKind::Patch:
            return false; // open control surface, not a convex solid
        case SpatialKind::Field:
            return false; // SDF expressions (morph/boolean) may be non-convex
        case SpatialKind::SmoothSurface:
            return geom::isConvex(smoothData);
        case SpatialKind::ComplexShape:
            return true; // capped cylinder/cone and rounded box are convex
        case SpatialKind::Polyhedron:
            return geometryType == GeometryType::Polyhedron
                ? polyhedronData.getIsConvex()
                : true;
    }
    return false;
}

bool Object::computeLocalPointPenetration(const glm::vec3& localPoint,
                                          glm::vec3& outSurfacePoint,
                                          glm::vec3& outLocalNormal) const {
    SpatialKind spatialKind = getSpatialKind();
    switch (spatialKind) {
        case SpatialKind::Patch:
            return false; // open control surface, not an enclosed collision volume
        case SpatialKind::SmoothSurface:
        case SpatialKind::ComplexShape:
        case SpatialKind::Field: {
            auto f = [&](const glm::vec3& p) {
                if (spatialKind == SpatialKind::Field) return geom::evalSdf(fieldData, p);
                if (spatialKind == SpatialKind::ComplexShape) return geom::implicitComplex(complexData, p);
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
        case SpatialKind::Polyhedron:
            break;
    }

    switch (geometryType) {
        case GeometryType::Cube: {
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
        case GeometryType::Sphere: {
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
        case GeometryType::Cylinder: {
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
        case GeometryType::Cone: {
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
        case GeometryType::Polyhedron: {
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
    constexpr float EPS = 1e-5f;

    if (getSpatialKind() != SpatialKind::Polyhedron ||
        other.getSpatialKind() != SpatialKind::Polyhedron) {
        return false;
    }

    auto cubePolyhedron = []() -> const PolyhedronData& {
        static const PolyhedronData cube = []() {
            PolyhedronData data;
            data.vertices = {
                {-0.5f, -0.5f, -0.5f},
                { 0.5f, -0.5f, -0.5f},
                { 0.5f,  0.5f, -0.5f},
                {-0.5f,  0.5f, -0.5f},
                {-0.5f, -0.5f,  0.5f},
                { 0.5f, -0.5f,  0.5f},
                { 0.5f,  0.5f,  0.5f},
                {-0.5f,  0.5f,  0.5f}
            };
            data.faces = {
                {0, 3, 2, 1},
                {4, 5, 6, 7},
                {0, 1, 5, 4},
                {3, 7, 6, 2},
                {1, 2, 6, 5},
                {0, 4, 7, 3}
            };
            data.recomputeAll();
            return data;
        }();
        return cube;
    };

    auto polyhedronBodyFor = [&](const Object& object) -> const PolyhedronData* {
        if (object.getSpatialKind() != SpatialKind::Polyhedron) return nullptr;
        if (object.geometryType == GeometryType::Polyhedron &&
            !object.polyhedronData.vertices.empty() &&
            !object.polyhedronData.faces.empty()) {
            return &object.polyhedronData;
        }
        return &cubePolyhedron();
    };

    const PolyhedronData* bodyA = polyhedronBodyFor(*this);
    const PolyhedronData* bodyB = polyhedronBodyFor(other);
    if (!bodyA || !bodyB) return false;

    auto toWorld = [](const glm::mat4& m, const PolyhedronData& data) {
        std::vector<glm::vec3> world;
        world.reserve(data.vertices.size());
        for (const auto& v : data.vertices) {
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

    auto satTouching = [&](const PolyhedronData& a, const glm::mat4& transformA,
                           const PolyhedronData& b, const glm::mat4& transformB) {
        if (a.vertices.empty() || a.faces.empty() || b.vertices.empty() || b.faces.empty()) {
            return false;
        }

        const std::vector<glm::vec3> worldA = toWorld(transformA, a);
        const std::vector<glm::vec3> worldB = toWorld(transformB, b);

        auto isSeparating = [&](const glm::vec3& axis) {
            if (glm::dot(axis, axis) < EPS * EPS) return false;
            float minA, maxA, minB, maxB;
            project(worldA, axis, minA, maxA);
            project(worldB, axis, minB, maxB);
            return (maxA < minB - EPS) || (maxB < minA - EPS);
        };

        for (const auto& face : a.faces) {
            if (face.size() < 3) continue;
            if (isSeparating(PolyhedronData::computeNewellNormal(worldA, face))) return false;
        }
        for (const auto& face : b.faces) {
            if (face.size() < 3) continue;
            if (isSeparating(PolyhedronData::computeNewellNormal(worldB, face))) return false;
        }

        const std::vector<glm::vec3> edgesA = collectEdges(worldA, a.faces);
        const std::vector<glm::vec3> edgesB = collectEdges(worldB, b.faces);
        for (const auto& eA : edgesA) {
            for (const auto& eB : edgesB) {
                glm::vec3 axis = glm::cross(eA, eB);
                if (glm::length(axis) > EPS) {
                    if (isSeparating(glm::normalize(axis))) return false;
                }
            }
        }

        return true;
    };

    auto componentsFor = [](const PolyhedronData& data) -> std::vector<const PolyhedronData*> {
        std::vector<const PolyhedronData*> parts;
        const auto& components = data.getConvexComponents();
        if (!data.getIsConvex() && !components.empty()) {
            parts.reserve(components.size());
            for (const auto& component : components) parts.push_back(&component);
        } else {
            parts.push_back(&data);
        }
        return parts;
    };

    const std::vector<const PolyhedronData*> partsA = componentsFor(*bodyA);
    const std::vector<const PolyhedronData*> partsB = componentsFor(*bodyB);
    for (const PolyhedronData* partA : partsA) {
        for (const PolyhedronData* partB : partsB) {
            if (satTouching(*partA, transform, *partB, other.transform)) return true;
        }
    }

    return false;
}

void Object::setTransform(const glm::mat4& t) {
    transform = t;
    syncRotationStateFromTransform(transform, !preserveRotationTargetOnTransformSet);
    updateCollisionZone(transform);
}

void Object::setAuthoritativeAxis(const glm::vec3& axis) {
    if (glm::dot(axis, axis) <= 1e-12f) {
        authoritativeAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        return;
    }
    authoritativeAxis = glm::normalize(axis);
}

void Object::setRotationEulerDegrees(const glm::vec3& degrees) {
    glm::vec3 wrapped(wrapDegrees(degrees.x), wrapDegrees(degrees.y), wrapDegrees(degrees.z));
    rotationEulerDegrees = wrapped;
    targetRotationEulerDegrees = wrapped;

    preserveRotationTargetOnTransformSet = true;
    setTransform(composeTransformWithRotation(transform, rotationEulerDegrees));
    preserveRotationTargetOnTransformSet = false;
}

void Object::setTargetRotationEulerDegrees(const glm::vec3& degrees) {
    targetRotationEulerDegrees = glm::vec3(wrapDegrees(degrees.x),
                                           wrapDegrees(degrees.y),
                                           wrapDegrees(degrees.z));
}

void Object::addTargetRotationDegrees(const glm::vec3& deltaDegrees) {
    setTargetRotationEulerDegrees(targetRotationEulerDegrees + deltaDegrees);
}

void Object::setRotationResponsiveness(float responsiveness) {
    rotationResponsiveness = std::max(0.1f, responsiveness);
}

bool Object::hasPendingRotation() const {
    return std::abs(shortestAngleDelta(rotationEulerDegrees.x, targetRotationEulerDegrees.x)) > 0.01f ||
           std::abs(shortestAngleDelta(rotationEulerDegrees.y, targetRotationEulerDegrees.y)) > 0.01f ||
           std::abs(shortestAngleDelta(rotationEulerDegrees.z, targetRotationEulerDegrees.z)) > 0.01f;
}

void Object::syncRotationStateFromTransform(const glm::mat4& sourceTransform, bool syncTarget) {
    rotationEulerDegrees = extractRotationDegreesFromTransform(sourceTransform);
    rotationEulerDegrees.x = wrapDegrees(rotationEulerDegrees.x);
    rotationEulerDegrees.y = wrapDegrees(rotationEulerDegrees.y);
    rotationEulerDegrees.z = wrapDegrees(rotationEulerDegrees.z);
    if (syncTarget) {
        targetRotationEulerDegrees = rotationEulerDegrees;
    }
}

glm::mat4 Object::composeTransformWithRotation(const glm::mat4& sourceTransform,
                                               const glm::vec3& rotationDegrees) const {
    glm::vec3 translation = glm::vec3(sourceTransform[3]);
    glm::vec3 scale = extractScaleFromTransform(sourceTransform);

    glm::mat4 rebuilt = glm::translate(glm::mat4(1.0f), translation);
    rebuilt = glm::rotate(rebuilt, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rebuilt = glm::rotate(rebuilt, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rebuilt = glm::rotate(rebuilt, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    rebuilt = glm::scale(rebuilt, scale);
    return rebuilt;
}

bool Object::advanceRotation(const glm::mat4& sourceTransform, float dt, glm::mat4& outTransform) {
    syncRotationStateFromTransform(sourceTransform, false);

    glm::vec3 next = rotationEulerDegrees;
    float blend = 1.0f - std::exp(-std::max(0.1f, rotationResponsiveness) * std::max(0.0f, dt));
    bool changed = false;

    for (int axis = 0; axis < 3; ++axis) {
        float delta = shortestAngleDelta(next[axis], targetRotationEulerDegrees[axis]);
        if (std::abs(delta) <= 0.01f) {
            next[axis] = targetRotationEulerDegrees[axis];
            continue;
        }
        next[axis] = wrapDegrees(next[axis] + delta * blend);
        changed = true;
    }

    rotationEulerDegrees = next;
    outTransform = composeTransformWithRotation(sourceTransform, rotationEulerDegrees);
    return changed;
}

bool Object::updateRotation(float dt) {
    glm::mat4 nextTransform(1.0f);
    if (!advanceRotation(transform, dt, nextTransform)) {
        return false;
    }

    preserveRotationTargetOnTransformSet = true;
    setTransform(nextTransform);
    preserveRotationTargetOnTransformSet = false;
    return true;
}

// ---------------------------------------------------------------------
// Automation
// ---------------------------------------------------------------------
void Object::setAutomationRest(const glm::mat4& rest) {
    _automation.rest = rest;
    _automation.restValid = true;
}

void Object::addAutomation(const Automation::Clip& clip) {
    if (!_automation.restValid) {
        _automation.rest = transform;
        _automation.restValid = true;
    }
    _automation.clips.push_back(clip);
}

void Object::clearAutomations() {
    _automation.clips.clear();
}

void Object::advanceAutomations(float dt) {
    std::vector<std::string> finished;
    Automation::advance(_automation, dt, &finished);
    for (const auto& name : finished) {
        Core::EventBus::instance().publish(Automation::ClipFinished{this, name});
    }
}

glm::mat4 Object::sampleAutomations(const glm::mat4& base) const {
    return Automation::compose(_automation, base);
}

bool Object::updateAutomations(float dt) {
    if (!Automation::active(_automation)) return false;
    if (!_automation.restValid) {
        _automation.rest = transform;
        _automation.restValid = true;
    }
    std::vector<std::string> finished;
    Automation::advance(_automation, dt, &finished);
    setTransform(Automation::compose(_automation, transform));
    for (const auto& name : finished) {
        Core::EventBus::instance().publish(Automation::ClipFinished{this, name});
    }
    return true;
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
        glm::vec3 normal = PolyhedronData::computeNewellNormal(polyhedronData.vertices, face);
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
    geometryType = GeometryType::Polyhedron;
    polyhedronData = data;
    polyhedronData.recomputeAll();
    _hasSmooth = false;   // a polyhedron is flat-faced, not a topology surface
    _hasComplex = false;
    _hasField = false;
    _hasPatch = false;
    _shapeKind = ShapeKind::Polyhedron;
    initFaceTextures();
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
    static std::atomic<uint64_t> nextObjectId{1};
    if (objectID.empty()) {
        objectID = "object-" + std::to_string(nextObjectId.fetch_add(1));
    }
    initFaceTextures();
    syncRotationStateFromTransform(transform);
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
