#include "Zone.hpp"
#include "../World/World.hpp"
#include <iostream>
#include <algorithm>
#include "GLFW/glfw3.h"

using Scope = Zone::Scope;

static const char* scopeToString(Scope scope) {
    switch(scope) {
        case Scope::Global:   return "Global";
        case Scope::World:    return "World";
        case Scope::Regional: return "Regional";
        case Scope::Local:    return "Local";
        case Scope::UI:       return "UI";
        default:              return "Unknown";
    }
}

constexpr float kBrushRadiusToStrokeWidth = 1000.0f;

static float strokeWidthForBrushRadius(float radius) {
    return std::max(1.0f, radius * kBrushRadiusToStrokeWidth);
}

void Zone::load() {
    // Load the world and its objects
    _world->load();
    std::cout << "🌍 Zone '" << _name << "' loaded with " << _world->objects().size() << " objects." << std::endl;
}

void Zone::unload() {
    // Unload the world and its objects
    _world->unload();
    std::cout << "🌍 Zone '" << _name << "' unloaded." << std::endl;
}

void Zone::syncFormationMembers(const std::vector<Singular*>& extraMembers) {
    _formation.addMember(_world.get());
    for (const auto& up : _world->getOwnedObjects()) {
        if (up) _formation.addMember(up.get());
    }
    for (auto* member : extraMembers) {
        _formation.addMember(member);
    }
}

void Zone::applyFormationRelations() {
    _formation.applyAttachmentRelations();
}

Zone::Zone(const std::string& name, Scope scope)
    : _name(name), _scope(scope), _world(std::make_unique<World>()), _formation(Form::ShapeType::Cube, {1.0f, 1.0f, 1.0f})
{
    // Default background tint (deep space blue)
    r = 0.05f; g = 0.05f; b = 0.1f;
    _formation.addMember(_world.get());
}

Zone::Zone(const std::string& name, float rF, float gF, float bF, Scope scope)
    : _name(name), _scope(scope), _world(std::make_unique<World>()), _formation(Form::ShapeType::Cube, {1.0f, 1.0f, 1.0f})
{
    r = rF; g = gF; b = bF;
    _formation.addMember(_world.get());
}

Zone::Zone(const Zone& other)
    : _name(other._name), _scope(other._scope), _qualities(other._qualities), _deletable(other._deletable), _world(std::make_unique<World>()), _formation(Form::ShapeType::Cube, {1.0f, 1.0f, 1.0f})
{
    r = other.r; g = other.g; b = other.b;
    strokes = other.strokes;
    drawR = other.drawR; drawG = other.drawG; drawB = other.drawB;
    drawMode = other.drawMode; isDrawing = other.isDrawing;
    _formation.addMember(_world.get());
}

Zone& Zone::operator=(const Zone& other)
{
    if(this==&other) return *this;
    Zone tmp(other);
    std::swap(_name, tmp._name);
    std::swap(_scope, tmp._scope);
    std::swap(_qualities, tmp._qualities);
    std::swap(_deletable, tmp._deletable);
    std::swap(r, tmp.r);
    std::swap(g, tmp.g);
    std::swap(b, tmp.b);
    std::swap(strokes, tmp.strokes);
    std::swap(drawR, tmp.drawR);
    std::swap(drawG, tmp.drawG);
    std::swap(drawB, tmp.drawB);
    std::swap(drawMode, tmp.drawMode);
    std::swap(isDrawing, tmp.isDrawing);
    std::swap(_world, tmp._world);
    std::swap(_formation, tmp._formation);
    return *this;
}

void Zone::describe() const {
    std::cout << "🌀 Entering zone: " << _name << " (" << scopeToString(_scope) << ")" << std::endl;

    if(!_qualities.empty()) {
        std::cout << "   Qualities:" << std::endl;
        for (const auto &q : _qualities) {
            std::cout << "     - " << q.first << ": " << q.second << std::endl;
        }
    }

    if(!_deletable.empty()) {
        std::cout << "   Deletable by:" << std::endl;
        for (const auto &d : _deletable) {
            std::cout << "     - " << d.first << ": " << (d.second?"yes":"no") << std::endl;
        }
    }
}

void Zone::applyTheme() const { glClearColor(r, g, b, 1.f); }

void Zone::startStroke(float x, float y) { 
    isDrawing = true;
    
    // Initialize brush system if needed
    if (!brushSystem) {
        initializeBrushSystem();
    }
    
    // Advanced brush system (primary)
    currentStrokePoints.clear();
    currentStrokePoints.push_back(glm::vec2(x, y));
    if (brushSystem) {
        brushSystem->saveStrokeState();
        brushSystem->paintDab(glm::vec2(x, y), getCurrentColor());
    }
    
    // Also store in legacy system for rendering
    currentStroke.points.clear();
    currentStroke.points.push_back(x);
    currentStroke.points.push_back(y);
    currentStroke.r = drawR;
    currentStroke.g = drawG;
    currentStroke.b = drawB;
    currentStroke.lineWidth = 2.0f; // Will be updated in endStroke if brush system is active
    
    printf("Start stroke at (%.1f, %.1f)\n", x, y);
}

void Zone::continueStroke(float x, float y) {
    if (isDrawing) {
        glm::vec2 previous = currentStrokePoints.empty() ? glm::vec2(x, y) : currentStrokePoints.back();

        // Advanced brush system (primary)
        currentStrokePoints.push_back(glm::vec2(x, y));
        if (brushSystem) {
            brushSystem->paintStroke(previous, glm::vec2(x, y), getCurrentColor());
        }
        
        // Also store in legacy system for rendering
        currentStroke.points.push_back(x);
        currentStroke.points.push_back(y);
        
        printf("Continue stroke at (%.1f, %.1f)\n", x, y);
    }
}

void Zone::endStroke() {
    isDrawing = false;
    
    // Save stroke to legacy system for rendering
    if (!currentStroke.points.empty()) {
        // Apply brush system settings to the completed stroke
        if (brushSystem) {
            currentStroke.lineWidth = strokeWidthForBrushRadius(brushSystem->getRadius());
        } else {
            currentStroke.lineWidth = 2.0f; // Default line width
        }
        
        strokes.push_back(currentStroke);
        printf("End stroke, total points: %zu\n", currentStroke.points.size() / 2);
        currentStroke.points.clear();
    }
    
    // Clear advanced brush system points
    currentStrokePoints.clear();
}

void Zone::clearArt() {
    strokes.clear();
    currentStroke.points.clear();
}

void Zone::setDrawColor(float r, float g, float b) {
    drawR = r;
    drawG = g;
    drawB = b;
}

// Advanced brush system methods
void Zone::initializeBrushSystem() {
    if (!brushSystem) {
        brushSystem = std::make_unique<BrushSystem>(512);
        printf("Brush System initialized for zone: %s\n", _name.c_str());
    } else {
        printf("Brush System already exists for zone: %s\n", _name.c_str());
    }
}

void Zone::setBrushType(BrushSystem::BrushType type) {
    if (brushSystem) {
        brushSystem->setBrushType(type);
    }
}

void Zone::setBrushRadius(float radius) {
    if (brushSystem) {
        brushSystem->setRadius(radius);
    }
}

void Zone::setBrushOpacity(float opacity) {
    if (brushSystem) {
        brushSystem->setOpacity(opacity);
    }
}

void Zone::setBrushFlow(float flow) {
    if (brushSystem) {
        brushSystem->setFlow(flow);
    }
}

void Zone::setBrushSpacing(float spacing) {
    if (brushSystem) {
        brushSystem->setSpacing(spacing);
    }
}

void Zone::setBrushDensity(float density) {
    if (brushSystem) {
        brushSystem->setDensity(density);
    }
}

void Zone::setBrushStrength(float strength) {
    if (brushSystem) {
        brushSystem->setStrength(strength);
    }
}

void Zone::setPressureSimulation(bool enabled) {
    if (brushSystem) {
        brushSystem->setPressureSimulation(enabled);
    }
}

void Zone::setPressureSensitivity(float sensitivity) {
    if (brushSystem) {
        brushSystem->setPressureSensitivity(sensitivity);
    }
}

void Zone::setStrokeInterpolation(bool enabled) {
    if (brushSystem) {
        brushSystem->setStrokeInterpolation(enabled);
    }
}

void Zone::setUseLayers(bool enabled) {
    if (brushSystem) {
        brushSystem->setUseLayers(enabled);
    }
}

void Zone::setActiveLayer(int layer) {
    if (brushSystem) {
        brushSystem->setActiveLayer(layer);
    }
}

void Zone::setLayerOpacity(float opacity) {
    if (brushSystem) {
        brushSystem->setLayerOpacity(opacity);
    }
}

void Zone::setBlendMode(BrushSystem::BlendMode mode) {
    if (brushSystem) {
        brushSystem->setBlendMode(mode);
    }
}

int Zone::addLayer() {
    if (brushSystem) {
        return brushSystem->addLayer();
    }
    return 0;
}

void Zone::deleteLayer(int layerIndex) {
    if (brushSystem) {
        brushSystem->deleteLayer(layerIndex);
    }
}

void Zone::setCloneActive(bool active) {
    if (brushSystem) {
        brushSystem->setCloneActive(active);
    }
}

void Zone::setCloneOffset(const glm::vec2& offset) {
    if (brushSystem) {
        brushSystem->setCloneOffset(offset);
    }
}

void Zone::setCloneSource(const glm::vec2& source) {
    if (brushSystem) {
        brushSystem->setCloneSource(source);
    }
}

void Zone::setCurrentPreset(int index) {
    if (brushSystem) {
        brushSystem->setCurrentPreset(index);
    }
}

void Zone::saveStrokeState() {
    if (brushSystem) {
        brushSystem->saveStrokeState();
    }
}

void Zone::undo() {
    if (designSystem) {
        designSystem->undo();
    }
    if (brushSystem) {
        brushSystem->undo();
    }
}

void Zone::redo() {
    if (designSystem) {
        designSystem->redo();
    }
    if (brushSystem) {
        brushSystem->redo();
    }
}

void Zone::clearHistory() {
    if (designSystem) {
        designSystem->clearHistory();
    }
    if (brushSystem) {
        brushSystem->clearHistory();
    }
}

void Zone::renderArt() const {
        // Ensure proper OpenGL state for 2D rendering
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white
        
    // Professional Design System (primary)
    if (designSystem) {
        designSystem->render();
    }
    
    bool brushCanvasVisible = false;
    if (brushSystem) {
        const auto& pixels = brushSystem->getCompositedTexture();
        const int textureSize = brushSystem->getTextureSize();
        brushCanvasVisible = textureSize > 0 && pixels.size() == static_cast<size_t>(textureSize * textureSize * 4) &&
            std::any_of(pixels.begin() + 3, pixels.end(), [step = size_t{0}](uint8_t alpha) mutable {
                bool isAlpha = (step++ % 4) == 0;
                return isAlpha && alpha > 0;
            });

        if (brushCanvasVisible) {
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);

            static GLuint brushCanvasTexture = 0;
            if (brushCanvasTexture == 0) {
                glGenTextures(1, &brushCanvasTexture);
            }

            glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, brushCanvasTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(static_cast<float>(viewport[2]), 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));
            glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, static_cast<float>(viewport[3]));
            glEnd();

            glBindTexture(GL_TEXTURE_2D, 0);
            glPopAttrib();
        }
    }

    // Stroke path overlay: retained vector paths for hit-testing/history and fallback rendering.
    if (brushSystem && !brushCanvasVisible) {
        // Draw completed stroke paths only when there is no pixel canvas yet.
        for (const auto& stroke : strokes) {
            if (stroke.points.size() < 4) continue; // Need at least 2 points
            glLineWidth(stroke.lineWidth); // Use stored line width
            glColor3f(stroke.r, stroke.g, stroke.b); // Use original stroke colors
            glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < stroke.points.size(); i += 2) {
                glVertex2f(stroke.points[i], stroke.points[i + 1]);
            }
            glEnd();
        }
        
        // Draw current stroke in progress with brush system settings
        if (isDrawing && !currentStrokePoints.empty() && currentStrokePoints.size() >= 2) {
            // Apply brush system settings only to the current stroke
            glLineWidth(strokeWidthForBrushRadius(brushSystem->getRadius()));
            
            // Apply opacity if it's less than 1.0 (enable blending for transparency)
            if (brushSystem->getOpacity() < 1.0f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(drawR, drawG, drawB, brushSystem->getOpacity());
            } else {
                glColor3f(drawR, drawG, drawB);
            }
            
            glBegin(GL_LINE_STRIP);
            for (const auto& point : currentStrokePoints) {
                glVertex2f(point.x, point.y);
            }
            glEnd();
            
            // Reset blending state
            if (brushSystem->getOpacity() < 1.0f) {
                glDisable(GL_BLEND);
            }
        }
    }
    
    // Stroke path fallback only if no brush system or design system exists.
    if (!brushSystem && !designSystem) {
        glLineWidth(2.0f);
        
        // Draw legacy strokes
        for (const auto& stroke : strokes) {
            if (stroke.points.size() < 4) continue; // Need at least 2 points
            glColor3f(stroke.r, stroke.g, stroke.b);
            glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < stroke.points.size(); i += 2) {
                glVertex2f(stroke.points[i], stroke.points[i + 1]);
            }
            glEnd();
        }
        
        // Draw the current stroke in progress (legacy)
        if (!currentStroke.points.empty()) {
            glColor3f(currentStroke.r, currentStroke.g, currentStroke.b);
            glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < currentStroke.points.size(); i += 2) {
                glVertex2f(currentStroke.points[i], currentStroke.points[i + 1]);
            }
            glEnd();
        }
    }
}



// Explicit destructor to anchor vtable
Zone::~Zone() {}

// ============================================================================
// Professional Design System Methods
// ============================================================================

void Zone::initializeDesignSystem() {
    if (!designSystem) {
        designSystem = std::make_unique<DesignSystem>();
        designSystem->initialize(this);
        printf("Design System initialized for zone: %s\n", _name.c_str());
    } else {
        printf("Design System already exists for zone: %s\n", _name.c_str());
    }
}

void Zone::setDesignTool(Tool::Type toolType) {
    if (designSystem) {
        designSystem->setCurrentTool(toolType);
    }
}

void Zone::startDesignDrawing(float x, float y) {
    if (designSystem) {
        designSystem->startDrawing(glm::vec2(x, y));
    }
}

void Zone::continueDesignDrawing(float x, float y) {
    if (designSystem) {
        designSystem->continueDrawing(glm::vec2(x, y));
    }
}

void Zone::endDesignDrawing() {
    if (designSystem) {
        designSystem->endDrawing();
    }
}

void Zone::addDesignText(const std::string& text, float x, float y) {
    if (designSystem) {
        designSystem->addText(text, glm::vec2(x, y));
    }
}

void Zone::addDesignShape(Tool::Type shapeType, float x, float y, float width, float height) {
    if (designSystem) {
        designSystem->addShape(shapeType, glm::vec2(x, y), glm::vec2(width, height));
    }
}

void Zone::startDesignSelection(float x, float y) {
    if (designSystem) {
        designSystem->startSelection(glm::vec2(x, y));
    }
}

void Zone::updateDesignSelection(float x, float y) {
    if (designSystem) {
        designSystem->updateSelection(glm::vec2(x, y));
    }
}

void Zone::endDesignSelection() {
    if (designSystem) {
        designSystem->endSelection();
    }
}

void Zone::clearDesignSelection() {
    if (designSystem) {
        designSystem->clearSelection();
    }
}

void Zone::addDesignEffect(Tool::Type effectType, float intensity) {
    if (designSystem) {
        designSystem->addEffect(effectType, intensity);
    }
}

void Zone::addDesignLayer() {
    if (designSystem) {
        designSystem->addLayer();
    }
}

void Zone::removeDesignLayer(int layerIndex) {
    if (designSystem) {
        designSystem->removeLayer(layerIndex);
    }
}

void Zone::setActiveDesignLayer(int layerIndex) {
    if (designSystem) {
        designSystem->setActiveLayer(layerIndex);
    }
}

void Zone::setDesignLayerOpacity(int layerIndex, float opacity) {
    if (designSystem) {
        designSystem->setLayerOpacity(layerIndex, opacity);
    }
}

int Zone::getActiveDesignLayer() const {
    return designSystem ? designSystem->getActiveLayer() : 0;
}

int Zone::getDesignLayerCount() const {
    return designSystem ? designSystem->getLayerCount() : 0;
}

float Zone::getDesignLayerOpacity(int layerIndex) const {
    return designSystem ? designSystem->getLayerOpacity(layerIndex) : 1.0f;
}
