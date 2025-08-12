#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "OurVerse/Tool.hpp"
#include "BrushSystem.hpp"

// Forward declarations
class Zone;

// Text system for professional text editing
class TextSystem {
public:
    struct TextStyle {
        std::string fontFamily;
        float fontSize;
        bool bold;
        bool italic;
        bool underline;
        bool strikethrough;
        glm::vec3 color;
        float opacity;
        glm::vec2 alignment; // -1 to 1 for left/right, top/bottom
        float lineSpacing;
        float letterSpacing;
        bool wordWrap;
        int maxLines; // 0 = unlimited
        
        TextStyle() : 
            fontFamily("Arial"), fontSize(24.0f), bold(false), italic(false), 
            underline(false), strikethrough(false), color(0.0f, 0.0f, 0.0f), 
            opacity(1.0f), alignment(0.0f, 0.0f), lineSpacing(1.2f), 
            letterSpacing(0.0f), wordWrap(true), maxLines(0) {}
    };

    struct TextElement {
        std::string text;
        glm::vec2 position;
        TextStyle style;
        glm::mat4 transform;
        bool selected = false;
        bool visible = true;
        std::string id;
    };

    TextSystem();
    ~TextSystem();

    // Text management
    std::string addText(const std::string& text, const glm::vec2& position, const TextStyle& style = TextStyle());
    void removeText(const std::string& id);
    void updateText(const std::string& id, const std::string& newText);
    void setTextStyle(const std::string& id, const TextStyle& style);
    void setTextPosition(const std::string& id, const glm::vec2& position);
    void setTextTransform(const std::string& id, const glm::mat4& transform);
    
    // Text selection
    void selectText(const std::string& id);
    void deselectAll();
    std::vector<std::string> getSelectedTexts() const;
    
    // Text rendering
    void renderTexts() const;
    
    // Text effects
    void applyTextEffect(const std::string& id, const std::string& effectType, float intensity = 1.0f);
    
    // Getters
    const std::vector<TextElement>& getTextElements() const { return _textElements; }
    TextElement* getTextElement(const std::string& id);

private:
    std::vector<TextElement> _textElements;
    std::unordered_map<std::string, size_t> _textIndexMap;
    std::vector<std::string> _selectedTexts;
    int _nextTextId = 1;
};

// Shape system for professional shape creation
class ShapeSystem {
public:
    enum class ShapeType {
        Rectangle,
        Ellipse,
        Polygon,
        Line,
        Arrow,
        Star,
        Heart,
        Custom
    };

    struct ShapeStyle {
        glm::vec3 fillColor;
        glm::vec3 strokeColor;
        float fillOpacity;
        float strokeOpacity;
        float strokeWidth;
        bool fillEnabled;
        bool strokeEnabled;
        std::string strokeStyle; // solid, dashed, dotted
        float cornerRadius; // for rectangles
        int sides; // for polygons
        float starPoints; // for stars
        
        ShapeStyle() : 
            fillColor(1.0f, 1.0f, 1.0f), strokeColor(0.0f, 0.0f, 0.0f), 
            fillOpacity(1.0f), strokeOpacity(1.0f), strokeWidth(2.0f), 
            fillEnabled(true), strokeEnabled(true), strokeStyle("solid"), 
            cornerRadius(0.0f), sides(6), starPoints(5) {}
    };

    struct ShapeElement {
        ShapeType type;
        glm::vec2 position;
        glm::vec2 size;
        float rotation = 0.0f;
        ShapeStyle style;
        glm::mat4 transform;
        bool selected = false;
        bool visible = true;
        std::string id;
        std::vector<glm::vec2> customPoints; // for custom shapes
    };

    ShapeSystem();
    ~ShapeSystem();

    // Shape management
    std::string addShape(ShapeType type, const glm::vec2& position, const glm::vec2& size, const ShapeStyle& style = ShapeStyle());
    std::string addCustomShape(const std::vector<glm::vec2>& points, const glm::vec2& position, const ShapeStyle& style = ShapeStyle());
    void removeShape(const std::string& id);
    void updateShape(const std::string& id, const glm::vec2& position, const glm::vec2& size);
    void setShapeStyle(const std::string& id, const ShapeStyle& style);
    void setShapeTransform(const std::string& id, const glm::mat4& transform);
    
    // Shape selection
    void selectShape(const std::string& id);
    void deselectAll();
    std::vector<std::string> getSelectedShapes() const;
    
    // Shape rendering
    void renderShapes() const;
    
    // Shape effects
    void applyShapeEffect(const std::string& id, const std::string& effectType, float intensity = 1.0f);
    
    // Getters
    const std::vector<ShapeElement>& getShapeElements() const { return _shapeElements; }
    ShapeElement* getShapeElement(const std::string& id);

private:
    std::vector<ShapeElement> _shapeElements;
    std::unordered_map<std::string, size_t> _shapeIndexMap;
    std::vector<std::string> _selectedShapes;
    int _nextShapeId = 1;
    
    // Shape rendering helper methods
    void renderRectangle(float width, float height, float cornerRadius) const;
    void renderEllipse(float width, float height) const;
    void renderLine(float width, float height) const;
    void renderPolygon(float width, float height, int sides) const;
    void renderStar(float width, float height, float points) const;
    void renderHeart(float width, float height) const;
    void renderArrow(float width, float height) const;
    void renderCustomShape(const std::vector<glm::vec2>& points) const;
};

// Effects system for professional visual effects
class EffectsSystem {
public:
    enum class EffectType {
        Blur,
        Sharpen,
        Noise,
        Emboss,
        Glow,
        Shadow,
        Gradient,
        Pattern,
        Colorize,
        Brightness,
        Contrast,
        Saturation,
        Hue,
        Invert,
        Sepia,
        Vintage,
        Neon
    };

    struct Effect {
        EffectType type;
        float intensity = 1.0f;
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec2 offset = glm::vec2(0.0f, 0.0f);
        float radius = 10.0f;
        bool enabled = true;
        std::string id;
    };

    EffectsSystem();
    ~EffectsSystem();

    // Effect management
    std::string addEffect(EffectType type, float intensity = 1.0f);
    void removeEffect(const std::string& id);
    void updateEffect(const std::string& id, const Effect& effect);
    void enableEffect(const std::string& id, bool enabled);
    
    // Effect rendering
    void applyEffects(std::vector<uint8_t>& pixels, int width, int height) const;
    
    // Preset effects
    void applyPresetEffect(const std::string& presetName);
    
    // Getters
    const std::vector<Effect>& getEffects() const { return _effects; }
    Effect* getEffect(const std::string& id);

private:
    std::vector<Effect> _effects;
    std::unordered_map<std::string, size_t> _effectIndexMap;
    int _nextEffectId = 1;
    
    // Effect application methods
    void applyBlur(std::vector<uint8_t>& pixels, int width, int height, float intensity) const;
    void applySharpen(std::vector<uint8_t>& pixels, int width, int height, float intensity) const;
    void applyNoise(std::vector<uint8_t>& pixels, int width, int height, float intensity) const;
    void applyGlow(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const;
    void applyShadow(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const;
    void applyGradient(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const;
};

// Selection system for professional selection tools
class SelectionSystem {
public:
    enum class SelectionType {
        Rectangle,
        Ellipse,
        Lasso,
        MagicWand
    };

    struct Selection {
        SelectionType type;
        std::vector<glm::vec2> points;
        glm::vec2 bounds[2]; // min and max bounds
        bool active = false;
        std::string id;
    };

    SelectionSystem();
    ~SelectionSystem();

    // Selection management
    std::string createSelection(SelectionType type, const std::vector<glm::vec2>& points);
    void removeSelection(const std::string& id);
    void clearAllSelections();
    
    // Selection operations
    void selectAll();
    void invertSelection();
    void expandSelection(float amount);
    void contractSelection(float amount);
    void featherSelection(float amount);
    
    // Selection rendering
    void renderSelections() const;
    
    // Selection queries
    bool isPointSelected(const glm::vec2& point) const;
    std::vector<glm::vec2> getSelectedPoints() const;
    
    // Getters
    const std::vector<Selection>& getSelections() const { return _selections; }

private:
    std::vector<Selection> _selections;
    std::unordered_map<std::string, size_t> _selectionIndexMap;
    int _nextSelectionId = 1;
};

// Transform system for professional transformation tools
class TransformSystem {
public:
    enum class TransformType {
        Move,
        Scale,
        Rotate,
        Skew,
        Distort,
        Perspective
    };

    struct Transform {
        TransformType type;
        glm::vec2 position;
        glm::vec2 scale = glm::vec2(1.0f, 1.0f);
        float rotation = 0.0f;
        glm::vec2 skew = glm::vec2(0.0f, 0.0f);
        glm::mat4 matrix = glm::mat4(1.0f);
        bool active = false;
        std::string id;
    };

    TransformSystem();
    ~TransformSystem();

    // Transform management
    std::string createTransform(TransformType type);
    void removeTransform(const std::string& id);
    void updateTransform(const std::string& id, const Transform& transform);
    
    // Transform operations
    void applyTransform(const std::string& id, const glm::mat4& matrix);
    void resetTransform(const std::string& id);
    
    // Transform rendering
    void renderTransforms() const;
    
    // Getters
    const std::vector<Transform>& getTransforms() const { return _transforms; }
    Transform* getTransform(const std::string& id);

private:
    std::vector<Transform> _transforms;
    std::unordered_map<std::string, size_t> _transformIndexMap;
    int _nextTransformId = 1;
};

// Main design system that coordinates all subsystems
class DesignSystem {
public:
    DesignSystem();
    ~DesignSystem();

    // System initialization
    void initialize(Zone* zone);
    void cleanup();

    // Tool management
    void setCurrentTool(Tool::Type toolType);
    // Getters
    Tool::Type getCurrentTool() const { return _currentTool; }
    
    // Subsystem getters
    TextSystem* getTextSystem() { return _textSystem.get(); }
    ShapeSystem* getShapeSystem() { return _shapeSystem.get(); }
    EffectsSystem* getEffectsSystem() { return _effectsSystem.get(); }
    SelectionSystem* getSelectionSystem() { return _selectionSystem.get(); }
    TransformSystem* getTransformSystem() { return _transformSystem.get(); }
    BrushSystem* getBrushSystem() { return _brushSystem.get(); }
    
    // Drawing operations
    void startDrawing(const glm::vec2& position);
    void continueDrawing(const glm::vec2& position);
    void endDrawing();
    
    // Text operations
    void addText(const std::string& text, const glm::vec2& position);
    void editText(const std::string& id, const std::string& newText);
    void removeText(const std::string& id);
    
    // Shape operations
    void addShape(Tool::Type shapeType, const glm::vec2& position, const glm::vec2& size);
    void editShape(const std::string& id, const glm::vec2& position, const glm::vec2& size);
    void removeShape(const std::string& id);
    
    // Selection operations
    void startSelection(const glm::vec2& position);
    void updateSelection(const glm::vec2& position);
    void endSelection();
    void clearSelection();
    
    // Transform operations
    void startTransform(const glm::vec2& position);
    void updateTransform(const glm::vec2& position);
    void endTransform();
    
    // Effect operations
    void addEffect(Tool::Type effectType, float intensity = 1.0f);
    void removeEffect(const std::string& id);
    
    // Layer operations
    void addLayer();
    void removeLayer(int layerIndex);
    void setActiveLayer(int layerIndex);
    void setLayerOpacity(int layerIndex, float opacity);
    
    // Rendering
    void render() const;
    void renderUI() const;
    
    // Undo/Redo
    void undo();
    void redo();
    void clearHistory();
    
    // Save/Load
    void saveDesign(const std::string& filename) const;
    void loadDesign(const std::string& filename);
    


private:
    // Subsystems
    std::unique_ptr<TextSystem> _textSystem;
    std::unique_ptr<ShapeSystem> _shapeSystem;
    std::unique_ptr<EffectsSystem> _effectsSystem;
    std::unique_ptr<SelectionSystem> _selectionSystem;
    std::unique_ptr<TransformSystem> _transformSystem;
    std::unique_ptr<BrushSystem> _brushSystem;
    
    // Current state
    Tool::Type _currentTool = Tool::Type::Brush;
    Zone* _zone = nullptr;
    
    // Drawing state
    bool _isDrawing = false;
    bool _isSelecting = false;
    bool _isTransforming = false;
    glm::vec2 _startPosition = glm::vec2(0.0f, 0.0f);
    glm::vec2 _currentPosition = glm::vec2(0.0f, 0.0f);
    
    // Layer system
    struct Layer {
        std::string name;
        bool visible = true;
        float opacity = 1.0f;
        bool locked = false;
        std::vector<std::string> elements; // IDs of elements in this layer
    };
    std::vector<Layer> _layers;
    int _activeLayer = 0;
    
    // History system
    struct HistoryEntry {
        std::string action;
        std::string data; // JSON serialized data
        float timestamp;
    };
    std::vector<HistoryEntry> _history;
    size_t _historyIndex = 0;
    
    // Helper methods
    void saveHistoryEntry(const std::string& action, const std::string& data);
    void clearFutureHistory();
    ShapeSystem::ShapeType mapToolToShapeType(Tool::Type toolType) const;
    EffectsSystem::EffectType mapToolToEffectType(Tool::Type toolType) const;
}; 