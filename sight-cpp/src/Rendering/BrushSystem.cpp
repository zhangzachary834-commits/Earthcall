#include "BrushSystem.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

BrushSystem::BrushSystem(int textureSize) : _textureSize(textureSize) {
    printf("Creating BrushSystem with texture size: %d\n", textureSize);
    // Initialize composited texture
    _compositedTexture.resize(textureSize * textureSize * 4, 0);
    // Initialize with a base layer
    addLayer();
    initializeDefaultPresets();
    printf("BrushSystem created successfully with %zu presets\n", _brushPresets.size());
}

BrushSystem::~BrushSystem() {
    // Cleanup handled by std::vector
}

void BrushSystem::setLayerOpacity(float opacity) {
    if (_activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        _layers[_activeLayer].opacity = std::clamp(opacity, 0.0f, 1.0f);
        compositeLayers();
    }
}

void BrushSystem::setBlendMode(BlendMode mode) {
    if (_activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        _layers[_activeLayer].blendMode = mode;
        compositeLayers();
    }
}

int BrushSystem::addLayer() {
    Layer newLayer;
    newLayer.pixels.resize(_textureSize * _textureSize * 4, 0);
    newLayer.opacity = 1.0f;
    newLayer.blendMode = BlendMode::Normal;
    newLayer.visible = true;
    newLayer.strokeHistory.clear();
    newLayer.undoStack.clear();
    
    _layers.push_back(newLayer);
    _activeLayer = static_cast<int>(_layers.size()) - 1;
    
    compositeLayers();
    return _activeLayer;
}

void BrushSystem::deleteLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size()) && _layers.size() > 1) {
        _layers.erase(_layers.begin() + layerIndex);
        if (_activeLayer >= static_cast<int>(_layers.size())) {
            _activeLayer = static_cast<int>(_layers.size()) - 1;
        }
        compositeLayers();
    }
}

void BrushSystem::addPreset(const BrushPreset& preset) {
    _brushPresets.push_back(preset);
}

void BrushSystem::setCurrentPreset(int index) {
    if (index >= 0 && index < static_cast<int>(_brushPresets.size())) {
        _currentPreset = index;
        const BrushPreset& preset = _brushPresets[index];
        _currentBrushType = preset.type;
        _brushRadius = preset.radius;
        _brushSoftness = preset.softness;
        _brushOpacity = preset.opacity;
        _brushFlow = preset.flow;
        _brushSpacing = preset.spacing;
        _brushDensity = preset.density;
        _brushStrength = preset.strength;
    }
}

void BrushSystem::paintDab(const glm::vec2& position, const glm::vec3& color, float pressure) {
    if (!isValidPosition(position) || _activeLayer < 0 || _activeLayer >= static_cast<int>(_layers.size())) {
        return;
    }

    Layer& layer = _layers[_activeLayer];
    uint8_t* targetBuffer = layer.pixels.data();
    
    // Calculate brush parameters
    float radius = _brushRadius * pressure;
    float opacity = _brushOpacity * pressure;
    float flow = _brushFlow;
    
    // Convert position to pixel coordinates
    int centerX = static_cast<int>(position.x * _textureSize);
    int centerY = static_cast<int>(position.y * _textureSize);
    int radiusPx = static_cast<int>(radius * _textureSize);
    int radiusSq = radiusPx * radiusPx;
    
    // Calculate brush bounds
    int x0 = std::max(0, centerX - radiusPx);
    int y0 = std::max(0, centerY - radiusPx);
    int x1 = std::min(_textureSize - 1, centerX + radiusPx);
    int y1 = std::min(_textureSize - 1, centerY + radiusPx);
    
    // Apply brush effect to each pixel
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - centerX;
            int dy = y - centerY;
            int distSq = dx * dx + dy * dy;
            
            if (distSq <= radiusSq) {
                float distNorm = std::sqrt(static_cast<float>(distSq)) / static_cast<float>(radiusPx);
                float intensity = std::clamp(1.0f - distNorm, 0.0f, 1.0f);
                
                // Apply softness
                if (_brushSoftness < 0.99f) {
                    intensity = std::pow(intensity, 1.0f / std::max(0.001f, _brushSoftness));
                }
                
                intensity *= opacity * flow;
                
                // Apply brush type specific effects
                switch (_currentBrushType) {
                    case BrushType::Normal:
                        applyBrushEffect(targetBuffer, x, y, color, intensity);
                        break;
                    case BrushType::Airbrush:
                        applyAirbrushEffect(targetBuffer, x, y, color, intensity);
                        break;
                    case BrushType::Chalk:
                        applyChalkEffect(targetBuffer, x, y, color, intensity);
                        break;
                    case BrushType::Spray:
                        applySprayEffect(targetBuffer, x, y, color, intensity);
                        break;
                    case BrushType::Smudge:
                        applySmudgeEffect(targetBuffer, x, y, intensity);
                        break;
                    case BrushType::Clone:
                        if (_cloneActive) {
                            applyCloneEffect(targetBuffer, x, y, intensity);
                        }
                        break;
                }
            }
        }
    }
    
    // Save stroke point for history
    StrokePoint point;
    point.position = position;
    point.radius = radius;
    point.opacity = opacity;
    point.color = color;
    point.timestamp = static_cast<float>(glfwGetTime());
    point.pressure = pressure;
    
    layer.strokeHistory.push_back({point});
    
    compositeLayers();
}

void BrushSystem::paintStroke(const glm::vec2& startPos, const glm::vec2& endPos, const glm::vec3& color) {
    if (!_useStrokeInterpolation) {
        paintDab(startPos, color);
        paintDab(endPos, color);
        return;
    }
    
    float distance = glm::length(endPos - startPos);
    float spacing = _brushSpacing;
    int steps = static_cast<int>(distance / spacing) + 1;
    
    std::vector<StrokePoint> strokePoints;
    
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        glm::vec2 pos = glm::mix(startPos, endPos, t);
        
        float pressure = calculatePressure(pos, static_cast<float>(glfwGetTime()));
        paintDab(pos, color, pressure);
        
        // Collect stroke points for history
        StrokePoint point;
        point.position = pos;
        point.radius = _brushRadius * pressure;
        point.opacity = _brushOpacity * pressure;
        point.color = color;
        point.timestamp = static_cast<float>(glfwGetTime());
        point.pressure = pressure;
        strokePoints.push_back(point);
    }
    
    // Save complete stroke to history
    if (_activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        _layers[_activeLayer].strokeHistory.push_back(strokePoints);
    }
}

void BrushSystem::paint2DStroke(const std::vector<glm::vec2>& points, const glm::vec3& color) {
    if (points.size() < 2) return;
    
    std::vector<StrokePoint> strokePoints;
    
    for (size_t i = 0; i < points.size(); ++i) {
        float pressure = calculatePressure(points[i], static_cast<float>(glfwGetTime()));
        paintDab(points[i], color, pressure);
        
        StrokePoint point;
        point.position = points[i];
        point.radius = _brushRadius * pressure;
        point.opacity = _brushOpacity * pressure;
        point.color = color;
        point.timestamp = static_cast<float>(glfwGetTime());
        point.pressure = pressure;
        strokePoints.push_back(point);
    }
    
    // Save stroke to history
    if (_activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        _layers[_activeLayer].strokeHistory.push_back(strokePoints);
    }
}

void BrushSystem::saveStrokeState() {
    if (_activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        _layers[_activeLayer].undoStack = _layers[_activeLayer].strokeHistory;
    }
}

void BrushSystem::undo() {
    if (_activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        Layer& layer = _layers[_activeLayer];
        if (!layer.undoStack.empty()) {
            layer.strokeHistory = layer.undoStack;
            // TODO: Implement full stroke recreation from history
            compositeLayers();
        }
    }
}

void BrushSystem::redo() {
    // TODO: Implement redo functionality
}

void BrushSystem::clearHistory() {
    for (auto& layer : _layers) {
        layer.strokeHistory.clear();
        layer.undoStack.clear();
    }
}

const std::vector<uint8_t>& BrushSystem::getCompositedTexture() const {
    return _compositedTexture;
}

void BrushSystem::updateTexture() {
    compositeLayers();
}

void BrushSystem::initializeDefaultPresets() {
    _brushPresets.clear();
    
    // Soft Brush
    BrushPreset softBrush;
    softBrush.name = "Soft Brush";
    softBrush.type = BrushType::Normal;
    softBrush.radius = 0.15f;
    softBrush.softness = 0.3f;
    softBrush.opacity = 0.7f;
    softBrush.flow = 0.8f;
    softBrush.spacing = 0.05f;
    softBrush.density = 0.5f;
    softBrush.strength = 0.5f;
    _brushPresets.push_back(softBrush);
    
    // Hard Brush
    BrushPreset hardBrush;
    hardBrush.name = "Hard Brush";
    hardBrush.type = BrushType::Normal;
    hardBrush.radius = 0.1f;
    hardBrush.softness = 1.0f;
    hardBrush.opacity = 1.0f;
    hardBrush.flow = 1.0f;
    hardBrush.spacing = 0.02f;
    hardBrush.density = 0.5f;
    hardBrush.strength = 0.5f;
    _brushPresets.push_back(hardBrush);
    
    // Airbrush
    BrushPreset airbrush;
    airbrush.name = "Airbrush";
    airbrush.type = BrushType::Airbrush;
    airbrush.radius = 0.2f;
    airbrush.softness = 0.5f;
    airbrush.opacity = 0.5f;
    airbrush.flow = 0.6f;
    airbrush.spacing = 0.1f;
    airbrush.density = 0.8f;
    airbrush.strength = 0.5f;
    _brushPresets.push_back(airbrush);
    
    // Chalk
    BrushPreset chalk;
    chalk.name = "Chalk";
    chalk.type = BrushType::Chalk;
    chalk.radius = 0.12f;
    chalk.softness = 0.2f;
    chalk.opacity = 0.9f;
    chalk.flow = 0.7f;
    chalk.spacing = 0.08f;
    chalk.density = 0.5f;
    chalk.strength = 0.5f;
    _brushPresets.push_back(chalk);
    
    // Smudge
    BrushPreset smudge;
    smudge.name = "Smudge";
    smudge.type = BrushType::Smudge;
    smudge.radius = 0.18f;
    smudge.softness = 0.4f;
    smudge.opacity = 1.0f;
    smudge.flow = 1.0f;
    smudge.spacing = 0.03f;
    smudge.density = 0.5f;
    smudge.strength = 0.7f;
    _brushPresets.push_back(smudge);
    
    // Clone
    BrushPreset clone;
    clone.name = "Clone";
    clone.type = BrushType::Clone;
    clone.radius = 0.15f;
    clone.softness = 0.6f;
    clone.opacity = 0.8f;
    clone.flow = 1.0f;
    clone.spacing = 0.05f;
    clone.density = 0.5f;
    clone.strength = 0.5f;
    _brushPresets.push_back(clone);
}

// Private methods

void BrushSystem::applyBrushEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity) {
    int idx = getPixelIndex(x, y);
    uint8_t* dst = &targetBuffer[idx];
    
    float inv = 1.0f - intensity;
    dst[0] = static_cast<uint8_t>(dst[0] * inv + color.r * 255.0f * intensity);
    dst[1] = static_cast<uint8_t>(dst[1] * inv + color.g * 255.0f * intensity);
    dst[2] = static_cast<uint8_t>(dst[2] * inv + color.b * 255.0f * intensity);
    dst[3] = 255;
}

void BrushSystem::applyAirbrushEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity) {
    // Add randomness for airbrush effect
    float randomFactor = 0.5f + 0.5f * (static_cast<float>(rand()) / RAND_MAX);
    intensity *= randomFactor * _brushDensity;
    
    if (static_cast<float>(rand()) / RAND_MAX < intensity) {
        applyBrushEffect(targetBuffer, x, y, color, intensity);
    }
}

void BrushSystem::applyChalkEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity) {
    // Add texture for chalk effect
    float textureFactor = 0.3f + 0.7f * (static_cast<float>(rand()) / RAND_MAX);
    intensity *= textureFactor;
    
    applyBrushEffect(targetBuffer, x, y, color, intensity);
}

void BrushSystem::applySprayEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity) {
    // Spray can effect with random distribution
    if (static_cast<float>(rand()) / RAND_MAX > 0.7f) {
        intensity *= 0.3f;
        applyBrushEffect(targetBuffer, x, y, color, intensity);
    }
}

void BrushSystem::applySmudgeEffect(uint8_t* targetBuffer, int x, int y, float intensity) {
    // Get surrounding pixels and blend them
    glm::vec3 blendedColor(0.0f);
    int sampleCount = 0;
    
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < _textureSize && ny >= 0 && ny < _textureSize) {
                blendedColor += getPixelColor(targetBuffer, nx, ny);
                sampleCount++;
            }
        }
    }
    
    if (sampleCount > 0) {
        blendedColor /= static_cast<float>(sampleCount);
        applyBrushEffect(targetBuffer, x, y, blendedColor, intensity * _brushStrength);
    }
}

void BrushSystem::applyCloneEffect(uint8_t* targetBuffer, int x, int y, float intensity) {
    // Calculate source position
    glm::vec2 sourcePos = _cloneSource + _cloneOffset;
    int sourceX = static_cast<int>(sourcePos.x * _textureSize);
    int sourceY = static_cast<int>(sourcePos.y * _textureSize);
    
    if (sourceX >= 0 && sourceX < _textureSize && sourceY >= 0 && sourceY < _textureSize) {
        glm::vec3 sourceColor = getPixelColor(targetBuffer, sourceX, sourceY);
        applyBrushEffect(targetBuffer, x, y, sourceColor, intensity);
    }
}

glm::vec3 BrushSystem::blendPixels(const glm::vec3& src, const glm::vec3& dst, BlendMode mode, float opacity) {
    glm::vec3 result;
    
    switch (mode) {
        case BlendMode::Normal:
            result = glm::mix(dst, src, opacity);
            break;
        case BlendMode::Multiply:
            result = glm::vec3(src.x * dst.x, src.y * dst.y, src.z * dst.z);
            result = glm::mix(dst, result, opacity);
            break;
        case BlendMode::Screen:
            result = glm::vec3(1.0f - (1.0f - src.x) * (1.0f - dst.x),
                              1.0f - (1.0f - src.y) * (1.0f - dst.y),
                              1.0f - (1.0f - src.z) * (1.0f - dst.z));
            result = glm::mix(dst, result, opacity);
            break;
        case BlendMode::Overlay:
            result = glm::vec3(dst.x < 0.5f ? 2.0f * src.x * dst.x : 1.0f - 2.0f * (1.0f - src.x) * (1.0f - dst.x),
                              dst.y < 0.5f ? 2.0f * src.y * dst.y : 1.0f - 2.0f * (1.0f - src.y) * (1.0f - dst.y),
                              dst.z < 0.5f ? 2.0f * src.z * dst.z : 1.0f - 2.0f * (1.0f - src.z) * (1.0f - dst.z));
            result = glm::mix(dst, result, opacity);
            break;
        case BlendMode::Add:
            result = glm::min(glm::vec3(1.0f), src + dst);
            result = glm::mix(dst, result, opacity);
            break;
        case BlendMode::Subtract:
            result = glm::max(glm::vec3(0.0f), dst - src);
            result = glm::mix(dst, result, opacity);
            break;
    }
    
    return result;
}

void BrushSystem::compositeLayers() {
    // Create a composited texture buffer
    static std::vector<uint8_t> compositedTexture;
    compositedTexture.resize(_textureSize * _textureSize * 4, 0);
    
    // Start with transparent background
    for (int i = 0; i < _textureSize * _textureSize * 4; i += 4) {
        compositedTexture[i] = 0;     // R
        compositedTexture[i + 1] = 0; // G
        compositedTexture[i + 2] = 0; // B
        compositedTexture[i + 3] = 0; // A (transparent)
    }
    
    // Composite all visible layers
    for (size_t i = 0; i < _layers.size(); ++i) {
        if (_layers[i].visible && !_layers[i].pixels.empty()) {
                            for (int y = 0; y < _textureSize; ++y) {
                    for (int x = 0; x < _textureSize; ++x) {
                        // Get source and destination colors
                        glm::vec3 srcColor = getPixelColor(_layers[i].pixels.data(), x, y);
                        glm::vec3 dstColor = getPixelColor(compositedTexture.data(), x, y);
                    
                    // Blend using the layer's blend mode and opacity
                    glm::vec3 blendedColor = blendPixels(srcColor, dstColor, _layers[i].blendMode, _layers[i].opacity);
                    
                    // Apply to composited texture
                    setPixelColor(compositedTexture.data(), x, y, blendedColor);
                }
            }
        }
    }
    
    // Store the composited result
    _compositedTexture = compositedTexture;
}

float BrushSystem::calculatePressure(const glm::vec2& currentPos, float currentTime) {
    if (!_usePressureSimulation) {
        return _currentPressure;
    }
    
    if (_lastTime > 0.0f) {
        float timeDelta = currentTime - _lastTime;
        if (timeDelta > 0.0f) {
            float speed = glm::length(currentPos - _lastPosition) / timeDelta;
            return std::clamp(1.0f - speed * _pressureSensitivity, 0.1f, 1.0f);
        }
    }
    
    _lastPosition = currentPos;
    _lastTime = currentTime;
    return _currentPressure;
}

bool BrushSystem::isValidPosition(const glm::vec2& pos) const {
    return pos.x >= 0.0f && pos.x <= 1.0f && pos.y >= 0.0f && pos.y <= 1.0f;
}

int BrushSystem::getPixelIndex(int x, int y) const {
    return (y * _textureSize + x) * 4;
}

glm::vec3 BrushSystem::getPixelColor(const uint8_t* buffer, int x, int y) const {
    int idx = getPixelIndex(x, y);
    return glm::vec3(buffer[idx] / 255.0f, buffer[idx + 1] / 255.0f, buffer[idx + 2] / 255.0f);
}

void BrushSystem::setPixelColor(uint8_t* buffer, int x, int y, const glm::vec3& color) {
    int idx = getPixelIndex(x, y);
    buffer[idx] = static_cast<uint8_t>(color.r * 255.0f);
    buffer[idx + 1] = static_cast<uint8_t>(color.g * 255.0f);
    buffer[idx + 2] = static_cast<uint8_t>(color.b * 255.0f);
    buffer[idx + 3] = 255;
} 