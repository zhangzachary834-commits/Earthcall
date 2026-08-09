#include "Body.hpp"
#include <iostream>
#include <algorithm>
#include "BodyPart/BodyPart.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Form/Form.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "BodyPart/Limb/Arm.hpp"
#include "BodyPart/Limb/Leg.hpp"
#include "BodyPart/Limb/Torso.hpp"
#include "BodyPart/Limb/Neck.hpp"
#include "Head/Head.hpp"

#include "BodyPart/Limb/Shoulder.hpp"
#include "BodyPart/Limb/ForeArm.hpp"
#include "BodyPart/Limb/ForeLeg.hpp"
#include "BodyPart/Limb/Foot.hpp"

Body::Body(std::string shape, std::string artStyle)
    : shape(shape), artStyle(artStyle), formation()
{
    // Voxel player default approx 1.8m tall, but our simplified avatar is ~1 unit tall in model space
    hitboxHeight = 1.0f;
}

void Body::describe() const {
    std::cout << "🧍 Body Shape: " << shape << ", Style: " << artStyle << std::endl;
    std::cout << "   Height: " << height << "m" << std::endl;
    std::cout << "   Adornments:" << std::endl;
    for (const auto& item : adornments) {
        std::cout << "   - " << item << std::endl;
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

void Body::setHeight(float h) {
    height = h;
    hitboxHeight = h;
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



// -----------------------------------------------------------------------------
//  Factory: Build a simple humanoid avatar using primitive BodyParts
// -----------------------------------------------------------------------------
Body Body::createBasicAvatar(const std::string& artStyle) {
    // Base body object
    Body avatar("Humanoid", artStyle);

    // ----------------------- Head -----------------------
    auto* head = new Head();
    head->setLocalTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.75f, 0.0f)));
    avatar.addPart(head);

    // ----------------------- Torso ----------------------
    auto* torso = new Torso();
    torso->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, 0.0f)));
    avatar.addPart(torso);

    // ----------------------- Lower Torso ----------------
    auto* lowerTorso = new BodyPart("LowerTorso", BodyPart::Type::Torso,
                                    Form(Form::ShapeType::Cube, {0.45f, 0.25f, 0.22f}));
    lowerTorso->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.15f, 0.0f)));
    avatar.addPart(lowerTorso);

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

    return avatar;
}
