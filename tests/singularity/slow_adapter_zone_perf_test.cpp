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
#include "json.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <unordered_set>
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

struct IsolatedWorldSandbox {
    std::filesystem::path path;
    std::string previousSaveRoot;

    explicit IsolatedWorldSandbox(const std::string& resolvedWorldPath) {
        path = std::filesystem::temp_directory_path() /
               ("earthcall-perf-sandbox-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path);

        const std::filesystem::path repoSaves = std::filesystem::absolute("saves");

        std::error_code ec;
        for (const std::string& item : {"laws", "materials", "homes", "persons", "identity"}) {
            if (std::filesystem::exists(repoSaves / item, ec)) {
                std::filesystem::copy(repoSaves / item, path / item,
                    std::filesystem::copy_options::recursive | std::filesystem::copy_options::copy_symlinks, ec);
            }
        }

        std::filesystem::create_directories(path / "worlds", ec);
        const std::filesystem::path worldFile(resolvedWorldPath);
        const std::filesystem::path targetWorld = path / "worlds" / worldFile.filename();
        std::filesystem::copy(worldFile, targetWorld, ec);

        std::filesystem::create_directories(path / "zones", ec);
        std::ifstream win(resolvedWorldPath);
        if (win) {
            try {
                nlohmann::json wjson;
                win >> wjson;
                std::unordered_set<std::string> zoneNames;
                if (wjson.contains("zones")) {
                    for (const auto& z : wjson["zones"]) {
                        if (z.is_string()) zoneNames.insert(z.get<std::string>());
                        else if (z.is_object() && z.contains("name")) zoneNames.insert(z["name"].get<std::string>());
                    }
                }
                for (const auto& zName : zoneNames) {
                    if (std::filesystem::exists(repoSaves / "zones" / zName, ec)) {
                        std::filesystem::copy(repoSaves / "zones" / zName, path / "zones" / zName,
                            std::filesystem::copy_options::recursive | std::filesystem::copy_options::copy_symlinks, ec);
                    }
                }
            } catch (...) {}
        }

        if (std::filesystem::is_empty(path / "zones", ec)) {
            if (std::filesystem::exists(repoSaves / "zones/Chess", ec)) {
                std::filesystem::copy(repoSaves / "zones/Chess", path / "zones/Chess",
                    std::filesystem::copy_options::recursive | std::filesystem::copy_options::copy_symlinks, ec);
            }
        }

        previousSaveRoot = SaveSystem::saveRoot();
        SaveSystem::setSaveRoot(path.string());
    }

    ~IsolatedWorldSandbox() {
        SaveSystem::setSaveRoot(previousSaveRoot);
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
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

    IsolatedWorldSandbox sandbox(world);
    const std::filesystem::path targetWorldInSandbox =
        std::filesystem::path(SaveSystem::saveRoot()) / "worlds" / std::filesystem::path(world).filename();
    const std::string sandboxWorldPath = targetWorldInSandbox.string();

    TestSupport::BootedEngineHarness h("Player", "humanoid", false);
    h.lawManager.setUseSlowAdapter(adapter);
    h.lawManager.setUseLawDirect(direct);
    h.loadWorld(sandboxWorldPath);

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

    return 0;
}
