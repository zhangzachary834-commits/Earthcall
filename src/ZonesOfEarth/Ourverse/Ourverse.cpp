#include "Ourverse.hpp"
#include <iostream>
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ConstructedBeing/Object/Object.hpp"
#include <unordered_set>
#include "Singularity/Screen/HighlightSystem.hpp"
#include "Singularity/Screen/ShadingSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <unordered_map>

extern ZoneManager mgr;

// This class is for the entire digital existence of Earthcall
// It's called "Ourverse" because it is our creation, an embodiment of everything as we relate to it

void Ourverse::addZone(Zone zone) {
    zones.push_back(zone);
}

void Ourverse::addHome(Home home) {
    homes.push_back(home);
}

// void Ourverse::relate(Relation relation) {
//     relations.push_back(relation);
// }
void Ourverse::relate(const std::shared_ptr<Relation>& relation) {
    if (!relation) return;
    relations.push_back(relation);
}

void Ourverse::display() const {
    std::cout << "🌐 OURVERSE STATUS 🌐" << std::endl;
    for (const auto& z : zones) z.describe();
    for (const auto& h : homes) h.welcome();
    for (const auto& r : relations) {
        if (r) r->describe();
    }
}


void Ourverse::updateObjectCollisions(glm::vec3& position, const Object& obj, const glm::mat4& transform) const {
    obj.updateCollisionZone(transform);
    glm::vec3 correction(0.0f);
    if (obj.computePointPenetration(position, correction)) {
        position += correction;
    }
}

void Ourverse::onUpdate(float deltaTime) {
    if (!cameraPos) return;
    // Determine the visible ground height so physics collisions align with the rendered plane
    float groundY = 0.0f;
    if (ownedObjects.size() > 1 && ownedObjects[1]) {
        const glm::mat4& gT = ownedObjects[1]->getTransform();
        // Column 1 represents the Y axis after scaling/rotation; its length is the current scale on Y
        float scaleY = glm::length(glm::vec3(gT[1]));
        groundY = gT[3][1] + 0.5f * scaleY; // translation Y + half the total height
    }

    if (Physics::getLegacyEngineEnabled()) {
        Physics::applyGravity(*cameraPos,
                              deltaTime,
                              groundY);
        
        // Ensure every owned object has a physics body (zone-level toggle is true by default)
        for (const auto& upObj : ownedObjects) {
            if (upObj) {
                Physics::getFormFor(upObj.get());
            }
        }

            Physics::updateBodies(ownedObjects, deltaTime, /*gravityAccel*/ 9.81f, /*airResistance*/ 0.1f, groundY);

            Physics::enforceCollisions(*cameraPos, ownedObjects);
    }
}

void Ourverse::clearDynamicObjects() {
    if (ownedObjects.size() > 2) {
        ownedObjects.erase(ownedObjects.begin() + 2, ownedObjects.end());
    }
}
