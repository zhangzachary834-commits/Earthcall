// Lag probe: does a frame of Earthcall still fit inside a frame?
//
// Zach asked for "a test that systematically checks for lag." Lag is not one
// bug, so this is not one assertion. It is four questions asked of the same
// loop, ordered from the one that flakes least to the one that flakes most:
//
//   1. SHAPE   — does per-frame work grow faster than the world does?
//                Timing-based, but a *ratio* of timings, so a slow machine
//                and a fast one agree. This is the check that finds an
//                accidental O(n^2) before a Person ever feels it.
//   2. QUIESCENCE — on a world where nothing happens, does the frame keep
//                finding work anyway? Deterministic, no clock involved.
//                This is the "event published as a level, not an edge" bug
//                CLAUDE.md names, and the population-growth bug beside it.
//   3. STEADY  — does the authored chess world hold a 60 Hz frame, with no
//                hitch and no drift over four seconds of frames?
//   4. LOAD    — does opening that world take longer than a Person will sit
//                still for?
//
// 3 and 4 are wall-clock, so every wall-clock budget here is multiplied by a
// calibration factor measured on the machine actually running the test (see
// calibrate()). The budgets are deliberately loose: this test is a tripwire
// for a regression that made the world *sluggish*, not a benchmark. A number
// that drifts up 20% will not fail it. A number that doubles will.
//
// The frame stepped below is Engine::update + LawManager::tick from
// src/Singularity/Core/EngineUpdate.cpp:119 and Engine.cpp:318, minus the
// three channels that need a live GLFW window and an ImGui frame
// (locomotion, creation tools, interaction). What remains is the part of a
// frame that scales with the size of the world, which is the part that lags.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <numeric>
#include <set>
#include <string>
#include <vector>

extern MaterialManager materials;
extern CategoryManager categories;

namespace {

using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
// Wall-clock budgets that are true on the machine they were written on and
// false everywhere else are how a perf test becomes a test everybody disables.
// So no budget below is stated in milliseconds directly: each is stated in
// milliseconds *of this machine*, scaled by how long a fixed, allocation-free
// mixed workload takes here versus the reference machine.
//
// The workload mixes floating-point arithmetic with pointer-chasing through a
// vector of indices, because that is the mix an Earthcall frame actually is:
// property lookups walking maps, and glm doing arithmetic on what it finds.
constexpr double kReferenceCalibrationMs = 12.0;   // Apple M-series, -O0 Debug, 2026-08-24

double calibrate() {
    constexpr int kN = 1 << 16;
    std::vector<int> hops(kN);
    for (int i = 0; i < kN; ++i) hops[i] = (i * 7919 + 13) & (kN - 1);

    double best = 1e9;
    for (int rep = 0; rep < 3; ++rep) {          // best-of-3: ignore a stolen slice
        volatile double sink = 0.0;
        const auto t0 = Clock::now();
        int at = 0;
        double acc = 0.0;
        for (int i = 0; i < kN * 8; ++i) {
            at = hops[at];
            acc += std::sqrt(static_cast<double>(at) + 1.0);
        }
        sink = acc;
        (void)sink;
        best = std::min(best, msSince(t0));
    }
    return best;
}

double gCalibration = 1.0;   // >1 means this machine is slower than the reference

// A budget is never allowed to *tighten* on a fast machine. A machine faster
// than the reference does not get a stricter test; it just passes with room.
double budget(double referenceMs) {
    return referenceMs * std::max(1.0, gCalibration);
}

// ---------------------------------------------------------------------------
// Frame timing statistics
// ---------------------------------------------------------------------------
struct Stats {
    double median = 0.0;
    double p95    = 0.0;
    double worst  = 0.0;
    double mean   = 0.0;
    double firstQuarterMean = 0.0;
    double lastQuarterMean  = 0.0;
};

Stats summarize(std::vector<double> samples) {
    assert(samples.size() >= 8 && "too few frames to say anything");
    Stats s;
    const size_t n = samples.size();
    const size_t q = n / 4;
    s.firstQuarterMean = std::accumulate(samples.begin(), samples.begin() + q, 0.0) / q;
    s.lastQuarterMean  = std::accumulate(samples.end() - q, samples.end(), 0.0) / q;
    s.mean  = std::accumulate(samples.begin(), samples.end(), 0.0) / n;
    std::sort(samples.begin(), samples.end());
    s.median = samples[n / 2];
    s.p95    = samples[static_cast<size_t>(n * 0.95)];
    s.worst  = samples.back();
    return s;
}

void report(const char* what, const Stats& s) {
    std::printf("  %-28s median %7.3f ms   p95 %7.3f   worst %7.3f   "
                "first-quarter %7.3f -> last-quarter %7.3f\n",
                what, s.median, s.p95, s.worst, s.firstQuarterMean, s.lastQuarterMean);
}

// ---------------------------------------------------------------------------
// One frame, headless
// ---------------------------------------------------------------------------
struct FrameResult {
    double ms = 0.0;
    size_t firings = 0;
};

FrameResult stepFrame(Zone& zone, LawManager& lawManager, double& worldTime, float dt) {
    FrameResult r;
    const auto t0 = Clock::now();
    zone.update(dt);
    zone.applyFormationRelations();
    worldTime += static_cast<double>(dt);
    Universe::instance().setClock(worldTime, static_cast<double>(dt));
    r.firings = lawManager.tick().size();
    r.ms = msSince(t0);
    return r;
}

int gFailures = 0;

void expect(bool ok, const std::string& message) {
    if (ok) {
        std::printf("  ok   %s\n", message.c_str());
    } else {
        std::printf("  LAG  %s\n", message.c_str());
        ++gFailures;
    }
}

} // namespace

// ===========================================================================
// 1. SHAPE — does the frame grow faster than the world?
// ===========================================================================
// A synthetic zone: n objects, each carrying an automation clip (so
// Zone::update has real per-object work), watched by one WhileTrue law (so
// LawManager::tick has real per-being work). Both of those are honestly
// linear in n. Anything in the frame that is *not* — a nested scan over the
// population, a rebuild of a cache that should have been kept — shows up as
// the ratio pulling away from 2 as n doubles.
//
// This is the check worth having even when the absolute numbers are fine:
// an O(n^2) frame is invisible on a 32-piece chess board and unusable in a
// world a Person has actually lived in.
namespace {

double medianFrameMsForPopulation(int n, int frames) {
    Object author("lag-probe-author");
    LawManager lawManager;
    auto zone = std::make_shared<Zone>("lag-probe", "test");

    for (int i = 0; i < n; ++i) {
        auto obj = std::make_shared<Object>("lag-probe-" + std::to_string(i));
        obj->setPosition(glm::vec3(static_cast<float>(i % 32),
                                   2.0f,
                                   static_cast<float>(i / 32)));
        Automation::Clip clip;
        clip.name = "lag-probe-spin";
        Automation::Track track;
        track.channel   = Automation::Channel::RotY;
        track.wave      = Automation::Wave::Sine;
        track.amplitude = 15.0f;
        track.frequency = 0.5f;
        track.phase     = static_cast<float>(i % 8) / 8.0f;
        clip.tracks.push_back(track);
        obj->addAutomation(clip);
        zone->addObject(std::move(obj));
    }

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        for (const auto& obj : zone->getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
    });
    Universe::instance().setRelationProvider([](std::vector<Relation*>&) {});

    // One law, watching everyone, all the time. Level-triggered on purpose:
    // WhileTrue is the most expensive honest activation there is, and the one
    // whose cost must stay linear.
    auto watch = lawManager.createLaw("lag-probe-watch", {&author});
    watch->setActivation(Law::Activation::WhileTrue);
    watch->setConditionModel(ConditionNode::compare(
        "position.y", ConditionNode::Op::Gt, PropertyValue(-1000.0)));
    watch->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.25)));

    double worldTime = 0.0;
    for (int i = 0; i < 4; ++i) stepFrame(*zone, lawManager, worldTime, 1.0f / 60.0f);  // warm

    std::vector<double> samples;
    samples.reserve(frames);
    for (int i = 0; i < frames; ++i) {
        samples.push_back(stepFrame(*zone, lawManager, worldTime, 1.0f / 60.0f).ms);
    }
    std::sort(samples.begin(), samples.end());

    Universe::instance().setProvider(nullptr);
    Universe::instance().setRelationProvider(nullptr);
    return samples[samples.size() / 2];
}

void checkFrameShape() {
    std::printf("\n1. SHAPE — per-frame cost against population size\n");

    const int populations[] = {128, 256, 512, 1024};
    double ms[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        ms[i] = medianFrameMsForPopulation(populations[i], 24);
        std::printf("  %5d objects  ->  %8.4f ms / frame   (%7.5f ms per object)\n",
                    populations[i], ms[i], ms[i] / populations[i]);
    }

    // Below this the timer, not the engine, is what is being measured.
    if (ms[0] < 0.02) {
        std::printf("  note: baseline under 20 us; ratios are timer noise, "
                    "reporting only\n");
        return;
    }

    for (int i = 1; i < 4; ++i) {
        const double ratio = ms[i] / ms[i - 1];
        char msg[256];
        std::snprintf(msg, sizeof(msg),
                      "doubling %d -> %d objects costs %.2fx (linear 2.0, quadratic 4.0)",
                      populations[i - 1], populations[i], ratio);
        expect(ratio <= 3.0, msg);
    }

    const double overall = ms[3] / ms[0];
    char msg[256];
    std::snprintf(msg, sizeof(msg),
                  "8x the world costs %.2fx the frame (linear 8.0, quadratic 64.0)",
                  overall);
    expect(overall <= 14.0, msg);
}

} // namespace

// ===========================================================================
// 2. QUIESCENCE — on a world where nothing happens, is the frame still busy?
// ===========================================================================
// No clock in this one, so it cannot flake. Three things must hold on a world
// nobody is touching:
//
//   * law firings per frame must not grow. A steady non-zero count is fine —
//     that is what WhileTrue *is*. A rising count is the level-as-edge bug:
//     something publishes "still happening" every frame and the agenda grows
//     under it.
//   * the population must not grow. A law that spawns on a condition instead
//     of on a transition adds an object per frame, and the frame that walks
//     them gets slower forever.
//   * no being may register the same property path twice. CLAUDE.md's rule
//     against calling buildProperties() from a constructor exists because
//     the vocabulary then registers twice — and every law evaluation after
//     that pays for the duplicate, silently.
namespace {

void checkQuiescence(Zone& zone, LawManager& lawManager, double& worldTime) {
    std::printf("\n2. QUIESCENCE — a world nobody is touching\n");

    const size_t populationBefore = zone.getOwnedObjects().size();

    std::vector<size_t> firings;
    firings.reserve(120);
    for (int i = 0; i < 120; ++i) {
        firings.push_back(stepFrame(zone, lawManager, worldTime, 1.0f / 60.0f).firings);
    }

    // Frames 0-9 are warm-up: OnBecomeTrue laws legitimately fire once as the
    // loaded world's conditions take hold for the first time.
    const size_t earlyPeak = *std::max_element(firings.begin() + 10, firings.begin() + 40);
    const size_t latePeak  = *std::max_element(firings.end() - 30, firings.end());
    const size_t total     = std::accumulate(firings.begin(), firings.end(), size_t{0});

    char msg[256];
    std::snprintf(msg, sizeof(msg),
                  "law firings per frame do not grow (frames 10-40 peak at %zu, "
                  "last 30 peak at %zu; %zu firings over 120 idle frames)",
                  earlyPeak, latePeak, total);
    expect(latePeak <= earlyPeak, msg);

    const size_t populationAfter = zone.getOwnedObjects().size();
    std::snprintf(msg, sizeof(msg),
                  "population stable over 120 idle frames (%zu -> %zu objects)",
                  populationBefore, populationAfter);
    expect(populationAfter == populationBefore, msg);

    size_t duplicated = 0;
    std::string firstOffender;
    for (const auto& obj : zone.getOwnedObjects()) {
        if (!obj) continue;
        std::set<std::string> seen;
        for (Property* p : obj->listProperties()) {
            if (!p) continue;
            if (!seen.insert(p->name()).second) {
                if (firstOffender.empty()) {
                    firstOffender = obj->getIdentifier() + "." + p->name();
                }
                ++duplicated;
            }
        }
    }
    std::snprintf(msg, sizeof(msg),
                  "no being registers a property path twice (%zu duplicates%s%s)",
                  duplicated,
                  firstOffender.empty() ? "" : ", first: ",
                  firstOffender.c_str());
    expect(duplicated == 0, msg);
}

// ===========================================================================
// 3. STEADY — does the authored world hold a frame?
// ===========================================================================
// 240 frames is four seconds at 60 Hz: long enough for a leak to show as
// drift, short enough that nobody stops running the suite because of it.
void checkSteadyFrame(Zone& zone, LawManager& lawManager, double& worldTime) {
    std::printf("\n3. STEADY — 240 frames of the loaded world\n");

    for (int i = 0; i < 10; ++i) stepFrame(zone, lawManager, worldTime, 1.0f / 60.0f);

    std::vector<double> samples;
    samples.reserve(240);
    for (int i = 0; i < 240; ++i) {
        samples.push_back(stepFrame(zone, lawManager, worldTime, 1.0f / 60.0f).ms);
    }
    const Stats s = summarize(samples);
    report("simulation frame", s);

    char msg[256];
    // 16.6 ms is the whole 60 Hz frame. The simulation half of it gets a
    // quarter; rendering, ImGui and the channels need the rest.
    const double medianBudget = budget(4.0);
    std::snprintf(msg, sizeof(msg),
                  "median simulation frame %.3f ms within %.3f ms budget "
                  "(quarter of a 60 Hz frame, calibrated x%.2f)",
                  s.median, medianBudget, std::max(1.0, gCalibration));
    expect(s.median <= medianBudget, msg);

    // A hitch is what a Person actually reports as "it lags" — a mean that is
    // fine and one frame in fifty that stutters.
    const double spikeBudget = budget(16.6);
    std::snprintf(msg, sizeof(msg),
                  "no hitch: worst frame %.3f ms within %.3f ms (a whole 60 Hz frame)",
                  s.worst, spikeBudget);
    expect(s.worst <= spikeBudget, msg);

    // Drift is the one that catches a slow leak: a frame that is 40% dearer
    // after four seconds is 10x dearer after a session.
    const double drift = s.firstQuarterMean > 1e-6
                       ? s.lastQuarterMean / s.firstQuarterMean : 1.0;
    std::snprintf(msg, sizeof(msg),
                  "no drift: last 60 frames cost %.2fx the first 60", drift);
    expect(drift <= 1.4 || s.lastQuarterMean <= budget(0.2), msg);
}

// ===========================================================================
// 4. LOAD — how long before the Person is in the world?
// ===========================================================================
void checkLoadTime(const std::string& filename, double loadMs, size_t objects) {
    std::printf("\n4. LOAD — opening the authored world\n");
    std::printf("  %s: %.1f ms for %zu objects\n", filename.c_str(), loadMs, objects);

    const double loadBudget = budget(4000.0);
    char msg[256];
    std::snprintf(msg, sizeof(msg),
                  "load %.1f ms within %.1f ms budget", loadMs, loadBudget);
    expect(loadMs <= loadBudget, msg);
}

} // namespace

int main(int argc, char** argv) {
    // Line-buffered: this test prints as it goes and is the one test in the
    // suite long enough that a Person will want to watch it work.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    std::string filename = (argc > 1) ? argv[1] : "saves/worlds/chess_app.json";
    if (argc <= 1 && !std::filesystem::exists(filename)) {
        if (std::filesystem::exists("../saves/worlds/chess_app.json"))
            filename = "../saves/worlds/chess_app.json";
    }
    {
        const auto p = std::filesystem::absolute(filename);
        if (p.parent_path().filename() == "worlds" &&
            p.parent_path().parent_path().filename() == "saves") {
            SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
        }
    }

    gCalibration = calibrate() / kReferenceCalibrationMs;
    std::printf("--- frame_lag_test: %s ---\n", filename.c_str());
    std::printf("machine calibration: x%.2f versus reference "
                "(>1 slower; budgets scale, never tighten)\n",
                gCalibration);

    // ---- the shape check needs no authored world, so it runs first and
    //      runs even if the save is missing.
    checkFrameShape();

    if (!std::filesystem::exists(filename)) {
        std::printf("\n%s not found — sections 2-4 need an authored world.\n",
                    filename.c_str());
        std::printf("\nfailures: %d\n", gFailures);
        return gFailures == 0 ? 0 : 1;
    }

    Core::Camera camera;
    MouseHandler mouseHandler;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    LawManager lawManager;
    lawManager.connectToEventBus();

    float currentColor[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;

    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouseHandler;
    ctx.currentColor = currentColor;
    ctx.player = &player;
    ctx.lawManager = &lawManager;
    ctx.worldTime = &worldTime;
    ctx.unpackForAuthoring = false;

    Singularity::Input::InteractionChannel::syncRegister(lawManager);
    if (auto* interaction = Singularity::Input::InteractionChannel::find(lawManager)) {
        interaction->setEnabled(true);
    }

    ZoneManager zones;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        if (zones.zones().empty()) return;
        auto active = zones.zones()[zones.currentIndex()];
        if (!active) return;
        beings.push_back(active.get());
        for (const auto& obj : active->getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
        for (const auto& rel : active->formation().relations().getAll()) {
            if (rel) beings.push_back(rel.get());
        }
        for (const auto& law : lawManager.getAll()) {
            if (law) beings.push_back(law.get());
        }
        for (const auto& material : materials.getAll()) {
            if (material) beings.push_back(material.get());
        }
        for (const auto& category : categories.getAll()) {
            if (category) beings.push_back(category.get());
        }
        beings.push_back(&player);
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& relations) {
        if (zones.zones().empty()) return;
        auto active = zones.zones()[zones.currentIndex()];
        if (!active) return;
        for (const auto& rel : active->formation().relations().getAll()) {
            if (rel) relations.push_back(rel.get());
        }
    });

    const auto loadStart = Clock::now();
    zones.loadState(filename, ctx);
    const double loadMs = msSince(loadStart);

    assert(!zones.zones().empty() && "nothing loaded");
    auto active = zones.zones()[zones.currentIndex()];
    assert(active);

    std::printf("\nworld: %s  objects=%zu  laws=%zu  relations=%zu\n",
                active->getIdentifier().c_str(),
                active->getOwnedObjects().size(),
                lawManager.getAll().size(),
                active->formation().relations().getAll().size());

    checkQuiescence(*active, lawManager, worldTime);
    checkSteadyFrame(*active, lawManager, worldTime);
    checkLoadTime(filename, loadMs, active->getOwnedObjects().size());

    std::printf("\nfailures: %d\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
