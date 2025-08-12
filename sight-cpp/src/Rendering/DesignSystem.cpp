#include "DesignSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Util/SaveSystem.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

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
    // TODO: Implement proper text rendering using OpenGL
    for (const auto& element : _textElements) {
        if (element.visible) {
            // Simple text rendering using OpenGL
            glPushMatrix();
            glLoadIdentity();
            glTranslatef(element.position.x, element.position.y, 0.0f);
            
            // Apply text style
            glColor3f(element.style.color.x, element.style.color.y, element.style.color.z);
            glLineWidth(1.0f);
            
            // Simple text rendering (placeholder - would use proper font rendering)
            glBegin(GL_LINES);
            // Draw a simple text indicator
            float textWidth = element.text.length() * 8.0f; // Approximate width
            glVertex2f(0, 0);
            glVertex2f(textWidth, 0);
            glVertex2f(0, 0);
            glVertex2f(0, 16);
            glVertex2f(textWidth, 0);
            glVertex2f(textWidth, 16);
            glVertex2f(0, 16);
            glVertex2f(textWidth, 16);
            glEnd();
            
            glPopMatrix();
        }
    }
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
    // TODO: Implement shape rendering using OpenGL
    for (const auto& element : _shapeElements) {
        if (element.visible) {
            glPushMatrix();
            glLoadIdentity();
            glTranslatef(element.position.x, element.position.y, 0.0f);
            glRotatef(element.rotation, 0.0f, 0.0f, 1.0f);
            
            // Apply shape style
            if (element.style.fillEnabled) {
                glColor3f(element.style.fillColor.x, element.style.fillColor.y, element.style.fillColor.z);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(element.style.fillColor.x, element.style.fillColor.y, element.style.fillColor.z, element.style.fillOpacity);
            }
            
            // Render shape based on type
            switch (element.type) {
                case ShapeType::Rectangle:
                    renderRectangle(element.size.x, element.size.y, element.style.cornerRadius);
                    break;
                case ShapeType::Ellipse:
                    renderEllipse(element.size.x, element.size.y);
                    break;
                case ShapeType::Line:
                    renderLine(element.size.x, element.size.y);
                    break;
                case ShapeType::Polygon:
                    renderPolygon(element.size.x, element.size.y, element.style.sides);
                    break;
                case ShapeType::Star:
                    renderStar(element.size.x, element.size.y, element.style.starPoints);
                    break;
                case ShapeType::Heart:
                    renderHeart(element.size.x, element.size.y);
                    break;
                case ShapeType::Arrow:
                    renderArrow(element.size.x, element.size.y);
                    break;
                case ShapeType::Custom:
                    renderCustomShape(element.customPoints);
                    break;
            }
            
            // Render stroke if enabled
            if (element.style.strokeEnabled) {
                glColor3f(element.style.strokeColor.x, element.style.strokeColor.y, element.style.strokeColor.z);
                glLineWidth(element.style.strokeWidth);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                
                // Re-render shape as wireframe for stroke
                switch (element.type) {
                    case ShapeType::Rectangle:
                        renderRectangle(element.size.x, element.size.y, element.style.cornerRadius);
                        break;
                    case ShapeType::Ellipse:
                        renderEllipse(element.size.x, element.size.y);
                        break;
                    case ShapeType::Line:
                        renderLine(element.size.x, element.size.y);
                        break;
                    case ShapeType::Polygon:
                        renderPolygon(element.size.x, element.size.y, element.style.sides);
                        break;
                    case ShapeType::Star:
                        renderStar(element.size.x, element.size.y, element.style.starPoints);
                        break;
                    case ShapeType::Heart:
                        renderHeart(element.size.x, element.size.y);
                        break;
                    case ShapeType::Arrow:
                        renderArrow(element.size.x, element.size.y);
                        break;
                    case ShapeType::Custom:
                        renderCustomShape(element.customPoints);
                        break;
                }
                
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            
            glDisable(GL_BLEND);
            glPopMatrix();
        }
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

// Shape rendering helper methods
void ShapeSystem::renderRectangle(float width, float height, float cornerRadius) const {
    if (cornerRadius <= 0.0f) {
        // Simple rectangle
        glBegin(GL_QUADS);
        glVertex2f(-width/2, -height/2);
        glVertex2f(width/2, -height/2);
        glVertex2f(width/2, height/2);
        glVertex2f(-width/2, height/2);
        glEnd();
    } else {
        // Rounded rectangle (simplified)
        glBegin(GL_QUADS);
        glVertex2f(-width/2 + cornerRadius, -height/2);
        glVertex2f(width/2 - cornerRadius, -height/2);
        glVertex2f(width/2 - cornerRadius, height/2);
        glVertex2f(-width/2 + cornerRadius, height/2);
        glEnd();
        
        glBegin(GL_QUADS);
        glVertex2f(-width/2, -height/2 + cornerRadius);
        glVertex2f(-width/2 + cornerRadius, -height/2 + cornerRadius);
        glVertex2f(-width/2 + cornerRadius, height/2 - cornerRadius);
        glVertex2f(-width/2, height/2 - cornerRadius);
        glEnd();
        
        glBegin(GL_QUADS);
        glVertex2f(width/2 - cornerRadius, -height/2 + cornerRadius);
        glVertex2f(width/2, -height/2 + cornerRadius);
        glVertex2f(width/2, height/2 - cornerRadius);
        glVertex2f(width/2 - cornerRadius, height/2 - cornerRadius);
        glEnd();
    }
}

void ShapeSystem::renderEllipse(float width, float height) const {
    const int segments = 32;
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = (width/2) * cos(angle);
        float y = (height/2) * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void ShapeSystem::renderLine(float width, float height) const {
    glBegin(GL_LINES);
    glVertex2f(-width/2, -height/2);
    glVertex2f(width/2, height/2);
    glEnd();
}

void ShapeSystem::renderPolygon(float width, float height, int sides) const {
    if (sides < 3) sides = 3;
    if (sides > 20) sides = 20;
    
    glBegin(GL_POLYGON);
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = (width/2) * cos(angle);
        float y = (height/2) * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void ShapeSystem::renderStar(float width, float height, float points) const {
    if (points < 3) points = 3;
    if (points > 20) points = 20;
    
    glBegin(GL_POLYGON);
    for (int i = 0; i < points * 2; ++i) {
        float angle = 2.0f * M_PI * i / (points * 2);
        float radius = (i % 2 == 0) ? width/2 : width/4;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void ShapeSystem::renderHeart(float width, float height) const {
    const int segments = 32;
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; ++i) {
        float t = 2.0f * M_PI * i / segments;
        float x = (width/2) * 16 * pow(sin(t), 3);
        float y = (height/2) * -(13 * cos(t) - 5 * cos(2*t) - 2 * cos(3*t) - cos(4*t));
        glVertex2f(x, y);
    }
    glEnd();
}

void ShapeSystem::renderArrow(float width, float height) const {
    glBegin(GL_POLYGON);
    // Arrow shaft
    glVertex2f(-width/2, -height/6);
    glVertex2f(width/3, -height/6);
    glVertex2f(width/3, height/6);
    glVertex2f(-width/2, height/6);
    // Arrow head
    glVertex2f(width/3, -height/2);
    glVertex2f(width/2, 0);
    glVertex2f(width/3, height/2);
    glEnd();
}

void ShapeSystem::renderCustomShape(const std::vector<glm::vec2>& points) const {
    if (points.size() < 3) return;
    
    glBegin(GL_POLYGON);
    for (const auto& point : points) {
        glVertex2f(point.x, point.y);
    }
    glEnd();
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
    if (pixels.size() < width * height * 4) return;
    
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
    if (pixels.size() < width * height * 4) return;
    
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
    // TODO: Implement glow effect
    printf("Applied glow effect with radius %.1f\n", effect.radius);
}

void EffectsSystem::applyShadow(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const {
    // TODO: Implement shadow effect
    printf("Applied shadow effect with offset (%.1f, %.1f)\n", effect.offset.x, effect.offset.y);
}

void EffectsSystem::applyGradient(std::vector<uint8_t>& pixels, int width, int height, const Effect& effect) const {
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
    std::string id = "selection_" + std::to_string(_nextSelectionId++);
    
    Selection selection;
    selection.type = type;
    selection.points = points;
    selection.active = true;
    selection.id = id;
    
    // Calculate bounds
    if (!points.empty()) {
        selection.bounds[0] = points[0];
        selection.bounds[1] = points[0];
        
        for (const auto& point : points) {
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
    // TODO: Implement select all functionality
    printf("Select all\n");
}

void SelectionSystem::invertSelection() {
    // TODO: Implement invert selection functionality
    printf("Invert selection\n");
}

void SelectionSystem::expandSelection(float amount) {
    // TODO: Implement expand selection functionality
    printf("Expand selection by %.1f\n", amount);
}

void SelectionSystem::contractSelection(float amount) {
    // TODO: Implement contract selection functionality
    printf("Contract selection by %.1f\n", amount);
}

void SelectionSystem::featherSelection(float amount) {
    // TODO: Implement feather selection functionality
    printf("Feather selection by %.1f\n", amount);
}

void SelectionSystem::renderSelections() const {
    for (const auto& selection : _selections) {
        if (selection.active) {
            glColor3f(0.0f, 0.5f, 1.0f); // Blue selection color
            glLineWidth(2.0f);
            glLineStipple(1, 0x00FF); // Dashed line
            glEnable(GL_LINE_STIPPLE);
            
            glBegin(GL_LINE_LOOP);
            for (const auto& point : selection.points) {
                glVertex2f(point.x, point.y);
            }
            glEnd();
            
            glDisable(GL_LINE_STIPPLE);
        }
    }
}

bool SelectionSystem::isPointSelected(const glm::vec2& point) const {
    // TODO: Implement point-in-selection test
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
            // Render transform handles
            glColor3f(1.0f, 1.0f, 0.0f); // Yellow handles
            glPointSize(8.0f);
            
            glBegin(GL_POINTS);
            glVertex2f(transform.position.x, transform.position.y);
            glEnd();
            
            // Render bounding box
            glColor3f(1.0f, 0.5f, 0.0f); // Orange bounding box
            glLineWidth(1.0f);
            
            glBegin(GL_LINE_LOOP);
            glVertex2f(transform.position.x - 20, transform.position.y - 20);
            glVertex2f(transform.position.x + 20, transform.position.y - 20);
            glVertex2f(transform.position.x + 20, transform.position.y + 20);
            glVertex2f(transform.position.x - 20, transform.position.y + 20);
            glEnd();
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
    _brushSystem = std::make_unique<BrushSystem>(64);
    
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
    
    if (_brushSystem) {
        // Use the current color from the zone instead of hardcoded values
        glm::vec3 currentColor = _zone->getCurrentColor();
        _brushSystem->paintDab(position, currentColor);
    }
    
    printf("Started drawing at (%.1f, %.1f)\n", position.x, position.y);
}

void DesignSystem::continueDrawing(const glm::vec2& position) {
    if (_isDrawing) {
        _currentPosition = position;
        
        if (_brushSystem) {
            // Use the current color from the zone instead of hardcoded values
            glm::vec3 currentColor = _zone->getCurrentColor();
            _brushSystem->paintStroke(_startPosition, position, currentColor);
        }
        
        printf("Continued drawing at (%.1f, %.1f)\n", position.x, position.y);
    }
}

void DesignSystem::endDrawing() {
    if (_isDrawing) {
        _isDrawing = false;
        
        if (_brushSystem) {
            _brushSystem->saveStrokeState();
        }
        
        printf("Ended drawing\n");
    }
}

void DesignSystem::addText(const std::string& text, const glm::vec2& position) {
    if (_textSystem) {
        std::string id = _textSystem->addText(text, position);
        _layers[_activeLayer].elements.push_back(id);
        saveHistoryEntry("add_text", "{\"id\":\"" + id + "\",\"text\":\"" + text + "\"}");
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
        saveHistoryEntry("remove_text", "{\"id\":\"" + id + "\"}");
    }
}

void DesignSystem::addShape(Tool::Type shapeType, const glm::vec2& position, const glm::vec2& size) {
    if (_shapeSystem) {
        ShapeSystem::ShapeType type = mapToolToShapeType(shapeType);
        std::string id = _shapeSystem->addShape(type, position, size);
        _layers[_activeLayer].elements.push_back(id);
        saveHistoryEntry("add_shape", "{\"id\":\"" + id + "\",\"type\":" + std::to_string(static_cast<int>(type)) + "}");
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
    newLayer.name = "Layer " + std::to_string(_layers.size());
    _layers.push_back(newLayer);
    _activeLayer = static_cast<int>(_layers.size()) - 1;
    printf("Added layer: %s\n", newLayer.name.c_str());
}

void DesignSystem::removeLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < static_cast<int>(_layers.size()) && _layers.size() > 1) {
        _layers.erase(_layers.begin() + layerIndex);
        if (_activeLayer >= static_cast<int>(_layers.size())) {
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

void DesignSystem::render() const {
    // Render all subsystems
    if (_textSystem) _textSystem->renderTexts();
    if (_shapeSystem) _shapeSystem->renderShapes();
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
        // TODO: Implement undo functionality
        printf("Undo: %s\n", _history[_historyIndex].action.c_str());
    }
}

void DesignSystem::redo() {
    if (_historyIndex < _history.size()) {
        // TODO: Implement redo functionality
        printf("Redo: %s\n", _history[_historyIndex].action.c_str());
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
        layersArray.push_back(layerJson);
    }
    j["layers"] = layersArray;
    j["activeLayer"] = _activeLayer;
    
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
    SaveSystem::writeJson(j, filename, SaveSystem::SaveType::DESIGN);
    printf("Saved design to: %s\n", filename.c_str());
}

void DesignSystem::loadDesign(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            
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
                    _layers.push_back(layer);
                }
            }
            
            _activeLayer = j.value("activeLayer", 0);
            
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