// Real Zone identity-store fresh-hydration witness for universal Singular
// Properties. This is deliberately not just a to_json/from_json unit test.
//
// Writer graph:
//   Zone
//     object.a -- authored Object* property --> object.b
//              -- authored dict.buddy ------> object.b
//              -- authored ScalarField
//
// The writer graph is destroyed. A fresh ZoneManager then hydrates only from
// the Zone identity store. References must bind to the NEW object.b pointer.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << "\n";
    } else {
        std::cout << "  ok: " << description << "\n";
    }
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Zone Singular-property fresh hydration test\n";
    std::cout << "============================================================\n";

    const auto sandbox =
        std::filesystem::temp_directory_path() /
        "earthcall_zone_singular_property_roundtrip";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    SaveSystem::setSaveRoot(sandbox.string());

    const std::string zoneId = "UniversalPropertyZone";
    Object* oldA = nullptr;
    Object* oldB = nullptr;

    {
        ZoneManager writer;
        auto zone = std::make_shared<Zone>(zoneId, "strict");
        zone->setDynamicProperty("authored.zone.mood", std::string("inhabited"));
        zone->setTelosId("lexeme.zone.persistence");

        auto a = std::make_shared<Object>("property-a");
        a->setObjectID("object.property-a");
        auto b = std::make_shared<Object>("property-b");
        b->setObjectID("object.property-b");

        oldA = a.get();
        oldB = b.get();

        a->setDynamicProperty("authored.friend", b.get());
        a->setDynamicProperty("authored.count", 73);
        a->setTelosId("lexeme.object.persistence");

        auto nested = std::make_shared<PropertyDict>();
        nested->elements["buddy"] = PropertyValue(b.get());
        nested->elements["label"] = PropertyValue(std::string("nested"));
        a->setDynamicProperty("authored.nested", nested);

        auto scalar = std::make_shared<OntoMath::ScalarField>();
        scalar->baseDensity = 4.5f;
        scalar->frequency = 2.25f;
        scalar->amplitude = 7.0f;
        a->setDynamicProperty("authored.field", scalar);

        zone->addObject(a);
        zone->addObject(b);
        writer.addZone(zone);
        writer.persistZones();
    }

    check(SaveSystem::zoneIdentityExists(zoneId),
          "writer committed Zone identity to isolated SaveRoot");

    ZoneManager reader;
    reader.hydrateFromZoneStore();

    std::shared_ptr<Zone> bootedZone;
    Object* bootedA = nullptr;
    Object* bootedB = nullptr;
    for (const auto& zone : reader.zones()) {
        if (!zone || zone->getIdentifier() != zoneId) continue;
        bootedZone = zone;
        for (const auto& object : zone->getOwnedObjects()) {
            if (!object) continue;
            if (object->getIdentifier() == "object.property-a") bootedA = object.get();
            if (object->getIdentifier() == "object.property-b") bootedB = object.get();
        }
    }

    check(bootedZone != nullptr, "fresh ZoneManager hydrated target Zone");
    check(bootedA != nullptr, "fresh ZoneManager hydrated object.a");
    check(bootedB != nullptr, "fresh ZoneManager hydrated object.b");
    check(bootedA != oldA, "object.a is a fresh process-lifetime instance");
    check(bootedB != oldB, "object.b is a fresh process-lifetime instance");

    if (bootedZone) {
        PropertyValue value;
        check(bootedZone->getDynamicProperty("authored.zone.mood", value) &&
                  std::holds_alternative<std::string>(value) &&
                  std::get<std::string>(value) == "inhabited",
              "Zone authored property survived identity-store hydration");
        check(bootedZone->telosId() == "lexeme.zone.persistence",
              "Zone registered telos survived identity-store hydration");
    }

    if (bootedA && bootedB) {
        PropertyValue value;

        check(bootedA->getDynamicProperty("authored.count", value) &&
                  std::holds_alternative<int>(value) &&
                  std::get<int>(value) == 73,
              "Object authored scalar survived identity-store hydration");

        check(bootedA->telosId() == "lexeme.object.persistence",
              "Object registered telos survived identity-store hydration");

        check(bootedA->getDynamicProperty("authored.friend", value) &&
                  std::holds_alternative<Object*>(value) &&
                  std::get<Object*>(value) == bootedB,
              "top-level Object* property rebound to fresh hydrated object.b");

        check(bootedA->getDynamicProperty("authored.nested", value) &&
                  std::holds_alternative<std::shared_ptr<PropertyDict>>(value),
              "nested PropertyDict survived identity-store hydration");
        if (std::holds_alternative<std::shared_ptr<PropertyDict>>(value)) {
            auto dict = std::get<std::shared_ptr<PropertyDict>>(value);
            bool nestedBound = false;
            if (dict) {
                auto buddy = dict->elements.find("buddy");
                nestedBound = buddy != dict->elements.end() &&
                    std::holds_alternative<Object*>(buddy->second) &&
                    std::get<Object*>(buddy->second) == bootedB;
            }
            check(nestedBound,
                  "nested Object* property rebound to fresh hydrated object.b");
        }

        check(bootedA->getDynamicProperty("authored.field", value) &&
                  std::holds_alternative<std::shared_ptr<OntoMath::ScalarField>>(value),
              "authored ScalarField survived identity-store hydration");
        if (std::holds_alternative<std::shared_ptr<OntoMath::ScalarField>>(value)) {
            auto field = std::get<std::shared_ptr<OntoMath::ScalarField>>(value);
            check(field && field->baseDensity == 4.5f &&
                      field->frequency == 2.25f && field->amplitude == 7.0f,
                  "authored ScalarField payload survived with non-default values");
        }
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "============================================================\n";
    std::cout << "Zone Singular-property roundtrip: " << g_checks
              << " checks, " << g_failures << " failures\n";
    std::cout << "============================================================\n";
    return g_failures == 0 ? 0 : 1;
}
