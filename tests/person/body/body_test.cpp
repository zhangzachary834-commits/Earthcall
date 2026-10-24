#include "Person/Body/Body.hpp"
#include "Person/Body/Head/Head.hpp"
#include "Person/Body/BodyPart/Limb/Torso.hpp"
#include <iostream>

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "PASSED: " << message << std::endl;
}

void testBodyCreation() {
    Body body("Humanoid", "Voxel");
    check(body.shape == "Humanoid", "Body shape is set correctly");
    check(body.artStyle == "Voxel", "Body art style is set correctly");
}

void testAddAndGetBodyPart() {
    Body body("Humanoid", "Voxel");
    auto* head = new Head();

    body.addPart(head);

    BodyPart* retrieved = body.getBodyPart("Head");
    check(retrieved != nullptr, "Added body part can be retrieved by name");
    check(retrieved == head, "Retrieved body part is the same as the added one");

    std::vector<BodyPart*> heads = body.getBodyPartsByType(BodyPart::Type::Head);
    check(heads.size() == 1, "getBodyPartsByType returns correct number of parts");
    check(heads[0] == head, "getBodyPartsByType returns the correct part");
    delete head;
}

void testRemoveBodyPart() {
    Body body("Humanoid", "Voxel");
    auto* head = new Head();

    body.addPart(head);
    body.removeBodyPart("Head");

    check(body.getBodyPart("Head") == nullptr, "Removed body part is no longer found");
    delete head;
}

void testBasicAvatarCreation() {
    Body avatar = Body::createBasicAvatar("LowPoly");
    check(avatar.shape == "Humanoid", "Basic avatar shape is Humanoid");
    check(avatar.getBodyPart("Head") != nullptr, "Basic avatar has a Head");
    check(avatar.getBodyPart("Torso") != nullptr, "Basic avatar has a Torso");
    check(avatar.getBodyPartsByType(BodyPart::Type::Arm).size() == 2, "Basic avatar has 2 arms");
    check(avatar.getBodyPartsByType(BodyPart::Type::Leg).size() == 2, "Basic avatar has 2 legs");
    for (auto* p : avatar.parts) {
        delete p;
    }
}

void testDistinctIdentitiesSharingDisplayName() {
    Body body("Humanoid", "Voxel");

    auto* leftWing = new BodyPart("Wing", BodyPart::Type::Undefined);
    leftWing->setIdentifier("left_wing_01");

    auto* rightWing = new BodyPart("Wing", BodyPart::Type::Undefined);
    rightWing->setIdentifier("right_wing_02");

    body.addPart(leftWing);
    body.addPart(rightWing);

    check(leftWing->getIdentifier() == "left_wing_01", "leftWing has distinct identifier");
    check(rightWing->getIdentifier() == "right_wing_02", "rightWing has distinct identifier");
    check(leftWing->getName() == "Wing", "leftWing has display name Wing");
    check(rightWing->getName() == "Wing", "rightWing has display name Wing");

    check(body.getBodyPart("left_wing_01") == leftWing, "Can retrieve leftWing specifically by identifier");
    check(body.getBodyPart("right_wing_02") == rightWing, "Can retrieve rightWing specifically by identifier");

    body.removeBodyPart("left_wing_01");
    check(body.getBodyPart("left_wing_01") == nullptr, "leftWing removed by identifier");
    check(body.getBodyPart("right_wing_02") == rightWing, "rightWing remains intact despite sharing display name Wing");

    delete leftWing;
    delete rightWing;
}

int main() {
    std::cout << "Running body_test...\n";
    testBodyCreation();
    testAddAndGetBodyPart();
    testRemoveBodyPart();
    testBasicAvatarCreation();
    testDistinctIdentitiesSharingDisplayName();
    std::cout << "All body_test assertions passed.\n";
    return 0;
}
