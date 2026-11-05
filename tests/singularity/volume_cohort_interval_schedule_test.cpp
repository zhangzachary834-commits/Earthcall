#include "Singularity/Screen/VolumeCohortIntervalSchedule.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using Rendering::VolumeCohortIntervalSchedule;
using Rendering::VolumeCohortIntervalTarget;
using Rendering::VolumeCohortRayInterval;

namespace {

void expectRun(const VolumeCohortIntervalSchedule& schedule,
               std::size_t index,
               double lo, double hi,
               VolumeCohortIntervalTarget target) {
    assert(index < schedule.runs.size());
    const auto& run = schedule.runs[index];
    assert(run.lo == lo);
    assert(run.hi == hi);
    assert(run.target == target);
}

} // namespace

int main() {
    // C5b witness 1: the motivating same-ray chronology.
    {
        const std::vector<VolumeCohortRayInterval> first{
            {0.0, 1.0}, {2.0, 3.0}
        };
        const std::vector<VolumeCohortRayInterval> second{
            {1.0, 2.0}
        };
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(first, second);
        assert(schedule.admitted);
        assert(!schedule.hasJointRuns());
        assert(schedule.runs.size() == 3);
        expectRun(schedule, 0, 0.0, 1.0, VolumeCohortIntervalTarget::First);
        expectRun(schedule, 1, 1.0, 2.0, VolumeCohortIntervalTarget::Second);
        expectRun(schedule, 2, 2.0, 3.0, VolumeCohortIntervalTarget::First);
    }

    // C5b execution witness: the consumer receives one near->far run stream,
    // preserving the motivating A -> B -> A chronology.
    {
        const std::vector<VolumeCohortRayInterval> first{
            {0.0, 1.0}, {2.0, 3.0}
        };
        const std::vector<VolumeCohortRayInterval> second{{1.0, 2.0}};
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(first, second);
        std::vector<VolumeCohortIntervalTarget> visited;
        const bool ok = Rendering::executeVolumeCohortIntervalSchedule(
            schedule,
            [&](const auto& run) {
                visited.push_back(run.target);
                return true;
            });
        assert(ok);
        assert((visited == std::vector<VolumeCohortIntervalTarget>{
            VolumeCohortIntervalTarget::First,
            VolumeCohortIntervalTarget::Second,
            VolumeCohortIntervalTarget::First
        }));
    }

    // C5b witness 2: overlapping support becomes Joint only on the overlap.
    {
        const std::vector<VolumeCohortRayInterval> first{{0.0, 2.0}};
        const std::vector<VolumeCohortRayInterval> second{{1.0, 3.0}};
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(first, second);
        assert(schedule.admitted);
        assert(schedule.hasJointRuns());
        assert(schedule.runs.size() == 3);
        expectRun(schedule, 0, 0.0, 1.0, VolumeCohortIntervalTarget::First);
        expectRun(schedule, 1, 1.0, 2.0, VolumeCohortIntervalTarget::Joint);
        expectRun(schedule, 2, 2.0, 3.0, VolumeCohortIntervalTarget::Second);
    }

    // C5b witness 3: nesting preserves the outer cohort around a local joint run.
    {
        const std::vector<VolumeCohortRayInterval> first{{0.0, 4.0}};
        const std::vector<VolumeCohortRayInterval> second{{1.0, 3.0}};
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(first, second);
        assert(schedule.admitted);
        assert(schedule.runs.size() == 3);
        expectRun(schedule, 0, 0.0, 1.0, VolumeCohortIntervalTarget::First);
        expectRun(schedule, 1, 1.0, 3.0, VolumeCohortIntervalTarget::Joint);
        expectRun(schedule, 2, 3.0, 4.0, VolumeCohortIntervalTarget::First);
    }

    // C5b witness 4: input serialization order cannot change the ray schedule.
    {
        const std::vector<VolumeCohortRayInterval> firstA{
            {0.0, 1.0}, {4.0, 5.0}, {2.0, 3.0}
        };
        const std::vector<VolumeCohortRayInterval> firstB{
            {2.0, 3.0}, {0.0, 1.0}, {4.0, 5.0}
        };
        const std::vector<VolumeCohortRayInterval> second{{1.0, 2.0}, {3.0, 4.0}};
        const auto a =
            Rendering::scheduleVolumeCohortRayIntervals(firstA, second);
        const auto b =
            Rendering::scheduleVolumeCohortRayIntervals(firstB, second);
        assert(a.admitted && b.admitted);
        assert(a.runs.size() == b.runs.size());
        for (std::size_t i = 0; i < a.runs.size(); ++i) {
            assert(a.runs[i].lo == b.runs[i].lo);
            assert(a.runs[i].hi == b.runs[i].hi);
            assert(a.runs[i].target == b.runs[i].target);
        }
    }

    // C5b witness 5: adjacent same-family intervals coalesce into one maximal run.
    {
        const std::vector<VolumeCohortRayInterval> first{
            {0.0, 1.0}, {1.0, 2.0}
        };
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(first, {});
        assert(schedule.admitted);
        assert(schedule.runs.size() == 1);
        expectRun(schedule, 0, 0.0, 2.0, VolumeCohortIntervalTarget::First);
    }

    // C5b witness 6: empty support is a valid empty schedule.
    {
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals({}, {});
        assert(schedule.admitted);
        assert(schedule.runs.empty());
    }

    // C5b witness 7: malformed proof input refuses instead of disappearing.
    {
        const std::vector<VolumeCohortRayInterval> bad{
            {1.0, 1.0},
            {2.0, std::numeric_limits<double>::quiet_NaN()}
        };
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(bad, {});
        assert(!schedule.admitted);
        assert(schedule.runs.empty());
        assert(!schedule.reason.empty());
    }

    // C5b witness 8: bounded schedule complexity is part of admission.
    {
        std::vector<VolumeCohortRayInterval> first;
        std::vector<VolumeCohortRayInterval> second;
        for (int i = 0; i < 8; ++i) {
            const double base = double(i) * 2.0;
            first.push_back({base, base + 0.5});
            second.push_back({base + 0.5, base + 1.0});
        }
        const auto schedule =
            Rendering::scheduleVolumeCohortRayIntervals(first, second, 4);
        assert(!schedule.admitted);
        assert(schedule.runs.empty());
        assert(schedule.reason == "interval run budget exceeded");
    }

    std::cout
        << "PASS volume_cohort_interval_schedule_test: "
        << "A-B-A, local overlap, permutation invariance, coalescing, "
        << "invalid-proof refusal and bounded complexity\n";
    return 0;
}
