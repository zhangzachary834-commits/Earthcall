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

} // namespace

int main() {
    std::array<Medium, 8> storage{{
        {0, true}, {1, true}, {2, true}, {3, true},
        {4, false}, {5, false}, {6, false}, {7, false}
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

    // This is the historical Pass-3 counterexample: the old positional split
    // changes physical membership merely because serialization order changed.
    const auto legacyA = legacyFirstFourSplit(authored);
    const auto legacyB = legacyFirstFourSplit(permuted);
    assert(ids(legacyA.first) != ids(legacyB.first));

    // Capability membership is invariant under the same permutation.
    const auto derived =
        Rendering::partitionVolumeMediaByRoutedCapability(permuted, predicate);
    assert(ids(derived.first) == ids(baseline.first));
    assert(ids(derived.second) == ids(baseline.second));

    // Count thresholds carry no meaning: the same predicate works for a mixed
    // five-member set and a three-member set without inventing a ">= 6" mode.
    const std::vector<const Medium*> five{
        &storage[0], &storage[4], &storage[1], &storage[5], &storage[2]
    };
    const auto fivePlan =
        Rendering::partitionVolumeMediaByRoutedCapability(five, predicate);
    assert((ids(fivePlan.first) == std::vector<int>{0, 1, 2}));
    assert((ids(fivePlan.second) == std::vector<int>{4, 5}));

    const std::vector<const Medium*> three{
        &storage[7], &storage[3], &storage[6]
    };
    const auto threePlan =
        Rendering::partitionVolumeMediaByRoutedCapability(three, predicate);
    assert((ids(threePlan.first) == std::vector<int>{3}));
    assert((ids(threePlan.second) == std::vector<int>{6, 7}));

    std::cout << "PASS volume transport cohort capability partition: "
              << "serialization order and media-count thresholds carry no semantics\n";
    return 0;
}
