// Regression test: Zone designation relies strictly on unique identifier,
// not display name spelling.
//
// "A name may point. It must not silently constitute."
//
// When two distinct Zone instances share the same display name (e.g. Zone A with
// identifier "sanctuary-alpha" and display name "Sanctuary", and Zone B with
// identifier "sanctuary-beta" and display name "Sanctuary"), objects belonging
// to Zone A must be designated strictly by Zone A's unique identifier
// ("sanctuary-alpha") rather than its display name ("Sanctuary").
// Otherwise, both objects end up with "Sanctuary" as a designation and their
// zone membership is conflated.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

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
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone_designation_identity_test...\n";
    std::cout << "============================================================\n";

    // Create two distinct zones with different identifiers but identical display names.
    auto zoneA = std::make_shared<Zone>("sanctuary-alpha", "strict");
    zoneA->setName("Sanctuary");

    auto zoneB = std::make_shared<Zone>("sanctuary-beta", "strict");
    zoneB->setName("Sanctuary");

    check(zoneA->getIdentifier() == "sanctuary-alpha", "Zone A identifier is sanctuary-alpha");
    check(zoneB->getIdentifier() == "sanctuary-beta", "Zone B identifier is sanctuary-beta");
    check(zoneA->name() == "Sanctuary" && zoneB->name() == "Sanctuary",
          "Zone A and Zone B share the same display name ('Sanctuary')");

    // Create objects for each zone.
    auto objA = std::make_shared<Object>();
    objA->setObjectID("object-a");
    zoneA->addObject(objA);

    auto objB = std::make_shared<Object>();
    objB->setObjectID("object-b");
    zoneB->addObject(objB);

    // Add zones to ZoneManager and hydrate/designate objects.
    ZoneManager mgr;
    mgr.addZone(zoneA);
    mgr.addZone(zoneB);

    // Apply zone designations as done during ZoneManager hydration / state processing
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& obj : z->getOwnedObjects()) {
            if (!obj) continue;
            obj->addZoneDesignation(z->getIdentifier());
        }
    }

    // Verify that Object A belongs ONLY to Zone A's unique identifier
    check(objA->belongsToZone("sanctuary-alpha"),
          "Object A belongs to Zone A's identifier ('sanctuary-alpha')");
    check(!objA->belongsToZone("sanctuary-beta"),
          "Object A does NOT belong to Zone B's identifier ('sanctuary-beta')");
    check(!objA->belongsToZone("Sanctuary"),
          "Object A does NOT use shared display name ('Sanctuary') as a zone designation");

    // Verify that Object B belongs ONLY to Zone B's unique identifier
    check(objB->belongsToZone("sanctuary-beta"),
          "Object B belongs to Zone B's identifier ('sanctuary-beta')");
    check(!objB->belongsToZone("sanctuary-alpha"),
          "Object B does NOT belong to Zone A's identifier ('sanctuary-alpha')");
    check(!objB->belongsToZone("Sanctuary"),
          "Object B does NOT use shared display name ('Sanctuary') as a zone designation");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_designation_identity_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_designation_identity_test: ALL OK\n";
    return 0;
}
