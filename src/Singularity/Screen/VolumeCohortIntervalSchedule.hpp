#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace Rendering {

struct VolumeCohortRayInterval {
    double lo = 0.0;
    double hi = 0.0;
};

enum class VolumeCohortIntervalTarget {
    First,
    Second,
    Joint
};

struct VolumeCohortIntervalRun {
    double lo = 0.0;
    double hi = 0.0;
    VolumeCohortIntervalTarget target = VolumeCohortIntervalTarget::Joint;
};

struct VolumeCohortIntervalSchedule {
    std::vector<VolumeCohortIntervalRun> runs;
    bool admitted = false;
    std::string reason;

    bool hasJointRuns() const {
        return std::any_of(runs.begin(), runs.end(), [](const auto& run) {
            return run.target == VolumeCohortIntervalTarget::Joint;
        });
    }
};

// C5b ray-lifetime proof IR. Partition one ray's conservative positive-support
// intervals into maximal near->far camera-order runs.
//
// This is NOT C5a's far->near framebuffer publication order. A stateful ray
// transport consumer advances near->far because transmittance flows along the
// ray. Lowering may later choose a different representation, but it must
// preserve this chronology.
//
// Invalid proof inputs or run-budget exhaustion refuse the schedule; callers
// must retain joint transport over their authoritative ray segment.
inline VolumeCohortIntervalSchedule
scheduleVolumeCohortRayIntervals(
    const std::vector<VolumeCohortRayInterval>& first,
    const std::vector<VolumeCohortRayInterval>& second,
    std::size_t maxRuns = 64) {
    VolumeCohortIntervalSchedule out;
    if (maxRuns == 0) {
        out.reason = "run budget is zero";
        return out;
    }

    std::vector<double> edges;
    edges.reserve(2 * (first.size() + second.size()));

    auto collect = [&](const std::vector<VolumeCohortRayInterval>& intervals,
                       const char* label) -> bool {
        for (const auto& interval : intervals) {
            if (!std::isfinite(interval.lo) || !std::isfinite(interval.hi) ||
                interval.lo < 0.0 || !(interval.hi > interval.lo)) {
                out.reason = std::string(label) + " interval is invalid";
                return false;
            }
            edges.push_back(interval.lo);
            edges.push_back(interval.hi);
        }
        return true;
    };

    if (!collect(first, "first") || !collect(second, "second"))
        return out;

    if (edges.empty()) {
        out.admitted = true;
        out.reason = "both cohorts prove empty support";
        return out;
    }

    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    auto activeAt = [](const std::vector<VolumeCohortRayInterval>& intervals,
                       double s) {
        return std::any_of(intervals.begin(), intervals.end(),
            [&](const auto& interval) {
                return interval.lo <= s && s < interval.hi;
            });
    };

    auto append = [&](double lo, double hi,
                      VolumeCohortIntervalTarget target) -> bool {
        if (!(hi > lo)) return true;
        if (!out.runs.empty()) {
            auto& prior = out.runs.back();
            if (prior.target == target && prior.hi == lo) {
                prior.hi = hi;
                return true;
            }
        }
        if (out.runs.size() >= maxRuns) {
            out.runs.clear();
            out.reason = "interval run budget exceeded";
            return false;
        }
        out.runs.push_back({lo, hi, target});
        return true;
    };

    for (std::size_t i = 1; i < edges.size(); ++i) {
        const double lo = edges[i - 1];
        const double hi = edges[i];
        if (!(hi > lo)) continue;

        const double mid = lo + 0.5 * (hi - lo);
        if (!(mid > lo && mid < hi)) {
            out.runs.clear();
            out.reason = "interval midpoint is not representable";
            return out;
        }

        const bool firstActive = activeAt(first, mid);
        const bool secondActive = activeAt(second, mid);
        if (!firstActive && !secondActive) continue;

        const auto target =
            firstActive && secondActive
                ? VolumeCohortIntervalTarget::Joint
                : firstActive
                    ? VolumeCohortIntervalTarget::First
                    : VolumeCohortIntervalTarget::Second;
        if (!append(lo, hi, target))
            return out;
    }

    out.admitted = true;
    out.reason = out.hasJointRuns()
        ? "proved camera-order schedule with local joint intervals"
        : "proved disjoint camera-order interval schedule";
    return out;
}

} // namespace Rendering
