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

void testDistinctIdentitiesSharingSpelling() {
    Body body("Humanoid", "Voxel");
    auto* part1 = new BodyPart("Wing", BodyPart::Type::Arm);
    auto* part2 = new BodyPart("Wing", BodyPart::Type::Arm);

    part1->setIdentifier("left-wing-id-101");
    part2->setIdentifier("right-wing-id-102");

    check(part1->getName() == part2->getName(), "Both parts share the same spelling/name");
    check(part1->getIdentifier() != part2->getIdentifier(), "Parts have distinct identifiers");

    body.addPart(part1);
    body.addPart(part2);

    check(body.getBodyPart("left-wing-id-101") == part1, "Retrieved part1 by distinct identifier");
    check(body.getBodyPart("right-wing-id-102") == part2, "Retrieved part2 by distinct identifier");

    body.removeBodyPart("left-wing-id-101");
    check(body.getBodyPart("left-wing-id-101") == nullptr, "Part1 removed by identifier");
    check(body.getBodyPart("right-wing-id-102") == part2, "Part2 remains after part1 removed by identifier");

    body.removeBodyPart("right-wing-id-102");
    check(body.getBodyPart("right-wing-id-102") == nullptr, "Part2 removed by identifier");

    delete part1;
    delete part2;
}

int main() {
    std::cout << "Running body_test...\n";
    testBodyCreation();
    testAddAndGetBodyPart();
    testRemoveBodyPart();
    testBasicAvatarCreation();
    testDistinctIdentitiesSharingSpelling();
    std::cout << "All body_test assertions passed.\n";
    return 0;
}
