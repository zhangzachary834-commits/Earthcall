// Regression witness for Zach's 2026-09-19 request: Go must be a Zone,
// not a world file a Person has to load through Assets first.
//
// The sandbox deliberately contains ONLY saves/zones/Go/zone.json,
// its .ecmatter physical sidecar, and saves/laws/<Go-law>/law.json.
// There is no worlds/ directory and this test never calls loadState().
// It boots, Moves to Go, proves the closure (materials, relations, laws,
// board manifestation), and executes placing Black and White stones through
// the authored Laws.

#include "support/test_harness.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "json.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

namespace {
int checks = 0;
int failures = 0;

void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}

Object* findObject(Zone& zone, const std::string& id) {
    for (const auto& object : zone.getOwnedObjects()) {
        if (object && object->getIdentifier() == id) return object.get();
    }
    return nullptr;
}

int asInt(Singular& being, const char* name, int fallback = -999) {
    PropertyValue value;
    if (!being.getDynamicProperty(name, value)) return fallback;
    if (const int* direct = std::get_if<int>(&value)) return *direct;
    double number = 0.0;
    return propertyValueToNumber(value, number) ? static_cast<int>(number) : fallback;
}

bool asBool(Singular& being, const char* name) {
    PropertyValue value;
    if (!being.getDynamicProperty(name, value)) return false;
    if (const bool* direct = std::get_if<bool>(&value)) return *direct;
    double number = 0.0;
    return propertyValueToNumber(value, number) && number != 0.0;
}

std::string asString(Singular& being, const char* name) {
    PropertyValue value;
    if (!being.getDynamicProperty(name, value)) return "";
    if (const std::string* direct = std::get_if<std::string>(&value)) return *direct;
    return "";
}

void click(Singularity::Input::InteractionChannel* interaction,
           LawManager& laws,
           Object* subject,
           const glm::vec3& world) {
    interaction->pointerWorld = world;
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", subject, nullptr, std::time(nullptr)});
    for (int i = 0; i < 5; ++i) {
        laws.tick();
    }
}

struct Scratch {
    std::filesystem::path path;
    ~Scratch() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
} // namespace

int main() {
    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/Go/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    saves = std::filesystem::absolute(saves);
    const auto sourceZone = saves / "zones/Go/zone.json";
    const auto sourceMatter = saves / "zones/Go/zone.ecmatter";

    check(std::filesystem::exists(sourceZone), "Zone-native Go identity exists");
    check(std::filesystem::exists(sourceMatter), "Zone-native Go physical matter (.ecmatter) exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson;
    {
        std::ifstream input(sourceZone);
        input >> zoneJson;
    }
    check(zoneJson.value("identifier", std::string{}) == "Go",
          "identity is exactly Go");
    check(zoneJson.contains("lawRefs") && zoneJson["lawRefs"].is_array() &&
              zoneJson["lawRefs"].size() == 3,
          "Go names all 3 authored Law roots");
    check(zoneJson.contains("materials") && zoneJson["materials"].is_array() &&
              zoneJson["materials"].size() == 5,
          "Go carries its 5 visual materials in the Zone identity");
    check(zoneJson.contains("matterGeneration") && zoneJson["matterGeneration"].is_object(),
          "Go identity links to its physical matterGeneration");

    const auto& authoredObjects = zoneJson["world"]["objects"];
    std::unordered_set<std::string> ids;
    for (const auto& objectJson : authoredObjects) {
        ids.insert(objectJson.value("objectID", std::string{}));
    }
    check(ids.count("object.go.board") == 1, "Zone owns the Goban board");
    check(ids.count("go_state") == 1, "Zone owns its extra-spatial game state");
    check(ids.count("grok-4.6") == 1, "Zone owns the recorded model-author referent");
    check(ids.count("category.go.intersection") == 1,
          "Zone owns the intersection-category endpoint needed during boot");
    check(ids.count("intersection_9_9") == 1, "Zone owns the Tengen intersection (9, 9)");

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_go_zone_native_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    const auto targetZoneDir = scratch.path / "zones/Go";
    std::filesystem::create_directories(targetZoneDir);
    std::filesystem::copy_file(sourceZone, targetZoneDir / "zone.json");
    if (std::filesystem::exists(sourceMatter)) {
        std::filesystem::copy_file(sourceMatter, targetZoneDir / "zone.ecmatter");
    }

    std::size_t copiedRoots = 0;
    for (const auto& refJson : zoneJson["lawRefs"]) {
        if (!refJson.is_string()) continue;
        const std::string ref = refJson.get<std::string>();
        const auto sourceLaw = saves / "laws" / ref / "law.json";
        const auto targetLawDir = scratch.path / "laws" / ref;
        check(std::filesystem::exists(sourceLaw), "shared Law root exists: " + ref);
        if (!std::filesystem::exists(sourceLaw)) continue;
        std::filesystem::create_directories(targetLawDir);
        std::filesystem::copy_file(sourceLaw, targetLawDir / "law.json");
        ++copiedRoots;
    }
    check(copiedRoots == 3, "all Go Law roots copied into isolated SaveRoot");
    check(!std::filesystem::exists(scratch.path / "worlds"),
          "isolated boot contains no legacy worlds directory");
    if (copiedRoots != 3) return 1;

    SaveSystem::setSaveRoot(scratch.path.string());
    {
        TestSupport::BootedEngineHarness harness;

        std::size_t goIndex = harness.zones.zones().size();
        for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
            const auto& zone = harness.zones.zones()[i];
            if (zone && zone->getIdentifier() == "Go") {
                goIndex = i;
                break;
            }
        }
        check(goIndex < harness.zones.zones().size(),
              "fresh boot discovers Go from saves/zones alone");
        if (goIndex >= harness.zones.zones().size()) return 1;

        check(harness.zones.switchTo(goIndex),
              "Move to Zone commits Go without loadState()");
        if (harness.zones.currentIndex() != goIndex) return 1;

        auto active = harness.zones.zones()[goIndex];
        check(harness.lawManager.find("law-go-click") != nullptr,
              "Go click Law loaded from its shared root");
        check(harness.lawManager.find("law-go-place-black") != nullptr,
              "Go place-black Law loaded from its shared root");
        check(harness.lawManager.find("law-go-place-white") != nullptr,
              "Go place-white Law loaded from its shared root");

        std::size_t loadedRoots = 0;
        for (const auto& refJson : zoneJson["lawRefs"]) {
            if (refJson.is_string() &&
                harness.lawManager.find(refJson.get<std::string>())) ++loadedRoots;
        }
        check(loadedRoots == 3, "all 3 Zone-scoped Go Laws are live");

        auto boardMaterial = materials.get("material.go.board");
        check(boardMaterial != nullptr, "board material hydrated from Zone identity");
        if (boardMaterial) {
            check(boardMaterial->faceTextures.size() == 6,
                  "board material keeps authored FaceTextures");
        }

        bool intersectionMembership = false;
        for (const auto& relation : active->formation().relations().getAll()) {
            if (!relation) continue;
            if (relation->type == "instance-of" &&
                relation->aId() == "intersection_9_9" &&
                relation->bId() == "category.go.intersection") {
                intersectionMembership = true;
                break;
            }
        }
        check(intersectionMembership, "intersection -> category relation bound during Zone-only boot");

        Object* board = findObject(*active, "object.go.board");
        Object* tengen = findObject(*active, "intersection_9_9");
        Object* state = findObject(*active, "go_state");
        check(board != nullptr, "board is present after Move to Zone");
        check(tengen != nullptr, "Tengen intersection (9, 9) is present after Move to Zone");
        check(state != nullptr, "Go state being is present after Move to Zone");
        if (!board || !tengen || !state || !harness.interaction) return 1;

        // Board manifestation check
        const glm::mat4& boardTransform = board->getTransform();
        check(std::fabs(boardTransform[0][0] - 10.0f) < 1e-3f &&
                  std::fabs(boardTransform[1][1] - 0.5f) < 1e-3f &&
                  std::fabs(boardTransform[2][2] - 10.0f) < 1e-3f &&
                  std::fabs(board->getPosition().y + 0.25f) < 1e-3f,
              "board manifests as the authored 10 x 0.5 x 10 prism at y=-0.25");

        // 361 intersections check
        std::size_t ixCount = 0;
        for (const auto& object : active->getOwnedObjects()) {
            if (object && object->getIdentifier().rfind("intersection_", 0) == 0) {
                ++ixCount;
            }
        }
        check(ixCount == 361, "all 361 intersections are present");

        // Check initial state
        check(asBool(*tengen, "is_empty"), "Tengen starts empty");
        check(asString(*state, "current_turn") == "black", "initial turn is black");

        // Gameplay interaction:
        // 1. Black places a stone at Tengen (9, 9) -> world (0.0, 0.005, 0.0)
        Universe::instance().setClock(0.0, 1.0 / 60.0);
        click(harness.interaction, harness.lawManager, tengen,
              glm::vec3(0.0f, 0.005f, 0.0f));

        check(!asBool(*tengen, "is_empty"), "Tengen is no longer empty after click");
        check(asString(*tengen, "stone_color") == "black", "Tengen has black stone");
        check(asString(*state, "current_turn") == "white", "turn advanced to white");

        // 2. White places a stone at (10, 10) -> world (0.5, 0.005, 0.5)
        Object* ix10_10 = findObject(*active, "intersection_10_10");
        check(ix10_10 != nullptr, "intersection (10, 10) exists");
        if (ix10_10) {
            click(harness.interaction, harness.lawManager, ix10_10,
                  glm::vec3(0.5f, 0.005f, 0.5f));

            check(!asBool(*tengen, "is_empty"), "intersection (10, 10) is no longer empty");
            check(asString(*ix10_10, "stone_color") == "white", "intersection (10, 10) has white stone");
            check(asString(*state, "current_turn") == "black", "turn advanced back to black");
        }
    }

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
