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
//                hitch and no drift over up to four seconds of frames?
//   4. LOAD    — does opening that world take longer than a Person will sit
//                still for?
//
// 3 and 4 are wall-clock, so every duration here is divided by a calibration
// factor measured on the machine actually running the test (see calibrate()),
// and that factor is measured again at the end: if the machine's speed moved
// while the measurements were being taken, every timing is still reported and
// none of them may fail the run. §2 never touches a clock and is enforced
// either way.
//
// Nothing here fails for merely being slow today. Each measurement is judged
// against an *aspiration* (what a frame ought to cost) and against a
// *baseline* checked in beside this file (what it costs today), and prints
// ok / STANDING / LAG / IMPROVED accordingly — see the block above judge().
// This is a tripwire for a change that made Earthcall slower, not a benchmark
// and not a scold. A number that drifts up 20% will not fail it. A number
// that doubles will.
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
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <cstdlib>
#include <ctime>
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
// Measured on an *idle* machine — 2.045 ms was the best of fifteen runs on an
// Apple M-series laptop, Debug -O0, 2026-08-24. Take this number from a quiet
// box or every duration in the report is silently scaled by the contention
// that was present when it was set. `frame_lag_test --calibrate` prints it.
constexpr double kReferenceCalibrationMs = 2.0;

double calibrate() {
    constexpr int kN = 1 << 16;
    std::vector<int> hops(kN);
    for (int i = 0; i < kN; ++i) hops[i] = (i * 7919 + 13) & (kN - 1);

    double best = 1e9;
    for (int rep = 0; rep < 3; ++rep) {          // best-of-3: ignore a stolen slice
        const auto t0 = Clock::now();
        int at = 0;
        double acc = 0.0;
        for (int i = 0; i < kN * 4; ++i) {       // dependent loads: memory latency
            at = hops[at];
            acc += std::sqrt(static_cast<double>(at) + 1.0);
        }
        std::map<std::string, double> churn;     // allocation + string compares,
        for (int i = 0; i < 2000; ++i) {         // which is most of a property read
            churn["path.segment." + std::to_string(i & 255)] += acc;
        }
        volatile double sink = acc + churn.size();
        (void)sink;
        best = std::min(best, msSince(t0));
    }
    return best;
}

double gCalibration = 1.0;   // >1 means this machine is slower than the reference

// A machine under load measures every duration long. That is not lag in
// Earthcall, it is lag in the room, and a test that cannot tell the two apart
// is a test that cries wolf. So the calibration workload is run again at the
// end: if the machine's speed moved while the measurements were being taken,
// every clock-derived verdict is reported and none of them is allowed to fail
// the run. The invariants in section 2 never touch a clock and are enforced
// either way.
constexpr double kClockTrustDrift = 1.4;

// ---------------------------------------------------------------------------
// Frame timing statistics
// ---------------------------------------------------------------------------
struct Stats {
    double median = 0.0;
    double p95    = 0.0;
    double worst  = 0.0;
    double mean   = 0.0;
    double firstQuarterMedian = 0.0;
    double lastQuarterMedian  = 0.0;
};

double medianOf(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

Stats summarize(std::vector<double> samples) {
    assert(samples.size() >= 8 && "too few frames to say anything");
    Stats s;
    const size_t n = samples.size();
    const size_t q = n / 4;
    // Medians, not means, at both ends: one 150 ms frame stolen by the window
    // server would otherwise read as a leak. Drift is about the typical frame
    // getting dearer, and the typical frame is the median.
    s.firstQuarterMedian = medianOf({samples.begin(), samples.begin() + q});
    s.lastQuarterMedian  = medianOf({samples.end() - q, samples.end()});
    s.mean  = std::accumulate(samples.begin(), samples.end(), 0.0) / n;
    std::sort(samples.begin(), samples.end());
    s.median = samples[n / 2];
    s.p95    = samples[static_cast<size_t>(n * 0.95)];
    s.worst  = samples.back();
    return s;
}

void report(const char* what, const Stats& s) {
    std::printf("  %-28s median %7.3f ms   p95 %7.3f   worst %7.3f   "
                "first-quarter %7.3f -> last-quarter %7.3f (medians)\n",
                what, s.median, s.p95, s.worst, s.firstQuarterMedian, s.lastQuarterMedian);
}

// ---------------------------------------------------------------------------
// One frame, headless
// ---------------------------------------------------------------------------
// Split by phase, because "the frame is slow" is not a finding — "the frame is
// slow *in collision*" is. The three phases are Engine::update's world half
// (EngineUpdate.cpp:163-165) and the law drain that follows it (Engine.cpp:318).
struct FrameResult {
    double zoneMs      = 0.0;   // Zone::update — automations, rotation, physics
    double relationsMs = 0.0;   // Zone::applyFormationRelations
    double lawMs       = 0.0;   // LawManager::tick — the Rete agenda drain
    double totalMs     = 0.0;
    size_t firings     = 0;

    double groundScanMs = 0.0;
    double rotationMs = 0.0;
    double automationMs = 0.0;
    double physicsMs = 0.0;
};

FrameResult stepFrame(Zone& zone, LawManager& lawManager, double& worldTime, float dt) {
    FrameResult r;
    const auto t0 = Clock::now();
    Zone::UpdateTiming zoneTiming;
    zone.update(dt, &zoneTiming);
    const auto t1 = Clock::now();
    zone.applyFormationRelations();
    const auto t2 = Clock::now();
    worldTime += static_cast<double>(dt);
    Universe::instance().setClock(worldTime, static_cast<double>(dt));
    r.firings = lawManager.tick().size();
    const auto t3 = Clock::now();

    r.zoneMs       = zoneTiming.totalMs;
    r.groundScanMs = zoneTiming.groundScanMs;
    r.rotationMs   = zoneTiming.rotationMs;
    r.automationMs = zoneTiming.automationMs;
    r.physicsMs    = zoneTiming.physicsMs;
    
    r.relationsMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    r.lawMs       = std::chrono::duration<double, std::milli>(t3 - t2).count();
    r.totalMs     = std::chrono::duration<double, std::milli>(t3 - t0).count();
    return r;
}

// Sampling is capped in wall-clock as well as in frames. A lag test that
// itself takes ten minutes to report lag is a lag test nobody runs — and
// "only 14 frames fit inside two seconds" is already the answer.
struct Samples {
    std::vector<FrameResult> frames;
    bool cutShort = false;

    std::vector<double> totals() const {
        std::vector<double> out;
        out.reserve(frames.size());
        for (const auto& f : frames) out.push_back(f.totalMs);
        return out;
    }
    double bestOf(double FrameResult::* field) const {
        double best = 1e18;
        for (const auto& f : frames) best = std::min(best, f.*field);
        return frames.empty() ? 0.0 : best;
    }
    double medianOf(double FrameResult::* field) const {
        std::vector<double> v;
        v.reserve(frames.size());
        for (const auto& f : frames) v.push_back(f.*field);
        std::sort(v.begin(), v.end());
        return v.empty() ? 0.0 : v[v.size() / 2];
    }
};

Samples runFrames(Zone& zone, LawManager& lawManager, double& worldTime,
                  int maxFrames, double wallCapMs, int minFrames = 12) {
    Samples s;
    s.frames.reserve(maxFrames);
    const auto start = Clock::now();
    for (int i = 0; i < maxFrames; ++i) {
        s.frames.push_back(stepFrame(zone, lawManager, worldTime, 1.0f / 60.0f));
        if (static_cast<int>(s.frames.size()) >= minFrames && msSince(start) > wallCapMs) {
            s.cutShort = (i + 1 < maxFrames);
            break;
        }
    }
    return s;
}

int gFailures = 0;       // hard: an invariant broke, no clock involved
int gClockFailures = 0;  // a duration regressed; only fatal if the clock is trustworthy

// ---------------------------------------------------------------------------
// Three verdicts, not two
// ---------------------------------------------------------------------------
// A perf test with only pass/fail has to choose between lying and being red
// forever. This one does neither. Every measurement is judged twice: against
// an *aspiration* (what a frame ought to cost) and against a *baseline*
// checked into the tree beside this file (what it costs today).
//
//   ok        — meets the aspiration.
//   STANDING  — misses the aspiration, matches the baseline. A known cost,
//               reprinted every single run so nobody forgets it is there.
//               Not a failure: it is already written down as a task.
//   LAG       — worse than the baseline. This is the failure. Something you
//               just did made Earthcall slower.
//   IMPROVED  — comfortably better than the baseline: re-record it, with
//               `frame_lag_test --rebaseline`, or the tripwire stays slack.
//
// Baseline times are normalised by the calibration factor, so the file means
// the same thing on a different machine (to the accuracy of the proxy
// workload — hence the generous 1.5x regression tolerance).
constexpr double kTimeRegressionTolerance     = 1.5;   // 50% dearer than the baseline fails
constexpr double kExponentRegressionTolerance = 0.20;  // n^k drifting up by .20 fails
constexpr double kImprovementNotice           = 0.75;  // 25% better: say so

std::map<std::string, double> gBaseline;
std::map<std::string, double> gMeasured;
bool gHaveBaseline = false;

// Under ctest the working directory is build/; run by hand it is the repo
// root. Try both rather than making the caller care.
std::string resolveRepoFile(const std::string& relative) {
    for (const char* prefix : {"", "../", "../../"}) {
        const std::string candidate = std::string(prefix) + relative;
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return relative;
}

const char* kBaselineFile = "tests/singularity/frame_lag_baseline.txt";

void loadBaseline() {
    std::ifstream in(resolveRepoFile(kBaselineFile));
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        gBaseline[line.substr(0, eq)] = std::strtod(line.c_str() + eq + 1, nullptr);
        gHaveBaseline = true;
    }
}

void writeBaseline(const std::string& worldName) {
    const std::string path = resolveRepoFile(kBaselineFile);
    std::ofstream out(path);
    if (!out) {
        std::printf("could not write %s\n", path.c_str());
        return;
    }
    const std::time_t now = std::time(nullptr);
    char stamp[64];
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    out << "# frame_lag_test baseline — what a frame of Earthcall costs today.\n"
        << "# Recorded " << stamp << " from world '" << worldName << "'.\n"
        << "# Times are milliseconds normalised by the calibration factor in\n"
        << "# frame_lag_test.cpp, so this file means the same thing on another\n"
        << "# machine. Exponents are the fitted k in cost ~ n^k and are already\n"
        << "# machine-independent.\n"
        << "#\n"
        << "# Re-record with:  ./build/frame_lag_test --rebaseline\n"
        << "# Only ever re-record DOWNWARD, or after a change whose cost was\n"
        << "# accepted deliberately — and say which in the commit message.\n"
        << "#\n"
        << "# One run writes one run's numbers. On a machine that is shared with\n"
        << "# a browser and an IDE, run it three or four times and raise these to\n"
        << "# the dearest of them by hand, or the tripwire sits under the noise\n"
        << "# and fires at nobody's change.\n";
    for (const auto& kv : gMeasured) {
        out << kv.first << "=" << kv.second << "\n";
    }
    std::printf("\nbaseline written to %s (%zu measurements)\n",
                path.c_str(), gMeasured.size());
}

// Hard invariant: no baseline, no tolerance. Used for the deterministic
// checks, which have no business drifting at all.
void expect(bool ok, const std::string& message) {
    if (ok) {
        std::printf("  ok        %s\n", message.c_str());
    } else {
        std::printf("  LAG       %s\n", message.c_str());
        ++gFailures;
    }
}

// Same as expect(), for a claim that is derived from durations: it is only
// allowed to fail the run when the machine held still (see kClockTrustDrift).
void expectTimed(bool ok, const std::string& message) {
    if (ok) {
        std::printf("  ok        %s\n", message.c_str());
    } else {
        std::printf("  LAG       %s\n", message.c_str());
        ++gClockFailures;
    }
}

// `worse` is the direction that means "more lag". Everything here is
// higher-is-worse, but saying so keeps the comparison honest at a glance.
void judge(const std::string& key, double measured, double aspiration,
           double regressionLimit, const char* unit, const std::string& what) {
    gMeasured[key] = measured;

    const auto found = gBaseline.find(key);
    const bool haveKey = (found != gBaseline.end());
    const double base = haveKey ? found->second : 0.0;

    char detail[320];
    if (haveKey) {
        std::snprintf(detail, sizeof(detail), "%s = %.3f %s (aspiration %.3f, baseline %.3f)",
                      what.c_str(), measured, unit, aspiration, base);
    } else {
        std::snprintf(detail, sizeof(detail), "%s = %.3f %s (aspiration %.3f, no baseline)",
                      what.c_str(), measured, unit, aspiration);
    }

    if (haveKey && measured > regressionLimit) {
        std::printf("  LAG       %s  <- worse than the recorded baseline\n", detail);
        ++gClockFailures;
        return;
    }
    if (measured <= aspiration) {
        std::printf("  ok        %s\n", detail);
        if (haveKey && base > aspiration) {
            std::printf("            (baseline is stale and slack — re-record it)\n");
        }
        return;
    }
    if (!haveKey) {
        std::printf("  STANDING  %s  <- no baseline yet; record one\n", detail);
        return;
    }
    if (measured <= base * kImprovementNotice) {
        std::printf("  IMPROVED  %s  <- re-record the baseline\n", detail);
        return;
    }
    std::printf("  STANDING  %s\n", detail);
}

void judgeTime(const std::string& key, double rawMs, double aspirationMs,
               const std::string& what) {
    const double normalised = rawMs / std::max(0.05, gCalibration);
    const auto found = gBaseline.find(key);
    const double limit = (found != gBaseline.end())
                       ? found->second * kTimeRegressionTolerance
                       : 1e18;
    judge(key, normalised, aspirationMs, limit, "ms", what);
}

void judgeExponent(const std::string& key, double k, double aspiration,
                   const std::string& what) {
    gMeasured[key] = k;
    const auto found = gBaseline.find(key);
    const bool haveKey = (found != gBaseline.end());
    const double base = haveKey ? found->second : 0.0;

    char detail[320];
    if (haveKey) {
        std::snprintf(detail, sizeof(detail), "%s = %.3f (aspiration %.3f, baseline %.3f)",
                      what.c_str(), k, aspiration, base);
    } else {
        std::snprintf(detail, sizeof(detail), "%s = %.3f (aspiration %.3f, no baseline)",
                      what.c_str(), k, aspiration);
    }

    const double baseLimit = haveKey ? base + kExponentRegressionTolerance : 1e18;
    const double regressionLimit = std::min(baseLimit, 2.0);

    if (haveKey && k > regressionLimit) {
        std::printf("  EXP-FAIL  %s  <- ALGORITHMIC REGRESSION\n", detail);
        ++gFailures; // Real failure, independent of machine load
        return;
    }
    if (k <= aspiration) {
        std::printf("  ok        %s\n", detail);
        if (haveKey && base > aspiration + kExponentRegressionTolerance * 2) {
            std::printf("            (baseline is stale and slack — re-record it)\n");
        }
        return;
    }
    if (!haveKey) {
        std::printf("  STANDING  %s  <- no baseline yet; record one\n", detail);
        return;
    }
    if (k <= base - kExponentRegressionTolerance * 2) {
        std::printf("  IMPROVED  %s  <- re-record the baseline\n", detail);
        return;
    }
    std::printf("  STANDING  %s\n", detail);
}

} // namespace

// ===========================================================================
// 1. SHAPE — does the frame grow faster than the world?
// ===========================================================================
// A synthetic zone: n objects, each carrying an automation clip (so
// Zone::update has real per-object work), watched by one WhileTrue law (so
// LawManager::tick has real per-being work). Both of those are honestly
// linear in n. Anything in the frame that is *not* — a scan over every pair,
// a cache rebuilt instead of kept — shows up as the fitted exponent pulling
// away from 1 toward 2.
//
// This is the check worth having even when the absolute numbers look fine:
// an O(n^2) frame is invisible on a 32-piece chess board and unusable in a
// world a Person has actually lived in. It is also the check that does not
// care how fast the machine is, because it compares the machine to itself.
namespace {

struct PopulationCost {
    double total     = 0.0;
    double zone      = 0.0;
    double relations = 0.0;
    double law       = 0.0;
    double groundScan = 0.0;
    double rotation   = 0.0;
    double automation = 0.0;
    double physics    = 0.0;
    int    frames    = 0;
};

PopulationCost costForPopulation(int n, double wallCapMs) {
    Object author("lag-probe-author");
    LawManager lawManager;
    auto zone = std::make_shared<Zone>("lag-probe", "test");

    for (int i = 0; i < n; ++i) {
        auto obj = std::make_shared<Object>("lag-probe-" + std::to_string(i));
        // Spread them out: a grid tight enough to be a world, loose enough
        // that this measures the frame and not one specific pile-up.
        obj->setPosition(glm::vec3(static_cast<float>(i % 32) * 3.0f,
                                   2.0f,
                                   static_cast<float>(i / 32) * 3.0f));
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
    for (int i = 0; i < 3; ++i) stepFrame(*zone, lawManager, worldTime, 1.0f / 60.0f);

    const Samples s = runFrames(*zone, lawManager, worldTime, 24, wallCapMs, 8);

    Universe::instance().setProvider(nullptr);
    Universe::instance().setRelationProvider(nullptr);

    // The *fastest* frame, not the median: this section asks how cost grows
    // with n, and a frame that lost its slice to the window server says
    // nothing about that. Best-case is the most reproducible estimate of the
    // real work, which is what a growth curve needs.
    PopulationCost c;
    c.total      = s.bestOf(&FrameResult::totalMs);
    c.zone       = s.bestOf(&FrameResult::zoneMs);
    c.relations  = s.bestOf(&FrameResult::relationsMs);
    c.law        = s.bestOf(&FrameResult::lawMs);
    c.groundScan = s.bestOf(&FrameResult::groundScanMs);
    c.rotation   = s.bestOf(&FrameResult::rotationMs);
    c.automation = s.bestOf(&FrameResult::automationMs);
    c.physics    = s.bestOf(&FrameResult::physicsMs);
    c.frames     = static_cast<int>(s.frames.size());
    return c;
}

// Least-squares fit of log(cost) against log(n): the exponent k in cost ~ n^k.
// 1.0 is linear, 2.0 is a nested scan over the population. Everything in this
// frame is specified to be linear, so the whole question is how far above 1
// the measurement sits and whether that number moves.
double fittedExponent(const std::vector<double>& ns, const std::vector<double>& costs) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int m = 0;
    for (size_t i = 0; i < ns.size(); ++i) {
        if (costs[i] <= 1e-6) continue;         // below the timer's resolution
        const double x = std::log(ns[i]);
        const double y = std::log(costs[i]);
        sx += x; sy += y; sxx += x * x; sxy += x * y;
        ++m;
    }
    if (m < 2) return 0.0;
    const double denom = m * sxx - sx * sx;
    if (std::fabs(denom) < 1e-12) return 0.0;
    return (m * sxy - sx * sy) / denom;
}

void checkFrameShape() {
    std::printf("\n1. SHAPE — per-frame cost against population size\n");

    const std::vector<double> populations = {64, 128, 256, 512};
    std::vector<PopulationCost> costs;
    for (double n : populations) {
        costs.push_back(costForPopulation(static_cast<int>(n), 300.0));
        const PopulationCost& c = costs.back();
        std::printf("  %5d objects -> frame %8.3f ms  "
                    "(zone %8.3f [g:%.3f r:%.3f a:%.3f p:%.3f]  relations %6.3f  law %8.3f)  over %d frames\n",
                    static_cast<int>(n), c.total, c.zone, c.groundScan, c.rotation, c.automation, c.physics, c.relations, c.law, c.frames);
    }

    // Every phase of this frame is specified to be linear in the population,
    // so 1.0 is the aspiration everywhere and 1.15 is the slack allowed for
    // cache effects and allocator behaviour at these sizes.
    struct Phase { const char* key; const char* name; double PopulationCost::* field; };
    const Phase phases[] = {
        {"shape.exponent.frame",     "whole frame",         &PopulationCost::total},
        {"shape.exponent.zone",      "Zone::update",        &PopulationCost::zone},
        {"shape.exponent.z_ground",  "  groundScan",        &PopulationCost::groundScan},
        {"shape.exponent.z_rot",     "  rotation",          &PopulationCost::rotation},
        {"shape.exponent.z_auto",    "  automation",        &PopulationCost::automation},
        {"shape.exponent.z_phys",    "  physics",           &PopulationCost::physics},
        {"shape.exponent.relations", "formation relations", &PopulationCost::relations},
        {"shape.exponent.law",       "LawManager::tick",    &PopulationCost::law},
    };

    for (const Phase& phase : phases) {
        std::vector<double> series;
        for (const auto& c : costs) series.push_back(c.*(phase.field));
        // A phase whose dearest measurement is a few microseconds has no
        // measurable growth curve — fitting one produces a number that swings
        // between runs and means nothing. Say so instead of judging it.
        const double dearest = *std::max_element(series.begin(), series.end());
        const double k = fittedExponent(populations, series);
        if (k == 0.0 || dearest < 0.05) {
            std::printf("  note      %s costs %.4f ms at %d objects — too little to fit a "
                        "growth curve to; reported, not judged\n",
                        phase.name, dearest, static_cast<int>(populations.back()));
            continue;
        }
        judgeExponent(phase.key, k, 1.15,
                      std::string(phase.name) + " grows as n^k (linear 1.0, quadratic 2.0), k");
    }
}

} // namespace

// ===========================================================================
// 2. QUIESCENCE — on a world where nothing happens, is the frame still busy?
// ===========================================================================
// No clock in this one, so it cannot flake. Three things must hold on a world
// nobody is touching:
//
//   * law firings per frame must not grow. A steady non-zero count is fine —
//     that is what WhileTrue *is*. A rising count is the level-as-edge bug
//     CLAUDE.md names: something publishes "still happening" every frame and
//     the agenda grows underneath it.
//   * the population must not grow. A law that spawns on a condition instead
//     of on a transition adds an object per frame, and every frame that walks
//     the population is slower from then on.
//   * no being may register the same property path twice. CLAUDE.md's rule
//     against calling buildProperties() from a constructor exists because the
//     vocabulary then registers twice — and every law evaluation afterwards
//     pays for the duplicate, silently.
namespace {

void checkQuiescence(Zone& zone, LawManager& lawManager, double& worldTime) {
    std::printf("\n2. QUIESCENCE — a world nobody is touching\n");

    const size_t populationBefore = zone.getOwnedObjects().size();

    const Samples s = runFrames(zone, lawManager, worldTime, 120, 2000.0, 24);
    const size_t n = s.frames.size();
    std::printf("  %zu idle frames%s\n", n, s.cutShort ? " (wall-clock cap reached)" : "");

    std::vector<size_t> firings;
    for (const auto& f : s.frames) firings.push_back(f.firings);

    // The first fifth is warm-up: OnBecomeTrue laws legitimately fire once as
    // a freshly loaded world's conditions take hold for the first time.
    const size_t warm = std::max<size_t>(4, n / 5);
    const size_t earlyPeak = *std::max_element(firings.begin() + warm,
                                               firings.begin() + warm + (n - warm) / 2);
    const size_t latePeak  = *std::max_element(firings.begin() + warm + (n - warm) / 2,
                                               firings.end());
    const size_t total     = std::accumulate(firings.begin(), firings.end(), size_t{0});

    char msg[300];
    std::snprintf(msg, sizeof(msg),
                  "law firings per frame do not grow (first half after warm-up peaks at "
                  "%zu, second half at %zu; %zu firings over %zu idle frames)",
                  earlyPeak, latePeak, total, n);
    expect(latePeak <= earlyPeak, msg);

    const size_t populationAfter = zone.getOwnedObjects().size();
    std::snprintf(msg, sizeof(msg),
                  "population stable over %zu idle frames (%zu -> %zu objects)",
                  n, populationBefore, populationAfter);
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
// Up to 240 frames — four seconds at 60 Hz — or four wall-clock seconds,
// whichever comes first. Long enough for a leak to show as drift; short
// enough that nobody stops running the suite because of it.
void checkSteadyFrame(Zone& zone, LawManager& lawManager, double& worldTime) {
    std::printf("\n3. STEADY — the loaded world, frame after frame\n");

    for (int i = 0; i < 10; ++i) stepFrame(zone, lawManager, worldTime, 1.0f / 60.0f);

    const Samples samples = runFrames(zone, lawManager, worldTime, 240, 2500.0, 24);
    const Stats s = summarize(samples.totals());
    std::printf("  %zu frames%s\n", samples.frames.size(),
                samples.cutShort ? " (wall-clock cap reached)" : "");
    report("simulation frame", s);
    std::printf("  %-28s zone %8.3f ms   relations %7.3f ms   law %8.3f ms  (medians)\n",
                "by phase", samples.medianOf(&FrameResult::zoneMs),
                samples.medianOf(&FrameResult::relationsMs),
                samples.medianOf(&FrameResult::lawMs));

    // 16.6 ms is the whole 60 Hz frame. The simulation half of it gets a
    // third; rendering, ImGui and the three windowed channels need the rest.
    judgeTime("steady.median_ms", s.median, 5.5, "median simulation frame");
    judgeTime("steady.zone_ms", samples.medianOf(&FrameResult::zoneMs), 3.0,
              "  of that, Zone::update");
    judgeTime("steady.law_ms", samples.medianOf(&FrameResult::lawMs), 2.0,
              "  of that, LawManager::tick");

    // A hitch is what a Person actually reports as "it lags" — a mean that is
    // fine and one frame in fifty that stutters.
    // p95, not the single worst frame: on a shared machine the worst frame
    // belongs to whatever else is running. A hitch a Person actually notices
    // is one that recurs, and a recurring hitch moves the 95th percentile.
    judgeTime("steady.p95_ms", s.p95, 16.6, "95th-percentile frame (a hitch that recurs)");

    // Drift is the one that catches a slow leak: a frame 40% dearer after
    // four seconds is unrecognisable after a session. This one is a hard
    // invariant — it compares the run to itself, so no machine and no
    // baseline can excuse it.
    const double drift = s.firstQuarterMedian > 1e-6
                       ? s.lastQuarterMedian / s.firstQuarterMedian : 1.0;
    char msg[256];
    std::snprintf(msg, sizeof(msg),
                  "no drift: last quarter of the run costs %.2fx the first quarter", drift);
    expectTimed(drift <= 1.4 || s.lastQuarterMedian <= 0.2 * std::max(1.0, gCalibration), msg);
}

// ===========================================================================
// 4. LOAD — how long before the Person is in the world?
// ===========================================================================
void checkLoadTime(const std::string& filename, double loadMs, size_t objects) {
    std::printf("\n4. LOAD — opening the authored world\n");
    std::printf("  %s: %.1f ms for %zu objects in the active Zone\n",
                filename.c_str(), loadMs, objects);

    // Four seconds is roughly where opening a world stops feeling like
    // opening a world and starts feeling like waiting for one.
    judgeTime("load.ms", loadMs, 4000.0, "time to open the world");
}

} // namespace

int main(int argc, char** argv) {
    // Line-buffered: this test prints as it goes and is the one test in the
    // suite long enough that a Person will want to watch it work.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    bool rebaseline = false;
    std::string filename;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--rebaseline") rebaseline = true;
        else if (arg == "--calibrate") {
            // Print the raw calibration workload time and stop. Run this a
            // few times before recording a baseline: if the number will not
            // hold still, neither will anything else this test measures.
            std::printf("%.3f ms  (reference machine: %.3f ms)\n",
                        calibrate(), kReferenceCalibrationMs);
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            std::printf("frame_lag_test [world.json] [--rebaseline]\n"
                        "  world.json    a save to measure (default saves/worlds/chess_app.json)\n"
                        "  --rebaseline  record today's numbers as the new tripwire\n"
                        "  --calibrate   print this machine's speed against the reference and stop\n");
            return 0;
        }
        else filename = arg;
    }
    const bool defaultWorld = filename.empty();
    if (defaultWorld) filename = resolveRepoFile("saves/worlds/chess_app.json");
    {
        const auto p = std::filesystem::absolute(filename);
        if (p.parent_path().filename() == "worlds" &&
            p.parent_path().parent_path().filename() == "saves") {
            SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
        }
    }

    gCalibration = calibrate() / kReferenceCalibrationMs;
    loadBaseline();
    std::printf("--- frame_lag_test: %s ---\n", filename.c_str());
    std::printf("machine calibration: x%.2f versus the reference machine "
                "(>1 means slower; every time below is divided by it)\n",
                gCalibration);
    std::printf("baseline: %s\n", gHaveBaseline
                ? resolveRepoFile(kBaselineFile).c_str()
                : "none on disk — every miss reports STANDING, nothing can fail");

    // ---- the shape check needs no authored world, so it runs first and
    //      runs even if the save is missing.
    checkFrameShape();

    if (!std::filesystem::exists(filename)) {
        std::printf("\n%s not found — sections 2-4 need an authored world.\n",
                    filename.c_str());
        std::printf("\nbroken invariants: %d   timing regressions: %d\n",
                    gFailures, gClockFailures);
        return (gFailures + gClockFailures) == 0 ? 0 : 1;
    }

    class DummyLagRenderer : public Renderer {
    public:
        void drawMesh(const geom::TessMesh&, const RenderMaterial&) override {}
        void drawImplicit(const geom::SdfNode&, const glm::vec3&, const RenderMaterial&, const geom::FieldNode*, uint64_t, uint32_t, const geom::HeightGrid*) override {}
        void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>&, const glm::vec4&, float, Blend) override {}
        void drawOverlay(const geom::TessMesh&, const glm::vec4&, float, bool) override {}
        void drawSolid(const std::vector<glm::vec3>&, const glm::vec4&, Blend, bool) override {}
        void drawImage2D(const uint8_t*, uint32_t, uint32_t, const glm::vec4&, const glm::vec4&) override {}
        TextureHandle uploadTexture(TextureHandle, const uint8_t*, uint32_t, uint32_t) override { return 1; }
        void releaseTexture(TextureHandle) override {}
        void begin2D(uint32_t, uint32_t) override {}
        void end2D() override {}
        void drawLines2D(const std::vector<glm::vec2>&, const glm::vec4&, float) override {}
        void drawTris2D(const std::vector<glm::vec2>&, const glm::vec4&) override {}
    };
    DummyLagRenderer dummyRenderer;
    setCurrentRenderer(&dummyRenderer);

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
    ctx.person = &player;
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

    // Was the machine steady while all of that was measured?
    const double calibrationAfter = calibrate() / kReferenceCalibrationMs;
    const double drift = std::max(calibrationAfter, gCalibration) /
                         std::max(1e-6, std::min(calibrationAfter, gCalibration));
    const bool clockTrustworthy = drift <= kClockTrustDrift;
    std::printf("\nmachine steadiness: calibration x%.2f at the start, x%.2f at the end "
                "(drift %.2fx)\n", gCalibration, calibrationAfter, drift);
    if (!clockTrustworthy) {
        std::printf("  the machine's speed moved under the measurement. Every duration\n"
                    "  above is reported, none of them can fail this run. Section 2's\n"
                    "  invariants never touch a clock and were enforced regardless.\n");
    }

    if (rebaseline && !clockTrustworthy) {
        std::printf("\n--rebaseline refused: a baseline recorded on a contended machine\n"
                    "is a tripwire set at the wrong height. Re-run on a quiet one.\n");
        rebaseline = false;
    }

    if (rebaseline) {
        // Only the default world may write the baseline: the file records one
        // world's cost, and quietly overwriting it from a different save is
        // how a tripwire becomes a fiction.
        if (defaultWorld) writeBaseline(active->getIdentifier());
        else std::printf("\n--rebaseline ignored: the baseline records the default "
                         "world, and this run measured %s\n", filename.c_str());
    }

    const int fatal = gFailures + (clockTrustworthy ? gClockFailures : 0);
    std::printf("\nbroken invariants: %d   timing regressions: %d%s\n",
                gFailures, gClockFailures,
                (gClockFailures && !clockTrustworthy) ? "  (not counted: machine contended)" : "");
    std::printf("STANDING lines are costs already recorded in the baseline, not failures.\n"
                "They are tasks — see docs/Agenda/Tasks/To-do list.md, Performance.\n");
    return fatal == 0 ? 0 : 1;
}
