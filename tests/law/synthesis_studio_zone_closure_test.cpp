// Regression witness for Zach's 2026-09-14 live report: the Living Studio's
// buttons still visibly depressed, but notes were silent and the matching
// resonance spheres no longer swelled. Generic control First Movers were alive;
// the Zone's own authored Law closure had been left behind in the legacy World.
//
// This test deliberately does NOT call loadState() and never loads
// synthesis_studio_living.json as a World. It boots from a temporary
// saves/zones/SynthesisStudio.LivingInstrument identity plus its shared
// saves/laws/<id>/law.json roots, enters through ZoneManager::switchTo(), then
// proves a C5 activation reaches the audio sink and drives the matching
// resonator larger through the authored note -> resonance -> sculpture chain.

#include "support/test_harness.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "json.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
int checks = 0;
int failures = 0;
void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}
bool near(double a, double b, double eps = 0.02) { return std::fabs(a-b) < eps; }
double number(Singular& being, const std::string& path, double fallback = -1e9) {
    PropertyValue value;
    double result = fallback;
    if (lawGetValue(being, PropertyPath::parse(path), value)) {
        propertyValueToNumber(value, result);
    }
    return result;
}
struct SoundedNote { double frequency = 0.0; double amplitude = 0.0; std::string timbre; };
struct Scratch {
    std::filesystem::path path;
    ~Scratch() {
        registerAudioSink(nullptr);
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
}

int main() {
    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/SynthesisStudio.LivingInstrument/zone.json")) {
        saves = std::filesystem::path("..") / "saves";
    }
    saves = std::filesystem::absolute(saves);
    const auto sourceZone = saves / "zones/SynthesisStudio.LivingInstrument/zone.json";
    check(std::filesystem::exists(sourceZone), "restored Living Studio Zone identity exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson;
    {
        std::ifstream in(sourceZone);
        in >> zoneJson;
    }
    check(zoneJson.contains("lawRefs") && zoneJson["lawRefs"].is_array() &&
              !zoneJson["lawRefs"].empty(),
          "Living Studio names a Zone-native authored Law closure");
    if (!zoneJson.contains("lawRefs") || !zoneJson["lawRefs"].is_array()) return 1;

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_living_zone_closure_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))};
    const auto targetZoneDir = scratch.path / "zones/SynthesisStudio.LivingInstrument";
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
    check(copiedRoots == zoneJson["lawRefs"].size(),
          "all Living Studio Law roots copied into isolated Zone-native boot sandbox");
    if (copiedRoots != zoneJson["lawRefs"].size()) return 1;

    SaveSystem::setSaveRoot(scratch.path.string());
    TestSupport::BootedEngineHarness harness("Zach");

    std::size_t livingIndex = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        const auto& zone = harness.zones.zones()[i];
        if (zone && zone->getIdentifier() == "SynthesisStudio.LivingInstrument") {
            livingIndex = i;
            break;
        }
    }
    check(livingIndex < harness.zones.zones().size(),
          "boot discovers Living Studio from its Zone identity without a World load");
    if (livingIndex >= harness.zones.zones().size()) return 1;

    check(harness.zones.switchTo(livingIndex),
          "Move to Zone resolves and commits the Living Studio shared Law closure");
    if (harness.zones.currentIndex() != livingIndex) return 1;

    check(harness.lawManager.find("law-studio-pad-play") != nullptr,
          "Zone activation loads the note playback Law");
    check(harness.lawManager.find("law-studio-resonance-struck") != nullptr,
          "Zone activation loads the note-to-resonator Law");
    check(harness.lawManager.find("law-studio-resonance-envelope") != nullptr,
          "Zone activation loads the resonance envelope Law");
    check(harness.lawManager.find("law-studio-resonance-sculpture") != nullptr,
          "Zone activation loads the sphere-growth sculpture Law");

    auto zone = harness.zones.zones()[livingIndex];
    auto findObject = [&](const std::string& id) -> Object* {
        for (const auto& object : zone->getOwnedObjects()) {
            if (object && object->getIdentifier() == id) return object.get();
        }
        return nullptr;
    };
    Object* pad = findObject("hud.pad.c5");
    Object* resonator = findObject("studio.resonance.c5");
    check(pad != nullptr, "C5 Living pad exists");
    check(resonator != nullptr, "C5 spatial resonator exists");
    if (!pad || !resonator) return 1;

    const double authoredBaseRadius = number(*resonator, "resonanceRadius");
    check(near(authoredBaseRadius, 0.13, 0.01), "C5 resonator keeps its authored base radius");

    std::vector<SoundedNote> sounded;
    registerAudioSink([&](Singular&, double frequency, double amplitude,
                         const std::string& timbre, std::string&) {
        sounded.push_back({frequency, amplitude, timbre});
        return true;
    });

    // Generic click/depression is already covered by InteractionChannel tests and
    // is the part Zach can still see working. Publish the generic archetype's
    // output directly so this witness isolates the missing specialized Zone
    // closure rather than duplicating the input-channel test.
    double time = 20.0;
    Universe::instance().setClock(time, 1.0 / 60.0);
    Core::EventBus::instance().publish(
        ECA::Event{"control-activated", pad, nullptr, std::time(nullptr)});
    harness.lawManager.tick();

    check(!sounded.empty(), "C5 activation reaches the real PlayAudio sink through Zone-loaded Law text");
    if (!sounded.empty()) {
        check(near(sounded.front().frequency, 523.25, 0.05),
              "C5 sounds its authored 523.25 Hz frequency");
        check(sounded.front().amplitude > 0.0,
              "C5 sounds with positive authored amplitude");
    }

    // note-played is published by the playback Law. Give that event and the
    // continuous envelope/sculpture Laws several ordinary ticks to propagate.
    double maxRadius = number(*resonator, "shape.r");
    double struckAt = number(*resonator, "struckAt");
    for (int i = 0; i < 12; ++i) {
        time += 0.04;
        Universe::instance().setClock(time, 0.04);
        harness.lawManager.tick();
        struckAt = number(*resonator, "struckAt");
        maxRadius = std::max(maxRadius, number(*resonator, "shape.r"));
    }
    check(struckAt > 0.0,
          "note-played reaches the matching C5 resonator and records a strike");
    check(maxRadius > authoredBaseRadius + 0.01,
          "matching C5 resonance sphere grows above its authored base radius");

    registerAudioSink(nullptr);
    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
