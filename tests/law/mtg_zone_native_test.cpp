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

std::string asString(Singular& being, const char* name, const std::string& fallback = "") {
    PropertyValue value;
    if (!being.getDynamicProperty(name, value)) return fallback;
    if (const std::string* direct = std::get_if<std::string>(&value)) return *direct;
    return fallback;
}

void click(Singularity::Input::InteractionChannel* interaction,
           LawManager& laws,
           Object* subject,
           const glm::vec3& world = glm::vec3(0.0f)) {
    if (interaction) {
        interaction->pointerWorld = world;
    }
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
    std::cout << "====================================================\n";
    std::cout << "Running MTG Zone Native & Game Rules Integration Test\n";
    std::cout << "====================================================\n";

    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/MTG/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    saves = std::filesystem::absolute(saves);
    const auto sourceZone = saves / "zones/MTG/zone.json";

    check(std::filesystem::exists(sourceZone), "MTG Zone identity file exists at saves/zones/MTG/zone.json");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson = SaveSystem::readSaveData(sourceZone.string());
    check(zoneJson.value("identifier", std::string{}) == "MTG", "identity identifier is exactly 'MTG'");
    check(zoneJson.contains("lawRefs") && zoneJson["lawRefs"].is_array() &&
              zoneJson["lawRefs"].size() == 14,
          "MTG names all 14 authored Law roots in lawRefs");
    check(zoneJson.contains("materials") && zoneJson["materials"].is_array() &&
              zoneJson["materials"].size() >= 10,
          "MTG carries its full set of aesthetic materials in the Zone identity");

    const auto& authoredObjects = zoneJson["world"]["objects"];
    std::unordered_set<std::string> ids;
    for (const auto& objectJson : authoredObjects) {
        ids.insert(objectJson.value("objectID", std::string{}));
    }
    check(ids.count("object.mtg.table") == 1, "Zone owns the wooden table");
    check(ids.count("object.mtg.playmat") == 1, "Zone owns the playmat");
    check(ids.count("object.mtg.stack_zone") == 1, "Zone owns the stack circle");
    check(ids.count("button.mtg.pass_phase") == 1, "Zone owns button.mtg.pass_phase");
    check(ids.count("button.mtg.resolve") == 1, "Zone owns button.mtg.resolve");
    check(ids.count("card.forest_1") == 1, "Zone owns card.forest_1");
    check(ids.count("card.mountain_1") == 1, "Zone owns card.mountain_1");
    check(ids.count("card.grizzly_bears_1") == 1, "Zone owns card.grizzly_bears_1");
    check(ids.count("card.lightning_bolt_1") == 1, "Zone owns card.lightning_bolt_1");
    check(ids.count("counter.mtg.life1") == 1, "Zone owns player 1 life crystal");
    check(ids.count("counter.mtg.life2") == 1, "Zone owns player 2 life crystal");
    check(ids.count("state.mtg") == 1, "Zone owns state.mtg game state object");
    check(ids.count("author.antigravity") == 1, "Zone owns author.antigravity first-mover referent");

    // Scratch sandbox isolation test
    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_mtg_zone_native_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    const auto targetZoneDir = scratch.path / "zones/MTG";
    std::filesystem::create_directories(targetZoneDir);
    std::filesystem::copy_file(sourceZone, targetZoneDir / "zone.json");
    check(TestSupport::copyZoneMatterClosure(sourceZone, targetZoneDir),
          "MTG sandbox carries its exact physical matter generation");

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
    check(copiedRoots == 14, "all 14 MTG Law roots copied into isolated SaveRoot");
    if (copiedRoots != 14) return 1;

    SaveSystem::setSaveRoot(scratch.path.string());
    {
        TestSupport::BootedEngineHarness harness;

        std::size_t mtgIndex = harness.zones.zones().size();
        for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
            const auto& zone = harness.zones.zones()[i];
            if (zone && zone->getIdentifier() == "MTG") {
                mtgIndex = i;
                break;
            }
        }
        check(mtgIndex < harness.zones.zones().size(), "fresh boot discovers MTG from saves/zones");
        if (mtgIndex >= harness.zones.zones().size()) return 1;

        check(harness.zones.switchTo(mtgIndex), "Move to Zone commits MTG without errors");
        if (harness.zones.currentIndex() != mtgIndex) return 1;

        auto active = harness.zones.zones()[mtgIndex];

        // Verify laws loaded
        std::size_t loadedRoots = 0;
        for (const auto& refJson : zoneJson["lawRefs"]) {
            if (refJson.is_string() && harness.lawManager.find(refJson.get<std::string>())) {
                ++loadedRoots;
            }
        }
        check(loadedRoots == 14, "all 14 MTG Laws loaded and active in LawManager");

        // Verify materials loaded
        check(materials.get("material.mtg.table") != nullptr, "material.mtg.table loaded");
        check(materials.get("material.mtg.playmat") != nullptr, "material.mtg.playmat loaded");
        check(materials.get("material.mtg.card.forest") != nullptr, "material.mtg.card.forest loaded");
        check(materials.get("material.mtg.card.mountain") != nullptr, "material.mtg.card.mountain loaded");
        check(materials.get("material.mtg.card.bears") != nullptr, "material.mtg.card.bears loaded");
        check(materials.get("material.mtg.card.bolt") != nullptr, "material.mtg.card.bolt loaded");

        // Verify objects present in zone
        Object* table = findObject(*active, "object.mtg.table");
        Object* playmat = findObject(*active, "object.mtg.playmat");
        Object* btnPass = findObject(*active, "button.mtg.pass_phase");
        Object* btnResolve = findObject(*active, "button.mtg.resolve");
        Object* forest = findObject(*active, "card.forest_1");
        Object* mountain = findObject(*active, "card.mountain_1");
        Object* bears = findObject(*active, "card.grizzly_bears_1");
        Object* bolt = findObject(*active, "card.lightning_bolt_1");
        Object* state = findObject(*active, "state.mtg");

        check(table != nullptr, "table object is present in MTG zone");
        check(playmat != nullptr, "playmat object is present in MTG zone");
        check(btnPass != nullptr, "button.mtg.pass_phase is present in MTG zone");
        check(btnResolve != nullptr, "button.mtg.resolve is present in MTG zone");
        check(forest != nullptr, "card.forest_1 is present in MTG zone");
        check(mountain != nullptr, "card.mountain_1 is present in MTG zone");
        check(bears != nullptr, "card.grizzly_bears_1 is present in MTG zone");
        check(bolt != nullptr, "card.lightning_bolt_1 is present in MTG zone");
        check(state != nullptr, "state.mtg is present in MTG zone");

        if (!table || !playmat || !btnPass || !btnResolve || !forest || !mountain || !bears || !bolt || !state) {
            std::cerr << "Aborting gameplay test: missing required objects\n";
            return 1;
        }

        // Spatial manifestation assertions: everything must be above ground (y >= 0.50)!
        check(table->getPosition().y >= 0.2f, "table is raised above ground (y=0.25, top surface at 0.5)");
        check(playmat->getPosition().y >= 0.5f, "playmat rests on table at y >= 0.50");
        check(btnPass->getPosition().y >= 0.52f, "pass button rests above playmat at y >= 0.52");
        check(btnResolve->getPosition().y >= 0.52f, "resolve button rests above playmat at y >= 0.52");
        check(forest->getPosition().y >= 0.52f, "forest card rests above playmat at y >= 0.52");
        check(bears->getPosition().y >= 0.52f, "bears card rests above playmat at y >= 0.52");

        // ====================================================================
        // MTG RULES INTEGRATION TEST
        // ====================================================================
        Universe::instance().setClock(0.0, 1.0 / 60.0);

        // Initial State Verification
        check(asInt(*state, "phase") == 0, "initial phase is 0 (Untap)");
        check(asInt(*state, "turnNumber") == 1, "initial turn is 1");
        check(asInt(*state, "player2Life") == 20, "opponent starts with 20 life");
        check(asInt(*state, "manaGreen") == 0, "green mana pool starts at 0");
        check(asInt(*state, "manaRed") == 0, "red mana pool starts at 0");
        check(asInt(*state, "stackCount") == 0, "stack starts empty (0)");
        check(asString(*forest, "zone") == "hand", "forest starts in hand");
        check(asString(*mountain, "zone") == "hand", "mountain starts in hand");
        check(asString(*bears, "zone") == "hand", "grizzly bears starts in hand");
        check(asString(*bolt, "zone") == "hand", "lightning bolt starts in hand");

        // Step 1: Advance Phases (Untap -> Upkeep -> Draw -> Main 1)
        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 1, "passed to phase 1 (Upkeep)");

        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 2, "passed to phase 2 (Draw)");

        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 3, "passed to phase 3 (Main 1)");

        // Step 2: Play Forest to battlefield
        click(harness.interaction, harness.lawManager, forest);
        check(asString(*forest, "zone") == "battlefield", "forest entered battlefield");
        check(asInt(*state, "landPlayedThisTurn") == 1, "landPlayedThisTurn set to 1");
        check(forest->getPosition().z < 2.0f, "forest moved spatially to battlefield row");

        // Step 3: Tap Forest for Green Mana
        check(asInt(*forest, "isTapped") == 0, "forest starts untapped");
        click(harness.interaction, harness.lawManager, forest);
        check(asInt(*forest, "isTapped") == 1, "forest is tapped (isTapped == 1)");
        check(std::fabs(forest->getRotationEulerDegrees().y - 90.0f) < 1e-2f,
              "forest visually rotated 90 degrees on y-axis for tap");
        check(asInt(*state, "manaGreen") == 1, "green mana pool incremented to 1");

        // Step 4: Cast Grizzly Bears onto the Stack
        click(harness.interaction, harness.lawManager, bears);
        check(asInt(*state, "manaGreen") == 0, "green mana spent (pool back to 0)");
        check(asString(*bears, "zone") == "stack", "bears moved to zone 'stack'");
        check(asInt(*state, "stackCount") == 1, "stackCount incremented to 1");
        check(bears->getPosition().y >= 0.58f, "bears elevated onto 3D stack position");

        // Step 5: Resolve the Stack
        click(harness.interaction, harness.lawManager, btnResolve);
        check(asString(*bears, "zone") == "battlefield", "grizzly bears resolved to battlefield");
        check(asInt(*state, "stackCount") == 0, "stack is now empty (0)");

        // Step 6: Advance to Combat Phase (Main 1 -> Combat)
        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 4, "passed to phase 4 (Combat)");

        // Step 7: Attack with Grizzly Bears
        check(asInt(*bears, "isTapped") == 0, "bears is untapped before attack");
        click(harness.interaction, harness.lawManager, bears);
        check(asInt(*bears, "isTapped") == 1, "bears tapped to attack");
        check(asInt(*bears, "isAttacking") == 1, "bears isAttacking is 1");
        check(std::fabs(bears->getRotationEulerDegrees().y - 90.0f) < 1e-2f,
              "bears visually rotated 90 degrees to attack");
        check(bears->getPosition().z < 0.0f, "bears moved forward spatially across combat line");
        check(asInt(*state, "player2Life") == 18, "grizzly bears dealt 2 combat damage (player2Life is 18)");

        // Step 8: Pass Through Main 2 and End Step to Cycle Turn
        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 5, "passed to phase 5 (Main 2)");

        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 6, "passed to phase 6 (End)");

        // Cycle from End back to Untap
        click(harness.interaction, harness.lawManager, btnPass);
        check(asInt(*state, "phase") == 0, "turn cycled back to phase 0 (Untap)");
        check(asInt(*state, "turnNumber") == 2, "turn advanced to 2");
        check(asInt(*state, "landPlayedThisTurn") == 0, "land drop count reset for new turn");
        check(asInt(*forest, "isTapped") == 0, "forest untapped automatically on untap step");
        check(std::fabs(forest->getRotationEulerDegrees().y) < 1e-2f,
              "forest rotation reset to 0 degrees");
        check(asInt(*bears, "isTapped") == 0, "grizzly bears untapped automatically on untap step");
        check(std::fabs(bears->getRotationEulerDegrees().y) < 1e-2f,
              "bears rotation reset to 0 degrees");

        // Step 9: Turn 2 Gameplay (Mountain + Lightning Bolt)
        click(harness.interaction, harness.lawManager, btnPass); // Upkeep (1)
        click(harness.interaction, harness.lawManager, btnPass); // Draw (2)
        click(harness.interaction, harness.lawManager, btnPass); // Main 1 (3)
        check(asInt(*state, "phase") == 3, "turn 2 in Main 1 phase");

        // Play Mountain
        click(harness.interaction, harness.lawManager, mountain);
        check(asString(*mountain, "zone") == "battlefield", "mountain entered battlefield");
        check(asInt(*state, "landPlayedThisTurn") == 1, "land played on turn 2");

        // Tap Mountain for Red Mana
        click(harness.interaction, harness.lawManager, mountain);
        check(asInt(*mountain, "isTapped") == 1, "mountain tapped for mana");
        check(asInt(*state, "manaRed") == 1, "red mana pool is 1");

        // Cast Lightning Bolt
        click(harness.interaction, harness.lawManager, bolt);
        check(asInt(*state, "manaRed") == 0, "red mana spent");
        check(asString(*bolt, "zone") == "stack", "lightning bolt cast onto stack");
        check(asInt(*state, "stackCount") == 1, "stackCount is 1");

        // Resolve Lightning Bolt
        click(harness.interaction, harness.lawManager, btnResolve);
        check(asInt(*state, "player2Life") == 15, "lightning bolt dealt 3 damage (player2Life is 15)");
        check(asString(*bolt, "zone") == "graveyard", "lightning bolt resolved to graveyard");
        check(asInt(*state, "stackCount") == 0, "stack count returned to 0");

        // Step 10: State-Based Actions (SBA)
        // Lethal damage to creature:
        check(asString(*bears, "zone") == "battlefield", "bears is on battlefield before lethal damage");
        bears->setDynamicProperty("damage", PropertyValue(2)); // 2 damage >= 2 toughness
        harness.lawManager.tick();
        check(asString(*bears, "zone") == "graveyard",
              "SBA: lethal damage automatically sent bears to graveyard");

        // Player loss state-based action:
        check(!asBool(*state, "gameOver"), "game is not over yet");
        state->setDynamicProperty("player2Life", PropertyValue(0));
        harness.lawManager.tick();
        check(asBool(*state, "gameOver"),
              "SBA: player2Life <= 0 automatically triggered gameOver = true");
    }

    std::cout << "\n====================================================\n";
    std::cout << "MTG Zone Native Test Results: "
              << (checks - failures) << "/" << checks << " checks passed\n";
    std::cout << "====================================================\n";
    return failures == 0 ? 0 : 1;
}
