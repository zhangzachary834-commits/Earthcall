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

void testDistinctBodyPartsLookupAndRemoval() {
    Body body("Humanoid", "Voxel");
    auto* arm1 = new BodyPart("Arm", BodyPart::Type::Arm, ObjectTypes::ShapeKind::Cube, {1.0f, 1.0f, 1.0f});
    auto* arm2 = new BodyPart("Arm", BodyPart::Type::Arm, ObjectTypes::ShapeKind::Cube, {1.0f, 1.0f, 1.0f});

    arm1->setIdentifier("arm.left");
    arm2->setIdentifier("arm.right");

    body.addPart(arm1);
    body.addPart(arm2);

    check(body.getBodyPart("arm.left") == arm1, "Can retrieve arm1 by exact identifier 'arm.left'");
    check(body.getBodyPart("arm.right") == arm2, "Can retrieve arm2 by exact identifier 'arm.right'");

    // A display name must not shadow a different part's exact identifier.
    check(body.getBodyPart("Arm") == arm1, "Display-name fallback finds first matching arm");
    auto* namedById = new BodyPart("Leg", BodyPart::Type::Leg, ObjectTypes::ShapeKind::Cube, {1.0f, 1.0f, 1.0f});
    namedById->setIdentifier("Arm");
    body.addPart(namedById);
    check(body.getBodyPart("Arm") == namedById, "Exact identifier wins over earlier display-name match");
    body.removeBodyPart("Arm");
    check(!body.formation.hasMember(namedById), "Removing exact ID releases Formation membership");
    check(body.getBodyPart("Arm") == arm1, "Display-name fallback resumes after exact-ID removal");
    check(body.getBodyPart("arm.right") == arm2, "Unrelated identity survives exact-ID removal");
    delete namedById;

    // Removing arm1 by unique identifier leaves arm2 intact despite matching display name "Arm"
    body.removeBodyPart("arm.left");
    check(body.getBodyPart("arm.left") == nullptr, "arm.left is removed");
    check(body.getBodyPart("arm.right") == arm2, "arm.right remains in body");

    delete arm1;
    delete arm2;
}

int main() {
    std::cout << "Running body_test...\n";
    testBodyCreation();
    testAddAndGetBodyPart();
    testRemoveBodyPart();
    testBasicAvatarCreation();
    testDistinctBodyPartsLookupAndRemoval();
    std::cout << "All body_test assertions passed.\n";
    return 0;
}
