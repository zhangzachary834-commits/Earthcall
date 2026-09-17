#include "FaceTexture.hpp"

#include "Singularity/Screen/Renderer.hpp"

#include <algorithm>
#include <cmath>

void FaceTexture::create(int w, int h, uint32_t initColorRGBA) {
    width = w;
    height = h;
    pixels.resize(width * height * 4);
    for (int i = 0; i < width * height; ++i) {
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
    layers.emplace_back(width * height * 4, 0);
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
                                         static_cast<uint32_t>(width),
                                         static_cast<uint32_t>(height));
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
    if (width <= 0 || height <= 0 || !std::isfinite(uv.x) || !std::isfinite(uv.y) ||
        uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(width) * height * 4;
    if (pixels.size() != expected) return false;

    const int x = std::min(width - 1, static_cast<int>(std::floor(uv.x * width)));
    const int y = std::min(height - 1, static_cast<int>(std::floor(uv.y * height)));
    return writeRegion(x, y, x + 1, y + 1, std::vector<glm::vec3>{color});
}

bool FaceTexture::writePixelWithRadius(const glm::vec2& uv, const glm::vec3& color, int radius) {
    if (radius <= 1) {
        return writePixel(uv, color);
    }
    if (width <= 0 || height <= 0 || !std::isfinite(uv.x) || !std::isfinite(uv.y) ||
        uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
        return false;
    }
    const int cx = std::min(width - 1, static_cast<int>(std::floor(uv.x * width)));
    const int cy = std::min(height - 1, static_cast<int>(std::floor(uv.y * height)));
    const int r = radius - 1;
    const int x0 = std::max(0, cx - r);
    const int x1 = std::min(width - 1, cx + r);
    const int y0 = std::max(0, cy - r);
    const int y1 = std::min(height - 1, cy + r);
    std::vector<glm::ivec2> coordinates;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            coordinates.push_back({x, y});
        }
    }
    std::vector<glm::vec3> colors(coordinates.size(), color);
    bool ok = writeSamples(coordinates, colors);
    if (ok) revision++;
    return ok;
}

bool FaceTexture::writeLine(const glm::vec2& uv0, const glm::vec2& uv1,
                            const glm::vec3& color, int radius) {
    if (width <= 0 || height <= 0 || !std::isfinite(uv0.x) || !std::isfinite(uv0.y) ||
        !std::isfinite(uv1.x) || !std::isfinite(uv1.y)) {
        return false;
    }
    const glm::vec2 c0 = glm::clamp(uv0, glm::vec2(0.0f), glm::vec2(1.0f));
    const glm::vec2 c1 = glm::clamp(uv1, glm::vec2(0.0f), glm::vec2(1.0f));
    const int x0 = std::min(width - 1, static_cast<int>(std::floor(c0.x * width)));
    const int y0 = std::min(height - 1, static_cast<int>(std::floor(c0.y * height)));
    const int x1 = std::min(width - 1, static_cast<int>(std::floor(c1.x * width)));
    const int y1 = std::min(height - 1, static_cast<int>(std::floor(c1.y * height)));

    if (x0 == x1 && y0 == y1) {
        return writePixelWithRadius(c1, color, radius);
    }

    std::vector<glm::ivec2> linePoints;
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    int curX = x0;
    int curY = y0;
    while (true) {
        linePoints.push_back({curX, curY});
        if (curX == x1 && curY == y1) break;
        const int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            curX += sx;
        }
        if (e2 < dx) {
            err += dx;
            curY += sy;
        }
    }

    std::vector<glm::ivec2> coordinates;
    if (radius <= 1) {
        coordinates = std::move(linePoints);
    } else {
        std::vector<bool> visited(static_cast<std::size_t>(width * height), false);
        const int r = radius - 1;
        for (const auto& pt : linePoints) {
            const int min_x = std::max(0, pt.x - r);
            const int max_x = std::min(width - 1, pt.x + r);
            const int min_y = std::max(0, pt.y - r);
            const int max_y = std::min(height - 1, pt.y + r);
            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {
                    const std::size_t idx = static_cast<std::size_t>(y * width + x);
                    if (!visited[idx]) {
                        visited[idx] = true;
                        coordinates.push_back({x, y});
                    }
                }
            }
        }
    }

    std::vector<glm::vec3> colors(coordinates.size(), color);
    bool ok = writeSamples(coordinates, colors);
    if (ok) revision++;
    return ok;
}

bool FaceTexture::writeRegion(int x0, int y0, int x1, int y1,
                              const std::vector<glm::vec3>& colors) {
    if (width <= 0 || height <= 0 || x0 < 0 || y0 < 0 || x1 <= x0 || y1 <= y0 ||
        x1 > width || y1 > height ||
        colors.size() != static_cast<std::size_t>(x1 - x0) * (y1 - y0)) {
        return false;
    }
    std::vector<glm::ivec2> coordinates;
    coordinates.reserve(colors.size());
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) coordinates.emplace_back(x, y);
    bool ok = writeSamples(coordinates, colors);
    if (ok) revision++;
    return ok;
}


bool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,
                               const std::vector<glm::vec3>& colors) {
    if (width <= 0 || height <= 0 || coordinates.size() != colors.size()) return false;
    
    int minX = width, minY = height, maxX = -1, maxY = -1;
    
    for (const glm::ivec2& xy : coordinates) {
        if (xy.x < 0 || xy.y < 0 || xy.x >= width || xy.y >= height) return false;
        if (xy.x < minX) minX = xy.x;
        if (xy.y < minY) minY = xy.y;
        if (xy.x > maxX) maxX = xy.x;
        if (xy.y > maxY) maxY = xy.y;
    }
    
    if (minX > maxX || minY > maxY) return false;
    
    for (const glm::vec3& color : colors) {
        if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
            !std::isfinite(color.b)) {
            return false;
        }
    }
    const std::size_t expected = static_cast<std::size_t>(width) * height * 4;
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
            const std::size_t offset = static_cast<std::size_t>(xy.y * width + xy.x) * 4;
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
        compositeLayers(); // Wait, compositeLayers updates the whole thing! 
    } else if (!write(pixels)) {
        return false;
    }
    
    if (id == 0 || useLayers) {
        uploadToGPU();
    } else {
        uint32_t regionW = maxX - minX + 1;
        uint32_t regionH = maxY - minY + 1;
        currentRenderer().uploadTextureRegion(id, pixels.data(), width, height, minX, minY, regionW, regionH);
    }
    revision++;
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
