// Regression test demonstrating that distinct Zone identities sharing the same
// display name spelling do not conflate object zone designations.
//
// Philosophy: A name may point. It must not silently constitute.

#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

int main() {
    // Construct two distinct Zone instances with unique identifiers
    // but sharing identical display names ("Sanctuary").
    auto zoneA = std::make_shared<Zone>("zone.sanctuary-alpha", "strict");
    zoneA->setName("Sanctuary");

    auto zoneB = std::make_shared<Zone>("zone.sanctuary-beta", "strict");
    zoneB->setName("Sanctuary");

    assert(zoneA->getIdentifier() != zoneB->getIdentifier() && "Identifiers must be distinct");
    assert(zoneA->name() == zoneB->name() && "Display names must be identical");

    // Construct an object and place it in Zone A via ZoneManager
    auto objA = std::make_shared<Object>("object-alpha");
    zoneA->addObject(objA);

    ZoneManager mgr;
    mgr.addZone(zoneA);
    mgr.addZone(zoneB);

    // Hydrate zone designations on global objects as ZoneManager does
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& obj : z->getOwnedObjects()) {
            if (!obj) continue;
            obj->addZoneDesignation(z->getIdentifier());
            mgr.getGlobalObjects().push_back(obj);
        }
    }

    // Verify objA belongs strictly to zoneA's unique identifier
    assert(objA->belongsToZone(zoneA->getIdentifier()) &&
           "Object A must belong to zone A's unique identifier");

    // Verify objA DOES NOT belong to zoneB's unique identifier
    assert(!objA->belongsToZone(zoneB->getIdentifier()) &&
           "Object A must NOT belong to zone B's unique identifier");

    // Verify objA DOES NOT carry the display name ("Sanctuary") as a zone designation,
    // ensuring display name spelling does not silently constitute identity.
    assert(!objA->belongsToZone("Sanctuary") &&
           "Display name spelling must not be added as a zone designation");

    std::printf("zone_designation_identity_test: OK\n");
    return 0;
}
