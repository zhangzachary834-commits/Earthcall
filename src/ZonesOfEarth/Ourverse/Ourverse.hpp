#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <map>

#include "../Zone/Zone.hpp"
#include "../HomesOfEarth/Home.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "../Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"

class Ourverse : public Singular {
public:
    void display() const;
    void renderModeUI();

    void updateObjectCollisions(glm::vec3& position, const Object& obj, const glm::mat4& transform) const;

    void onUpdate(float deltaTime = 0.016f);

    void setCamera(glm::vec3* cam) { cameraPos = cam; }
    glm::vec3* getCamera() const { return cameraPos; }

    void addOwnedObject(std::shared_ptr<Object> obj) { ownedObjects.push_back(std::move(obj)); }
    const std::vector<std::shared_ptr<Object>>& getOwnedObjects() const { return ownedObjects; }

    // Mutable access (use with caution)
    std::vector<std::shared_ptr<Object>>& getOwnedObjectsMutable() { return ownedObjects; }

    // Remove all objects spawned dynamically (keep baseline 0 and 1)
    void clearDynamicObjects();

    // Singular interface
    std::string getIdentifier() const override { return "Ourverse"; }

    std::shared_ptr<Zone> getPrimaryGatheringZone() const { return primaryGatheringZone; }
    void setPrimaryGatheringZone(std::shared_ptr<Zone> zone) { primaryGatheringZone = std::move(zone); }

    Formation& getLaws() { return laws; }
    const Formation& getLaws() const { return laws; }

private:
    void buildProperties() override {}

    glm::vec3* cameraPos = nullptr;
    // ownedObjects is used for physics integration.
    std::vector<std::shared_ptr<Object>> ownedObjects;
    std::shared_ptr<Zone> primaryGatheringZone;
    Formation laws;
};

struct InteractionEvent {
    std::string description;
    std::time_t timestamp;
    Object* other;
    // ... further relational or symbolic data
};
