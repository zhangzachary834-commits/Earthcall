#pragma once

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

} // namespace Rendering
