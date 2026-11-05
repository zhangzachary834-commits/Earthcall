#include "Singularity/Screen/VolumeTransportCohortPlanner.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

namespace {

struct Medium {
    int id = 0;
    bool routed = false;
    double nearDistance = 0.0;
    double farDistance = 0.0;
};

std::vector<int> ids(const std::vector<const Medium*>& media) {
    std::vector<int> out;
    out.reserve(media.size());
    for (const auto* medium : media) out.push_back(medium->id);
    std::sort(out.begin(), out.end());
    return out;
}

std::pair<std::vector<const Medium*>, std::vector<const Medium*>>
legacyFirstFourSplit(const std::vector<const Medium*>& media) {
    const std::size_t cut = std::min<std::size_t>(4, media.size());
    return {
        std::vector<const Medium*>(media.begin(), media.begin() + cut),
        std::vector<const Medium*>(media.begin() + cut, media.end())
    };
}

Rendering::VolumeCohortSupportInterval intervalFor(const Medium* medium) {
    if (!medium) return {};
    return {medium->nearDistance, medium->farDistance, true};
}

Rendering::VolumeProjectedMediumSupport projected(
    uint32_t x, uint32_t y, uint32_t w, uint32_t h,
    double nearDistance, double farDistance) {
    return {
        {x, y, w, h},
        {nearDistance, farDistance, true}
    };
}

uint64_t area(const std::vector<Rendering::VolumeScreenRect>& rects) {
    uint64_t total = 0;
    for (const auto& rect : rects)
        total += uint64_t(rect.width) * rect.height;
    return total;
}

} // namespace

int main() {
    std::array<Medium, 8> storage{{
        {0, true, 2.0, 4.0}, {1, true, 2.5, 4.5},
        {2, true, 3.0, 5.0}, {3, true, 3.5, 5.5},
        {4, false, 12.0, 14.0}, {5, false, 12.5, 15.0},
        {6, false, 13.0, 16.0}, {7, false, 14.0, 17.0}
    }};

    std::vector<const Medium*> authored;
    for (const auto& medium : storage) authored.push_back(&medium);

    const auto predicate = [](const Medium* medium) {
        return medium && medium->routed;
    };

    const auto baseline =
        Rendering::partitionVolumeMediaByRoutedCapability(authored, predicate);
    assert((ids(baseline.first) == std::vector<int>{0, 1, 2, 3}));
    assert((ids(baseline.second) == std::vector<int>{4, 5, 6, 7}));

    // Same authored beings, deliberately hostile serialization order.
    const std::vector<const Medium*> permuted{
        &storage[4], &storage[0], &storage[5], &storage[1],
        &storage[6], &storage[2], &storage[7], &storage[3]
    };

    // Historical Pass-3 counterexample: positional membership changes merely
    // because serialization order changed.
    const auto legacyA = legacyFirstFourSplit(authored);
    const auto legacyB = legacyFirstFourSplit(permuted);
    assert(ids(legacyA.first) != ids(legacyB.first));

    // Capability membership is invariant under the same permutation.
    const auto derived =
        Rendering::partitionVolumeMediaByRoutedCapability(permuted, predicate);
    assert(ids(derived.first) == ids(baseline.first));
    assert(ids(derived.second) == ids(baseline.second));

    // Count thresholds carry no meaning.
    const std::vector<const Medium*> five{
        &storage[0], &storage[4], &storage[1], &storage[5], &storage[2]
    };
    const auto fivePlan =
        Rendering::partitionVolumeMediaByRoutedCapability(five, predicate);
    assert((ids(fivePlan.first) == std::vector<int>{0, 1, 2}));
    assert((ids(fivePlan.second) == std::vector<int>{4, 5}));

    // C3: aggregate each execution family's camera-relative support shell.
    const auto routedSupport =
        Rendering::aggregateVolumeCohortSupport(baseline.first, intervalFor);
    const auto generalSupport =
        Rendering::aggregateVolumeCohortSupport(baseline.second, intervalFor);
    assert(routedSupport.valid && generalSupport.valid);
    assert(routedSupport.nearDistance == 2.0);
    assert(routedSupport.farDistance == 5.5);
    assert(generalSupport.nearDistance == 12.0);
    assert(generalSupport.farDistance == 17.0);
    assert(Rendering::proveWholeScreenRadialOrder(routedSupport, generalSupport) ==
           Rendering::VolumeCohortOrder::FirstBeforeSecond);

    // A camera/query change can reverse chronology without changing capability.
    for (int i = 0; i < 4; ++i) {
        storage[i].nearDistance += 20.0;
        storage[i].farDistance += 20.0;
    }
    const auto routedAfterCameraMove =
        Rendering::aggregateVolumeCohortSupport(baseline.first, intervalFor);
    const auto generalAfterCameraMove =
        Rendering::aggregateVolumeCohortSupport(baseline.second, intervalFor);
    assert(Rendering::proveWholeScreenRadialOrder(
               routedAfterCameraMove, generalAfterCameraMove) ==
           Rendering::VolumeCohortOrder::SecondBeforeFirst);

    // Overlap is not guessed into an order.
    storage[0].nearDistance = 10.0;
    storage[0].farDistance = 15.0;
    storage[1].nearDistance = 10.5;
    storage[1].farDistance = 15.5;
    storage[2].nearDistance = 11.0;
    storage[2].farDistance = 16.0;
    storage[3].nearDistance = 11.5;
    storage[3].farDistance = 16.5;
    const auto overlappingRouted =
        Rendering::aggregateVolumeCohortSupport(baseline.first, intervalFor);
    assert(Rendering::proveWholeScreenRadialOrder(
               overlappingRouted, generalSupport) ==
           Rendering::VolumeCohortOrder::Unknown);

    // C4 witness 1: whole-screen shells overlap, but projected screen support
    // separates the conflicting depth bands. Local proof recovers the split.
    {
        const std::vector<Rendering::VolumeProjectedMediumSupport> first{
            projected(0,   0, 128, 128, 2.0, 5.0),
            projected(128, 0, 128, 128, 20.0, 24.0)
        };
        const std::vector<Rendering::VolumeProjectedMediumSupport> second{
            projected(0,   0, 128, 128, 10.0, 14.0),
            projected(128, 0, 128, 128, 30.0, 34.0)
        };
        const auto plan = Rendering::planVolumeCohortTiles(
            256, 128, 128, 128, first, second);
        assert(!plan.conflictingOrders);
        assert(plan.commonOrder ==
               Rendering::VolumeCohortOrder::FirstBeforeSecond);
        assert(plan.jointRegions.empty());
        assert(area(plan.orderedRegions) == uint64_t(256) * 128);
    }

    // C4 witness 2: one tile is provably ordered and its neighbor is genuinely
    // ambiguous. Only the ambiguous tile becomes joint transport.
    {
        const std::vector<Rendering::VolumeProjectedMediumSupport> first{
            projected(0,   0, 128, 128, 2.0, 5.0),
            projected(128, 0, 128, 128, 10.0, 18.0)
        };
        const std::vector<Rendering::VolumeProjectedMediumSupport> second{
            projected(0,   0, 128, 128, 10.0, 14.0),
            projected(128, 0, 128, 128, 12.0, 20.0)
        };
        const auto plan = Rendering::planVolumeCohortTiles(
            256, 128, 128, 128, first, second);
        assert(plan.hasLocalSplit());
        assert(plan.commonOrder ==
               Rendering::VolumeCohortOrder::FirstBeforeSecond);
        assert(area(plan.orderedRegions) == uint64_t(128) * 128);
        assert(area(plan.jointRegions) == uint64_t(128) * 128);
    }

    // C4 witness 3: opposite proven chronologies in adjacent tiles are not
    // flattened into one draw order. C4 refuses and gives every active tile to
    // the joint path; C5 may later schedule these intervals independently.
    {
        const std::vector<Rendering::VolumeProjectedMediumSupport> first{
            projected(0,   0, 128, 128, 2.0, 5.0),
            projected(128, 0, 128, 128, 20.0, 24.0)
        };
        const std::vector<Rendering::VolumeProjectedMediumSupport> second{
            projected(0,   0, 128, 128, 10.0, 14.0),
            projected(128, 0, 128, 128, 8.0, 12.0)
        };
        const auto plan = Rendering::planVolumeCohortTiles(
            256, 128, 128, 128, first, second);
        assert(plan.conflictingOrders);
        assert(plan.commonOrder == Rendering::VolumeCohortOrder::Unknown);
        assert(plan.orderedRegions.empty());
        assert(area(plan.jointRegions) == uint64_t(256) * 128);
    }

    // C4 witness 4: horizontally and vertically adjacent ordered tiles coalesce
    // so proof granularity does not automatically become draw-call granularity.
    {
        const std::vector<Rendering::VolumeProjectedMediumSupport> first{
            projected(0, 0, 256, 256, 2.0, 5.0)
        };
        const std::vector<Rendering::VolumeProjectedMediumSupport> second{
            projected(0, 0, 256, 256, 10.0, 14.0)
        };
        const auto plan = Rendering::planVolumeCohortTiles(
            256, 256, 128, 128, first, second);
        assert(!plan.conflictingOrders);
        assert(plan.orderedRegions.size() == 1);
        assert(plan.orderedRegions[0].x == 0);
        assert(plan.orderedRegions[0].y == 0);
        assert(plan.orderedRegions[0].width == 256);
        assert(plan.orderedRegions[0].height == 256);
    }

    // C4 witness 5: proof failure is uncertainty, not disappearance. A
    // screen-overlapping medium with an invalid radial certificate forces joint
    // transport on that tile.
    {
        Rendering::VolumeProjectedMediumSupport invalid{
            {0, 0, 128, 128},
            {}
        };
        const std::vector<Rendering::VolumeProjectedMediumSupport> first{invalid};
        const std::vector<Rendering::VolumeProjectedMediumSupport> second{
            projected(0, 0, 128, 128, 10.0, 14.0)
        };
        const auto plan = Rendering::planVolumeCohortTiles(
            128, 128, 128, 128, first, second);
        assert(plan.commonOrder == Rendering::VolumeCohortOrder::Unknown);
        assert(plan.orderedRegions.empty());
        assert(area(plan.jointRegions) == uint64_t(128) * 128);
    }

    std::cout << "PASS volume transport cohort planner: capability membership is "
                 "permutation-invariant; whole-screen and tile-local chronology "
                 "remain proof-derived and fail closed\n";
    return 0;
}
