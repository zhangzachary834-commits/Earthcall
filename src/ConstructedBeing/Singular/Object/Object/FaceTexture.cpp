#include "FaceTexture.hpp"

#include "Singularity/Screen/Renderer.hpp"

#include <algorithm>
#include <cmath>

void FaceTexture::create(uint32_t initColorRGBA) {
    pixels.resize(size * size * 4);
    for (int i = 0; i < size * size; ++i) {
        reinterpret_cast<uint32_t*>(pixels.data())[i] = initColorRGBA;
    }

    layers.clear();
    layerOpacities.clear();
    blendModes.clear();
    strokeHistory.clear();
    undoStack.clear();

    addLayer();

    uploadToGPU();
}

void FaceTexture::addLayer() {
    layers.emplace_back(size * size * 4, 0);
    layerOpacities.push_back(1.0f);
    blendModes.push_back(0);
    strokeHistory.emplace_back();
    undoStack.emplace_back();
}

void FaceTexture::deleteLayer(int layerIndex) {
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

void FaceTexture::setLayerOpacity(int layerIndex, float opacity) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(layerOpacities.size())) {
        layerOpacities[layerIndex] = std::clamp(opacity, 0.0f, 1.0f);
        updateWholeGPU();
    }
}

void FaceTexture::setBlendMode(int layerIndex, int mode) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(blendModes.size())) {
        blendModes[layerIndex] = mode;
        updateWholeGPU();
    }
}

void FaceTexture::uploadToGPU() const {
    // The backend owns the texture object and the sampler/mipmap policy; this only
    // says "these pixels are the paint now". A backend that reads the CPU pixels
    // straight off RenderMaterial::albedoPixels returns 0 and keeps no handle.
    id = currentRenderer().uploadTexture(id, pixels.data(),
                                         static_cast<uint32_t>(size),
                                         static_cast<uint32_t>(size));
}

void FaceTexture::updateWholeGPU() const {
    if (useLayers) {
        compositeLayers();
    }
    uploadToGPU();
}

void FaceTexture::compositeLayers() const {
    std::fill(pixels.begin(), pixels.end(), 0);
    for (size_t i = 0; i < layers.size(); ++i) {
        if (layerOpacities[i] > 0.0f) {
            blendLayer(static_cast<int>(i));
        }
    }
}

void FaceTexture::blendLayer(int layerIndex) const {
    const std::vector<uint8_t>& layer = layers[layerIndex];
    float opacity   = layerOpacities[layerIndex];
    int   blendMode = blendModes[layerIndex];

    for (size_t i = 0; i < pixels.size(); i += 4) {
        glm::vec4 dst(pixels[i]/255.0f, pixels[i+1]/255.0f, pixels[i+2]/255.0f, pixels[i+3]/255.0f);
        glm::vec4 src(layer[i]/255.0f,  layer[i+1]/255.0f,  layer[i+2]/255.0f,  layer[i+3]/255.0f);

        glm::vec4 result = blendPixels(src, dst, blendMode, opacity);

        pixels[i]   = static_cast<uint8_t>(result.r * 255);
        pixels[i+1] = static_cast<uint8_t>(result.g * 255);
        pixels[i+2] = static_cast<uint8_t>(result.b * 255);
        pixels[i+3] = static_cast<uint8_t>(result.a * 255);
    }
}

glm::vec4 FaceTexture::blendPixels(const glm::vec4& src, const glm::vec4& dst, int blendMode, float opacity) const {
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

bool FaceTexture::writePixel(const glm::vec2& uv, const glm::vec3& color) {
    if (size <= 0 || !std::isfinite(uv.x) || !std::isfinite(uv.y) ||
        uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(size) * size * 4;
    if (pixels.size() != expected) return false;

    const int x = std::min(size - 1, static_cast<int>(std::floor(uv.x * size)));
    const int y = std::min(size - 1, static_cast<int>(std::floor(uv.y * size)));
    return writeRegion(x, y, x + 1, y + 1, std::vector<glm::vec3>{color});
}

bool FaceTexture::writeRegion(int x0, int y0, int x1, int y1,
                              const std::vector<glm::vec3>& colors) {
    if (size <= 0 || x0 < 0 || y0 < 0 || x1 <= x0 || y1 <= y0 ||
        x1 > size || y1 > size ||
        colors.size() != static_cast<std::size_t>(x1 - x0) * (y1 - y0)) {
        return false;
    }
    std::vector<glm::ivec2> coordinates;
    coordinates.reserve(colors.size());
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) coordinates.emplace_back(x, y);
    return writeSamples(coordinates, colors);
}

bool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,
                               const std::vector<glm::vec3>& colors) {
    if (size <= 0 || coordinates.size() != colors.size()) return false;
    for (const glm::ivec2& xy : coordinates) {
        if (xy.x < 0 || xy.y < 0 || xy.x >= size || xy.y >= size) return false;
    }
    for (const glm::vec3& color : colors) {
        if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
            !std::isfinite(color.b)) {
            return false;
        }
    }
    const std::size_t expected = static_cast<std::size_t>(size) * size * 4;
    if (pixels.size() != expected) return false;
    const auto channel = [](float value) {
        return static_cast<uint8_t>(std::lround(value * 255.0f));
    };

    auto write = [&](std::vector<uint8_t>& buffer) {
        if (buffer.size() != expected) return false;
        std::size_t i = 0;
        for (const glm::ivec2& xy : coordinates) {
            const glm::vec3 clamped = glm::clamp(
                colors[i++], glm::vec3(0.0f), glm::vec3(1.0f));
            const std::size_t offset = static_cast<std::size_t>(xy.y * size + xy.x) * 4;
            buffer[offset] = channel(clamped.r);
            buffer[offset + 1] = channel(clamped.g);
            buffer[offset + 2] = channel(clamped.b);
            buffer[offset + 3] = 255;
        }
        return true;
    };

    if (useLayers) {
        if (activeLayer < 0 || activeLayer >= static_cast<int>(layers.size()) ||
            !write(layers[activeLayer])) {
            return false;
        }
        compositeLayers();
    } else if (!write(pixels)) {
        return false;
    }
    uploadToGPU();
    return true;
}

void FaceTexture::saveStrokeState() {
    if (activeLayer >= 0 && activeLayer < static_cast<int>(strokeHistory.size())) {
        undoStack[activeLayer] = strokeHistory[activeLayer];
    }
}

void FaceTexture::undo() {
    if (activeLayer >= 0 && activeLayer < static_cast<int>(strokeHistory.size()) &&
        !undoStack[activeLayer].empty()) {
        strokeHistory[activeLayer] = undoStack[activeLayer];
        std::fill(layers[activeLayer].begin(), layers[activeLayer].end(), 0);
        updateWholeGPU();
    }
}
