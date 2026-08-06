#include "Zone.hpp"
#include "../World/World.hpp"
#include "Form/Singular/Property/ComputedProperty.hpp"
#include <iostream>
#include <algorithm>
#include "GLFW/glfw3.h"
#include "Rendering/Renderer.hpp"
#include "Form/Object/Formation/Menu/stb_easy_font.h"

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

std::string Zone::scopeName() const { return scopeToString(_scope); }

// A Zone's truthful surface — what laws may read and (where honest) write.
// Deliberately NOT Object::buildProperties(): a zone is extra-spatial, so
// position/shape/mass would be fictions here.
void Zone::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "name", this, &Zone::propName));            // read-only: identity is not a slot
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, glm::vec3>>(
        "color", this, &Zone::tint, &Zone::setTint));       // background tint (r/g/b legible)
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, glm::vec3>>(
        "drawColor", this, &Zone::getCurrentColor, &Zone::setDrawColorV));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "scope", this, &Zone::scopeName));          // read-only for now
    // Read-only: ownership transfer is a covenant between Persons, not a
    // property write. The GOVERNANCE meaning of ownership (jurisdiction,
    // priority ceilings) is the next stage; the record comes first.
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "owner", this, &Zone::propOwner));
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

Zone::Zone(const std::string& name, const std::string& joyOrdering, Scope scope)
    : _name(name), _scope(scope), _joyOrdering(joyOrdering), _world(std::make_unique<World>()), _formation()
{
    // Default background tint (deep space blue)
    r = 0.05f; g = 0.05f; b = 0.1f;
    _formation.addMember(_world.get());
}

Zone::Zone(const std::string& name, const std::string& joyOrdering, float rF, float gF, float bF, Scope scope)
    : _name(name), _scope(scope), _joyOrdering(joyOrdering), _world(std::make_unique<World>()), _formation()
{
    r = rF; g = gF; b = bF;
    _formation.addMember(_world.get());
}

Zone::Zone(const Zone& other)
    : _name(other._name), _scope(other._scope), _qualities(other._qualities), _deletable(other._deletable), _joyOrdering(other._joyOrdering), _ownerId(other._ownerId), _world(std::make_unique<World>()), _formation()
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
    std::swap(_joyOrdering, tmp._joyOrdering);
    std::swap(_ownerId, tmp._ownerId);
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

// The zone's tint reaches the screen as the clear colour passed to
// Renderer::beginFrame (see Game::render), so there is no separate GL state to
// set here. Kept as a hook because callers still express "apply this zone".
void Zone::applyTheme() const {}

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

void Zone::renderArt(bool useLegacy2DTools) const {
    // Called from inside GameRender's begin2D scope, so screen-space projection,
    // depth-test-off and alpha blending are already established. Each draw below
    // carries its own colour, so there is nothing left to reset by hand.
    //
    // Blending is now always on where it used to be toggled per stroke; with an
    // alpha of 1.0 the blend equation reduces to a plain overwrite, so opaque
    // strokes land on exactly the same pixels as before.
    Renderer& r = currentRenderer();

    // Flat [x0,y0,x1,y1,...] point arrays -> the boundary's segment list.
    auto polyline = [](const std::vector<float>& pts) {
        std::vector<glm::vec2> v;
        v.reserve(pts.size() / 2);
        for (size_t i = 0; i + 1 < pts.size(); i += 2) v.push_back({pts[i], pts[i + 1]});
        return draw::stripToSegments(v, /*closed=*/false);
    };

    // Professional Design System (legacy)
    if (designSystem && useLegacy2DTools) {
        designSystem->render();
    }
    
    // New 2D Object Rendering
    if (!useLegacy2DTools) {
        for (const auto* singular : _formation.getMembers()) {
            const auto* obj = dynamic_cast<const Object*>(singular);
            if (!obj) continue;
            
            if (obj->getShapeKind() == Object::ShapeKind::Shape2D) {
                float w = obj->getShapeParams().width2D;
                float h = obj->getShapeParams().height2D;
                glm::vec3 pos = obj->getTransform()[3];
                glm::vec4 color(obj->faceColors[0][0], obj->faceColors[0][1], obj->faceColors[0][2], 1.0f);
                
                std::vector<glm::vec2> pts = {
                    {pos.x - w/2, pos.y - h/2},
                    {pos.x + w/2, pos.y - h/2},
                    {pos.x + w/2, pos.y + h/2},
                    {pos.x - w/2, pos.y + h/2}
                };
                r.drawLines2D(draw::stripToSegments(pts, /*closed=*/true), color, 2.0f);
            } else if (obj->getShapeKind() == Object::ShapeKind::Text2D) {
                std::string text = obj->getTextString();
                if (text.empty()) text = "Text";
                char vertexBuffer[24000];
                int quads = stb_easy_font_print(0.0f, 0.0f, const_cast<char*>(text.c_str()), nullptr, vertexBuffer, sizeof(vertexBuffer));
                if (quads > 0) {
                    float fontSize = obj->getShapeParams().width2D; // Just use width2D as font size for now
                    if (fontSize <= 0.0f) fontSize = 16.0f;
                    const float scale = std::max(0.25f, fontSize / 16.0f);
                    std::vector<glm::vec2> tris = draw::easyFontToTris(vertexBuffer, quads);
                    glm::vec3 pos = obj->getTransform()[3];
                    glm::vec2 pos2d(pos.x, pos.y);
                    for (glm::vec2& p : tris) p = pos2d + p * scale;
                    r.drawTris2D(tris, glm::vec4(obj->faceColors[0][0], obj->faceColors[0][1], obj->faceColors[0][2], 1.0f));
                }
            }
        }
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
            // The painted canvas, stretched over the whole framebuffer. The texture
            // upload and the quad both live behind drawImage2D now — the pixels are
            // regenerated every frame, so there is nothing worth caching here.
            const glm::ivec4& vp = r.viewport();
            r.drawImage2D(pixels.data(),
                          static_cast<uint32_t>(textureSize), static_cast<uint32_t>(textureSize),
                          glm::vec4(0.0f, 0.0f, static_cast<float>(vp.z), static_cast<float>(vp.w)),
                          glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    // Stroke path overlay: retained vector paths for hit-testing/history and fallback rendering.
    if (brushSystem && !brushCanvasVisible) {
        // Draw completed stroke paths only when there is no pixel canvas yet.
        for (const auto& stroke : strokes) {
            if (stroke.points.size() < 4) continue; // Need at least 2 points
            r.drawLines2D(polyline(stroke.points),
                          glm::vec4(stroke.r, stroke.g, stroke.b, 1.0f),
                          stroke.lineWidth); // Use stored line width
        }

        // Draw current stroke in progress with brush system settings
        if (isDrawing && !currentStrokePoints.empty() && currentStrokePoints.size() >= 2) {
            std::vector<glm::vec2> pts(currentStrokePoints.begin(), currentStrokePoints.end());
            r.drawLines2D(draw::stripToSegments(pts, /*closed=*/false),
                          glm::vec4(drawR, drawG, drawB, brushSystem->getOpacity()),
                          strokeWidthForBrushRadius(brushSystem->getRadius()));
        }
    }
    
    // Stroke path fallback only if no brush system or design system exists.
    if (!brushSystem && !designSystem) {
        // Draw legacy strokes
        for (const auto& stroke : strokes) {
            if (stroke.points.size() < 4) continue; // Need at least 2 points
            r.drawLines2D(polyline(stroke.points),
                          glm::vec4(stroke.r, stroke.g, stroke.b, 1.0f), 2.0f);
        }

        // Draw the current stroke in progress (legacy)
        if (!currentStroke.points.empty()) {
            r.drawLines2D(polyline(currentStroke.points),
                          glm::vec4(currentStroke.r, currentStroke.g, currentStroke.b, 1.0f), 2.0f);
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
