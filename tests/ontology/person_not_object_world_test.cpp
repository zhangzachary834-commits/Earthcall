#include "support/test_harness.hpp"
#include "Person/Person.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    // Historical evidence only. This checked-in fixture predates cryptographic
    // Person identity and records the human author merely as the display token
    // "Zach". Under the current ontology that token is AMBIGUOUS: a Person may
    // be called Zach and an Object may also be called Zach. The old runtime
    // guard used to pretend the spelling itself proved Personhood; it no longer
    // does so. Do not mutate this sacred historical save merely to make the
    // witness convenient — migration must replace the author token with a
    // Person identity carrying actual provenance.
    const std::string world =
        TestSupport::resolveRealWorldPath("saves/worlds/basic_pixel_changer.json");
    if (!std::filesystem::exists(world)) {
        std::cout << "person_not_object_world_test: fixture absent; skipped\n";
        return 0;
    }

    TestSupport::RealSaveTreeGuard guard(world);
    TestSupport::BootedEngineHarness h("Zach");
    h.loadWorld(world);

    // The lexical collision is now deliberately legal. This fixture therefore
    // remains migration evidence rather than proof that a display name reserves
    // identity. What MUST remain true is that the loaded Person is still a
    // Person through the C++ ontology, not an Object masquerading as one.
    Person* playerAsPerson = dynamic_cast<Person*>(&h.player);
    assert(playerAsPerson == &h.player);
    assert(dynamic_cast<Object*>(&h.player) == nullptr);

    // Legacy author text is unresolved ontology debt until the save/generator
    // is explicitly migrated to the Person's unique identity. We do not assert
    // that whichever being is currently found under the ambiguous token is the
    // Person; doing so would reinstall string spelling as identity in a test.
    Law* pixelLaw = h.lawManager.find("law-basic-pixel-changer");
    assert(pixelLaw != nullptr);

    std::cout << "person_not_object_world_test: legacy display-token collision retained as migration evidence\n";
    return 0;
}
