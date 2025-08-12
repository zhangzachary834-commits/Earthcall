#pragma once

#include <vector>
#include <string>
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
        bool visible;
    };

    // Constructor
    BrushSystem(int textureSize = 64);
    ~BrushSystem();

    // Brush settings
    void setBrushType(BrushType type) { _currentBrushType = type; }
    void setRadius(float radius) { _brushRadius = radius; }
    void setSoftness(float softness) { _brushSoftness = softness; }
    void setOpacity(float opacity) { _brushOpacity = opacity; }
    void setFlow(float flow) { _brushFlow = flow; }
    void setSpacing(float spacing) { _brushSpacing = spacing; }
    void setDensity(float density) { _brushDensity = density; }
    void setStrength(float strength) { _brushStrength = strength; }

    // Pressure simulation
    void setPressureSimulation(bool enabled) { _usePressureSimulation = enabled; }
    void setPressureSensitivity(float sensitivity) { _pressureSensitivity = sensitivity; }
    void setCurrentPressure(float pressure) { _currentPressure = pressure; }

    // Stroke interpolation
    void setStrokeInterpolation(bool enabled) { _useStrokeInterpolation = enabled; }

    // Layer system
    void setUseLayers(bool enabled) { _useLayers = enabled; }
    void setActiveLayer(int layer) { _activeLayer = layer; }
    void setLayerOpacity(float opacity);
    void setBlendMode(BlendMode mode);
    int addLayer();
    void deleteLayer(int layerIndex);
    int getActiveLayer() const { return _activeLayer; }
    int getLayerCount() const { return static_cast<int>(_layers.size()); }
    const std::vector<Layer>& getLayers() const { return _layers; }

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
    float _brushRadius = 0.1f;
    float _brushSoftness = 1.0f;
    float _brushOpacity = 1.0f;
    float _brushFlow = 1.0f;
    float _brushSpacing = 0.1f;
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
    float calculatePressure(const glm::vec2& currentPos, float currentTime);
    
    // Utility functions
    bool isValidPosition(const glm::vec2& pos) const;
    int getPixelIndex(int x, int y) const;
    glm::vec3 getPixelColor(const uint8_t* buffer, int x, int y) const;
    void setPixelColor(uint8_t* buffer, int x, int y, const glm::vec3& color);
}; 