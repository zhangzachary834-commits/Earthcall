#pragma once

#include <vector>
#include <memory>
#include "Form/Object/Object.hpp"
#include <glm/glm.hpp>

class Object; // forward declaration

// Represents the independent 3-D world that lives inside a Zone.
// For now this is only a lightweight container that can be expanded later.
class World : public Singular {
public:
    enum class Mode { Creative=0, Survival, Spectator };

    // Advance simulation by delta-time (seconds)
    void update(float dt = 0.016f);

    // Draw all visible content belonging to this world
    void render() const;

    // Scene graph -------------------------------------------------------
    void addObject(std::unique_ptr<Object> obj);
    ~World();
    const std::vector<std::unique_ptr<Object>>& objects() const { return _objects; }
    // Access owned objects (read-only / mutable)
    const std::vector<std::unique_ptr<Object>>& getOwnedObjects() const { return _objects; }
    std::vector<std::unique_ptr<Object>>& getOwnedObjectsMutable() { return _objects; }

    // Physics & camera --------------------------------------------------
    void setCamera(glm::vec3* cam) { _cameraPos = cam; }
    void togglePhysics() { physicsEnabled = !physicsEnabled; }
    bool isPhysicsEnabled() const { return physicsEnabled; }
    void setMode(Mode m){ mode = m; }
    void load();
    void unload();
    Mode getMode() const { return mode; }

    // Draw the ground plane — temporary until its integrated into the broader object and zone creation system
    void drawGround();

    // Singular interface
    std::string getIdentifier() const override { return "World"; }

private:
    std::vector<std::unique_ptr<Object>> _objects;
    glm::vec3* _cameraPos = nullptr;
    bool physicsEnabled = true;
    Mode mode = Mode::Creative;
}; 