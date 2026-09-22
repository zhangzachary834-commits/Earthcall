#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <cstdint>

class BrushSystem {
public:
    // Brush types
    enum class BrushType { 
        Normal = 0, 
        Airbrush, 
        Chalk, 
        Spray, 
        Smudge, 
        Clone 
    };

    // Blend modes
    enum class BlendMode {
        Normal = 0,
        Multiply,
        Screen,
        Overlay,
        Add,
        Subtract
    };

    // Brush preset structure
    struct BrushPreset {
        std::string name;
        BrushType type;
        float radius;
        float softness;
        float opacity;
        float flow;
        float spacing;
        float density;
        float strength;
    };

    // Stroke point for history
    struct StrokePoint {
        glm::vec2 position;
        float radius;
        float opacity;
        glm::vec3 color;
        float timestamp;
        float pressure;
    };

    // Layer structure
    struct Layer {
        std::vector<uint8_t> pixels;  // RGBA8 pixel buffer
        float opacity;
        BlendMode blendMode;
        std::vector<std::vector<StrokePoint>> strokeHistory;
        std::vector<std::vector<StrokePoint>> undoStack;
        std::vector<std::vector<uint8_t>> pixelUndoStack;
        std::vector<std::vector<uint8_t>> pixelRedoStack;
        bool visible;
    };

    // Constructor
    BrushSystem(int textureSize = 512);
    ~BrushSystem();

    // Brush settings
    void setBrushType(BrushType type) { _currentBrushType = type; }
    void setRadius(float radius) { _brushRadius = std::clamp(radius, 0.001f, 0.08f); }
    void setSoftness(float softness) { _brushSoftness = std::clamp(softness, 0.001f, 1.0f); }
    void setOpacity(float opacity) { _brushOpacity = std::clamp(opacity, 0.0f, 1.0f); }
    void setFlow(float flow) { _brushFlow = std::clamp(flow, 0.0f, 1.0f); }
    void setSpacing(float spacing) { _brushSpacing = std::clamp(spacing, 0.0005f, 2.0f); }
    void setDensity(float density) { _brushDensity = std::clamp(density, 0.0f, 5.0f); }
    void setStrength(float strength) { _brushStrength = std::clamp(strength, 0.0f, 5.0f); }

    // Pressure simulation
    void setPressureSimulation(bool enabled) { _usePressureSimulation = enabled; }
    void setPressureSensitivity(float sensitivity) { _pressureSensitivity = std::clamp(sensitivity, 0.01f, 5.0f); }
    void setCurrentPressure(float pressure) { _currentPressure = std::clamp(pressure, 0.1f, 1.0f); }

    // Stroke interpolation
    void setStrokeInterpolation(bool enabled) { _useStrokeInterpolation = enabled; }

    // Layer system
    void setUseLayers(bool enabled) { _useLayers = enabled; compositeLayers(); }
    void setActiveLayer(int layer);
    void setLayerOpacity(float opacity);
    void setBlendMode(BlendMode mode);
    int addLayer();
    void deleteLayer(int layerIndex);
    void clearLayer(int layerIndex);
    void clearAllLayers();
    int getActiveLayer() const { return _activeLayer; }
    int getLayerCount() const { return static_cast<int>(_layers.size()); }
    const std::vector<Layer>& getLayers() const { return _layers; }
    float getLayerOpacity() const;
    BlendMode getBlendMode() const;
    void replaceLayers(const std::vector<Layer>& layers, int activeLayer, bool useLayers);
    void replaceLayers(std::vector<Layer>&& layers, int activeLayer, bool useLayers);

    // Clone tool
    void setCloneActive(bool active) { _cloneActive = active; }
    void setCloneOffset(const glm::vec2& offset) { _cloneOffset = offset; }
    void setCloneSource(const glm::vec2& source) { _cloneSource = source; }

    // Presets
    void addPreset(const BrushPreset& preset);
    void setCurrentPreset(int index);
    int getCurrentPreset() const { return _currentPreset; }
    const std::vector<BrushPreset>& getPresets() const { return _brushPresets; }

    // Core painting functions
    void paintDab(const glm::vec2& position, const glm::vec3& color, float pressure = 1.0f);
    void paintStroke(const glm::vec2& startPos, const glm::vec2& endPos, const glm::vec3& color);
    void eraseDab(const glm::vec2& position, float radius);
    bool sampleColor(const glm::vec2& position, glm::vec3& colorOut) const;
    
    // 2D specific painting (for Zone strokes)
    void paint2DStroke(const std::vector<glm::vec2>& points, const glm::vec3& color);

    // Undo/Redo
    void saveStrokeState();
    void undo();
    void redo();
    void clearHistory();

    // Getters
    BrushType getBrushType() const { return _currentBrushType; }
    float getRadius() const { return _brushRadius; }
    float getSoftness() const { return _brushSoftness; }
    float getOpacity() const { return _brushOpacity; }
    float getFlow() const { return _brushFlow; }
    float getSpacing() const { return _brushSpacing; }
    float getDensity() const { return _brushDensity; }
    float getStrength() const { return _brushStrength; }
    bool getPressureSimulation() const { return _usePressureSimulation; }
    float getPressureSensitivity() const { return _pressureSensitivity; }
    bool getStrokeInterpolation() const { return _useStrokeInterpolation; }
    bool getUseLayers() const { return _useLayers; }
    bool getCloneActive() const { return _cloneActive; }

    // Texture access (for 3D objects)
    const std::vector<uint8_t>& getCompositedTexture() const;
    int getTextureSize() const { return _textureSize; }
    void updateTexture();

    // Initialize default presets
    void initializeDefaultPresets();

private:
    // Brush settings
    BrushType _currentBrushType = BrushType::Normal;
    float _brushRadius = 0.001f;
    float _brushSoftness = 1.0f;
    float _brushOpacity = 1.0f;
    float _brushFlow = 1.0f;
    float _brushSpacing = 0.0005f;
    float _brushDensity = 0.5f;
    float _brushStrength = 0.5f;

    // Pressure simulation
    bool _usePressureSimulation = false;
    float _pressureSensitivity = 1.0f;
    float _currentPressure = 1.0f;
    glm::vec2 _lastPosition = glm::vec2(-1.0f, -1.0f);
    float _lastTime = 0.0f;

    // Stroke interpolation
    bool _useStrokeInterpolation = true;

    // Layer system
    bool _useLayers = false;
    int _activeLayer = 0;
    std::vector<Layer> _layers;
    int _textureSize;
    
    // Composited texture
    std::vector<uint8_t> _compositedTexture;
    int _compositeBatchDepth = 0;
    bool _compositeDirty = false;

    // Clone tool
    bool _cloneActive = false;
    glm::vec2 _cloneOffset = glm::vec2(0.0f, 0.0f);
    glm::vec2 _cloneSource = glm::vec2(0.0f, 0.0f);

    // Presets
    std::vector<BrushPreset> _brushPresets;
    int _currentPreset = 0;

    // Internal methods
    void applyBrushEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity);
    void applyAirbrushEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity);
    void applyChalkEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity);
    void applySprayEffect(uint8_t* targetBuffer, int x, int y, const glm::vec3& color, float intensity);
    void applySmudgeEffect(uint8_t* targetBuffer, int x, int y, float intensity);
    void applyCloneEffect(uint8_t* targetBuffer, int x, int y, float intensity);
    
    glm::vec3 blendPixels(const glm::vec3& src, const glm::vec3& dst, BlendMode mode, float opacity);
    void compositeLayers();
    void beginCompositeBatch();
    void endCompositeBatch();
    void requestComposite();
    float calculatePressure(const glm::vec2& currentPos, float currentTime);
    glm::vec2 normalizePosition(const glm::vec2& pos) const;
    
    // Utility functions
    bool isValidPosition(const glm::vec2& pos) const;
    int getPixelIndex(int x, int y) const;
    glm::vec3 getPixelColor(const uint8_t* buffer, int x, int y) const;
    void setPixelColor(uint8_t* buffer, int x, int y, const glm::vec3& color);
}; 
