// End-to-end witness for the authored Second-Nature Law Forge.
//
// This boots ONLY the Zone-native identity + its shared Law roots, enters the
// Zone, clicks an authored instrument Object, lets its authored Law invoke the
// universal Singular set-to-set creation seam, then uses and saves the newborn
// Law. Finally it leaves and re-enters the Zone to prove the newborn is truly
// part of the Zone's authored lawRefs closure rather than a session-only global.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/SecondNatureLawAuthoring.hpp"
#include "json.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {
int checks = 0;
int failures = 0;

void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}

bool near(float a, float b, float eps = 0.03f) {
    return std::fabs(a - b) < eps;
}

glm::vec3 colorOf(Singular& being) {
    PropertyValue value;
    if (!lawGetValue(being, PropertyPath::parse("color"), value)) return glm::vec3(-99.0f);
    if (const auto* color = std::get_if<glm::vec3>(&value)) return *color;
    return glm::vec3(-99.0f);
}

std::string textProperty(Singular& being, const std::string& path) {
    PropertyValue value;
    if (!lawGetValue(being, PropertyPath::parse(path), value)) return {};
    if (const auto* text = std::get_if<std::string>(&value)) return *text;
    return {};
}

bool arrayContains(const nlohmann::json& array, const std::string& value) {
    if (!array.is_array()) return false;
    for (const auto& item : array) {
        if (item.is_string() && item.get<std::string>() == value) return true;
    }
    return false;
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
    // Force the thin authored-interface adapter into this test binary even
    // when earthcall_core is linked as an archive whose unused objects may be
    // dead-stripped. The real app gets the same idempotent install at boot.
    SecondNatureLawAuthoring::install();

    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/SecondNatureLawForge/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    saves = std::filesystem::absolute(saves);

    const auto sourceZone = saves / "zones/SecondNatureLawForge/zone.json";
    check(std::filesystem::exists(sourceZone), "Second-Nature Law Forge Zone identity exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson;
    {
        std::ifstream in(sourceZone);
        in >> zoneJson;
    }
    check(zoneJson.contains("lawRefs") && zoneJson["lawRefs"].is_array() &&
              zoneJson["lawRefs"].size() == 4,
          "Forge names its four authored prototype/instrument Laws through lawRefs");
    if (!zoneJson.contains("lawRefs") || !zoneJson["lawRefs"].is_array()) return 1;

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_second_nature_forge_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    const auto targetZoneDir = scratch.path / "zones/SecondNatureLawForge";
    std::filesystem::create_directories(targetZoneDir);
    std::filesystem::copy_file(sourceZone, targetZoneDir / "zone.json");

    std::size_t copiedRoots = 0;
    for (const auto& refJson : zoneJson["lawRefs"]) {
        if (!refJson.is_string()) continue;
        const std::string ref = refJson.get<std::string>();
        const auto sourceLaw = saves / "laws" / ref / "law.json";
        const auto targetLawDir = scratch.path / "laws" / ref;
        check(std::filesystem::exists(sourceLaw), "shared Forge Law root exists: " + ref);
        if (!std::filesystem::exists(sourceLaw)) continue;
        std::filesystem::create_directories(targetLawDir);
        std::filesystem::copy_file(sourceLaw, targetLawDir / "law.json");
        ++copiedRoots;
    }
    check(copiedRoots == zoneJson["lawRefs"].size(),
          "all Forge Law roots copied into isolated Zone-native boot sandbox");
    if (copiedRoots != zoneJson["lawRefs"].size()) return 1;

    SaveSystem::setSaveRoot(scratch.path.string());
    TestSupport::BootedEngineHarness harness("Zach");
    harness.zones.bindLive();

    std::size_t forgeIndex = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        const auto& zone = harness.zones.zones()[i];
        if (zone && zone->getIdentifier() == "SecondNatureLawForge") {
            forgeIndex = i;
            break;
        }
    }
    check(forgeIndex < harness.zones.zones().size(),
          "boot discovers Forge from Zone identity without a World load");
    if (forgeIndex >= harness.zones.zones().size()) return 1;

    check(harness.zones.switchTo(forgeIndex),
          "entering Forge activates its authored Law closure");
    if (harness.zones.currentIndex() != forgeIndex) return 1;

    check(harness.lawManager.find("law-forge-tool-gold-invoke") != nullptr,
          "gold authoring instrument Law is active");
    Law* prototype = harness.lawManager.find("law-forge-prototype-click-gold");
    check(prototype != nullptr, "gold prototype is an ordinary Law in the closure");
    check(prototype && !prototype->isEnabled(),
          "prototype remains inert until derived");

    auto forge = harness.zones.zones()[forgeIndex];
    auto findObject = [&](const std::string& id) -> Object* {
        for (const auto& object : forge->getOwnedObjects()) {
            if (object && object->getIdentifier() == id) return object.get();
        }
        return nullptr;
    };

    Object* tool = findObject("law-forge-tool-gold");
    Object* target = findObject("law-forge-demo-target");
    Object* state = findObject("law-forge-state");
    check(tool != nullptr, "gold Forge instrument Object exists");
    check(target != nullptr, "Forge demo target exists");
    check(state != nullptr, "Forge authored state Object exists");
    if (!tool || !target || !state) return 1;

    // Person gesture: CLICK THE AUTHORED INSTRUMENT. The click wakes its Law;
    // that Law writes the prototype selection and publishes the Forge event.
    // No test-only direct call to the creation API is used here.
    harness.worldTime = 10.0;
    Universe::instance().setClock(harness.worldTime, 1.0 / 60.0);
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", tool, nullptr, std::time(nullptr)});
    harness.lawManager.tick();

    const std::string newbornId = "law-forge-prototype-click-gold.branch-1";
    Law* newborn = harness.lawManager.find(newbornId);
    check(newborn != nullptr, "clicking authored instrument derives a newborn Law");
    if (!newborn) return 1;
    check(newborn->isEnabled(), "newborn Law is live");
    check(newborn->hasConditionModel() &&
              newborn->conditionModel()->toJson().dump().find("law-forge-demo-target") != std::string::npos,
          "newborn Law binds explicit $TARGET to the chosen target identity");
    check(textProperty(*state, "forgeLastCreated") == newbornId,
          "authored state exposes the newborn stable identity");

    // Use the newborn. The target starts neutral; clicking it now invokes the
    // derived click->gold Law exactly like any other authored Law.
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", target, nullptr, std::time(nullptr)});
    harness.lawManager.tick();
    const glm::vec3 gold = colorOf(*target);
    check(near(gold.x, 1.0f) && near(gold.y, 0.63f) && near(gold.z, 0.14f),
          "newborn Law actually governs the target: click makes it gold");

    // "Keep the instrument": Save Zone must extend authored lawRefs and write
    // the newborn as a shared Law root. Merely living in LawManager is not enough.
    check(harness.zones.persistActiveZone(),
          "Save Zone accepts the Forge after Law birth");
    const nlohmann::json persistedZone = SaveSystem::readZoneIdentity("SecondNatureLawForge");
    check(arrayContains(persistedZone.value("lawRefs", nlohmann::json::array()), newbornId),
          "newborn Law becomes authored Zone membership in persisted lawRefs");
    check(SaveSystem::readLawIdentity(newbornId).is_object(),
          "newborn Law gets its own shared persistent Law root");

    // Leave. Because the newborn entered the active Zone closure rather than
    // the global register by accident, departure must release it.
    auto elsewhere = std::make_shared<Zone>("forge-test-elsewhere", "strict");
    harness.zones.addZone(elsewhere);
    const std::size_t elsewhereIndex = harness.zones.zones().size() - 1;
    check(harness.zones.switchTo(elsewhereIndex), "leave Forge for another Zone");
    check(harness.lawManager.find(newbornId) == nullptr,
          "newborn Law leaves runtime when its Zone leaves runtime");

    // Re-enter. switchTo now has to resolve the just-written lawRef/root and
    // restore exactly the same authored Law from storage.
    check(harness.zones.switchTo(forgeIndex), "re-enter persisted Forge Zone");
    Law* restored = harness.lawManager.find(newbornId);
    check(restored != nullptr, "re-entry restores newborn Law from Zone-native closure");
    if (restored) {
        check(restored->hasConditionModel() &&
                  restored->conditionModel()->toJson().dump().find("law-forge-demo-target") != std::string::npos,
              "restored newborn keeps its bound target in authored Law text");
    }

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
