#include "Singularity/Storage/SaveSystem.hpp"
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

void click2D(Singularity::Input::InteractionChannel* interaction,
             LawManager& laws,
             Object* subject) {
    (void)interaction;
    if (!subject) return;
    // In 2D, the click event passes subject and triggers object-clicked
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
}

int main() {
    std::cout << "Running chess2d_zone_native_test...\n";

    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/Chess2D/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    saves = std::filesystem::absolute(saves);
    const auto sourceZone = saves / "zones/Chess2D/zone.json";

    check(std::filesystem::exists(sourceZone), "Zone-native Chess2D identity exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson = SaveSystem::readSaveData(sourceZone.string());
    check(zoneJson.value("identifier", std::string{}) == "Chess2D",
          "identity is exactly Chess2D");
    check(zoneJson.contains("lawRefs") && zoneJson["lawRefs"].is_array() &&
              zoneJson["lawRefs"].size() == 19,
          "Chess2D names all 19 authored Law roots");
    check(zoneJson.contains("materials") && zoneJson["materials"].is_array() &&
              zoneJson["materials"].size() == 6,
          "Chess2D carries 6 visual materials in Zone identity");

    // Copy to isolated Scratch save root to verify complete self-contained boot
    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_chess2d_zone_native_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    const auto targetZoneDir = scratch.path / "zones/Chess2D";
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
    check(copiedRoots == 19, "all 19 Chess2D Law roots copied into isolated SaveRoot");

    SaveSystem::setSaveRoot(scratch.path.string());
    {
        TestSupport::BootedEngineHarness harness;

        std::size_t chess2dIndex = harness.zones.zones().size();
        for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
            const auto& zone = harness.zones.zones()[i];
            if (zone && zone->getIdentifier() == "Chess2D") {
                chess2dIndex = i;
                break;
            }
        }
        check(chess2dIndex < harness.zones.zones().size(),
              "fresh boot discovers Chess2D from saves/zones alone");
        if (chess2dIndex >= harness.zones.zones().size()) return 1;

        check(harness.zones.switchTo(chess2dIndex),
              "Move to Zone commits Chess2D");
        if (harness.zones.currentIndex() != chess2dIndex) return 1;

        auto active = harness.zones.zones()[chess2dIndex];

        // 1. Verify Laws live
        check(harness.lawManager.find("law-chess2d-click") != nullptr,
              "law-chess2d-click loaded and live");
        check(harness.lawManager.find("law-chess2d-select") != nullptr,
              "law-chess2d-select loaded and live");
        check(harness.lawManager.find("law-chess2d-pawn-w-step") != nullptr,
              "law-chess2d-pawn-w-step loaded and live");
        check(harness.lawManager.find("law-chess2d-capture") != nullptr,
              "law-chess2d-capture loaded and live");

        std::size_t liveLaws = 0;
        for (const auto& refJson : zoneJson["lawRefs"]) {
            if (refJson.is_string() && harness.lawManager.find(refJson.get<std::string>())) {
                ++liveLaws;
            }
        }
        check(liveLaws == 19, "all 19 Zone-scoped Chess2D Laws are active");

        // 2. Verify Materials hydrated
        check(materials.get("material.chess2d.frame") != nullptr, "frame material resolved");
        check(materials.get("material.chess2d.light_square") != nullptr, "light square material resolved");
        check(materials.get("material.chess2d.dark_square") != nullptr, "dark square material resolved");
        check(materials.get("material.chess2d.piece_white") != nullptr, "white piece material resolved");
        check(materials.get("material.chess2d.piece_black") != nullptr, "black piece material resolved");
        check(materials.get("material.chess2d.tray") != nullptr, "tray material resolved");

        // 3. Verify State and UI beings
        Object* state = findObject(*active, "state.chess2d");
        Object* frame = findObject(*active, "object.chess2d.frame");
        Object* title = findObject(*active, "ui.chess2d.title");
        Object* turnBanner = findObject(*active, "ui.chess2d.turn");
        Object* trayW = findObject(*active, "object.chess2d.tray.white");
        Object* trayB = findObject(*active, "object.chess2d.tray.black");

        check(state != nullptr, "state.chess2d is present");
        check(frame != nullptr && frame->is2D(), "board frame is present and is2D() == true");
        check(title != nullptr && title->is2D(), "title banner is present and is2D() == true");
        check(turnBanner != nullptr && turnBanner->is2D(), "turn banner is present and is2D() == true");
        check(trayW != nullptr && trayW->is2D(), "white tray is present and is2D() == true");
        check(trayB != nullptr && trayB->is2D(), "black tray is present and is2D() == true");

        // 4. Verify 64 Squares
        std::size_t squareCount = 0;
        for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                std::string sqId = "square-" + std::to_string(x) + "-" + std::to_string(y);
                Object* sq = findObject(*active, sqId);
                if (sq && sq->is2D() && asInt(*sq, "gridX") == x && asInt(*sq, "gridY") == y) {
                    ++squareCount;
                }
            }
        }
        check(squareCount == 64, "all 64 board squares manifest with Shape2D and valid grid coordinates");

        // 5. Verify 32 Pieces
        std::size_t pieceCount = 0;
        for (const auto& obj : active->getOwnedObjects()) {
            if (obj && obj->getIdentifier().rfind("piece-", 0) == 0) {
                if (obj->is2D() && asBool(*obj, "onBoard")) {
                    ++pieceCount;
                }
            }
        }
        check(pieceCount == 32, "all 32 chess pieces manifest with Shape2D and onBoard == true");

        // Specific pieces inspect
        Object* pawnE2 = findObject(*active, "piece-white-pawn-4-1");
        Object* squareE4 = findObject(*active, "square-4-3");
        check(pawnE2 != nullptr && pawnE2->is2D(), "e2 white pawn manifests as 2D object");
        check(squareE4 != nullptr && squareE4->is2D(), "e4 square manifests as 2D object");
        check(asString(*pawnE2, "label2D") == "P", "pawn carries 'P' label glyph");
        check(asInt(*state, "turn") == 0, "initial turn is White (0)");

        // Initial manifestation coordinates check:
        // x2D for x=4: 140 + 4*60 + 6 = 386.0
        // y2D for y=1: 100 + (7-1)*60 + 6 = 466.0
        check(std::fabs(pawnE2->getX2D() - 386.0f) < 1.0f, "e2 pawn has correct initial x2D (386)");
        check(std::fabs(pawnE2->getY2D() - 466.0f) < 1.0f, "e2 pawn has correct initial y2D (466)");

        // -------------------------------------------------------------------
        // MOVE 1: White e2-e4
        // -------------------------------------------------------------------
        click2D(harness.interaction, harness.lawManager, pawnE2);
        check(asBool(*pawnE2, "isSelected"), "e2 pawn is selected on click");
        check(asInt(*state, "selectedX") == 4 && asInt(*state, "selectedY") == 1,
              "state recorded selected coords (4, 1)");
        check(pawnE2->getZOrder2D() == 20, "selected piece zOrder2D lifted to 20");

        click2D(harness.interaction, harness.lawManager, squareE4);
        check(asInt(*pawnE2, "gridX") == 4 && asInt(*pawnE2, "gridY") == 3,
              "e2 pawn moved logically to e4 (4, 3)");
        check(!asBool(*pawnE2, "isSelected"), "pawn is no longer selected after move");
        check(pawnE2->getZOrder2D() == 10, "pawn zOrder2D restored to 10");
        check(asBool(*pawnE2, "hasMoved"), "pawn hasMoved marked true");
        check(asInt(*state, "turn") == 1, "turn advanced to Black (1)");

        // Manifestation coordinates at e4:
        // x2D for x=4: 386.0
        // y2D for y=3: 100 + (7-3)*60 + 6 = 346.0
        check(std::fabs(pawnE2->getX2D() - 386.0f) < 1.0f, "pawn manifested at e4 x2D (386)");
        check(std::fabs(pawnE2->getY2D() - 346.0f) < 1.0f, "pawn manifested at e4 y2D (346)");

        // -------------------------------------------------------------------
        // MOVE 2: Black e7-e5
        // -------------------------------------------------------------------
        std::cout << "\n--- Testing Move 2: Black e7-e5 ---\n";
        Object* pawnE7 = findObject(*active, "piece-black-pawn-4-6");
        Object* squareE5 = findObject(*active, "square-4-4");
        check(pawnE7 != nullptr, "black e7 pawn found");
        check(squareE5 != nullptr, "e5 square found");

        click2D(harness.interaction, harness.lawManager, pawnE7);
        check(asBool(*pawnE7, "isSelected"), "black e7 pawn is selected");

        click2D(harness.interaction, harness.lawManager, squareE5);
        check(asInt(*pawnE7, "gridX") == 4 && asInt(*pawnE7, "gridY") == 4,
              "black e7 pawn moved to e5 (4, 4)");
        check(asInt(*state, "turn") == 0, "turn flipped back to White (0)");

        // -------------------------------------------------------------------
        // MOVE 3: White d2-d4
        // -------------------------------------------------------------------
        std::cout << "\n--- Testing Move 3: White d2-d4 ---\n";
        Object* pawnD2 = findObject(*active, "piece-white-pawn-3-1");
        Object* squareD4 = findObject(*active, "square-3-3");
        check(pawnD2 != nullptr && squareD4 != nullptr, "white d2 pawn and d4 square exist");

        click2D(harness.interaction, harness.lawManager, pawnD2);
        check(asBool(*pawnD2, "isSelected"), "white d2 pawn selected");

        click2D(harness.interaction, harness.lawManager, squareD4);
        check(asInt(*pawnD2, "gridX") == 3 && asInt(*pawnD2, "gridY") == 3,
              "white d2 pawn moved to d4 (3, 3)");
        check(asInt(*state, "turn") == 1, "turn advanced to Black (1)");

        // -------------------------------------------------------------------
        // MOVE 4: Black captures White on d4 (exd4)
        // -------------------------------------------------------------------
        std::cout << "\n--- Testing Move 4: Black captures White on d4 (exd4) ---\n";
        // Click black e5 pawn
        click2D(harness.interaction, harness.lawManager, pawnE7);
        check(asBool(*pawnE7, "isSelected"), "black e5 pawn selected");

        // Click white d4 pawn (enemy victim!)
        click2D(harness.interaction, harness.lawManager, pawnD2);
        check(asInt(*pawnE7, "gridX") == 3 && asInt(*pawnE7, "gridY") == 3,
              "black pawn captured onto d4 (3, 3)");
        check(!asBool(*pawnD2, "onBoard"), "captured white pawn marked onBoard == false");
        check(asInt(*pawnD2, "gridX") == -1 && asInt(*pawnD2, "gridY") == -1,
              "captured white pawn removed from grid (-1, -1)");
        check(pawnD2->getX2D() == 70.0f, "captured white pawn moved to side capture tray (x2D=70)");
        check(asInt(*state, "turn") == 0, "turn flipped to White (0)");

        // -------------------------------------------------------------------
        // Selection Switching & Deselection
        // -------------------------------------------------------------------
        std::cout << "\n--- Testing Selection Switching & Self-Deselect ---\n";
        Object* knightB1 = findObject(*active, "piece-white-knight-1-0");
        Object* bishopC1 = findObject(*active, "piece-white-bishop-2-0");
        check(knightB1 != nullptr && bishopC1 != nullptr, "knight and bishop present");

        click2D(harness.interaction, harness.lawManager, knightB1);
        check(asBool(*knightB1, "isSelected"), "knight b1 selected");

        click2D(harness.interaction, harness.lawManager, bishopC1);
        check(asBool(*bishopC1, "isSelected"), "bishop c1 selected after clicking friendly piece");
        check(!asBool(*knightB1, "isSelected"), "knight b1 deselected automatically");

        // Click bishop again to deselect self
        click2D(harness.interaction, harness.lawManager, bishopC1);
        check(!asBool(*bishopC1, "isSelected"), "bishop c1 deselected on second click");
        check(!asBool(*state, "selectionActive"), "selectionActive cleared");

        // -------------------------------------------------------------------
        // Illegal Move Refusal
        // -------------------------------------------------------------------
        std::cout << "\n--- Testing Illegal Move Refusal ---\n";
        Object* pawnA2 = findObject(*active, "piece-white-pawn-0-1");
        Object* squareA5 = findObject(*active, "square-0-4"); // 3 steps forward = illegal
        click2D(harness.interaction, harness.lawManager, pawnA2);
        check(asBool(*pawnA2, "isSelected"), "a2 pawn selected");

        click2D(harness.interaction, harness.lawManager, squareA5);
        check(asInt(*pawnA2, "gridX") == 0 && asInt(*pawnA2, "gridY") == 1,
              "illegal move refused: pawn remains at a2 (0, 1)");
        check(asInt(*state, "turn") == 0, "turn does not advance on illegal move");
    }

    std::cout << "\n" << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
