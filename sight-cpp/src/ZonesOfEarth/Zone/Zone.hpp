#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../World/World.hpp"
#include "Form/Object/Formation/Formations.hpp"
#include <memory>
#include "Form/Object/Object.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/DesignSystem.hpp"

class World; // forward decl

class Zone : public Singular
{
public:
    struct Stroke {
        std::vector<float> points;
        float r, g, b;
        float lineWidth; // Store the line width used for this stroke
    };

    enum class Scope {
        Global,
        World,
        Regional,
        Local,
        UI
    };

    using Qualities = std::unordered_map<std::string, std::string>;
    using Deletability = std::unordered_map<std::string, bool>;

    Zone(const std::string &name, Scope scope = Scope::Local);
    Zone(const std::string &name, float r, float g, float b, Scope scope = Scope::Local);

    Zone(const Zone&);
    Zone& operator=(const Zone&);
    Zone(Zone&&) noexcept = default;
    Zone& operator=(Zone&&) noexcept = default;

    size_t current = 0;

    float r, g, b;            // background tint
    std::vector<Stroke> strokes; // Each stroke has its own color (legacy)
    Stroke currentStroke; // The stroke currently being drawn (legacy)
    
    // Drawing color (default to golden)
    float drawR = 1.0f, drawG = 0.9f, drawB = 0.2f;

    bool drawMode = false;
    bool isDrawing = false;   // Track if currently drawing a stroke
    
    // ------------------------------------------------------------
    // Formations
    Formations& getFormation() { return _formation; }
    const Formations& getFormation() const { return _formation; }
    void addToFormation(Singular* s) { _formation.addMember(s); }
    void removeFromFormation(Singular* s) { _formation.removeMember(s); }
    void addToFormation(const std::vector<Singular*>& members) { 
        for(auto* member : members) {
            _formation.addMember(member);
        }
    }
    void removeFromFormation(const std::vector<Singular*>& members) { 
        for(auto* member : members) {
            _formation.removeMember(member);
        }
    }

    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // 2D Creation System
    // Advanced brush system
    std::unique_ptr<BrushSystem> brushSystem;
    std::vector<glm::vec2> currentStrokePoints; // Current stroke points for advanced brush
    
    // Elements System
    // Currently inspired by Wix's elements designer
    std::unique_ptr<DesignSystem> designSystem;
    // ------------------------------------------------------------

    virtual ~Zone();

    void describe() const;
    void applyTheme() const;

    void startStroke(float x, float y);
    void continueStroke(float x, float y);
    void endStroke();
    void clearArt();
    virtual void renderArt() const;
    void setDrawColor(float r, float g, float b);
    glm::vec3 getCurrentColor() const { return glm::vec3(drawR, drawG, drawB); }
    
    // Advanced brush system methods
    void initializeBrushSystem();
    BrushSystem* getBrushSystem() { return brushSystem.get(); }
    void setBrushType(BrushSystem::BrushType type);
    void setBrushRadius(float radius);
    void setBrushOpacity(float opacity);
    void setBrushFlow(float flow);
    void setBrushSpacing(float spacing);
    void setBrushDensity(float density);
    void setBrushStrength(float strength);
    void setPressureSimulation(bool enabled);
    void setPressureSensitivity(float sensitivity);
    void setStrokeInterpolation(bool enabled);
    void setUseLayers(bool enabled);
    void setActiveLayer(int layer);
    void setLayerOpacity(float opacity);
    void setBlendMode(BrushSystem::BlendMode mode);
    int addLayer();
    void deleteLayer(int layerIndex);
    void setCloneActive(bool active);
    void setCloneOffset(const glm::vec2& offset);
    void setCloneSource(const glm::vec2& source);
    void setCurrentPreset(int index);
    void saveStrokeState();
    void undo();
    void redo();
    void clearHistory();
    
    // Holistic 2D design system methods
    void initializeDesignSystem();
    DesignSystem* getDesignSystem() { return designSystem.get(); }
    const DesignSystem* getDesignSystem() const { return designSystem.get(); }
    void setDesignTool(Tool::Type toolType);
    void startDesignDrawing(float x, float y);
    void continueDesignDrawing(float x, float y);
    void endDesignDrawing();
    void addDesignText(const std::string& text, float x, float y);
    void addDesignShape(Tool::Type shapeType, float x, float y, float width, float height);
    void startDesignSelection(float x, float y);
    void updateDesignSelection(float x, float y);
    void endDesignSelection();
    void clearDesignSelection();
    void addDesignEffect(Tool::Type effectType, float intensity = 1.0f);
    void addDesignLayer();
    void removeDesignLayer(int layerIndex);
    void setActiveDesignLayer(int layerIndex);
    void setDesignLayerOpacity(int layerIndex, float opacity);

    const std::string& name() const { return _name; }
    // Safe accessors for diagnostics
    const Qualities& getQualities() const { return _qualities; }
    const Deletability& getDeletability() const { return _deletable; }

    // Access the 3-D world belonging to this zone
    World& world() { return *_world; }
    const World& world() const { return *_world; }

    // Conceptual metadata ------------------------------------------------
    void setScope(Scope scope) { _scope = scope; }
    Scope scope() const { return _scope; }

    void setQuality(const std::string &key, const std::string &value) { _qualities[key] = value; }
    const std::string &quality(const std::string &key) const { return _qualities.at(key); }
    const Qualities &qualities() const { return _qualities; }

    // Per-person deletability -------------------------------------------
    void setDeletable(const std::string &person, bool flag) { _deletable[person] = flag; }
    bool isDeletable(const std::string &person) const {
        auto it = _deletable.find(person);
        return it != _deletable.end() ? it->second : false;
    }
    const Deletability &deletability() const { return _deletable; }

private:
    std::string _name;
    Scope _scope;
    Qualities _qualities;
    Deletability _deletable;
    std::unique_ptr<World> _world; // per-zone world instance
    Formations _formation;
    // Removed cache; formation members are rebuilt on copy

public:
// Bruh
// USE POINTERS!!!!!
    Formations& formation() { return _formation; }
    const Formations& formation() const { return _formation; }
    void load();
    void unload();

    // Singular interface
    std::string getIdentifier() const override { return _name; }
};
