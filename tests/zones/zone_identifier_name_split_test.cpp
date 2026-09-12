// Zone identifier/name split (Zach, 2026-09-09, live in the running app):
// "Yes Zone should absolutely have a real identifier/name split. It's a
// Singular." Found while verifying Sol's Invariant 4 — the console showed
// applyMatterFlatBuffer refusing entities across "3 live objects across
// Zones (Basic 2D Button Zone, Basic 2D Button Zone, Basic 2D Button
// Zone)". Traced to a real, structural bug, not a one-off stale save:
//
//   zoneIdFromJson (ZoneManager.cpp)   preferred "identifier", fell back to "name"
//   makeZoneFromJson (ZoneSerialization.cpp) preferred "name", fell back to "identifier"
//
// A Zone identity record whose two fields differ — exactly the shape the
// whole identifier/name split exists for — could never be recognized as
// already-live by admitFromJson's/hydrateFromZoneStore's own dedup check
// (which computes its "is this already here?" key via zoneIdFromJson),
// because the Zone actually constructed via makeZoneFromJson reported a
// DIFFERENT getIdentifier(). Every load minted another duplicate live Zone
// object sharing the same eventual identifier as one already there — this
// is what Invariant 3's ambiguity refusal was correctly surfacing.
//
// Confirmed concretely on disk: saves/zones/Basic2DButtonZone/zone.json has
// identifier "Basic2DButtonZone" but name "Basic 2D Button Zone" — the
// exact same class of divergence as the BasicPixelChanger/"Basic Pixel
// Changer" case Sol flagged for Invariant 6, just not yet repaired in that
// file (which stays untouched here — this test writes its own sandboxed
// identity records, no real save file is read or written).
//
// The fix: Zone gained a real, distinct _identifier field (previously
// getIdentifier() just returned _name — there was no split to have).
// zoneIdFromJson moved to ZoneSerialization.hpp/.cpp as the ONE shared
// resolution both makeZoneFromJson and every admission check must agree
// on — a second, independently-written copy is exactly how this drifted
// out of sync before.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <filesystem>
#include <fstream>
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
    std::cout << "Running Zone identifier/name split...\n";
    std::cout << "============================================================\n";

    // ---- Direct makeZoneFromJson checks, no disk involved. ----
    {
        nlohmann::json divergent;
        divergent["identifier"] = "Basic2DButtonZone";
        divergent["name"] = "Basic 2D Button Zone";
        auto zone = makeZoneFromJson(divergent);
        check(zone->getIdentifier() == "Basic2DButtonZone",
              "a record whose identifier differs from its name constructs with the IDENTIFIER as identity");
        check(zone->name() == "Basic 2D Button Zone",
              "...and keeps the record's own name as display, not the identifier");

        nlohmann::json selfConsistent;
        selfConsistent["identifier"] = "Basic 2D Button Zone";
        selfConsistent["name"] = "Basic 2D Button Zone";
        auto other = makeZoneFromJson(selfConsistent);
        check(other->getIdentifier() == "Basic 2D Button Zone",
              "a self-consistent record's identity is unaffected by the split");
        check(zone->getIdentifier() != other->getIdentifier(),
              "the two records — real disk shapes, per saves/zones/Basic2DButtonZone/ and "
              "saves/zones/Basic 2D Button Zone/ — resolve to DIFFERENT identities, not a collision");

        // zoneIdFromJson (the shared resolution admission/dedup checks use)
        // must agree with what makeZoneFromJson actually built — this is
        // the invariant that was broken.
        check(zoneIdFromJson(divergent) == zone->getIdentifier(),
              "zoneIdFromJson and makeZoneFromJson agree on identity for the divergent record");
        check(zoneIdFromJson(selfConsistent) == other->getIdentifier(),
              "zoneIdFromJson and makeZoneFromJson agree on identity for the self-consistent record");

        nlohmann::json roundTrip = zoneToJson(*zone);
        check(roundTrip.value("identifier", std::string{}) == "Basic2DButtonZone" &&
              roundTrip.value("name", std::string{}) == "Basic 2D Button Zone",
              "zoneToJson round-trips both fields distinctly");
    }

    // ---- Every pre-existing call shape (single string, no JSON) is unchanged. ----
    {
        auto plain = std::make_shared<Zone>("Ordinary Zone", "strict");
        check(plain->getIdentifier() == "Ordinary Zone" && plain->name() == "Ordinary Zone",
              "a Zone constructed the ordinary way still has identifier == name (no behavior change)");
    }

    // ---- The real shape: hydrateFromZoneStore() over two identity folders ----
    // ---- whose documents diverge, driven through the actual ZoneManager. ----
    {
        auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identifier_name_split";
        std::filesystem::remove_all(sandbox);
        std::filesystem::create_directories(sandbox);
        SaveSystem::setSaveRoot(sandbox.string());

        nlohmann::json divergentOnDisk;
        divergentOnDisk["identifier"] = "Basic2DButtonZone";
        divergentOnDisk["name"] = "Basic 2D Button Zone";
        SaveSystem::writeZoneIdentity("Basic2DButtonZone", divergentOnDisk);

        nlohmann::json selfConsistentOnDisk;
        selfConsistentOnDisk["identifier"] = "Basic 2D Button Zone";
        selfConsistentOnDisk["name"] = "Basic 2D Button Zone";
        SaveSystem::writeZoneIdentity("Basic 2D Button Zone", selfConsistentOnDisk);

        ZoneManager mgr;
        mgr.hydrateFromZoneStore();
        // A second call stands in for a later Load / reload in the same
        // session (EngineInit.cpp calls this once at boot; loadState calls
        // it again on every load) — must not mint more duplicates either.
        mgr.hydrateFromZoneStore();

        std::size_t basic2DButtonCount = 0;
        std::size_t basicSpacedCount = 0;
        for (const auto& z : mgr.zones()) {
            if (!z) continue;
            if (z->getIdentifier() == "Basic2DButtonZone") ++basic2DButtonCount;
            if (z->getIdentifier() == "Basic 2D Button Zone") ++basicSpacedCount;
        }
        check(basic2DButtonCount == 1,
              "exactly one live Zone for 'Basic2DButtonZone' after two hydration passes (was N via collision before the fix)");
        check(basicSpacedCount == 1,
              "exactly one live Zone for 'Basic 2D Button Zone' after two hydration passes");

        std::filesystem::remove_all(sandbox);
        SaveSystem::setSaveRoot("");
    }

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_identifier_name_split_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_identifier_name_split_test: ALL OK\n";
    return 0;
}
