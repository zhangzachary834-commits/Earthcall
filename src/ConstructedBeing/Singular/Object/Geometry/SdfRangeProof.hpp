#pragma once

#include "Sdf.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace geom {

// A regular, fixed-depth bitmap derived from the adaptive SDF range theorem.
//
// This is acceleration permission, never authored truth. A set bit means the
// corresponding regular cell is wholly proved f > 0 by the CPU hierarchy and
// may therefore be skipped by an outside-in marcher. A clear bit says nothing:
// the exact authored marcher remains authoritative there.
struct SdfPositiveProofGrid {
    uint8_t depth = 0;
    uint32_t dim = 0;
    uint32_t positiveCells = 0;
    std::vector<uint32_t> words;

    bool hasPositiveCells() const { return positiveCells != 0; }
};

// Coalesce adaptive positive-outside proofs into a regular target-depth grid.
//
// A target cell is positive iff either:
//   * an ancestor/node proves the entire region f > 0 directly, or
//   * every one of the eight partitioning children recursively proves f > 0.
//
// Missing/malformed children, negative cells, ambiguous leaves, and unknown
// ranges all fail open. The hierarchy remains the mathematical authority; this
// function may discard useful proof, but it may never manufacture permission.
inline SdfPositiveProofGrid derivePositiveRangeProofGrid(
    const SdfRangeHierarchy& hierarchy,
    uint8_t targetDepth) {
    SdfPositiveProofGrid out;
    out.depth = targetDepth;

    // The bitmap uses a 32-bit linear cell index. Depth 10 is the largest cubic
    // power-of-two grid whose cell count still fits that representation.
    if (targetDepth > 10u || hierarchy.nodes.empty()) return out;

    const uint32_t dim = 1u << targetDepth;
    const uint64_t cellCount64 =
        static_cast<uint64_t>(dim) * dim * dim;
    if (cellCount64 > std::numeric_limits<uint32_t>::max()) return out;

    out.dim = dim;
    const uint32_t cellCount = static_cast<uint32_t>(cellCount64);
    out.words.assign((cellCount + 31u) / 32u, 0u);

    auto setProofBit = [&](uint32_t x, uint32_t y, uint32_t z) {
        const uint32_t linear = x + dim * (y + dim * z);
        const uint32_t word = linear >> 5u;
        const uint32_t mask = 1u << (linear & 31u);
        if ((out.words[word] & mask) == 0u) {
            out.words[word] |= mask;
            ++out.positiveCells;
        }
    };

    const auto& nodes = hierarchy.nodes;

    auto subtreeProvesPositive =
        [&](auto&& self, uint32_t sourceIndex, uint32_t expectedDepth) -> bool {
        if (sourceIndex >= nodes.size()) return false;

        const SdfRangeNode& node = nodes[sourceIndex];
        if (node.depth != expectedDepth) return false;
        if (rangeNodeProvesPositiveOutside(node)) return true;
        if (node.childCount != 8u) return false;

        for (uint32_t child = 0; child < 8u; ++child) {
            const uint64_t childIndex64 =
                static_cast<uint64_t>(node.firstChild) + child;
            if (childIndex64 >= nodes.size()) return false;
            if (!self(self,
                      static_cast<uint32_t>(childIndex64),
                      expectedDepth + 1u)) {
                return false;
            }
        }
        return true;
    };

    auto rasterize =
        [&](auto&& self,
            uint32_t sourceIndex,
            uint32_t expectedDepth,
            uint32_t cellX,
            uint32_t cellY,
            uint32_t cellZ) -> void {
        if (sourceIndex >= nodes.size() || expectedDepth > targetDepth) return;

        const SdfRangeNode& node = nodes[sourceIndex];
        if (node.depth != expectedDepth) return;

        if (rangeNodeProvesPositiveOutside(node)) {
            const uint32_t span = 1u << (targetDepth - expectedDepth);
            const uint32_t baseX = cellX * span;
            const uint32_t baseY = cellY * span;
            const uint32_t baseZ = cellZ * span;
            for (uint32_t z = 0; z < span; ++z) {
                for (uint32_t y = 0; y < span; ++y) {
                    for (uint32_t x = 0; x < span; ++x) {
                        setProofBit(baseX + x, baseY + y, baseZ + z);
                    }
                }
            }
            return;
        }

        if (expectedDepth == targetDepth) {
            if (subtreeProvesPositive(
                    subtreeProvesPositive, sourceIndex, expectedDepth)) {
                setProofBit(cellX, cellY, cellZ);
            }
            return;
        }

        if (node.childCount != 8u) return;

        for (uint32_t child = 0; child < 8u; ++child) {
            const uint64_t childIndex64 =
                static_cast<uint64_t>(node.firstChild) + child;
            if (childIndex64 >= nodes.size()) return;
            self(self,
                 static_cast<uint32_t>(childIndex64),
                 expectedDepth + 1u,
                 cellX * 2u + ((child & 1u) != 0u ? 1u : 0u),
                 cellY * 2u + ((child & 2u) != 0u ? 1u : 0u),
                 cellZ * 2u + ((child & 4u) != 0u ? 1u : 0u));
        }
    };

    rasterize(rasterize, 0u, 0u, 0u, 0u, 0u);

    if (!out.hasPositiveCells()) {
        // Preserve fail-open semantics and avoid carrying an all-zero allocation.
        out.words.clear();
    }
    return out;
}

} // namespace geom
