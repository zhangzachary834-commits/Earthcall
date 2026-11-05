#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace Rendering {

// Derived execution partition only. This does not create authored meaning and
// intentionally preserves relative order *inside* each capability family while
// making membership depend solely on the supplied structural predicate.
template <class Medium, class RoutedPredicate>
std::pair<std::vector<Medium>, std::vector<Medium>>
partitionVolumeMediaByRoutedCapability(const std::vector<Medium>& media,
                                       RoutedPredicate&& routedAdmissible) {
    std::vector<Medium> routed;
    std::vector<Medium> general;
    routed.reserve(media.size());
    general.reserve(media.size());
    for (const auto& medium : media) {
        (routedAdmissible(medium) ? routed : general).push_back(medium);
    }
    return {std::move(routed), std::move(general)};
}

// Camera-relative conservative radial support. This is deliberately not an
// authored "near/far" label. It is derived proof state for one query origin.
struct VolumeCohortSupportInterval {
    double nearDistance = std::numeric_limits<double>::infinity();
    double farDistance = -std::numeric_limits<double>::infinity();
    bool valid = false;
};

enum class VolumeCohortOrder {
    Unknown,
    FirstBeforeSecond,
    SecondBeforeFirst
};

template <class Medium, class IntervalProvider>
VolumeCohortSupportInterval
aggregateVolumeCohortSupport(const std::vector<Medium>& media,
                             IntervalProvider&& intervalForMedium) {
    VolumeCohortSupportInterval out;
    if (media.empty()) return out;

    for (const auto& medium : media) {
        const auto interval = intervalForMedium(medium);
        if (!interval.valid ||
            !std::isfinite(interval.nearDistance) ||
            !std::isfinite(interval.farDistance) ||
            interval.nearDistance < 0.0 ||
            interval.farDistance < interval.nearDistance) {
            return {};
        }
        out.nearDistance = std::min(out.nearDistance, interval.nearDistance);
        out.farDistance = std::max(out.farDistance, interval.farDistance);
    }
    out.valid = true;
    return out;
}

// Sufficient whole-screen chronology proof.
//
// If every possible point in A is closer to the camera than every possible
// point in B, then any ray that intersects both must encounter A first.
// Overlapping radial shells are not evidence of wrong order; they are simply
// insufficient for this coarse certificate and therefore return Unknown.
//
// This certificate is intentionally conservative. C4/C5 may later refine
// ambiguous regions per tile/ray/interval without weakening this proof.
inline VolumeCohortOrder
proveWholeScreenRadialOrder(const VolumeCohortSupportInterval& first,
                            const VolumeCohortSupportInterval& second,
                            double epsilon = 1e-5) {
    if (!first.valid || !second.valid || epsilon < 0.0)
        return VolumeCohortOrder::Unknown;

    if (first.farDistance + epsilon <= second.nearDistance)
        return VolumeCohortOrder::FirstBeforeSecond;
    if (second.farDistance + epsilon <= first.nearDistance)
        return VolumeCohortOrder::SecondBeforeFirst;
    return VolumeCohortOrder::Unknown;
}

} // namespace Rendering
