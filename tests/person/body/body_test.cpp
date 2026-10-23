#include "Person/Body/Body.hpp"
#include "Person/Body/Head/Head.hpp"
#include "Person/Voice/Voice.hpp"
#include "Person/Voice/VoicePart/VoicePart.hpp"
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

void testDistinctIdentitiesSameName() {
    Body body("Humanoid", "Voxel");
    auto* arm1 = new BodyPart("Arm", BodyPart::Type::Arm);
    auto* arm2 = new BodyPart("Arm", BodyPart::Type::Arm);

    // A Formation is already a uniquely identified being; display names are not IDs.
    check(arm1->getIdentifier() != arm2->getIdentifier(), "Same-named arms have distinct default Formation IDs");
    check(arm1->getIdentifier() == arm1->Formation::getIdentifier(), "BodyPart uses Formation identity");
    check(arm1->setIdentifier("arm.left"), "Authoring a stable Formation ID succeeds");
    check(arm2->setIdentifier("arm.right"), "Authoring a second stable Formation ID succeeds");
    check(arm1->getName() == "Arm" && arm2->getName() == "Arm", "Identifiers do not change display names");

    body.addPart(arm1);
    body.addPart(arm2);
    check(body.getBodyPart("arm.left") == arm1, "Exact left-arm ID resolves");
    check(body.getBodyPart("arm.right") == arm2, "Exact right-arm ID resolves");
    check(body.getBodyPart("Arm") == nullptr, "Ambiguous display name is refused");
    body.removeBodyPart("Arm");
    check(body.formation.hasMember(arm1) && body.formation.hasMember(arm2),
          "Ambiguous removal does not silently remove either arm");

    auto* idShadowsName = new BodyPart("Leg", BodyPart::Type::Leg);
    check(idShadowsName->setIdentifier("Arm"), "Exact ID can coincide with another part's display name");
    body.addPart(idShadowsName);
    check(body.getBodyPart("Arm") == idShadowsName, "Exact ID takes precedence over display-name fallback");
    body.removeBodyPart("Arm");
    check(!body.formation.hasMember(idShadowsName), "Exact-ID removal releases Formation membership");
    check(body.getBodyPart("Arm") == nullptr, "Ambiguous display name remains refused");
    delete idShadowsName;

    body.removeBodyPart("arm.left");
    check(!body.formation.hasMember(arm1), "Removed arm is released from Formation");
    check(body.getBodyPart("arm.left") == nullptr, "Removed identity no longer resolves");
    check(body.getBodyPart("arm.right") == arm2, "Unrelated identity survives removal");
    check(body.getBodyPart("Arm") == arm2, "Unique display-name fallback remains supported");
    body.removeBodyPart("Arm");
    check(!body.formation.hasMember(arm2), "Unique-name removal releases remaining member");
    delete arm1;
    delete arm2;
}

void testVoiceIdentityAndRemoval() {
    Voice voice("Melodic");
    auto* pitch1 = new VoicePart("Pitch", VoicePart::Type::Pitch);
    auto* pitch2 = new VoicePart("Pitch", VoicePart::Type::Pitch);
    check(pitch1->getIdentifier() != pitch2->getIdentifier(), "Same-named voice parts have unique Formation IDs");
    check(pitch1->getIdentifier() == pitch1->Formation::getIdentifier(), "VoicePart uses Formation identity");
    check(pitch1->setIdentifier("pitch.base") && pitch2->setIdentifier("pitch.harmony"),
          "Voice identifiers can be authored without changing display names");
    voice.addPart(pitch1);
    voice.addPart(pitch2);
    check(voice.getVoicePart("pitch.base") == pitch1, "Exact voice ID resolves");
    check(voice.getVoicePart("pitch.harmony") == pitch2, "Second voice ID resolves");
    check(voice.getVoicePart("Pitch") == nullptr, "Ambiguous voice display name is refused");
    voice.removeVoicePart("Pitch");
    check(voice.formation.hasMember(pitch1) && voice.formation.hasMember(pitch2),
          "Ambiguous voice removal preserves both members");

    auto* exact = new VoicePart("Timbre", VoicePart::Type::Timbre);
    check(exact->setIdentifier("Pitch"), "Voice ID can shadow a display name");
    voice.addPart(exact);
    check(voice.getVoicePart("Pitch") == exact, "Exact voice ID wins over ambiguous names");
    voice.removeVoicePart("Pitch");
    check(!voice.formation.hasMember(exact), "Exact voice removal releases Formation membership");
    delete exact;

    voice.removeVoicePart("pitch.base");
    check(!voice.formation.hasMember(pitch1), "Voice removal releases correct member");
    check(voice.getVoicePart("pitch.harmony") == pitch2, "Other voice identity survives");
    check(voice.getVoicePart("Pitch") == pitch2, "Unique voice display-name fallback works");
    voice.removeVoicePart("Pitch");
    check(!voice.formation.hasMember(pitch2), "Unique voice-name removal releases member");
    delete pitch1;
    delete pitch2;
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
    testDistinctIdentitiesSameName();
    testVoiceIdentityAndRemoval();
    testBasicAvatarCreation();
    std::cout << "All body_test assertions passed.\n";
    return 0;
}
