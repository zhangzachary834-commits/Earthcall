#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
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

// C4 screen-space proof substrate. Rectangles are half-open pixel bounds.
// They are derived query geometry, never authored field identity.
struct VolumeScreenRect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

inline bool volumeScreenRectsOverlap(const VolumeScreenRect& a,
                                     const VolumeScreenRect& b) {
    if (a.width == 0 || a.height == 0 || b.width == 0 || b.height == 0)
        return false;
    const uint64_t ax1 = uint64_t(a.x) + a.width;
    const uint64_t ay1 = uint64_t(a.y) + a.height;
    const uint64_t bx1 = uint64_t(b.x) + b.width;
    const uint64_t by1 = uint64_t(b.y) + b.height;
    return uint64_t(a.x) < bx1 && uint64_t(b.x) < ax1 &&
           uint64_t(a.y) < by1 && uint64_t(b.y) < ay1;
}

struct VolumeProjectedMediumSupport {
    VolumeScreenRect screen;
    VolumeCohortSupportInterval radial;
};

struct VolumeCohortTilePlan {
    VolumeCohortOrder commonOrder = VolumeCohortOrder::Unknown;
    std::vector<VolumeScreenRect> orderedRegions;
    std::vector<VolumeScreenRect> jointRegions;
    bool conflictingOrders = false;

    bool hasLocalSplit() const {
        return !conflictingOrders && !orderedRegions.empty() && !jointRegions.empty();
    }
};

inline std::vector<VolumeScreenRect>
coalesceVolumeTileMask(const std::vector<uint8_t>& mask,
                       uint32_t tilesX, uint32_t tilesY,
                       uint32_t tileWidth, uint32_t tileHeight,
                       uint32_t viewportWidth, uint32_t viewportHeight) {
    std::vector<VolumeScreenRect> rows;
    if (!tilesX || !tilesY || mask.size() != size_t(tilesX) * tilesY)
        return rows;

    // Horizontal runs first.
    for (uint32_t ty = 0; ty < tilesY; ++ty) {
        uint32_t tx = 0;
        while (tx < tilesX) {
            while (tx < tilesX && !mask[size_t(ty) * tilesX + tx]) ++tx;
            if (tx >= tilesX) break;
            const uint32_t begin = tx;
            while (tx < tilesX && mask[size_t(ty) * tilesX + tx]) ++tx;
            const uint32_t x = begin * tileWidth;
            const uint32_t y = ty * tileHeight;
            const uint32_t x1 = std::min(viewportWidth, tx * tileWidth);
            const uint32_t y1 = std::min(viewportHeight, y + tileHeight);
            if (x1 > x && y1 > y)
                rows.push_back({x, y, x1 - x, y1 - y});
        }
    }

    // Merge vertically adjacent equal-width runs so a large certified region
    // remains a small number of physical draw calls.
    std::vector<VolumeScreenRect> merged;
    for (const auto& rect : rows) {
        bool extended = false;
        for (auto& prior : merged) {
            if (prior.x == rect.x && prior.width == rect.width &&
                uint64_t(prior.y) + prior.height == rect.y) {
                prior.height += rect.height;
                extended = true;
                break;
            }
        }
        if (!extended) merged.push_back(rect);
    }
    return merged;
}

inline VolumeCohortTilePlan
planVolumeCohortTiles(uint32_t viewportWidth, uint32_t viewportHeight,
                      uint32_t tileWidth, uint32_t tileHeight,
                      const std::vector<VolumeProjectedMediumSupport>& first,
                      const std::vector<VolumeProjectedMediumSupport>& second,
                      double epsilon = 1e-5) {
    VolumeCohortTilePlan out;
    if (!viewportWidth || !viewportHeight || !tileWidth || !tileHeight)
        return out;

    const uint32_t tilesX = (viewportWidth + tileWidth - 1u) / tileWidth;
    const uint32_t tilesY = (viewportHeight + tileHeight - 1u) / tileHeight;
    std::vector<uint8_t> ordered(size_t(tilesX) * tilesY, 0u);
    std::vector<uint8_t> joint(size_t(tilesX) * tilesY, 0u);
    std::vector<uint8_t> active(size_t(tilesX) * tilesY, 0u);

    auto supportForTile = [&](const std::vector<VolumeProjectedMediumSupport>& supports,
                              const VolumeScreenRect& tile) {
        VolumeCohortSupportInterval aggregate;
        bool any = false;
        for (const auto& support : supports) {
            if (!volumeScreenRectsOverlap(support.screen, tile))
                continue;
            // Once a medium may touch this tile, lack of a valid radial proof
            // is uncertainty, never absence.
            if (!support.radial.valid ||
                !std::isfinite(support.radial.nearDistance) ||
                !std::isfinite(support.radial.farDistance) ||
                support.radial.nearDistance < 0.0 ||
                support.radial.farDistance < support.radial.nearDistance)
                return std::pair<bool, VolumeCohortSupportInterval>{true, {}};
            any = true;
            aggregate.nearDistance =
                std::min(aggregate.nearDistance, support.radial.nearDistance);
            aggregate.farDistance =
                std::max(aggregate.farDistance, support.radial.farDistance);
        }
        aggregate.valid = any;
        return std::pair<bool, VolumeCohortSupportInterval>{any, aggregate};
    };

    for (uint32_t ty = 0; ty < tilesY; ++ty) {
        for (uint32_t tx = 0; tx < tilesX; ++tx) {
            const uint32_t x = tx * tileWidth;
            const uint32_t y = ty * tileHeight;
            const VolumeScreenRect tile{
                x, y,
                std::min(tileWidth, viewportWidth - x),
                std::min(tileHeight, viewportHeight - y)
            };
            const auto firstSupport = supportForTile(first, tile);
            const auto secondSupport = supportForTile(second, tile);
            const size_t index = size_t(ty) * tilesX + tx;

            if (!firstSupport.first && !secondSupport.first)
                continue;
            active[index] = 1u;

            // A tile touched by only one cohort is independently composable and
            // does not constrain the common chronology of dual-cohort tiles.
            if (!firstSupport.first || !secondSupport.first) {
                ordered[index] = 1u;
                continue;
            }

            const auto order =
                proveWholeScreenRadialOrder(firstSupport.second,
                                            secondSupport.second, epsilon);
            if (order == VolumeCohortOrder::Unknown) {
                joint[index] = 1u;
                continue;
            }
            if (out.commonOrder == VolumeCohortOrder::Unknown)
                out.commonOrder = order;
            else if (out.commonOrder != order)
                out.conflictingOrders = true;
            ordered[index] = 1u;
        }
    }

    if (out.conflictingOrders) {
        out.commonOrder = VolumeCohortOrder::Unknown;
        out.orderedRegions.clear();
        out.jointRegions =
            coalesceVolumeTileMask(active, tilesX, tilesY, tileWidth, tileHeight,
                                   viewportWidth, viewportHeight);
        return out;
    }

    // If the cohorts never share a tile, draw order is irrelevant; choose a
    // deterministic order while preserving the proof that their screen support
    // is disjoint under this conservative projection.
    if (out.commonOrder == VolumeCohortOrder::Unknown &&
        std::none_of(joint.begin(), joint.end(), [](uint8_t v) { return v != 0; })) {
        out.commonOrder = VolumeCohortOrder::FirstBeforeSecond;
    }

    out.orderedRegions =
        coalesceVolumeTileMask(ordered, tilesX, tilesY, tileWidth, tileHeight,
                               viewportWidth, viewportHeight);
    out.jointRegions =
        coalesceVolumeTileMask(joint, tilesX, tilesY, tileWidth, tileHeight,
                               viewportWidth, viewportHeight);
    return out;
}

} // namespace Rendering
