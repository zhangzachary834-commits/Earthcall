// Regression witness for Zach's 2026-09-18 request: Chess must be a Zone,
// not a world file a Person has to load through Assets first.
//
// The sandbox deliberately contains ONLY saves/zones/Chess/zone.json and
// saves/laws/<Chess-law>/law.json. There is no worlds/ directory and this
// test never calls loadState(). It boots, Moves to Chess, proves the closure,
// and executes e2-e4 through the authored Laws.

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

void click(Singularity::Input::InteractionChannel* interaction,
           LawManager& laws,
           Object* subject,
           const glm::vec3& world) {
    interaction->pointerWorld = world;
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", subject, nullptr, std::time(nullptr)});
    laws.tick();
}

struct Scratch {
    std::filesystem::path path;
    ~Scratch() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
}

int main() {
    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/Chess/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    saves = std::filesystem::absolute(saves);
    const auto sourceZone = saves / "zones/Chess/zone.json";

    check(std::filesystem::exists(sourceZone), "Zone-native Chess identity exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson;
    {
        std::ifstream input(sourceZone);
        input >> zoneJson;
    }
    check(zoneJson.value("identifier", std::string{}) == "Chess",
          "identity is exactly Chess");
    check(zoneJson.contains("lawRefs") && zoneJson["lawRefs"].is_array() &&
              zoneJson["lawRefs"].size() == 69,
          "Chess names all 69 authored Law roots");
    check(zoneJson.contains("materials") && zoneJson["materials"].is_array() &&
              zoneJson["materials"].size() == 3,
          "Chess carries its three visual materials in the Zone identity");

    const auto& authoredObjects = zoneJson["world"]["objects"];
    std::unordered_set<std::string> ids;
    for (const auto& objectJson : authoredObjects) {
        ids.insert(objectJson.value("objectID", std::string{}));
    }
    check(ids.count("object.chess.board") == 1, "Zone owns the board");
    check(ids.count("state.chess") == 1, "Zone owns its extra-spatial game state");
    check(ids.count("grok-4.6") == 1, "Zone owns the recorded model-author referent");
    check(ids.count("category.chess.piece") == 1,
          "Zone owns the piece-category endpoint needed during boot");

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_chess_zone_native_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    const auto targetZoneDir = scratch.path / "zones/Chess";
    std::filesystem::create_directories(targetZoneDir);
    std::filesystem::copy_file(sourceZone, targetZoneDir / "zone.json");

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
    check(copiedRoots == 69, "all Chess Law roots copied into isolated SaveRoot");
    check(!std::filesystem::exists(scratch.path / "worlds"),
          "isolated boot contains no legacy worlds directory");
    if (copiedRoots != 69) return 1;

    SaveSystem::setSaveRoot(scratch.path.string());
    {
        TestSupport::BootedEngineHarness harness;

        std::size_t chessIndex = harness.zones.zones().size();
        for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
            const auto& zone = harness.zones.zones()[i];
            if (zone && zone->getIdentifier() == "Chess") {
                chessIndex = i;
                break;
            }
        }
        check(chessIndex < harness.zones.zones().size(),
              "fresh boot discovers Chess from saves/zones alone");
        if (chessIndex >= harness.zones.zones().size()) return 1;

        check(harness.zones.switchTo(chessIndex),
              "Move to Zone commits Chess without loadState()");
        if (harness.zones.currentIndex() != chessIndex) return 1;

        auto active = harness.zones.zones()[chessIndex];
        check(harness.lawManager.find("law-chess-click") != nullptr,
              "Chess click Law loaded from its shared root");
        check(harness.lawManager.find("law-chess-pawn-w-double") != nullptr,
              "Chess pawn movement Law loaded from its shared root");

        std::size_t loadedRoots = 0;
        for (const auto& refJson : zoneJson["lawRefs"]) {
            if (refJson.is_string() &&
                harness.lawManager.find(refJson.get<std::string>())) ++loadedRoots;
        }
        check(loadedRoots == 69, "all 69 Zone-scoped Chess Laws are live");

        auto boardMaterial = materials.get("material.chess.board");
        check(boardMaterial != nullptr, "checkerboard material hydrated from Zone identity");
        if (boardMaterial) {
            check(boardMaterial->faceTextures.size() >= 3,
                  "board material keeps authored FaceTextures");
        }

        bool pieceMembership = false;
        std::unordered_set<std::string> categorizedLaws;
        for (const auto& relation : active->formation().relations().getAll()) {
            if (!relation) continue;
            if (relation->type == "instance-of" &&
                relation->aId() == "piece-white-pawn-4-1" &&
                relation->bId() == "category.chess.piece") {
                pieceMembership = true;
            }
            if (relation->type == "instance-of" &&
                relation->aId().rfind("law-chess-", 0) == 0 &&
                relation->bId().rfind("category.chess.law", 0) == 0) {
                categorizedLaws.insert(relation->aId());
            }
        }
        check(pieceMembership, "piece -> category relation bound during Zone-only boot");
        check(categorizedLaws.size() == 69,
              "deferred Law-category relations bind after Zone Law activation");

        Object* board = findObject(*active, "object.chess.board");
        Object* pawn = findObject(*active, "piece-white-pawn-4-1");
        Object* state = findObject(*active, "state.chess");
        check(board != nullptr, "board is present after Move to Zone");
        check(pawn != nullptr, "e2 pawn is present after Move to Zone");
        check(state != nullptr, "Chess state being is present after Move to Zone");
        if (!board || !pawn || !state || !harness.interaction) return 1;

        Universe::instance().setClock(0.0, 1.0 / 60.0);
        click(harness.interaction, harness.lawManager, pawn,
              glm::vec3(0.5f, 0.3f, -2.5f));
        check(asBool(*pawn, "isSelected"), "e2 pawn selects through Zone-loaded Laws");

        click(harness.interaction, harness.lawManager, board,
              glm::vec3(0.5f, 0.0f, -0.5f));
        check(asInt(*pawn, "gridX") == 4 && asInt(*pawn, "gridY") == 3,
              "Zone-only Chess executes e2-e4");
        check(asInt(*state, "turn") == 1, "e2-e4 advances authored state to black");
        check(std::fabs(pawn->getPosition().z + 0.5f) < 1e-3f,
              "moved pawn manifests at e4's authored spatial position");
    }

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
