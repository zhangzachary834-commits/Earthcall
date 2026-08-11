#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../World/World.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include <memory>
#include "ConstructedBeing/Object/Object.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Legacy/DesignSystem.hpp"

class World; // forward decl
namespace OntoMath {
    class ScalarField;
    class VectorField;
}
namespace geom {
    class FieldNode;
}

class Zone : public Object
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

    Zone(const std::string &name, const std::string& joyOrdering, Scope scope = Scope::Local);
    Zone(const std::string &name, const std::string& joyOrdering, float r, float g, float b, Scope scope = Scope::Local);

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
    // Formation
    Formation& getFormation() { return _formation; }
    const Formation& getFormation() const { return _formation; }
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
    virtual void renderArt(bool useLegacy2DTools = false) const;
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
    int getActiveDesignLayer() const;
    int getDesignLayerCount() const;
    float getDesignLayerOpacity(int layerIndex) const;

    const std::string& name() const { return _name; }
    // Safe accessors for diagnostics
    const Qualities& getQualities() const { return _qualities; }
    const Deletability& getDeletability() const { return _deletable; }

    // ------------------------------------------------------------------
    // Legibility (manifesto: a Zone is an extra-spatial Object — a being).
    // ComputedProperty getters/setters for buildProperties; by-value
    // returns because the underlying storage is loose floats.
    std::string propName() const { return _name; }
    std::string scopeName() const;
    glm::vec3 tint() const { return glm::vec3(r, g, b); }
    void setTint(const glm::vec3& c) { r = c.x; g = c.y; b = c.z; }
    void setDrawColorV(const glm::vec3& c) { setDrawColor(c.x, c.y, c.z); }

    // Ownership (manifesto: "Every Person has a Home they fully own").
    // Recorded as the owner's identifier; empty = unowned commons. Exposed
    // read-only — transferring a zone is a governance act, not a Set.
    const std::string& owner() const { return _ownerId; }
    std::string propOwner() const { return _ownerId; }   // by-value for ComputedProperty
    void setOwner(const std::string& personId) {
        _ownerId = personId;
        if (!personId.empty()) _deletable[personId] = true;
    }

    // Access the 3-D world belonging to this zone
    World& world() { return *_world; }
    const World& world() const { return *_world; }

    // Hierarchy (Embedding) ----------------------------------------------
    const std::string& getParentZone() const { return _parentZoneName; }
    void setParentZone(const std::string& pZone) { _parentZoneName = pZone; }

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
    // Zone-specific surface, NOT Object's: a zone is extra-spatial, so
    // registering position/shape/mass here would be a lie. What a zone
    // truthfully has: identity, colors, scope, owner.
    void buildProperties() override;

    std::string _name;
    std::string _parentZoneName; // Name of the zone this is embedded in, if any
    Scope _scope;
    Qualities _qualities;
    Deletability _deletable;
    std::string _joyOrdering;
    std::string _ownerId;   // owning Person's identifier ("" = commons)
    std::unique_ptr<World> _world; // per-zone world instance
    Formation _formation;
    
    // The raw Singularity substrates for spatial math
    std::shared_ptr<OntoMath::ScalarField> _spatialField;
    std::shared_ptr<OntoMath::VectorField> _spatialVectorField;
    
    // The spatial root object (a FieldNode) that shares the pointers 
    // and is a member of the formation
    std::shared_ptr<geom::FieldNode> _spatialRootObject;
    // Removed cache; formation members are rebuilt on copy

public:
// Bruh
// USE POINTERS!!!!!
    Formation& formation() { return _formation; }
    const Formation& formation() const { return _formation; }
    void load();
    void unload();
    void syncFormationMembers(const std::vector<Singular*>& extraMembers = {});
    void applyFormationRelations();

    // Singular interface
    std::string getIdentifier() const override { return _name; }
};
