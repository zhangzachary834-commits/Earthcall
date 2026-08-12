#pragma once

#include "ConstructedBeing/Material/Material.hpp"
#include <glm/glm.hpp>

// PaintToolSurface provides the methods to mutate a Material's face textures.
// It isolates the painting/blending logic from the Material being itself.
class PaintToolSurface {
public:
    PaintToolSurface(Material& material);

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

private:
    Material& _material;
};
