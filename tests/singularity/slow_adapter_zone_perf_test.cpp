// Timing-only Slow Adapter world probe.
//
// Runs ONE authored world in ONE fresh process, with adapter ON or OFF.
// The caller runs the binary twice so EventBus subscriptions and warm state
// cannot contaminate the comparison. This is a headless CPU/simulation probe:
// reported FPS is simulation-equivalent throughput (1000 / frame cost), NOT
// rendered GPU FPS.
//
// Usage:
//   slow_adapter_zone_perf_test <world.json> --adapter=on|off --direct=on|off [--frames=N]
//
// Zach, 2026-09-17: measure the adapter across multiple real Zones after moving
// it to an independent same-thread clock.

#include "support/test_harness.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <string>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;

double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

double percentile(std::vector<double> v, double q) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const std::size_t i = std::min(v.size() - 1,
        static_cast<std::size_t>(q * static_cast<double>(v.size() - 1)));
    return v[i];
}

struct Frame {
    double total = 0.0;
    double law = 0.0;
    double maintenance = 0.0;
};
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
            "usage: slow_adapter_zone_perf_test <world.json> --adapter=on|off --direct=on|off [--frames=N]\n");
        return 2;
    }

    std::string world = argv[1];
    bool adapter = true;
    bool direct = true;
    int frames = 240;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--adapter=on") adapter = true;
        else if (arg == "--adapter=off") adapter = false;
        else if (arg == "--direct=on") direct = true;
        else if (arg == "--direct=off") direct = false;
        else if (arg.rfind("--frames=", 0) == 0) frames = std::max(24, std::atoi(arg.c_str() + 9));
    }

    world = TestSupport::resolveRealWorldPath(world);
    if (!std::filesystem::exists(world)) {
        std::fprintf(stderr, "world not found: %s\n", world.c_str());
        return 2;
    }

    // Isolate save root without copying 1GB of saves/worlds or 252MB of all zones on every performance run.
    namespace fs = std::filesystem;
    const fs::path p = fs::absolute(world);
    const fs::path realSaves = p.parent_path().parent_path();

    const fs::path scratch = fs::temp_directory_path() /
        ("earthcall_perf_scratch_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    fs::create_directories(scratch / "worlds", ec);
    fs::create_directories(scratch / "zones", ec);
    fs::create_directories(scratch / "homes", ec);

    const fs::path scratchWorldFile = scratch / "worlds" / p.filename();
    if (fs::exists(p, ec)) {
        fs::copy_file(p, scratchWorldFile, fs::copy_options::overwrite_existing, ec);
    }

    // Copy small shared definitions (laws, homes, materials, persons) so world laws resolve
    for (const auto& sharedDir : {"laws", "homes", "materials", "persons", "categories"}) {
        if (fs::exists(realSaves / sharedDir, ec)) {
            fs::copy(realSaves / sharedDir, scratch / sharedDir, fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
        }
    }

    if (fs::exists(p, ec) && fs::exists(realSaves / "zones", ec)) {
        try {
            nlohmann::json wj = SaveSystem::readSaveData(p.string());
            std::vector<std::string> zoneNames;
            if (wj.contains("currentZoneId") && wj["currentZoneId"].is_string()) {
                zoneNames.push_back(wj["currentZoneId"].get<std::string>());
            }
            if (wj.contains("zoneRefs") && wj["zoneRefs"].is_array()) {
                for (const auto& zr : wj["zoneRefs"]) {
                    if (zr.is_string()) zoneNames.push_back(zr.get<std::string>());
                }
            }
            if (wj.contains("zones") && wj["zones"].is_array()) {
                for (const auto& zj : wj["zones"]) {
                    if (zj.is_object()) {
                        if (zj.contains("identifier") && zj["identifier"].is_string()) {
                            zoneNames.push_back(zj["identifier"].get<std::string>());
                        }
                        if (zj.contains("name") && zj["name"].is_string()) {
                            zoneNames.push_back(zj["name"].get<std::string>());
                        }
                    }
                }
            }
            for (const auto& zName : zoneNames) {
                const fs::path srcZoneDir = realSaves / "zones" / SaveSystem::sanitizeLabel(zName);
                const fs::path dstZoneDir = scratch / "zones" / SaveSystem::sanitizeLabel(zName);
                if (fs::exists(srcZoneDir, ec) && !fs::exists(dstZoneDir, ec)) {
                    fs::copy(srcZoneDir, dstZoneDir, fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
                }
            }
        } catch (...) {}
    }

    const std::string prevRoot = SaveSystem::saveRoot();
    SaveSystem::setSaveRoot(scratch.string());

    TestSupport::BootedEngineHarness h("Player", "humanoid", false);
    h.lawManager.setUseSlowAdapter(adapter);
    h.lawManager.setUseLawDirect(direct);
    h.loadWorld(scratchWorldFile.string());

    if (h.zones.zones().empty()) {
        std::fprintf(stderr, "world loaded no zones: %s\n", world.c_str());
        return 3;
    }
    auto zone = h.zones.zones()[h.zones.currentIndex()];
    if (!zone) return 3;

    constexpr double hz = 60.0;
    constexpr float dt = 1.0f / 60.0f;
    double schedulerWall = 0.0;
    h.lawManager.serviceSlowAdapterClock(schedulerWall); // independent-clock prime

    // Warm foreground state without measuring it. The adapter's own timeline
    // advances too when ON, just as it would in a live 60 Hz session.
    for (int i = 0; i < 30; ++i) {
        zone->update(dt);
        zone->applyFormationRelations();
        h.worldTime += dt;
        Universe::instance().setClock(h.worldTime, dt);
        h.lawManager.tick();
        schedulerWall += 1.0 / hz;
        h.lawManager.serviceSlowAdapterClock(schedulerWall);
    }

    std::vector<Frame> samples;
    samples.reserve(frames);
    for (int i = 0; i < frames; ++i) {
        const auto t0 = Clock::now();
        zone->update(dt);
        zone->applyFormationRelations();
        h.worldTime += dt;
        Universe::instance().setClock(h.worldTime, dt);

        const auto law0 = Clock::now();
        h.lawManager.tick();
        const auto law1 = Clock::now();

        schedulerWall += 1.0 / hz;
        const auto maint0 = Clock::now();
        h.lawManager.serviceSlowAdapterClock(schedulerWall);
        const auto maint1 = Clock::now();

        const auto t1 = Clock::now();
        samples.push_back(Frame{ms(t0, t1), ms(law0, law1), ms(maint0, maint1)});
    }

    std::vector<double> totals, laws, maint;
    totals.reserve(samples.size()); laws.reserve(samples.size()); maint.reserve(samples.size());
    for (const auto& f : samples) {
        totals.push_back(f.total);
        laws.push_back(f.law);
        maint.push_back(f.maintenance);
    }

    const double median = percentile(totals, 0.50);
    const double p95 = percentile(totals, 0.95);
    const double lawMedian = percentile(laws, 0.50);
    const double maintenanceMedian = percentile(maint, 0.50);
    const double maintenanceP95 = percentile(maint, 0.95);
    const double eqFps = median > 1e-9 ? 1000.0 / median : 0.0;

    std::size_t tierDirect = 0, tierAdapter = 0, tierVocabulary = 0, tierSweep = 0;
    for (const auto& law : h.lawManager.getAll()) {
        if (!law) continue;
        const std::string tier = h.lawManager.candidateTierFor(*law);
        if (tier == "law-direct") ++tierDirect;
        else if (tier == "adapter-road") ++tierAdapter;
        else if (tier == "vocabulary") ++tierVocabulary;
        else ++tierSweep;
    }

    std::printf(
        "ADAPTER_PERF world=%s zone=%s adapter=%s direct=%s objects=%zu laws=%zu relations=%zu "
        "frames=%d frame_median_ms=%.6f frame_p95_ms=%.6f sim_eq_fps=%.2f "
        "law_median_ms=%.6f maintenance_median_ms=%.6f maintenance_p95_ms=%.6f "
        "maintenance_runs=%llu roads_known=%zu roads_pending=%zu "
        "tier_direct=%zu tier_adapter=%zu tier_vocabulary=%zu tier_sweep=%zu\n",
        world.c_str(), zone->getIdentifier().c_str(), adapter ? "on" : "off",
        direct ? "on" : "off",
        zone->getOwnedObjects().size(), h.lawManager.getAll().size(),
        zone->formation().relations().getAll().size(), frames,
        median, p95, eqFps, lawMedian, maintenanceMedian, maintenanceP95,
        static_cast<unsigned long long>(h.lawManager.slowAdapterMaintenanceRuns()),
        h.lawManager.slowAdapter().roadsKnown(), h.lawManager.slowAdapter().roadsPending(),
        tierDirect, tierAdapter, tierVocabulary, tierSweep);

    SaveSystem::setSaveRoot(prevRoot);
    fs::remove_all(scratch, ec);

    return 0;
}
