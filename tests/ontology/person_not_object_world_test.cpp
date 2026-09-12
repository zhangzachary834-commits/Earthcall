#include "support/test_harness.hpp"
#include "Person/Person.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    // This fixture is intentionally historical evidence: its checked-in
    // categories bag contains an extra-spatial Object whose objectID is Zach.
    // Drive the same ZoneManager::loadState path the Person-facing Load office
    // uses and prove the runtime boundary refuses that counterfeit.
    const std::string world =
        TestSupport::resolveRealWorldPath("saves/worlds/basic_pixel_changer.json");
    if (!std::filesystem::exists(world)) {
        std::cout << "person_not_object_world_test: fixture absent; skipped\n";
        return 0;
    }

    // loadState may evolve Zone/Home identities as part of ordinary migration.
    // Keep the live repository byte-identical after the test.
    TestSupport::RealSaveTreeGuard guard(world);
    TestSupport::BootedEngineHarness h("Zach");
    h.loadWorld(world);

    // The exact historical bug: CategoryManager used to hydrate this entry and
    // Universe then contained an Object called Zach alongside the Person.
    assert(categories.get("Zach") == nullptr);

    int zachCount = 0;
    Person* zachPerson = nullptr;
    for (Singular* being : Universe::instance().beings()) {
        if (!being || being->getIdentifier() != "Zach") continue;
        ++zachCount;
        if (auto* person = dynamic_cast<Person*>(being)) zachPerson = person;
    }

    assert(zachCount == 1);
    assert(zachPerson == &h.player);

    // This is why the old counterfeit existed in the first place: authored Law
    // reattachment needed a being called Zach. The fix is complete only if the
    // Law now reattaches to the actual Person rather than merely losing an
    // author when CategoryManager refuses the Object-shaped stand-in.
    Law* pixelLaw = h.lawManager.find("law-basic-pixel-changer");
    assert(pixelLaw != nullptr);
    const auto& authors = pixelLaw->authors().getMembers();
    assert(authors.size() == 1);
    assert(authors.front() == &h.player);

    // Saving categories after the live load must not resurrect the impostor.
    const std::string categoriesJson = categories.toJson().dump();
    assert(categoriesJson.find("\"objectID\":\"Zach\"") == std::string::npos);

    std::cout << "person_not_object_world_test: live world contains Zach the Person, not Object Zach\n";
    return 0;
}
