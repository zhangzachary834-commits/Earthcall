// Invariant 6, storage boundary (Sol, agent intercom "Basic Pixel Changer
// Zone Identity Bug 9-7-26", 2026-09-09, Stage 1): "Change Zone/Home
// identity enumeration so the directory key remains explicit all the way
// through read/admission; do not return a document identifier and then use
// it to reconstruct a possibly different path... For a mismatch, emit one
// structured, actionable refusal naming path, directory key, document
// identifier, and referrers. Do not rename folders, rewrite JSON, merge
// Zones, or silently choose either side... Add the real BasicPixelChanger /
// Basic Pixel Changer shape as a temp-root fixture, not by touching Zach's
// file. Acceptance criterion: mismatch produces zero writes and no phantom
// live Zone; matching identity still hydrates."
//
// This test reproduces that exact real shape (folder "BasicPixelChanger",
// document identifier "Basic Pixel Changer") entirely inside a sandboxed
// save root — the real saves/zones/BasicPixelChanger/zone.json is never
// read or written here (it has separately been repaired with Zach's
// authorization; this test exists so the CLASS of bug stays caught even if
// a future save drifts the same way again).

#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <filesystem>
#include <iostream>
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

bool hasLiveZone(ZoneManager& mgr, const std::string& identifier) {
    for (const auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == identifier) return true;
    }
    return false;
}

std::size_t liveZoneCount(ZoneManager& mgr) { return mgr.zones().size(); }

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Zone identity boundary (Invariant 6, Stage 1: directory key vs. document identity)...\n";
    std::cout << "============================================================\n";

    // ---- Case 1: the real BasicPixelChanger/"Basic Pixel Changer" shape. ----
    // ---- Mismatch refuses: zero writes, no phantom live Zone.           ----
    {
        auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identity_boundary_mismatch";
        std::filesystem::remove_all(sandbox);
        std::filesystem::create_directories(sandbox);
        SaveSystem::setSaveRoot(sandbox.string());

        nlohmann::json mismatched;
        mismatched["identifier"] = "Basic Pixel Changer"; // space — the real pre-repair shape
        mismatched["name"] = "Basic Pixel Changer";
        SaveSystem::writeZoneIdentity("BasicPixelChanger", mismatched); // folder key — no space

        ZoneManager mgr;
        mgr.hydrateFromZoneStore();

        check(!hasLiveZone(mgr, "BasicPixelChanger"),
              "a folder/identifier mismatch does not admit under the directory key");
        check(!hasLiveZone(mgr, "Basic Pixel Changer"),
              "...nor under the document's own (mismatched) identifier — no phantom live Zone either way");
        check(liveZoneCount(mgr) == 0, "zero live Zones — nothing was guessed at");

        // "Zero writes": hydrating must not have touched the identity store.
        const auto stillOnDisk = SaveSystem::readZoneIdentity("BasicPixelChanger");
        check(stillOnDisk.is_object() &&
              stillOnDisk.value("identifier", std::string{}) == "Basic Pixel Changer",
              "the on-disk record is completely untouched — no auto-repair, no rename, no rewrite");

        // A second hydration pass must refuse identically, not accumulate
        // a duplicate warning path or somehow admit on retry.
        mgr.hydrateFromZoneStore();
        check(liveZoneCount(mgr) == 0, "a repeated hydration pass still admits nothing for the mismatched entry");

        std::filesystem::remove_all(sandbox);
        SaveSystem::setSaveRoot("");
    }

    // ---- Case 2: matching identity still hydrates (control case). ----
    {
        auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identity_boundary_matching";
        std::filesystem::remove_all(sandbox);
        std::filesystem::create_directories(sandbox);
        SaveSystem::setSaveRoot(sandbox.string());

        nlohmann::json matching;
        matching["identifier"] = "BasicPixelChanger";
        matching["name"] = "Basic Pixel Changer"; // name may still differ — that's the OTHER split, not this one
        SaveSystem::writeZoneIdentity("BasicPixelChanger", matching);

        ZoneManager mgr;
        mgr.hydrateFromZoneStore();
        check(hasLiveZone(mgr, "BasicPixelChanger"),
              "a folder key that matches the document's own identifier hydrates normally");
        check(liveZoneCount(mgr) == 1, "exactly one live Zone for the matching entry");

        std::filesystem::remove_all(sandbox);
        SaveSystem::setSaveRoot("");
    }

    // ---- Case 3: two different directories both claim the same stable  ----
    // ---- identity. Refuse BOTH — no last-record-wins, no arbitrary     ----
    // ---- "first by directory order" pick. Directory keys are unique on ----
    // ---- a real filesystem, so two SEPARATE, each internally           ----
    // ---- self-consistent (directoryKey == identity) entries can only   ----
    // ---- collide across the two identity stores that share one        ----
    // ---- namespace — Homes (saves/homes/) and Zones (saves/zones/).    ----
    {
        auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identity_boundary_duplicate";
        std::filesystem::remove_all(sandbox);
        std::filesystem::create_directories(sandbox);
        SaveSystem::setSaveRoot(sandbox.string());

        nlohmann::json asHome;
        asHome["identifier"] = "SharedIdentity";
        asHome["name"] = "Shared, as a Home";
        SaveSystem::writeHomeIdentity("SharedIdentity", asHome);

        nlohmann::json asZone;
        asZone["identifier"] = "SharedIdentity";
        asZone["name"] = "Shared, as a Zone";
        SaveSystem::writeZoneIdentity("SharedIdentity", asZone);

        ZoneManager mgr;
        mgr.hydrateFromZoneStore();

        check(!hasLiveZone(mgr, "SharedIdentity"),
              "an identity claimed by both a Home entry and a Zone entry admits neither — "
              "Homes and Zones share one identity namespace");
        check(liveZoneCount(mgr) == 0, "zero live Zones for the duplicate-claimed identity");

        std::filesystem::remove_all(sandbox);
        SaveSystem::setSaveRoot("");
    }

    // ---- Case 4: a document naming no identifier or name at all. ----
    // ---- Refuses gracefully, no crash.                            ----
    {
        auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identity_boundary_empty";
        std::filesystem::remove_all(sandbox);
        std::filesystem::create_directories(sandbox);
        SaveSystem::setSaveRoot(sandbox.string());

        nlohmann::json empty = nlohmann::json::object();
        SaveSystem::writeZoneIdentity("NamelessFolder", empty);

        ZoneManager mgr;
        mgr.hydrateFromZoneStore();
        check(liveZoneCount(mgr) == 0, "a document with no identifier/name at all admits nothing, does not crash");

        std::filesystem::remove_all(sandbox);
        SaveSystem::setSaveRoot("");
    }

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_identity_boundary_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_identity_boundary_test: ALL OK\n";
    return 0;
}
