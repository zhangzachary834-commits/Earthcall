#pragma once

#include <cstdint>
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Per-face paintable texture with multi-layer compositing and stroke history.
// Pulled out of Object to keep paint/blend code separate from object semantics.
struct FaceTexture {
    GLuint id = 0;
    mutable std::vector<uint8_t> pixels;     // RGBA8 buffer, size×size×4
    int size = 64;

    std::vector<std::vector<uint8_t>> layers;
    std::vector<float> layerOpacities;
    std::vector<int>   blendModes;
    int  activeLayer = 0;
    bool useLayers   = false;

    struct StrokePoint {
        glm::vec2 uv;
        float radius;
        float opacity;
        glm::vec3 color;
        float timestamp;
    };
    std::vector<std::vector<StrokePoint>> strokeHistory;
    std::vector<std::vector<StrokePoint>> undoStack;

    void create(uint32_t initColorRGBA = 0xFFFFFFFFu);
    void addLayer();
    void deleteLayer(int layerIndex);
    void setLayerOpacity(int layerIndex, float opacity);
    void setBlendMode(int layerIndex, int mode);

    void uploadToGPU() const;
    void updateWholeGPU() const;
    void compositeLayers() const;
    void blendLayer(int layerIndex) const;
    glm::vec4 blendPixels(const glm::vec4& src, const glm::vec4& dst, int blendMode, float opacity) const;

    void saveStrokeState();
    void undo();
};
