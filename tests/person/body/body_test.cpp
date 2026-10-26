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

int main() {
    std::cout << "Running body_test...\n";
    testBodyCreation();
    testAddAndGetBodyPart();
    testRemoveBodyPart();
    testBasicAvatarCreation();
    std::cout << "All body_test assertions passed.\n";
    return 0;
}
