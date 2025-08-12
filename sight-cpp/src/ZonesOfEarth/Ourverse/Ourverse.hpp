#pragma once
#include <vector>
#include <string>
#include <memory>

#include "../Zone/Zone.hpp"
#include "../HomesOfEarth/Home.hpp"
#include "Relation/Relation.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Formations.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "../Physics/Physics.hpp"
#include "Singular.hpp"

class Ourverse : public Singular {
public:
    enum class GameMode {
        Creative,
        Survival,
        Spectator
    };

    GameMode mode = GameMode::Creative;
    bool physicsEnabled = true;

    void setMode(GameMode m) { mode = m; }
    GameMode getMode() const { return mode; }
    void togglePhysics() { physicsEnabled = !physicsEnabled; }
    bool isPhysicsEnabled() const { return physicsEnabled; }

    std::vector<Zone> zones;
    std::vector<Home> homes;
    std::vector<Relation> relations;

    void addZone(Zone zone);
    void addHome(Home home);
    void relate(Relation relation);
    void display() const;
    void renderModeUI();

    void updateObjectCollisions(glm::vec3& position, const Object& obj, const glm::mat4& transform) const;

    void onUpdate(float deltaTime = 0.016f);

    void setCamera(glm::vec3* cam) { cameraPos = cam; }
    glm::vec3* getCamera() const { return cameraPos; }

    void addOwnedObject(std::unique_ptr<Object> obj) { ownedObjects.push_back(std::move(obj)); }
    const std::vector<std::unique_ptr<Object>>& getOwnedObjects() const { return ownedObjects; }

    // Mutable access (use with caution)
    std::vector<std::unique_ptr<Object>>& getOwnedObjectsMutable() { return ownedObjects; }

    // Remove all objects spawned dynamically (keep baseline 0 and 1)
    void clearDynamicObjects();

    // Singular interface
    std::string getIdentifier() const override { return "Ourverse"; }

private:
    glm::vec3* cameraPos = nullptr;
    std::vector<std::unique_ptr<Object>> ownedObjects;
};

struct InteractionEvent {
    std::string description;
    std::time_t timestamp;
    Object* other;
    // ... further relational or symbolic data
};