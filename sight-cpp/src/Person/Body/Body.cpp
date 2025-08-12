#include "Body.hpp"
#include <iostream>
#include <algorithm>
#include "BodyPart/BodyPart.hpp"
#include "Form/Object/Formation/Formations.hpp"
#include "Form/Form.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "BodyPart/Limb/Arm.hpp"
#include "BodyPart/Limb/Leg.hpp"
#include "BodyPart/Limb/Torso.hpp"
#include "BodyPart/Limb/Neck.hpp"
#include "Head/Head.hpp"
#include "BodyPart/Chest.hpp"
#include "BodyPart/Stomach.hpp"
#include "BodyPart/Limb/Shoulder.hpp"
#include "BodyPart/Limb/ForeArm.hpp"
#include "BodyPart/Limb/ForeLeg.hpp"
#include "BodyPart/Limb/Foot.hpp"

Body::Body(std::string shape, std::string artStyle)
    : shape(shape), artStyle(artStyle), formation(Form::ShapeType::Cube, {1.0f, 1.0f, 1.0f})
{
    // Voxel player default approx 1.8m tall, but our simplified avatar is ~1 unit tall in model space
    hitboxHeight = 1.0f;
    updateMeasurements();
}

void Body::describe() const {
    std::cout << "🧍 Body Shape: " << shape << ", Style: " << artStyle << std::endl;
    std::cout << "   Type: " << static_cast<int>(bodyType) << ", Proportions: " << static_cast<int>(proportions) << std::endl;
    std::cout << "   Height: " << height << "m, Weight: " << weight << "kg" << std::endl;
    std::cout << "   Muscle: " << muscleMass << ", Body Fat: " << bodyFat << std::endl;
    std::cout << "   Adornments:" << std::endl;
    for (const auto& item : adornments) {
        std::cout << "   - " << item << std::endl;
    }
    std::cout << "   Clothing:" << std::endl;
    for (const auto& item : clothing) {
        std::cout << "   - " << item.first << " (" << (item.second.isEquipped ? "Equipped" : "Unequipped") << ")" << std::endl;
    }
    std::cout << "   Parts:" << std::endl;
    for (const auto* p : parts) {
        if (p)
            std::cout << "   - " << p->getName() << std::endl;
    }
}

void Body::addAdornment(const std::string& item) {
    adornments.push_back(item);
}

void Body::addPart(BodyPart* part) {
    if (!part) return;
    parts.push_back(part);
    formation.addMember(static_cast<Object*>(part));
}

void Body::draw() const {
    for (const auto* p : parts) {
        if (p) p->draw();
    }
}

void Body::setBodyType(BodyType type) {
    bodyType = type;
    // Could trigger body part regeneration here
}

void Body::setProportions(Proportions props) {
    proportions = props;
    updateMeasurements();
    scaleBodyParts();
}

void Body::setHeight(float h) {
    height = h;
    hitboxHeight = h;
    updateMeasurements();
    scaleBodyParts();
}

void Body::setWeight(float w) {
    weight = w;
    updateMeasurements();
}

void Body::setMuscleMass(float mass) {
    muscleMass = std::clamp(mass, 0.0f, 1.0f);
    updateMeasurements();
    scaleBodyParts();
}

void Body::setBodyFat(float fat) {
    bodyFat = std::clamp(fat, 0.0f, 1.0f);
    updateMeasurements();
    scaleBodyParts();
}

void Body::updateMeasurements() {
    float baseScale = height / 1.8f;  // Normalize to 1.8m
    
    switch (proportions) {
        case Proportions::Child:
            baseScale *= 0.7f;
            measurements.chest = 0.6f * baseScale;
            measurements.waist = 0.5f * baseScale;
            measurements.hips = 0.6f * baseScale;
            measurements.shoulders = 0.3f * baseScale;
            measurements.armLength = 0.5f * baseScale;
            measurements.legLength = 0.6f * baseScale;
            break;
        case Proportions::Teen:
            baseScale *= 0.85f;
            measurements.chest = 0.75f * baseScale;
            measurements.waist = 0.6f * baseScale;
            measurements.hips = 0.75f * baseScale;
            measurements.shoulders = 0.4f * baseScale;
            measurements.armLength = 0.6f * baseScale;
            measurements.legLength = 0.75f * baseScale;
            break;
        case Proportions::Adult:
            measurements.chest = 0.9f * baseScale * (1.0f + muscleMass * 0.3f);
            measurements.waist = 0.7f * baseScale * (1.0f + bodyFat * 0.4f);
            measurements.hips = 0.9f * baseScale * (1.0f + bodyFat * 0.3f);
            measurements.shoulders = 0.5f * baseScale * (1.0f + muscleMass * 0.2f);
            measurements.armLength = 0.7f * baseScale;
            measurements.legLength = 0.9f * baseScale;
            break;
        case Proportions::Elder:
            baseScale *= 0.9f;
            measurements.chest = 0.8f * baseScale;
            measurements.waist = 0.8f * baseScale;
            measurements.hips = 0.8f * baseScale;
            measurements.shoulders = 0.45f * baseScale;
            measurements.armLength = 0.65f * baseScale;
            measurements.legLength = 0.8f * baseScale;
            break;
    }
}

void Body::scaleBodyParts() {
    for (auto* part : parts) {
        if (!part) continue;
        
        glm::vec3 scale(1.0f);
        
        // Scale based on body part type
        if (part->getName().find("Arm") != std::string::npos) {
            scale.x = measurements.armLength / 0.7f;
        } else if (part->getName().find("Leg") != std::string::npos) {
            scale.x = measurements.legLength / 0.9f;
        } else if (part->getName().find("Torso") != std::string::npos) {
            scale.x = measurements.chest / 0.9f;
            scale.z = measurements.chest / 0.9f;
        } else if (part->getName().find("Chest") != std::string::npos) {
            scale.x = measurements.chest / 0.9f;
            scale.z = measurements.chest / 0.9f;
        } else if (part->getName().find("Stomach") != std::string::npos) {
            scale.x = measurements.waist / 0.7f;
            scale.z = measurements.waist / 0.7f;
        }
        
        // Apply muscle mass scaling
        if (muscleMass > 0.5f) {
            scale *= (1.0f + (muscleMass - 0.5f) * 0.2f);
        }
        
        // Apply body fat scaling
        if (bodyFat > 0.3f) {
            scale *= (1.0f + (bodyFat - 0.3f) * 0.1f);
        }
        
        glm::mat4 currentTransform = part->localTransform();
        glm::mat4 scaledTransform = glm::scale(glm::mat4(1.0f), scale) * currentTransform;
        part->setLocalTransform(scaledTransform);
    }
}

void Body::addClothing(const Clothing& item) {
    clothing[item.name] = item;
}

bool Body::equipClothing(const std::string& itemName) {
    auto it = clothing.find(itemName);
    if (it == clothing.end()) return false;
    
    // Unequip any existing item in the same slot
    for (auto& item : clothing) {
        if (item.second.slot == it->second.slot && item.second.isEquipped) {
            item.second.isEquipped = false;
        }
    }
    
    it->second.isEquipped = true;
    return true;
}

bool Body::unequipClothing(const std::string& itemName) {
    auto it = clothing.find(itemName);
    if (it == clothing.end()) return false;
    
    it->second.isEquipped = false;
    return true;
}

const Body::Clothing* Body::getEquippedClothing(const std::string& slot) const {
    for (const auto& item : clothing) {
        if (item.second.slot == slot && item.second.isEquipped) {
            return &item.second;
        }
    }
    return nullptr;
}

float Body::getTotalProtection() const {
    float total = 0.0f;
    for (const auto& item : clothing) {
        if (item.second.isEquipped) {
            total += item.second.protection;
        }
    }
    return total;
}

float Body::getTotalWarmth() const {
    float total = 0.0f;
    for (const auto& item : clothing) {
        if (item.second.isEquipped) {
            total += item.second.warmth;
        }
    }
    return total;
}

BodyPart* Body::getBodyPart(const std::string& name) const {
    for (auto* part : parts) {
        if (part && part->getName() == name) {
            return part;
        }
    }
    return nullptr;
}

std::vector<BodyPart*> Body::getBodyPartsByType(BodyPart::Type type) const {
    std::vector<BodyPart*> result;
    for (auto* part : parts) {
        if (part && part->getType() == type) {
            result.push_back(part);
        }
    }
    return result;
}

void Body::removeBodyPart(const std::string& name) {
    auto it = std::remove_if(parts.begin(), parts.end(),
        [&name](const BodyPart* part) {
            return part && part->getName() == name;
        });
    parts.erase(it, parts.end());
}

bool Body::isInjured() const {
    // Check if any body parts are damaged
    for (const auto* part : parts) {
        if (part && part->getColor()[0] < 0.8f) {  // Red tint indicates injury
            return true;
        }
    }
    return false;
}

bool Body::isHealthy() const {
    return !isInjured();
}

float Body::getOverallHealth() const {
    float totalHealth = 0.0f;
    int partCount = 0;
    
    for (const auto* part : parts) {
        if (part) {
            const float* color = part->getColor();
            // Use color as health indicator (green = healthy, red = injured)
            float health = color[1];  // Green component
            totalHealth += health;
            partCount++;
        }
    }
    
    return partCount > 0 ? totalHealth / partCount : 1.0f;
}

// -----------------------------------------------------------------------------
//  Factory: Build a simple humanoid avatar using primitive BodyParts
// -----------------------------------------------------------------------------
Body Body::createBasicAvatar(const std::string& artStyle) {
    return createCustomAvatar(artStyle, BodyType::Humanoid, Proportions::Adult);
}

Body Body::createCustomAvatar(const std::string& artStyle, BodyType type, Proportions props) {
    // Base body object
    Body avatar("Humanoid", artStyle);
    avatar.setBodyType(type);
    avatar.setProportions(props);

    // ----------------------- Head -----------------------
    auto* head = new Head();
    head->setLocalTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.75f, 0.0f)));
    avatar.addPart(head);

    // ----------------------- Torso ----------------------
    auto* torso = new Torso();
    torso->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, 0.0f)));
    avatar.addPart(torso);

    // ----------------------- Chest & Stomach -----------
    auto* chest = new Chest();
    chest->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.45f, 0.0f)));
    avatar.addPart(chest);

    auto* stomach = new Stomach();
    stomach->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.125f, 0.0f)));
    avatar.addPart(stomach);

    // ----------------------- Neck ----------------------
    auto* neck = new Neck();
    neck->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.7f, 0.0f)));
    avatar.addPart(neck);

    // ----------------------- Shoulders -----------------
    auto* leftShoulder = new Shoulder(Shoulder::Side::Left);
    leftShoulder->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, 0.6f, 0.0f)));
    avatar.addPart(leftShoulder);

    auto* rightShoulder = new Shoulder(Shoulder::Side::Right);
    rightShoulder->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, 0.6f, 0.0f)));
    avatar.addPart(rightShoulder);

    // ----------------------- Arms -----------------------
    auto* leftArm = new Arm(Arm::Side::Left);
    leftArm->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, 0.25f, 0.0f)));
    avatar.addPart(leftArm);

    auto* rightArm = new Arm(Arm::Side::Right);
    rightArm->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, 0.25f, 0.0f)));
    avatar.addPart(rightArm);

    // -------------------- Forearms ---------------------
    auto* leftForeArm = new ForeArm(ForeArm::Side::Left);
    leftForeArm->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-0.35f, -0.05f, 0.0f)));
    avatar.addPart(leftForeArm);

    auto* rightForeArm = new ForeArm(ForeArm::Side::Right);
    rightForeArm->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.35f, -0.05f, 0.0f)));
    avatar.addPart(rightForeArm);

    // ----------------------- Legs -----------------------
    auto* leftLeg = new Leg(Leg::Side::Left);
    leftLeg->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-0.15f, -0.5f, 0.0f)));
    avatar.addPart(leftLeg);

    auto* rightLeg = new Leg(Leg::Side::Right);
    rightLeg->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, -0.5f, 0.0f)));
    avatar.addPart(rightLeg);

    // -------------------- Forelegs ---------------------
    auto* leftForeLeg = new ForeLeg(ForeLeg::Side::Left);
    leftForeLeg->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-0.15f, -0.9f, 0.0f)));
    avatar.addPart(leftForeLeg);

    auto* rightForeLeg = new ForeLeg(ForeLeg::Side::Right);
    rightForeLeg->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, -0.9f, 0.0f)));
    avatar.addPart(rightForeLeg);

    // ----------------------- Feet ----------------------
    auto* leftFoot = new Foot(Foot::Side::Left);
    leftFoot->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-0.15f, -1.15f, 0.1f)));
    avatar.addPart(leftFoot);

    auto* rightFoot = new Foot(Foot::Side::Right);
    rightFoot->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, -1.15f, 0.1f)));
    avatar.addPart(rightFoot);

    // Apply customizations
    avatar.scaleBodyParts();

    return avatar;
}

Body Body::createChildAvatar(const std::string& artStyle) {
    return createCustomAvatar(artStyle, BodyType::Humanoid, Proportions::Child);
}

Body Body::createElderAvatar(const std::string& artStyle) {
    return createCustomAvatar(artStyle, BodyType::Humanoid, Proportions::Elder);
}
