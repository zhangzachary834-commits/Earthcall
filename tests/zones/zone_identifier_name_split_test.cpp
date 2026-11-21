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
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/MaterialPersistenceMigration.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>


extern MaterialManager materials;

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

    // ---- Gate B: old Zone default is scoped, global default stays current. ----
    {
        // Reset the global registry to the current canonical matte baseline.
        materials.loadFromJson(nlohmann::json::array());
        auto canonicalBefore = materials.defaultMaterial();
        check(canonicalBefore && canonicalBefore->responseExpr,
              "Gate B test begins with canonical current material.default");

        nlohmann::json legacyZone{
            {"identifier", "Legacy Default Zone"},
            {"name", "Legacy Default Zone"},
            {"materials", nlohmann::json::array({
                nlohmann::json{
                    {"name", "default"},
                    {"ambient", 0.2f},
                    {"diffuse", 0.8f},
                    {"specular", 1.0f},
                    {"shininess", 32.0f}
                }
            })},
            {"world", {
                {"objects", nlohmann::json::array({
                    nlohmann::json{
                        {"objectID", "legacy-default-cube"},
                        {"shapeKind", 0},
                        {"materialId", "material.default"}
                    }
                })}
            }}
        };

        auto legacy = makeZoneFromJson(legacyZone);
        check(legacy && legacy->getOwnedObjects().size() == 1,
              "old Zone identity hydrates its Object through the real Zone path");

        const std::string expectedScoped =
            MaterialPersistenceMigration::scopedLegacyDefaultId(
                "zone:Legacy Default Zone");
        auto legacyObject = legacy->getOwnedObjects().front();
        check(legacyObject &&
                  legacyObject->materialId() == expectedScoped,
              "old Zone Object material.default reference is rewritten to scoped compatibility identity");

        auto scopedLegacy = materials.get(expectedScoped);
        check(scopedLegacy && scopedLegacy->responseExpr,
              "scoped old default enters the live MaterialManager with explicit migrated response");

        std::set<std::string> scopedBindings;
        if (scopedLegacy && scopedLegacy->responseExpr) {
            for (const auto& piece : scopedLegacy->responseExpr->pieces) {
                if (piece.mathNode)
                    piece.mathNode->collectPropertyBindings(scopedBindings);
                if (piece.whereLEZero)
                    piece.whereLEZero->collectPropertyBindings(scopedBindings);
            }
        }
        check(scopedBindings.count("specular") == 1,
              "scoped old Zone default preserves historical specular compatibility law");

        auto canonicalAfter = materials.defaultMaterial();
        std::set<std::string> canonicalBindings;
        if (canonicalAfter && canonicalAfter->responseExpr) {
            for (const auto& piece : canonicalAfter->responseExpr->pieces) {
                if (piece.mathNode)
                    piece.mathNode->collectPropertyBindings(canonicalBindings);
            }
        }
        check(canonicalAfter &&
                  canonicalBindings.empty(),
              "hydrating old Zone does not poison canonical current material.default");

        nlohmann::json persisted = zoneToJson(*legacy);
        check(persisted["world"]["objects"][0]["materialId"] == expectedScoped,
              "first re-save persists scoped material identity so migration is one-way");
        bool persistedScopedMaterial = false;
        if (persisted.contains("materials") &&
            persisted["materials"].is_array()) {
            for (const auto& material : persisted["materials"]) {
                if (MaterialPersistenceMigration::materialIdentity(material) ==
                    expectedScoped) {
                    persistedScopedMaterial =
                        material.contains("responseExpr");
                    break;
                }
            }
        }
        check(persistedScopedMaterial,
              "first re-save persists explicit legacy AST under scoped identity");

        // Restore clean current default for the rest of this independent test.
        materials.loadFromJson(nlohmann::json::array());
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
