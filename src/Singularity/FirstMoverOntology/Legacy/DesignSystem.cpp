#include "DesignSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ConstructedBeing/Singular/Object/Formation/Menu/stb_easy_font.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <utility>
#include "Singularity/Screen/Renderer.hpp"

namespace {

std::vector<glm::vec2> rectangleFromBounds(const glm::vec2& a, const glm::vec2& b) {
    const float minX = std::min(a.x, b.x);
    const float minY = std::min(a.y, b.y);
    const float maxX = std::max(a.x, b.x);
    const float maxY = std::max(a.y, b.y);
    return {
        glm::vec2(minX, minY),
        glm::vec2(maxX, minY),
        glm::vec2(maxX, maxY),
        glm::vec2(minX, maxY)
    };
}

bool pointInPolygon(const glm::vec2& point, const std::vector<glm::vec2>& polygon) {
    if (polygon.size() < 3) return false;

    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const glm::vec2& a = polygon[i];
        const glm::vec2& b = polygon[j];
        const bool intersects = ((a.y > point.y) != (b.y > point.y)) &&
            (point.x < (b.x - a.x) * (point.y - a.y) / ((b.y - a.y) + 0.000001f) + a.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

nlohmann::json vec2ToJson(const glm::vec2& value) {
    return nlohmann::json::array({value.x, value.y});
}

nlohmann::json vec3ToJson(const glm::vec3& value) {
    return nlohmann::json::array({value.x, value.y, value.z});
}

float jsonFloatAt(const nlohmann::json& value, size_t index, float fallback) {
    if (!value.is_array() || index >= value.size() || !value[index].is_number()) {
        return fallback;
    }
    return value[index].get<float>();
}

glm::vec2 vec2FromJson(const nlohmann::json& value, const glm::vec2& fallback = glm::vec2(0.0f)) {
    return glm::vec2(jsonFloatAt(value, 0, fallback.x), jsonFloatAt(value, 1, fallback.y));
}

glm::vec3 vec3FromJson(const nlohmann::json& value, const glm::vec3& fallback = glm::vec3(0.0f)) {
    return glm::vec3(jsonFloatAt(value, 0, fallback.x),
                     jsonFloatAt(value, 1, fallback.y),
                     jsonFloatAt(value, 2, fallback.z));
}

nlohmann::json textStyleToJson(const TextSystem::TextStyle& style) {
    return {
        {"fontFamily", style.fontFamily},
        {"fontSize", style.fontSize},
        {"bold", style.bold},
        {"italic", style.italic},
        {"underline", style.underline},
        {"strikethrough", style.strikethrough},
        {"color", vec3ToJson(style.color)},
        {"opacity", style.opacity},
        {"alignment", vec2ToJson(style.alignment)},
        {"lineSpacing", style.lineSpacing},
        {"letterSpacing", style.letterSpacing},
        {"wordWrap", style.wordWrap},
        {"maxLines", style.maxLines}
    };
}

TextSystem::TextStyle textStyleFromJson(const nlohmann::json& value) {
    TextSystem::TextStyle style;
    if (!value.is_object()) return style;
    style.fontFamily = value.value("fontFamily", style.fontFamily);
    style.fontSize = value.value("fontSize", style.fontSize);
    style.bold = value.value("bold", style.bold);
    style.italic = value.value("italic", style.italic);
    style.underline = value.value("underline", style.underline);
    style.strikethrough = value.value("strikethrough", style.strikethrough);
    if (value.contains("color")) style.color = vec3FromJson(value["color"], style.color);
    style.opacity = value.value("opacity", style.opacity);
    if (value.contains("alignment")) style.alignment = vec2FromJson(value["alignment"], style.alignment);
    style.lineSpacing = value.value("lineSpacing", style.lineSpacing);
    style.letterSpacing = value.value("letterSpacing", style.letterSpacing);
    style.wordWrap = value.value("wordWrap", style.wordWrap);
    style.maxLines = value.value("maxLines", style.maxLines);
    return style;
}

nlohmann::json shapeStyleToJson(const ShapeSystem::ShapeStyle& style) {
    return {
        {"fillColor", vec3ToJson(style.fillColor)},
        {"strokeColor", vec3ToJson(style.strokeColor)},
        {"fillOpacity", style.fillOpacity},
        {"strokeOpacity", style.strokeOpacity},
        {"strokeWidth", style.strokeWidth},
        {"fillEnabled", style.fillEnabled},
        {"strokeEnabled", style.strokeEnabled},
        {"strokeStyle", style.strokeStyle},
        {"cornerRadius", style.cornerRadius},
        {"sides", style.sides},
        {"starPoints", style.starPoints}
    };
}

ShapeSystem::ShapeStyle shapeStyleFromJson(const nlohmann::json& value) {
    ShapeSystem::ShapeStyle style;
    if (!value.is_object()) return style;
    if (value.contains("fillColor")) style.fillColor = vec3FromJson(value["fillColor"], style.fillColor);
    if (value.contains("strokeColor")) style.strokeColor = vec3FromJson(value["strokeColor"], style.strokeColor);
    style.fillOpacity = value.value("fillOpacity", style.fillOpacity);
    style.strokeOpacity = value.value("strokeOpacity", style.strokeOpacity);
    style.strokeWidth = value.value("strokeWidth", style.strokeWidth);
    style.fillEnabled = value.value("fillEnabled", style.fillEnabled);
    style.strokeEnabled = value.value("strokeEnabled", style.strokeEnabled);
    style.strokeStyle = value.value("strokeStyle", style.strokeStyle);
    style.cornerRadius = value.value("cornerRadius", style.cornerRadius);
    style.sides = value.value("sides", style.sides);
    style.starPoints = value.value("starPoints", style.starPoints);
    return style;
}

} // namespace

// ============================================================================
// TextSystem Implementation
// ============================================================================

TextSystem::TextSystem() {
    printf("TextSystem initialized\n");
}

TextSystem::~TextSystem() {
    // Cleanup handled by std::vector
}

std::string TextSystem::addText(const std::string& text, const glm::vec2& position, const TextStyle& style) {
    std::string id = "text_" + std::to_string(_nextTextId++);
    
    TextElement element;
    element.text = text;
    element.position = position;
    element.style = style;
    element.transform = glm::mat4(1.0f);
    element.id = id;
    
    _textElements.push_back(element);
    _textIndexMap[id] = _textElements.size() - 1;
    
    printf("Added text: %s at (%.1f, %.1f)\n", text.c_str(), position.x, position.y);
    return id;
}

void TextSystem::removeText(const std::string& id) {
    auto it = _textIndexMap.find(id);
    if (it != _textIndexMap.end()) {
        size_t index = it->second;
        _textElements.erase(_textElements.begin() + index);
        _textIndexMap.erase(it);
        _selectedTexts.erase(std::remove(_selectedTexts.begin(), _selectedTexts.end(), id), _selectedTexts.end());
        
        // Update indices
        for (auto& pair : _textIndexMap) {
            if (pair.second > index) {
                pair.second--;
            }
        }
    }
}

void TextSystem::updateText(const std::string& id, const std::string& newText) {
    TextElement* element = getTextElement(id);
    if (element) {
        element->text = newText;
    }
}

void TextSystem::setTextStyle(const std::string& id, const TextStyle& style) {
    TextElement* element = getTextElement(id);
    if (element) {
        element->style = style;
    }
}

void TextSystem::setTextPosition(const std::string& id, const glm::vec2& position) {
    TextElement* element = getTextElement(id);
    if (element) {
        element->position = position;
    }
}

void TextSystem::setTextTransform(const std::string& id, const glm::mat4& transform) {
    TextElement* element = getTextElement(id);
    if (element) {
        element->transform = transform;
    }
}

void TextSystem::selectText(const std::string& id) {
    // Deselect all first
    for (auto& element : _textElements) {
        element.selected = false;
    }
    _selectedTexts.clear();
    
    // Select the specified text
    TextElement* element = getTextElement(id);
    if (element) {
        element->selected = true;
        _selectedTexts.push_back(id);
    }
}

void TextSystem::deselectAll() {
    for (auto& element : _textElements) {
        element.selected = false;
    }
    _selectedTexts.clear();
}

std::vector<std::string> TextSystem::getSelectedTexts() const {
    return _selectedTexts;
}

void TextSystem::renderTexts() const {
    for (const auto& element : _textElements) {
        renderTextElement(element, 1.0f);
    }
}

void TextSystem::renderText(const std::string& id, float layerOpacity) const {
    auto it = _textIndexMap.find(id);
    if (it != _textIndexMap.end() && it->second < _textElements.size()) {
        renderTextElement(_textElements[it->second], layerOpacity);
    }
}

void TextSystem::renderTextElement(const TextElement& element, float layerOpacity) const {
    if (!element.visible) {
        return;
    }

    std::string text = element.text.empty() ? "Text" : element.text;
    char vertexBuffer[24000];
    int quads = stb_easy_font_print(0.0f, 0.0f, const_cast<char*>(text.c_str()), nullptr, vertexBuffer, sizeof(vertexBuffer));
    if (quads <= 0) return;

    // stb_easy_font draws at a fixed 16px cell, so font size is a scale — applied
    // to the glyph points here rather than via the retired matrix stack.
    const float scale = std::max(0.25f, element.style.fontSize / 16.0f);
    std::vector<glm::vec2> tris = draw::easyFontToTris(vertexBuffer, quads);
    for (glm::vec2& p : tris) p = element.position + p * scale;

    currentRenderer().drawTris2D(
        tris, glm::vec4(element.style.color,
                        std::clamp(element.style.opacity * layerOpacity, 0.0f, 1.0f)));
}

void TextSystem::applyTextEffect(const std::string& id, const std::string& effectType, float intensity) {
    TextElement* element = getTextElement(id);
    if (element) {
        // Apply text effects like glow, shadow, etc.
        printf("Applied %s effect to text %s with intensity %.2f\n", effectType.c_str(), id.c_str(), intensity);
    }
}

TextSystem::TextElement* TextSystem::getTextElement(const std::string& id) {
    auto it = _textIndexMap.find(id);
    if (it != _textIndexMap.end() && it->second < _textElements.size()) {
        return &_textElements[it->second];
    }
    return nullptr;
}

// ============================================================================
// ShapeSystem Implementation
// ============================================================================

ShapeSystem::ShapeSystem() {
    printf("ShapeSystem initialized\n");
}

ShapeSystem::~ShapeSystem() {
    // Cleanup handled by std::vector
}

std::string ShapeSystem::addShape(ShapeType type, const glm::vec2& position, const glm::vec2& size, const ShapeStyle& style) {
    std::string id = "shape_" + std::to_string(_nextShapeId++);
    
    ShapeElement element;
    element.type = type;
    element.position = position;
    element.size = size;
    element.style = style;
    element.transform = glm::mat4(1.0f);
    element.id = id;
    
    _shapeElements.push_back(element);
    _shapeIndexMap[id] = _shapeElements.size() - 1;
    
    printf("Added shape type %d at (%.1f, %.1f) size (%.1f, %.1f)\n", 
           static_cast<int>(type), position.x, position.y, size.x, size.y);
    return id;
}

std::string ShapeSystem::addCustomShape(const std::vector<glm::vec2>& points, const glm::vec2& position, const ShapeStyle& style) {
    std::string id = "shape_" + std::to_string(_nextShapeId++);
    
    ShapeElement element;
    element.type = ShapeType::Custom;
    element.position = position;
    element.size = glm::vec2(100.0f, 100.0f); // Default size
    element.style = style;
    element.transform = glm::mat4(1.0f);
    element.id = id;
    element.customPoints = points;
    
    _shapeElements.push_back(element);
    _shapeIndexMap[id] = _shapeElements.size() - 1;
    
    printf("Added custom shape with %zu points at (%.1f, %.1f)\n", points.size(), position.x, position.y);
    return id;
}

void ShapeSystem::removeShape(const std::string& id) {
    auto it = _shapeIndexMap.find(id);
    if (it != _shapeIndexMap.end()) {
        size_t index = it->second;
        _shapeElements.erase(_shapeElements.begin() + index);
        _shapeIndexMap.erase(it);
        _selectedShapes.erase(std::remove(_selectedShapes.begin(), _selectedShapes.end(), id), _selectedShapes.end());
        
        // Update indices
        for (auto& pair : _shapeIndexMap) {
            if (pair.second > index) {
                pair.second--;
            }
        }
    }
}

void ShapeSystem::updateShape(const std::string& id, const glm::vec2& position, const glm::vec2& size) {
    ShapeElement* element = getShapeElement(id);
    if (element) {
        element->position = position;
        element->size = size;
    }
}

void ShapeSystem::setShapeStyle(const std::string& id, const ShapeStyle& style) {
    ShapeElement* element = getShapeElement(id);
    if (element) {
        element->style = style;
    }
}

void ShapeSystem::setShapeTransform(const std::string& id, const glm::mat4& transform) {
    ShapeElement* element = getShapeElement(id);
    if (element) {
        element->transform = transform;
    }
}

void ShapeSystem::selectShape(const std::string& id) {
    // Deselect all first
    for (auto& element : _shapeElements) {
        element.selected = false;
    }
    _selectedShapes.clear();
    
    // Select the specified shape
    ShapeElement* element = getShapeElement(id);
    if (element) {
        element->selected = true;
        _selectedShapes.push_back(id);
    }
}

void ShapeSystem::deselectAll() {
    for (auto& element : _shapeElements) {
        element.selected = false;
    }
    _selectedShapes.clear();
}

std::vector<std::string> ShapeSystem::getSelectedShapes() const {
    return _selectedShapes;
}

void ShapeSystem::renderShapes() const {
    for (const auto& element : _shapeElements) {
        renderShapeElement(element, 1.0f);
    }
}

void ShapeSystem::renderShape(const std::string& id, float layerOpacity) const {
    auto it = _shapeIndexMap.find(id);
    if (it != _shapeIndexMap.end() && it->second < _shapeElements.size()) {
        renderShapeElement(_shapeElements[it->second], layerOpacity);
    }
}

void ShapeSystem::renderShapeElement(const ShapeElement& element, float layerOpacity) const {
    if (!element.visible) {
        return;
    }

    // Local space -> screen space. The GL version pushed a translate+rotate onto
    // the matrix stack; with no stack under WebGPU, the transform is applied to
    // the points as they are built. A Line is the one shape with an open loop.
    const float rad = glm::radians(element.rotation);
    const float c = std::cos(rad), s = std::sin(rad);
    auto place = [&](const std::vector<glm::vec2>& loop) {
        std::vector<glm::vec2> out;
        out.reserve(loop.size());
        for (const glm::vec2& p : loop)
            out.push_back({element.position.x + p.x * c - p.y * s,
                           element.position.y + p.x * s + p.y * c});
        return out;
    };

    const ShapeLoops loops = buildShape(element);
    const bool openLoop = (element.type == ShapeType::Line);
    Renderer& r = currentRenderer();

    if (element.style.fillEnabled && !openLoop) {
        const glm::vec4 color(element.style.fillColor,
                              std::clamp(element.style.fillOpacity * layerOpacity, 0.0f, 1.0f));
        for (const auto& loop : loops)
            r.drawTris2D(draw::fanToTris(place(loop)), color);
    }

    if (element.style.strokeEnabled || openLoop) {
        const glm::vec4 color(element.style.strokeColor,
                              std::clamp(element.style.strokeOpacity * layerOpacity, 0.0f, 1.0f));
        for (const auto& loop : loops)
            r.drawLines2D(draw::stripToSegments(place(loop), !openLoop),
                          color, element.style.strokeWidth);
    }
}

void ShapeSystem::applyShapeEffect(const std::string& id, const std::string& effectType, float intensity) {
    ShapeElement* element = getShapeElement(id);
    if (element) {
        // Apply shape effects like glow, shadow, etc.
        printf("Applied %s effect to shape %s with intensity %.2f\n", effectType.c_str(), id.c_str(), intensity);
    }
}

ShapeSystem::ShapeElement* ShapeSystem::getShapeElement(const std::string& id) {
    auto it = _shapeIndexMap.find(id);
    if (it != _shapeIndexMap.end() && it->second < _shapeElements.size()) {
        return &_shapeElements[it->second];
    }
    return nullptr;
}

// Shape geometry builders. These used to emit GL directly and were invoked twice
// per element — once filled, once under glPolygonMode(GL_LINE) for the stroke.
// Returning the geometry instead lets both passes share one construction, and
// keeps the exact primitive decomposition GL had.
ShapeSystem::ShapeLoops ShapeSystem::buildRectangle(float width, float height, float cornerRadius) {
    if (cornerRadius <= 0.0f) {
        // Simple rectangle
        return {{{-width/2, -height/2}, {width/2, -height/2},
                 {width/2, height/2},   {-width/2, height/2}}};
    }
    // Rounded rectangle (simplified): a centre band plus a left and right band.
    // Three separate quads, as before — so a stroked one still shows the seams.
    return {
        {{-width/2 + cornerRadius, -height/2}, {width/2 - cornerRadius, -height/2},
         {width/2 - cornerRadius,  height/2},  {-width/2 + cornerRadius, height/2}},
        {{-width/2, -height/2 + cornerRadius}, {-width/2 + cornerRadius, -height/2 + cornerRadius},
         {-width/2 + cornerRadius, height/2 - cornerRadius}, {-width/2, height/2 - cornerRadius}},
        {{width/2 - cornerRadius, -height/2 + cornerRadius}, {width/2, -height/2 + cornerRadius},
         {width/2, height/2 - cornerRadius}, {width/2 - cornerRadius, height/2 - cornerRadius}},
    };
}

ShapeSystem::ShapeLoops ShapeSystem::buildEllipse(float width, float height) {
    const int segments = 32;
    std::vector<glm::vec2> ring;
    ring.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        ring.push_back({(width/2) * std::cos(angle), (height/2) * std::sin(angle)});
    }
    return {ring};
}

ShapeSystem::ShapeLoops ShapeSystem::buildLine(float width, float height) {
    return {{{-width/2, -height/2}, {width/2, height/2}}};
}

ShapeSystem::ShapeLoops ShapeSystem::buildPolygon(float width, float height, int sides) {
    if (sides < 3) sides = 3;
    if (sides > 20) sides = 20;
    std::vector<glm::vec2> ring;
    ring.reserve(sides);
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        ring.push_back({(width/2) * std::cos(angle), (height/2) * std::sin(angle)});
    }
    return {ring};
}

ShapeSystem::ShapeLoops ShapeSystem::buildStar(float width, float height, float points) {
    if (points < 3) points = 3;
    if (points > 20) points = 20;
    const float outerRadius = std::min(width, height) * 0.5f;
    const int n = static_cast<int>(points) * 2;
    std::vector<glm::vec2> ring;
    ring.reserve(n);
    for (int i = 0; i < n; ++i) {
        float angle = 2.0f * M_PI * i / n;
        float radius = (i % 2 == 0) ? outerRadius : outerRadius * 0.5f;
        ring.push_back({radius * std::cos(angle), radius * std::sin(angle)});
    }
    return {ring};
}

ShapeSystem::ShapeLoops ShapeSystem::buildHeart(float width, float height) {
    const int segments = 32;
    std::vector<glm::vec2> ring;
    ring.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        float t = 2.0f * M_PI * i / segments;
        ring.push_back({(width/2) * 16 * std::pow(std::sin(t), 3),
                        (height/2) * -(13 * std::cos(t) - 5 * std::cos(2*t)
                                       - 2 * std::cos(3*t) - std::cos(4*t))});
    }
    return {ring};
}

ShapeSystem::ShapeLoops ShapeSystem::buildArrow(float width, float height) {
    return {{
        // Arrow shaft
        {-width/2, -height/6}, {width/3, -height/6},
        {width/3, height/6},   {-width/2, height/6},
        // Arrow head
        {width/3, -height/2},  {width/2, 0}, {width/3, height/2},
    }};
}

ShapeSystem::ShapeLoops ShapeSystem::buildCustomShape(const std::vector<glm::vec2>& points) {
    if (points.size() < 3) return {};
    return {points};
}

ShapeSystem::ShapeLoops ShapeSystem::buildShape(const ShapeElement& e) {
    switch (e.type) {
        case ShapeType::Rectangle: return buildRectangle(e.size.x, e.size.y, e.style.cornerRadius);
        case ShapeType::Ellipse:   return buildEllipse(e.size.x, e.size.y);
        case ShapeType::Line:      return buildLine(e.size.x, e.size.y);
        case ShapeType::Polygon:   return buildPolygon(e.size.x, e.size.y, e.style.sides);
        case ShapeType::Star:      return buildStar(e.size.x, e.size.y, e.style.starPoints);
        case ShapeType::Heart:     return buildHeart(e.size.x, e.size.y);
        case ShapeType::Arrow:     return buildArrow(e.size.x, e.size.y);
        case ShapeType::Custom:    return buildCustomShape(e.customPoints);
    }
    return {};
}

// ============================================================================
// EffectsSystem Implementation
// ============================================================================

EffectsSystem::EffectsSystem() {
    printf("EffectsSystem initialized\n");
}

EffectsSystem::~EffectsSystem() {
    // Cleanup handled by std::vector
}

std::string EffectsSystem::addEffect(EffectType type, float intensity) {
    std::string id = "effect_" + std::to_string(_nextEffectId++);
    
    Effect effect;
    effect.type = type;
    effect.intensity = intensity;
    effect.id = id;
    
    _effects.push_back(effect);
    _effectIndexMap[id] = _effects.size() - 1;
    
    printf("Added effect type %d with intensity %.2f\n", static_cast<int>(type), intensity);
    return id;
}

void EffectsSystem::removeEffect(const std::string& id) {
    auto it = _effectIndexMap.find(id);
    if (it != _effectIndexMap.end()) {
        size_t index = it->second;
        _effects.erase(_effects.begin() + index);
        _effectIndexMap.erase(it);
        
        // Update indices
        for (auto& pair : _effectIndexMap) {
            if (pair.second > index) {
                pair.second--;
            }
        }
    }
}

void EffectsSystem::updateEffect(const std::string& id, const Effect& effect) {
    Effect* existingEffect = getEffect(id);
    if (existingEffect) {
        *existingEffect = effect;
    }
}

void EffectsSystem::enableEffect(const std::string& id, bool enabled) {
    Effect* effect = getEffect(id);
    if (effect) {
        effect->enabled = enabled;
    }
}

void EffectsSystem::applyEffects(std::vector<uint8_t>& pixels, int width, int height) const {
    for (const auto& effect : _effects) {
        if (!effect.enabled) continue;
        
        switch (effect.type) {
            case EffectType::Blur:
                applyBlur(pixels, width, height, effect.intensity);
                break;
            case EffectType::Sharpen:
                applySharpen(pixels, width, height, effect.intensity);
                break;
            case EffectType::Noise:
                applyNoise(pixels, width, height, effect.intensity);
                break;
            case EffectType::Glow:
                applyGlow(pixels, width, height, effect);
                break;
            case EffectType::Shadow:
                applyShadow(pixels, width, height, effect);
                break;
            case EffectType::Gradient:
                applyGradient(pixels, width, height, effect);
                break;
            default:
                break;
        }
    }
}

void EffectsSystem::applyPresetEffect(const std::string& presetName) {
    // TODO: Implement preset effects
    printf("Applied preset effect: %s\n", presetName.c_str());
}

EffectsSystem::Effect* EffectsSystem::getEffect(const std::string& id) {
    auto it = _effectIndexMap.find(id);
    if (it != _effectIndexMap.end() && it->second < _effects.size()) {
        return &_effects[it->second];
    }
    return nullptr;
}

void EffectsSystem::applyBlur(std::vector<uint8_t>& pixels, int width, int height, float intensity) const {
    // Simple box blur implementation
    if (pixels.size() < static_cast<size_t>(width * height * 4)) return;
    
    std::vector<uint8_t> temp = pixels;
    int radius = static_cast<int>(intensity * 5.0f);
    radius = std::max(1, std::min(radius, 10));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;
            
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int index = (ny * width + nx) * 4;
                        r += temp[index];
                        g += temp[index + 1];
                        b += temp[index + 2];
                        a += temp[index + 3];
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                int index = (y * width + x) * 4;
                pixels[index] = r / count;
                pixels[index + 1] = g / count;
                pixels[index + 2] = b / count;
                pixels[index + 3] = a / count;
            }
        }
    }
}

void EffectsSystem::applySharpen(std::vector<uint8_t>& pixels, int width, int height, float intensity) const {
    // Simple sharpen implementation
    if (pixels.size() < static_cast<size_t>(width * height * 4)) return;
    
    std::vector<uint8_t> temp = pixels;
    float factor = intensity * 0.5f;
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int index = (y * width + x) * 4;
            
            for (int c = 0; c < 4; ++c) {
                int current = temp[index + c];
                int neighbors = temp[((y-1) * width + x) * 4 + c] +
                               temp[((y+1) * width + x) * 4 + c] +
                               temp[(y * width + x-1) * 4 + c] +
                               temp[(y * width + x+1) * 4 + c];
                
                int sharpened = static_cast<int>(current + factor * (current * 4 - neighbors));
                pixels[index + c] = std::clamp(sharpened, 0, 255);
            }
        }
    }
}

void EffectsSystem::applyNoise(std::vector<uint8_t>& pixels, int width, int height, float intensity) const {
    (void)width;
    (void)height;
    // Simple noise implementation
    for (size_t i = 0; i < pixels.size(); i += 4) {
        int noise = static_cast<int>((rand() % 100 - 50) * intensity);
        
        for (int c = 0; c < 3; ++c) { // RGB only
            int value = pixels[i + c] + noise;
            pixels[i + c] = std::clamp(value, 0, 255);
        }
    }
}

void EffectsSystem::applyGlow(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const {
    (void)pixels;
    (void)width;
    (void)height;
    // TODO: Implement glow effect
    printf("Applied glow effect with radius %.1f\n", effect.radius);
}

void EffectsSystem::applyShadow(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const {
    (void)pixels;
    (void)width;
    (void)height;
    // TODO: Implement shadow effect
    printf("Applied shadow effect with offset (%.1f, %.1f)\n", effect.offset.x, effect.offset.y);
}

void EffectsSystem::applyGradient(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const {
    (void)pixels;
    (void)width;
    (void)height;
    // TODO: Implement gradient effect
    printf("Applied gradient effect with color (%.1f, %.1f, %.1f)\n", effect.color.x, effect.color.y, effect.color.z);
}

// ============================================================================
// SelectionSystem Implementation
// ============================================================================

SelectionSystem::SelectionSystem() {
    printf("SelectionSystem initialized\n");
}

SelectionSystem::~SelectionSystem() {
    // Cleanup handled by std::vector
}

std::string SelectionSystem::createSelection(SelectionType type, const std::vector<glm::vec2>& points) {
    if (points.empty()) {
        return "";
    }

    std::string id = "selection_" + std::to_string(_nextSelectionId++);
    
    Selection selection;
    selection.type = type;
    selection.points = (type == SelectionType::Rectangle && points.size() == 2)
        ? rectangleFromBounds(points[0], points[1])
        : points;
    selection.active = true;
    selection.id = id;
    
    // Calculate bounds
    if (!selection.points.empty()) {
        selection.bounds[0] = selection.points[0];
        selection.bounds[1] = selection.points[0];
        
        for (const auto& point : selection.points) {
            selection.bounds[0].x = std::min(selection.bounds[0].x, point.x);
            selection.bounds[0].y = std::min(selection.bounds[0].y, point.y);
            selection.bounds[1].x = std::max(selection.bounds[1].x, point.x);
            selection.bounds[1].y = std::max(selection.bounds[1].y, point.y);
        }
    }
    
    _selections.push_back(selection);
    _selectionIndexMap[id] = _selections.size() - 1;
    
    printf("Created selection type %d with %zu points\n", static_cast<int>(type), points.size());
    return id;
}

void SelectionSystem::removeSelection(const std::string& id) {
    auto it = _selectionIndexMap.find(id);
    if (it != _selectionIndexMap.end()) {
        size_t index = it->second;
        _selections.erase(_selections.begin() + index);
        _selectionIndexMap.erase(it);
        
        // Update indices
        for (auto& pair : _selectionIndexMap) {
            if (pair.second > index) {
                pair.second--;
            }
        }
    }
}

void SelectionSystem::clearAllSelections() {
    _selections.clear();
    _selectionIndexMap.clear();
}

void SelectionSystem::selectAll() {
    createSelection(SelectionType::Rectangle, {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f)});
}

void SelectionSystem::invertSelection() {
    for (auto& selection : _selections) {
        selection.active = !selection.active;
    }
}

void SelectionSystem::expandSelection(float amount) {
    amount = std::max(0.0f, amount);
    for (auto& selection : _selections) {
        if (!selection.active) continue;
        selection.bounds[0] -= glm::vec2(amount);
        selection.bounds[1] += glm::vec2(amount);
        selection.points = rectangleFromBounds(selection.bounds[0], selection.bounds[1]);
        selection.type = SelectionType::Rectangle;
    }
}

void SelectionSystem::contractSelection(float amount) {
    amount = std::max(0.0f, amount);
    for (auto& selection : _selections) {
        if (!selection.active) continue;
        selection.bounds[0] += glm::vec2(amount);
        selection.bounds[1] -= glm::vec2(amount);
        if (selection.bounds[0].x > selection.bounds[1].x) {
            std::swap(selection.bounds[0].x, selection.bounds[1].x);
        }
        if (selection.bounds[0].y > selection.bounds[1].y) {
            std::swap(selection.bounds[0].y, selection.bounds[1].y);
        }
        selection.points = rectangleFromBounds(selection.bounds[0], selection.bounds[1]);
        selection.type = SelectionType::Rectangle;
    }
}

void SelectionSystem::featherSelection(float amount) {
    expandSelection(amount * 0.5f);
}

void SelectionSystem::renderSelections() const {
    for (const auto& selection : _selections) {
        if (selection.active) {
            // The marching-ants outline. GL_LINE_STIPPLE with pattern 0x00FF was 8
            // pixels on, 8 off; that state does not exist outside fixed-function GL,
            // so the dashes are cut into the segment list instead.
            const auto loop = draw::stripToSegments(selection.points, /*closed=*/true);
            currentRenderer().drawLines2D(draw::dashSegments(loop, 8.0f, 8.0f),
                                          glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), // Blue selection color
                                          2.0f);
        }
    }
}

bool SelectionSystem::isPointSelected(const glm::vec2& point) const {
    for (const auto& selection : _selections) {
        if (!selection.active) continue;
        const bool inBounds =
            point.x >= selection.bounds[0].x && point.x <= selection.bounds[1].x &&
            point.y >= selection.bounds[0].y && point.y <= selection.bounds[1].y;
        if (!inBounds) continue;

        if (selection.type == SelectionType::Lasso) {
            if (pointInPolygon(point, selection.points)) {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

std::vector<glm::vec2> SelectionSystem::getSelectedPoints() const {
    std::vector<glm::vec2> points;
    for (const auto& selection : _selections) {
        if (selection.active) {
            points.insert(points.end(), selection.points.begin(), selection.points.end());
        }
    }
    return points;
}

// ============================================================================
// TransformSystem Implementation
// ============================================================================

TransformSystem::TransformSystem() {
    printf("TransformSystem initialized\n");
}

TransformSystem::~TransformSystem() {
    // Cleanup handled by std::vector
}

std::string TransformSystem::createTransform(TransformType type) {
    std::string id = "transform_" + std::to_string(_nextTransformId++);
    
    Transform transform;
    transform.type = type;
    transform.active = true;
    transform.id = id;
    
    _transforms.push_back(transform);
    _transformIndexMap[id] = _transforms.size() - 1;
    
    printf("Created transform type %d\n", static_cast<int>(type));
    return id;
}

void TransformSystem::removeTransform(const std::string& id) {
    auto it = _transformIndexMap.find(id);
    if (it != _transformIndexMap.end()) {
        size_t index = it->second;
        _transforms.erase(_transforms.begin() + index);
        _transformIndexMap.erase(it);
        
        // Update indices
        for (auto& pair : _transformIndexMap) {
            if (pair.second > index) {
                pair.second--;
            }
        }
    }
}

void TransformSystem::updateTransform(const std::string& id, const Transform& transform) {
    Transform* existingTransform = getTransform(id);
    if (existingTransform) {
        *existingTransform = transform;
    }
}

void TransformSystem::applyTransform(const std::string& id, const glm::mat4& matrix) {
    Transform* transform = getTransform(id);
    if (transform) {
        transform->matrix = matrix;
    }
}

void TransformSystem::resetTransform(const std::string& id) {
    Transform* transform = getTransform(id);
    if (transform) {
        transform->matrix = glm::mat4(1.0f);
        transform->position = glm::vec2(0.0f, 0.0f);
        transform->scale = glm::vec2(1.0f, 1.0f);
        transform->rotation = 0.0f;
        transform->skew = glm::vec2(0.0f, 0.0f);
    }
}

void TransformSystem::renderTransforms() const {
    for (const auto& transform : _transforms) {
        if (transform.active) {
            Renderer& r = currentRenderer();

            // Render transform handles. GL_POINTS + glPointSize(8) has no WebGPU
            // equivalent, so an 8px point becomes an 8px filled square.
            r.drawTris2D(draw::pointsToTris({transform.position}, 8.0f),
                         glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow handles

            // Render bounding box
            r.drawLines2D(draw::rectOutline({transform.position.x - 20, transform.position.y - 20,
                                             transform.position.x + 20, transform.position.y + 20}),
                          glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // Orange bounding box
                          1.0f);
        }
    }
}

TransformSystem::Transform* TransformSystem::getTransform(const std::string& id) {
    auto it = _transformIndexMap.find(id);
    if (it != _transformIndexMap.end() && it->second < _transforms.size()) {
        return &_transforms[it->second];
    }
    return nullptr;
}

// ============================================================================
// DesignSystem Implementation
// ============================================================================

DesignSystem::DesignSystem() {
    printf("DesignSystem initialized\n");
    
    // Initialize subsystems
    _textSystem = std::make_unique<TextSystem>();
    _shapeSystem = std::make_unique<ShapeSystem>();
    _effectsSystem = std::make_unique<EffectsSystem>();
    _selectionSystem = std::make_unique<SelectionSystem>();
    _transformSystem = std::make_unique<TransformSystem>();
    _brushSystem = std::make_unique<BrushSystem>(512);
    
    // Initialize default layer
    Layer defaultLayer;
    defaultLayer.name = "Background";
    _layers.push_back(defaultLayer);
}

DesignSystem::~DesignSystem() {
    cleanup();
}

void DesignSystem::initialize(Zone* zone) {
    _zone = zone;
    printf("DesignSystem initialized for zone: %s\n", zone ? zone->name().c_str() : "null");
}

void DesignSystem::cleanup() {
    _zone = nullptr;
    printf("DesignSystem cleaned up\n");
}

void DesignSystem::setCurrentTool(Tool::Type toolType) {
    _currentTool = toolType;
    printf("Current tool set to: %s\n", Tool(toolType).getTypeName().c_str());
}

void DesignSystem::startDrawing(const glm::vec2& position) {
    _isDrawing = true;
    _startPosition = position;
    _currentPosition = position;
    
    if (_brushSystem && _zone) {
        _brushSystem->saveStrokeState();
        // Use the current color from the zone instead of hardcoded values
        glm::vec3 currentColor = glm::vec3(1.0f);
        _brushSystem->paintDab(position, currentColor);
    }
    
    printf("Started drawing at (%.1f, %.1f)\n", position.x, position.y);
}

void DesignSystem::continueDrawing(const glm::vec2& position) {
    if (_isDrawing) {
        if (_brushSystem && _zone) {
            // Use the current color from the zone instead of hardcoded values
            glm::vec3 currentColor = glm::vec3(1.0f);
            _brushSystem->paintStroke(_currentPosition, position, currentColor);
        }
        _currentPosition = position;
        
        printf("Continued drawing at (%.1f, %.1f)\n", position.x, position.y);
    }
}

void DesignSystem::endDrawing() {
    if (_isDrawing) {
        _isDrawing = false;
        
        printf("Ended drawing\n");
    }
}

void DesignSystem::addText(const std::string& text, const glm::vec2& position) {
    if (_textSystem && _activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        std::string id = _textSystem->addText(text, position);
        _layers[_activeLayer].elements.push_back(id);
        saveHistoryEntry("add_text", nlohmann::json({
            {"id", id},
            {"text", text},
            {"position", vec2ToJson(position)},
            {"layer", _activeLayer}
        }).dump());
    }
}

void DesignSystem::editText(const std::string& id, const std::string& newText) {
    if (_textSystem) {
        _textSystem->updateText(id, newText);
        saveHistoryEntry("edit_text", "{\"id\":\"" + id + "\",\"text\":\"" + newText + "\"}");
    }
}

void DesignSystem::removeText(const std::string& id) {
    if (_textSystem) {
        _textSystem->removeText(id);
        for (auto& layer : _layers) {
            layer.elements.erase(std::remove(layer.elements.begin(), layer.elements.end(), id), layer.elements.end());
        }
        saveHistoryEntry("remove_text", "{\"id\":\"" + id + "\"}");
    }
}

void DesignSystem::addShape(Tool::Type shapeType, const glm::vec2& position, const glm::vec2& size) {
    if (_shapeSystem && _activeLayer >= 0 && _activeLayer < static_cast<int>(_layers.size())) {
        ShapeSystem::ShapeType type = mapToolToShapeType(shapeType);
        std::string id = _shapeSystem->addShape(type, position, size);
        _layers[_activeLayer].elements.push_back(id);
        saveHistoryEntry("add_shape", nlohmann::json({
            {"id", id},
            {"type", static_cast<int>(type)},
            {"position", vec2ToJson(position)},
            {"size", vec2ToJson(size)},
            {"layer", _activeLayer}
        }).dump());
    }
}

void DesignSystem::editShape(const std::string& id, const glm::vec2& position, const glm::vec2& size) {
    if (_shapeSystem) {
        _shapeSystem->updateShape(id, position, size);
        saveHistoryEntry("edit_shape", "{\"id\":\"" + id + "\"}");
    }
}

void DesignSystem::removeShape(const std::string& id) {
    if (_shapeSystem) {
        _shapeSystem->removeShape(id);
        for (auto& layer : _layers) {
            layer.elements.erase(std::remove(layer.elements.begin(), layer.elements.end(), id), layer.elements.end());
        }
        saveHistoryEntry("remove_shape", "{\"id\":\"" + id + "\"}");
    }
}

void DesignSystem::startSelection(const glm::vec2& position) {
    _isSelecting = true;
    _startPosition = position;
    _currentPosition = position;
    printf("Started selection at (%.1f, %.1f)\n", position.x, position.y);
}

void DesignSystem::updateSelection(const glm::vec2& position) {
    if (_isSelecting) {
        _currentPosition = position;
        printf("Updated selection to (%.1f, %.1f)\n", position.x, position.y);
    }
}

void DesignSystem::endSelection() {
    if (_isSelecting) {
        _isSelecting = false;
        
        if (_selectionSystem) {
            std::vector<glm::vec2> points = {_startPosition, _currentPosition};
            _selectionSystem->createSelection(SelectionSystem::SelectionType::Rectangle, points);
        }
        
        printf("Ended selection\n");
    }
}

void DesignSystem::clearSelection() {
    if (_selectionSystem) {
        _selectionSystem->clearAllSelections();
    }
}

void DesignSystem::startTransform(const glm::vec2& position) {
    _isTransforming = true;
    _startPosition = position;
    _currentPosition = position;
    printf("Started transform at (%.1f, %.1f)\n", position.x, position.y);
}

void DesignSystem::updateTransform(const glm::vec2& position) {
    if (_isTransforming) {
        _currentPosition = position;
        printf("Updated transform to (%.1f, %.1f)\n", position.x, position.y);
    }
}

void DesignSystem::endTransform() {
    if (_isTransforming) {
        _isTransforming = false;
        printf("Ended transform\n");
    }
}

void DesignSystem::addEffect(Tool::Type effectType, float intensity) {
    if (_effectsSystem) {
        EffectsSystem::EffectType type = mapToolToEffectType(effectType);
        std::string id = _effectsSystem->addEffect(type, intensity);
        saveHistoryEntry("add_effect", "{\"id\":\"" + id + "\",\"type\":" + std::to_string(static_cast<int>(type)) + "}");
    }
}

void DesignSystem::removeEffect(const std::string& id) {
    if (_effectsSystem) {
        _effectsSystem->removeEffect(id);
        saveHistoryEntry("remove_effect", "{\"id\":\"" + id + "\"}");
    }
}

void DesignSystem::addLayer() {
    Layer newLayer;
    newLayer.name = "Layer " + std::to_string(_layers.size() + 1);
    _layers.push_back(newLayer);
    _activeLayer = static_cast<int>(_layers.size()) - 1;
    printf("Added layer: %s\n", newLayer.name.c_str());
}

void DesignSystem::removeLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size()) && _layers.size() > 1) {
        _layers.erase(_layers.begin() + layerIndex);
        if (_activeLayer > layerIndex) {
            --_activeLayer;
        } else if (_activeLayer >= static_cast<int>(_layers.size())) {
            _activeLayer = static_cast<int>(_layers.size()) - 1;
        }
        printf("Removed layer %d\n", layerIndex);
    }
}

void DesignSystem::setActiveLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size())) {
        _activeLayer = layerIndex;
        printf("Active layer set to %d: %s\n", layerIndex, _layers[layerIndex].name.c_str());
    }
}

void DesignSystem::setLayerOpacity(int layerIndex, float opacity) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size())) {
        _layers[layerIndex].opacity = std::clamp(opacity, 0.0f, 1.0f);
        printf("Layer %d opacity set to %.2f\n", layerIndex, opacity);
    }
}

float DesignSystem::getLayerOpacity(int layerIndex) const {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size())) {
        return _layers[layerIndex].opacity;
    }
    return 1.0f;
}

const std::string& DesignSystem::getLayerName(int layerIndex) const {
    static const std::string fallback = "Layer";
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size())) {
        return _layers[layerIndex].name;
    }
    return fallback;
}

void DesignSystem::render() const {
    bool hasLayeredElements = false;
    for (const auto& layer : _layers) {
        if (!layer.elements.empty()) {
            hasLayeredElements = true;
            break;
        }
    }

    if (hasLayeredElements) {
        for (const auto& layer : _layers) {
            if (!layer.visible) {
                continue;
            }

            for (const auto& id : layer.elements) {
                if (id.rfind("text_", 0) == 0 && _textSystem) {
                    _textSystem->renderText(id, layer.opacity);
                } else if (id.rfind("shape_", 0) == 0 && _shapeSystem) {
                    _shapeSystem->renderShape(id, layer.opacity);
                }
            }
        }
    } else {
        if (_textSystem) _textSystem->renderTexts();
        if (_shapeSystem) _shapeSystem->renderShapes();
    }

    if (_selectionSystem) _selectionSystem->renderSelections();
    if (_transformSystem) _transformSystem->renderTransforms();
}

void DesignSystem::renderUI() const {
    // TODO: Implement UI rendering for the design system
    // This would show tool panels, property panels, etc.
}

void DesignSystem::undo() {
    if (_historyIndex > 0) {
        _historyIndex--;
        const auto& entry = _history[_historyIndex];
        try {
            nlohmann::json data = nlohmann::json::parse(entry.data);
            if (entry.action == "add_text" && _textSystem) {
                std::string id = data.value("id", "");
                _textSystem->removeText(id);
                for (auto& layer : _layers) {
                    layer.elements.erase(std::remove(layer.elements.begin(), layer.elements.end(), id), layer.elements.end());
                }
            } else if (entry.action == "add_shape" && _shapeSystem) {
                std::string id = data.value("id", "");
                _shapeSystem->removeShape(id);
                for (auto& layer : _layers) {
                    layer.elements.erase(std::remove(layer.elements.begin(), layer.elements.end(), id), layer.elements.end());
                }
            } else if (entry.action == "add_effect" && _effectsSystem) {
                _effectsSystem->removeEffect(data.value("id", ""));
            } else if (_brushSystem) {
                _brushSystem->undo();
            }
        } catch (const std::exception&) {
            if (_brushSystem) {
                _brushSystem->undo();
            }
        }
        printf("Undo: %s\n", entry.action.c_str());
    }
}

void DesignSystem::redo() {
    if (_historyIndex < _history.size()) {
        const auto& entry = _history[_historyIndex];
        try {
            nlohmann::json data = nlohmann::json::parse(entry.data);
            if (entry.action == "add_text" && _textSystem) {
                int layerIndex = std::clamp(data.value("layer", _activeLayer), 0, static_cast<int>(_layers.size()) - 1);
                std::string id = _textSystem->addText(
                    data.value("text", ""),
                    vec2FromJson(data.value("position", nlohmann::json::array())));
                _layers[layerIndex].elements.push_back(id);
            } else if (entry.action == "add_shape" && _shapeSystem) {
                int layerIndex = std::clamp(data.value("layer", _activeLayer), 0, static_cast<int>(_layers.size()) - 1);
                auto type = static_cast<ShapeSystem::ShapeType>(data.value("type", 0));
                std::string id = _shapeSystem->addShape(
                    type,
                    vec2FromJson(data.value("position", nlohmann::json::array())),
                    vec2FromJson(data.value("size", nlohmann::json::array()), glm::vec2(100.0f)));
                _layers[layerIndex].elements.push_back(id);
            } else if (entry.action == "add_effect" && _effectsSystem) {
                auto type = static_cast<EffectsSystem::EffectType>(data.value("type", 0));
                _effectsSystem->addEffect(type, 1.0f);
            } else if (_brushSystem) {
                _brushSystem->redo();
            }
        } catch (const std::exception&) {
            if (_brushSystem) {
                _brushSystem->redo();
            }
        }
        printf("Redo: %s\n", entry.action.c_str());
        _historyIndex++;
    }
}

void DesignSystem::clearHistory() {
    _history.clear();
    _historyIndex = 0;
    printf("History cleared\n");
}

void DesignSystem::saveDesign(const std::string& filename) const {
    nlohmann::json j;
    
    // Save layers
    nlohmann::json layersArray = nlohmann::json::array();
    for (const auto& layer : _layers) {
        nlohmann::json layerJson;
        layerJson["name"] = layer.name;
        layerJson["visible"] = layer.visible;
        layerJson["opacity"] = layer.opacity;
        layerJson["locked"] = layer.locked;
        layerJson["elements"] = layer.elements;
        layersArray.push_back(layerJson);
    }
    j["layers"] = layersArray;
    j["activeLayer"] = _activeLayer;

    if (_textSystem) {
        nlohmann::json textArray = nlohmann::json::array();
        for (const auto& text : _textSystem->getTextElements()) {
            textArray.push_back({
                {"id", text.id},
                {"text", text.text},
                {"position", vec2ToJson(text.position)},
                {"style", textStyleToJson(text.style)},
                {"selected", text.selected},
                {"visible", text.visible}
            });
        }
        j["texts"] = textArray;
    }

    if (_shapeSystem) {
        nlohmann::json shapeArray = nlohmann::json::array();
        for (const auto& shape : _shapeSystem->getShapeElements()) {
            nlohmann::json customPoints = nlohmann::json::array();
            for (const auto& point : shape.customPoints) {
                customPoints.push_back(vec2ToJson(point));
            }
            shapeArray.push_back({
                {"id", shape.id},
                {"type", static_cast<int>(shape.type)},
                {"position", vec2ToJson(shape.position)},
                {"size", vec2ToJson(shape.size)},
                {"rotation", shape.rotation},
                {"style", shapeStyleToJson(shape.style)},
                {"selected", shape.selected},
                {"visible", shape.visible},
                {"customPoints", customPoints}
            });
        }
        j["shapes"] = shapeArray;
    }

    if (_effectsSystem) {
        nlohmann::json effectArray = nlohmann::json::array();
        for (const auto& effect : _effectsSystem->getEffects()) {
            effectArray.push_back({
                {"id", effect.id},
                {"type", static_cast<int>(effect.type)},
                {"intensity", effect.intensity},
                {"color", vec3ToJson(effect.color)},
                {"offset", vec2ToJson(effect.offset)},
                {"radius", effect.radius},
                {"enabled", effect.enabled}
            });
        }
        j["effects"] = effectArray;
    }

    if (_selectionSystem) {
        nlohmann::json selectionArray = nlohmann::json::array();
        for (const auto& selection : _selectionSystem->getSelections()) {
            nlohmann::json points = nlohmann::json::array();
            for (const auto& point : selection.points) {
                points.push_back(vec2ToJson(point));
            }
            selectionArray.push_back({
                {"id", selection.id},
                {"type", static_cast<int>(selection.type)},
                {"points", points},
                {"active", selection.active}
            });
        }
        j["selections"] = selectionArray;
    }

    if (_transformSystem) {
        nlohmann::json transformArray = nlohmann::json::array();
        for (const auto& transform : _transformSystem->getTransforms()) {
            transformArray.push_back({
                {"id", transform.id},
                {"type", static_cast<int>(transform.type)},
                {"position", vec2ToJson(transform.position)},
                {"scale", vec2ToJson(transform.scale)},
                {"rotation", transform.rotation},
                {"skew", vec2ToJson(transform.skew)},
                {"active", transform.active}
            });
        }
        j["transforms"] = transformArray;
    }

    if (_brushSystem) {
        nlohmann::json brushJson;
        brushJson["activeLayer"] = _brushSystem->getActiveLayer();
        brushJson["useLayers"] = _brushSystem->getUseLayers();
        brushJson["textureSize"] = _brushSystem->getTextureSize();
        nlohmann::json brushLayers = nlohmann::json::array();
        for (const auto& layer : _brushSystem->getLayers()) {
            brushLayers.push_back({
                {"pixels", layer.pixels},
                {"opacity", layer.opacity},
                {"blendMode", static_cast<int>(layer.blendMode)},
                {"visible", layer.visible}
            });
        }
        brushJson["layers"] = brushLayers;
        j["brush"] = brushJson;
    }
    
    // Save history
    nlohmann::json historyArray = nlohmann::json::array();
    for (const auto& entry : _history) {
        nlohmann::json entryJson;
        entryJson["action"] = entry.action;
        entryJson["data"] = entry.data;
        entryJson["timestamp"] = entry.timestamp;
        historyArray.push_back(entryJson);
    }
    j["history"] = historyArray;
    j["historyIndex"] = _historyIndex;
    
    // Save current tool
    j["currentTool"] = static_cast<int>(_currentTool);
    
    // Use SaveSystem to write the file
    SaveSystem::writeSaveData(j, filename, SaveSystem::SaveType::DESIGN);
    printf("Saved design to: %s\n", filename.c_str());
}

void DesignSystem::loadDesign(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;

            std::unordered_map<std::string, std::string> idMap;
            if (_textSystem) {
                std::vector<std::string> ids;
                for (const auto& text : _textSystem->getTextElements()) ids.push_back(text.id);
                for (const auto& id : ids) _textSystem->removeText(id);
            }
            if (_shapeSystem) {
                std::vector<std::string> ids;
                for (const auto& shape : _shapeSystem->getShapeElements()) ids.push_back(shape.id);
                for (const auto& id : ids) _shapeSystem->removeShape(id);
            }
            if (_effectsSystem) {
                std::vector<std::string> ids;
                for (const auto& effect : _effectsSystem->getEffects()) ids.push_back(effect.id);
                for (const auto& id : ids) _effectsSystem->removeEffect(id);
            }
            if (_selectionSystem) {
                _selectionSystem->clearAllSelections();
            }
            if (_transformSystem) {
                std::vector<std::string> ids;
                for (const auto& transform : _transformSystem->getTransforms()) ids.push_back(transform.id);
                for (const auto& id : ids) _transformSystem->removeTransform(id);
            }
            
            // Load layers
            if (j.contains("layers")) {
                _layers.clear();
                const auto& layersArray = j["layers"];
                for (const auto& layerJson : layersArray) {
                    Layer layer;
                    layer.name = layerJson.value("name", "Layer");
                    layer.visible = layerJson.value("visible", true);
                    layer.opacity = layerJson.value("opacity", 1.0f);
                    layer.locked = layerJson.value("locked", false);
                    if (layerJson.contains("elements") && layerJson["elements"].is_array()) {
                        layer.elements = layerJson["elements"].get<std::vector<std::string>>();
                    }
                    _layers.push_back(layer);
                }
            }
            if (_layers.empty()) {
                Layer defaultLayer;
                defaultLayer.name = "Background";
                _layers.push_back(defaultLayer);
            }
            
            _activeLayer = j.value("activeLayer", 0);
            _activeLayer = std::clamp(_activeLayer, 0, static_cast<int>(_layers.size()) - 1);

            if (_textSystem && j.contains("texts") && j["texts"].is_array()) {
                for (const auto& textJson : j["texts"]) {
                    TextSystem::TextStyle style = textStyleFromJson(textJson.value("style", nlohmann::json::object()));
                    std::string newId = _textSystem->addText(
                        textJson.value("text", ""),
                        vec2FromJson(textJson.value("position", nlohmann::json::array()), glm::vec2(0.0f)),
                        style);
                    std::string oldId = textJson.value("id", newId);
                    idMap[oldId] = newId;
                    if (auto* text = _textSystem->getTextElement(newId)) {
                        text->visible = textJson.value("visible", text->visible);
                        text->selected = textJson.value("selected", text->selected);
                    }
                }
            }

            if (_shapeSystem && j.contains("shapes") && j["shapes"].is_array()) {
                for (const auto& shapeJson : j["shapes"]) {
                    ShapeSystem::ShapeType type = static_cast<ShapeSystem::ShapeType>(shapeJson.value("type", 0));
                    ShapeSystem::ShapeStyle style = shapeStyleFromJson(shapeJson.value("style", nlohmann::json::object()));
                    std::string newId;
                    if (type == ShapeSystem::ShapeType::Custom && shapeJson.contains("customPoints")) {
                        std::vector<glm::vec2> points;
                        for (const auto& pointJson : shapeJson["customPoints"]) {
                            points.push_back(vec2FromJson(pointJson));
                        }
                        newId = _shapeSystem->addCustomShape(points, vec2FromJson(shapeJson.value("position", nlohmann::json::array())), style);
                    } else {
                        newId = _shapeSystem->addShape(
                            type,
                            vec2FromJson(shapeJson.value("position", nlohmann::json::array())),
                            vec2FromJson(shapeJson.value("size", nlohmann::json::array()), glm::vec2(100.0f)),
                            style);
                    }
                    std::string oldId = shapeJson.value("id", newId);
                    idMap[oldId] = newId;
                    if (auto* shape = _shapeSystem->getShapeElement(newId)) {
                        shape->rotation = shapeJson.value("rotation", shape->rotation);
                        shape->visible = shapeJson.value("visible", shape->visible);
                        shape->selected = shapeJson.value("selected", shape->selected);
                    }
                }
            }

            for (auto& layer : _layers) {
                for (auto& id : layer.elements) {
                    auto mapped = idMap.find(id);
                    if (mapped != idMap.end()) {
                        id = mapped->second;
                    }
                }
            }

            if (_effectsSystem && j.contains("effects") && j["effects"].is_array()) {
                for (const auto& effectJson : j["effects"]) {
                    auto type = static_cast<EffectsSystem::EffectType>(effectJson.value("type", 0));
                    std::string id = _effectsSystem->addEffect(type, effectJson.value("intensity", 1.0f));
                    if (auto* effect = _effectsSystem->getEffect(id)) {
                        effect->color = vec3FromJson(effectJson.value("color", nlohmann::json::array()), effect->color);
                        effect->offset = vec2FromJson(effectJson.value("offset", nlohmann::json::array()), effect->offset);
                        effect->radius = effectJson.value("radius", effect->radius);
                        effect->enabled = effectJson.value("enabled", effect->enabled);
                    }
                }
            }

            if (_selectionSystem && j.contains("selections") && j["selections"].is_array()) {
                for (const auto& selectionJson : j["selections"]) {
                    std::vector<glm::vec2> points;
                    if (selectionJson.contains("points")) {
                        for (const auto& pointJson : selectionJson["points"]) {
                            points.push_back(vec2FromJson(pointJson));
                        }
                    }
                    auto type = static_cast<SelectionSystem::SelectionType>(selectionJson.value("type", 0));
                    _selectionSystem->createSelection(type, points);
                }
            }

            if (_transformSystem && j.contains("transforms") && j["transforms"].is_array()) {
                for (const auto& transformJson : j["transforms"]) {
                    auto type = static_cast<TransformSystem::TransformType>(transformJson.value("type", 0));
                    std::string id = _transformSystem->createTransform(type);
                    if (auto* transform = _transformSystem->getTransform(id)) {
                        transform->position = vec2FromJson(transformJson.value("position", nlohmann::json::array()), transform->position);
                        transform->scale = vec2FromJson(transformJson.value("scale", nlohmann::json::array()), transform->scale);
                        transform->rotation = transformJson.value("rotation", transform->rotation);
                        transform->skew = vec2FromJson(transformJson.value("skew", nlohmann::json::array()), transform->skew);
                        transform->active = transformJson.value("active", transform->active);
                    }
                }
            }

            if (_brushSystem && j.contains("brush") && j["brush"].is_object()) {
                std::vector<BrushSystem::Layer> brushLayers;
                const auto& brushJson = j["brush"];
                if (brushJson.contains("layers") && brushJson["layers"].is_array()) {
                    for (const auto& layerJson : brushJson["layers"]) {
                        BrushSystem::Layer layer;
                        layer.opacity = layerJson.value("opacity", 1.0f);
                        layer.blendMode = static_cast<BrushSystem::BlendMode>(layerJson.value("blendMode", 0));
                        layer.visible = layerJson.value("visible", true);
                        if (layerJson.contains("pixels") && layerJson["pixels"].is_array()) {
                            layer.pixels = layerJson["pixels"].get<std::vector<uint8_t>>();
                        }
                        brushLayers.push_back(std::move(layer));
                    }
                }
                _brushSystem->replaceLayers(
                    brushLayers,
                    brushJson.value("activeLayer", 0),
                    brushJson.value("useLayers", false));
            }
            
            // Load history
            if (j.contains("history")) {
                _history.clear();
                const auto& historyArray = j["history"];
                for (const auto& entryJson : historyArray) {
                    HistoryEntry entry;
                    entry.action = entryJson.value("action", "");
                    entry.data = entryJson.value("data", "");
                    entry.timestamp = entryJson.value("timestamp", 0.0f);
                    _history.push_back(entry);
                }
            }
            
            _historyIndex = j.value("historyIndex", 0);
            _historyIndex = std::min(_historyIndex, _history.size());
            
            // Load current tool
            if (j.contains("currentTool")) {
                _currentTool = static_cast<Tool::Type>(j["currentTool"].get<int>());
            }
            
            file.close();
            printf("Loaded design from: %s\n", filename.c_str());
        }
    } catch (const std::exception& e) {
        printf("Error loading design: %s\n", e.what());
    }
}

void DesignSystem::saveHistoryEntry(const std::string& action, const std::string& data) {
    clearFutureHistory();
    
    HistoryEntry entry;
    entry.action = action;
    entry.data = data;
    entry.timestamp = static_cast<float>(glfwGetTime());
    
    _history.push_back(entry);
    _historyIndex = _history.size();
    
    // Limit history size
    if (_history.size() > 100) {
        _history.erase(_history.begin());
        _historyIndex--;
    }
}

void DesignSystem::clearFutureHistory() {
    if (_historyIndex < _history.size()) {
        _history.erase(_history.begin() + _historyIndex, _history.end());
    }
}

ShapeSystem::ShapeType DesignSystem::mapToolToShapeType(Tool::Type toolType) const {
    switch (toolType) {
        case Tool::Type::Rectangle: return ShapeSystem::ShapeType::Rectangle;
        case Tool::Type::Ellipse: return ShapeSystem::ShapeType::Ellipse;
        case Tool::Type::Polygon: return ShapeSystem::ShapeType::Polygon;
        case Tool::Type::Line: return ShapeSystem::ShapeType::Line;
        case Tool::Type::Arrow: return ShapeSystem::ShapeType::Arrow;
        case Tool::Type::Star: return ShapeSystem::ShapeType::Star;
        case Tool::Type::Heart: return ShapeSystem::ShapeType::Heart;
        default: return ShapeSystem::ShapeType::Rectangle;
    }
}

EffectsSystem::EffectType DesignSystem::mapToolToEffectType(Tool::Type toolType) const {
    switch (toolType) {
        case Tool::Type::Blur: return EffectsSystem::EffectType::Blur;
        case Tool::Type::Sharpen: return EffectsSystem::EffectType::Sharpen;
        case Tool::Type::Noise: return EffectsSystem::EffectType::Noise;
        case Tool::Type::Emboss: return EffectsSystem::EffectType::Emboss;
        case Tool::Type::Glow: return EffectsSystem::EffectType::Glow;
        case Tool::Type::Shadow: return EffectsSystem::EffectType::Shadow;
        case Tool::Type::Gradient: return EffectsSystem::EffectType::Gradient;
        case Tool::Type::Pattern: return EffectsSystem::EffectType::Pattern;
        default: return EffectsSystem::EffectType::Blur;
    }
} 
