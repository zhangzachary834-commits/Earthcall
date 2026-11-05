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

    // Overlap is not guessed into an order. It must fall through to refinement
    // or joint/general transport.
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

    std::cout << "PASS volume transport cohort planner: capability membership is "
                 "permutation-invariant and whole-screen chronology is proof-derived\n";
    return 0;
}
