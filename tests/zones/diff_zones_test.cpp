#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running diff_zones_test...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_diff_zones_test";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    {
        ZoneManager mgr;

        // In-Memory vs In-Memory
        auto zoneA = std::make_shared<Zone>("ZoneA", "strict");
        auto obj1 = std::make_shared<Object>(); obj1->setObjectID("obj-shared"); zoneA->addObject(obj1);
        auto obj2 = std::make_shared<Object>(); obj2->setObjectID("obj-only-a"); zoneA->addObject(obj2);
        mgr.addZone(zoneA);

        auto zoneB = std::make_shared<Zone>("ZoneB", "strict");
        auto obj3 = std::make_shared<Object>(); obj3->setObjectID("obj-shared"); zoneB->addObject(obj3);
        auto obj4 = std::make_shared<Object>(); obj4->setObjectID("obj-only-b"); zoneB->addObject(obj4);
        mgr.addZone(zoneB);

        auto diff1 = mgr.diffZones("ZoneA", "ZoneB");
        check(diff1["a"] == "ZoneA", "diff contains correct a");
        check(diff1["b"] == "ZoneB", "diff contains correct b");
        check(diff1["shared"].size() == 1 && diff1["shared"][0] == "obj-shared", "diff shared correct in memory");
        check(diff1["onlyInA"].size() == 1 && diff1["onlyInA"][0] == "obj-only-a", "diff onlyInA correct in memory");
        check(diff1["onlyInB"].size() == 1 && diff1["onlyInB"][0] == "obj-only-b", "diff onlyInB correct in memory");

        // On-Disk vs On-Disk
        nlohmann::json diskA = {
            {"objects", {
                {{"objectID", "disk-shared"}},
                {{"objectID", "disk-only-a"}}
            }}
        };
        SaveSystem::writeZoneIdentity("DiskZoneA", diskA);

        nlohmann::json diskB = {
            {"world", {
                {"objects", {
                    {{"identifier", "disk-shared"}},
                    {{"identifier", "disk-only-b"}}
                }}
            }}
        };
        SaveSystem::writeZoneIdentity("DiskZoneB", diskB);

        auto diff2 = mgr.diffZones("DiskZoneA", "DiskZoneB");
        check(diff2["shared"].size() == 1 && diff2["shared"][0] == "disk-shared", "diff shared correct on disk");
        check(diff2["onlyInA"].size() == 1 && diff2["onlyInA"][0] == "disk-only-a", "diff onlyInA correct on disk");
        check(diff2["onlyInB"].size() == 1 && diff2["onlyInB"][0] == "disk-only-b", "diff onlyInB correct on disk");

        // In-Memory vs On-Disk
        auto diff3 = mgr.diffZones("ZoneA", "DiskZoneA");
        check(diff3["shared"].empty(), "no shared items between ZoneA and DiskZoneA");
        check(diff3["onlyInA"].size() == 2, "ZoneA has 2 items onlyInA");
        check(diff3["onlyInB"].size() == 2, "DiskZoneA has 2 items onlyInB");

        // Edge Cases
        auto diff4 = mgr.diffZones("NonExistentA", "NonExistentB");
        check(diff4["shared"].empty(), "NonExistent zones yield empty shared");
        check(diff4["onlyInA"].empty(), "NonExistent zones yield empty onlyInA");
        check(diff4["onlyInB"].empty(), "NonExistent zones yield empty onlyInB");
        check(diff4["a"] == "NonExistentA", "NonExistent diff contains correct a");
        check(diff4["b"] == "NonExistentB", "NonExistent diff contains correct b");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "diff_zones_test: FAILED\n";
        return 1;
    }
    std::cout << "diff_zones_test: ALL OK\n";
    return 0;
}
