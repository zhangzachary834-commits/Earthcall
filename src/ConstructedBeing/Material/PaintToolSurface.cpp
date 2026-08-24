// Material — face texture & painting subsystem (split from Object.cpp).
// Per-face texture init, fill/paint/stroke/smudge/clone/airbrush, and layer/history ops.

#include "PaintToolSurface.hpp"
#include "ConstructedBeing/Singular/Object/Contour.hpp"
#include "ConstructedBeing/Singular/Object/AngleTools.hpp"
#include "ConstructedBeing/Singular/Object/Automation/AutomationEvents.hpp"
#include <GLFW/glfw3.h>
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
#include "Singularity/Screen/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PaintToolSurface::PaintToolSurface(Material& material) : _material(material) {}



void PaintToolSurface::fillFaceColor(int faceIndex, float r, float g, float b) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    FaceTexture& tex = _material.faceTextures[faceIndex];
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


}

void PaintToolSurface::paintFace(int faceIndex, const glm::vec2& uv, float r, float g, float b, float radius, float softness) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    FaceTexture& tex = _material.faceTextures[faceIndex];
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

void PaintToolSurface::paintFaceAdvanced(int faceIndex, const glm::vec2& uv, float r, float g, float b, 
                              float radius, float softness, float opacity, float flow, int brushType) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    FaceTexture& tex = _material.faceTextures[faceIndex];
    
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

void PaintToolSurface::paintStroke(int faceIndex, const glm::vec2& startUV, const glm::vec2& endUV, 
                         float r, float g, float b, float radius, float softness, 
                         float opacity, float spacing) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    
    float distance = glm::length(endUV - startUV);
    int steps = static_cast<int>(distance / spacing) + 1;
    
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        glm::vec2 uv = glm::mix(startUV, endUV, t);
        paintFaceAdvanced(faceIndex, uv, r, g, b, radius, softness, opacity, 1.0f, 0);
    }
}

void PaintToolSurface::smudgeFace(int faceIndex, const glm::vec2& uv, float radius, float strength) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    FaceTexture& tex = _material.faceTextures[faceIndex];
    
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

void PaintToolSurface::cloneFace(int faceIndex, const glm::vec2& destUV, const glm::vec2& sourceUV, 
                       float radius, float opacity) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    FaceTexture& tex = _material.faceTextures[faceIndex];
    
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

void PaintToolSurface::airbrushFace(int faceIndex, const glm::vec2& uv, float r, float g, float b, 
                          float radius, float density, float opacity) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(_material.faceTextures.size())) return;
    FaceTexture& tex = _material.faceTextures[faceIndex];
    
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
void PaintToolSurface::addTextureLayer(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].addLayer();
        _material.faceTextures[faceIndex].useLayers = true;
    }
}

void PaintToolSurface::deleteTextureLayer(int faceIndex, int layerIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].deleteLayer(layerIndex);
    }
}

void PaintToolSurface::setActiveLayer(int faceIndex, int layerIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].activeLayer = layerIndex;
    }
}

void PaintToolSurface::setLayerOpacity(int faceIndex, int layerIndex, float opacity) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].setLayerOpacity(layerIndex, opacity);
    }
}

void PaintToolSurface::setBlendMode(int faceIndex, int layerIndex, int mode) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].setBlendMode(layerIndex, mode);
    }
}

// Undo/Redo methods
void PaintToolSurface::saveStrokeState(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].saveStrokeState();
    }
}

void PaintToolSurface::undoStroke(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        _material.faceTextures[faceIndex].undo();
    }
}

void PaintToolSurface::clearStrokeHistory(int faceIndex) {
    if (faceIndex >= 0 && faceIndex < static_cast<int>(_material.faceTextures.size())) {
        FaceTexture& tex = _material.faceTextures[faceIndex];
        for (auto& history : tex.strokeHistory) {
            history.clear();
        }
        for (auto& stack : tex.undoStack) {
            stack.clear();
        }
    }
}
