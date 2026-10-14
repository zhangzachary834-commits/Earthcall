#include "Singularity/Screen/VolumeZeroProof.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <string>

namespace Rendering {

bool VolumeZeroProof::proven(uint32_t x, uint32_t y, uint32_t z) const {
    if (x >= dims.x || y >= dims.y || z >= dims.z) return false;
    const uint32_t index = x + dims.x * (y + dims.y * z);
    const uint32_t word = static_cast<uint32_t>(words[index / kBitsPerWord]);
    return ((word >> (index % kBitsPerWord)) & 1u) != 0u;
}

bool densityProvenNonPositive(const OntoMath::Piecewise& density,
                              const glm::vec3& lo, const glm::vec3& hi,
                              uint32_t* rangeEvaluations) {
    using RV = OntoMath::MathNode::RangeValue;
    const OntoMath::Interval ix(lo.x, hi.x), iy(lo.y, hi.y), iz(lo.z, hi.z);
    const std::map<std::string, RV> vars = {
        {"x", RV::makeScalar(ix)},
        {"y", RV::makeScalar(iy)},
        {"z", RV::makeScalar(iz)},
        {OntoMath::kAmbientPointVar, RV::makeVector(ix, iy, iz)},
    };  

    for (const auto& piece : density.pieces) {
        if (!piece.mathNode) continue;   // the emitter skips these too
        if (rangeEvaluations) ++*rangeEvaluations;
        const auto range = piece.mathNode->evalRange(vars);
        if (!range || range->kind != OntoMath::ValueKind::Scalar) return false;
        // NaN compares false, so a poisoned bound is never a proof.
        if (!(range->scalar.hi <= 0.0f)) return false;
        // An unconditional piece always answers; later pieces are unreachable.
        if (!piece.hasLo && !piece.hasHi) return true;
    }
    // Every reachable piece is <= 0, and no match yields 0.
    return true;
}

namespace {

struct Builder {
    const OntoMath::Piecewise& density;
    glm::vec3 halfExtent;
    glm::vec3 cellSize;
    glm::vec3 margin;
    VolumeZeroProof& out;

    void mark(glm::uvec3 a, glm::uvec3 b) {
        for (uint32_t z = a.z; z < b.z; ++z)
            for (uint32_t y = a.y; y < b.y; ++y)
                for (uint32_t x = a.x; x < b.x; ++x) {
                    const uint32_t index = x + out.dims.x * (y + out.dims.y * z);
                    auto& word = out.words[index / VolumeZeroProof::kBitsPerWord];
                    const uint32_t bits = static_cast<uint32_t>(word) |
                        (1u << (index % VolumeZeroProof::kBitsPerWord));
                    word = static_cast<float>(bits);
                    ++out.provenCells;
                }
    }

    // Prove the cell block [a, b); split it along its longest axis if the
    // whole block cannot be proved, down to single cells.
    void prove(glm::uvec3 a, glm::uvec3 b) {
        const glm::vec3 lo = -halfExtent + glm::vec3(a) * cellSize - margin;
        const glm::vec3 hi = -halfExtent + glm::vec3(b) * cellSize + margin;
        if (densityProvenNonPositive(density, lo, hi, &out.rangeEvaluations)) {
            mark(a, b);
            return;
        }
        const glm::uvec3 span = b - a;
        int axis = 0;
        if (span.y > span[axis]) axis = 1;
        if (span.z > span[axis]) axis = 2;
        if (span[axis] <= 1u) return;   // a single unproven cell
        glm::uvec3 mid = b;
        mid[axis] = a[axis] + span[axis] / 2u;
        glm::uvec3 start = a;
        start[axis] = mid[axis];
        prove(a, mid);
        prove(start, b);
    }
};

} // namespace

VolumeZeroProof buildVolumeZeroProof(const OntoMath::Piecewise& density,
                                     const glm::vec3& halfExtent,
                                     uint32_t targetCells,
                                     float marginFraction) {
    VolumeZeroProof proof;
    const glm::vec3 extent = 2.0f * glm::abs(halfExtent);
    if (!(extent.x > 0.0f && extent.y > 0.0f && extent.z > 0.0f) ||
        !std::isfinite(extent.x + extent.y + extent.z) || targetCells == 0u) {
        return proof;
    }

    // Near-cubic cells sized from the box itself: side = cbrt(volume / target).
    const float side = std::cbrt(extent.x * extent.y * extent.z /
                                 static_cast<float>(targetCells));
    for (int axis = 0; axis < 3; ++axis) {
        const float cells = std::ceil(extent[axis] / side);
        proof.dims[axis] = static_cast<uint32_t>(std::clamp(
            cells, 1.0f, static_cast<float>(VolumeZeroProof::kMaxAxisCells)));
    }
    proof.totalCells = proof.dims.x * proof.dims.y * proof.dims.z;
    proof.words.assign(
        (proof.totalCells + VolumeZeroProof::kBitsPerWord - 1u) /
            VolumeZeroProof::kBitsPerWord,
        0.0f);

    const glm::vec3 cellSize = extent / glm::vec3(proof.dims);
    Builder builder{density, glm::abs(halfExtent), cellSize,
                    cellSize * marginFraction, proof};
    builder.prove(glm::uvec3(0u), proof.dims);

    // Zach's minimum-work direction: use the same sound cell proofs to bound
    // where any contribution could remain, once per revision, rather than
    // walking empty ray prefixes/tails every frame. Unknown cells remain in
    // the envelope. No new mathematical approximation or denser grid.
    glm::uvec3 first = proof.dims, last(0u);
    std::array<std::vector<glm::uvec2>, 3> bandFirst, bandLast;
    for (uint32_t axis = 0; axis < 3; ++axis) {
        bandFirst[axis].assign(proof.dims[axis],
                              glm::uvec2(proof.dims[(axis + 1) % 3], proof.dims[(axis + 2) % 3]));
        bandLast[axis].assign(proof.dims[axis], glm::uvec2(0u));
    }
    bool occupied = false;
    for (uint32_t z = 0; z < proof.dims.z; ++z)
        for (uint32_t y = 0; y < proof.dims.y; ++y)
            for (uint32_t x = 0; x < proof.dims.x; ++x) {
                if (proof.proven(x, y, z)) continue;
                occupied = true;
                first = glm::min(first, glm::uvec3(x, y, z));
                last = glm::max(last, glm::uvec3(x + 1u, y + 1u, z + 1u));
                const glm::uvec3 c(x, y, z);
                for (uint32_t axis = 0; axis < 3; ++axis) {
                    const glm::uvec2 cross(c[(axis + 1) % 3], c[(axis + 2) % 3]);
                    auto& lo = bandFirst[axis][c[axis]];
                    auto& hi = bandLast[axis][c[axis]];
                    lo = glm::min(lo, cross);
                    hi = glm::max(hi, cross + glm::uvec2(1u));
                }
            }
    proof.supportMin = glm::vec3(1.0f);
    proof.supportMax = glm::vec3(-1.0f);
    if (occupied) {
        const glm::vec3 margin = cellSize * marginFraction;
        proof.supportMin = -glm::abs(halfExtent) + glm::vec3(first) * cellSize - margin;
        proof.supportMax = -glm::abs(halfExtent) + glm::vec3(last) * cellSize + margin;
    }
    // The bounds follow whatever authored support the interval proofs admit.
    // Unknown cells MUST be included. Ranking affects only traversal cost.
    double best = INFINITY;
    for (uint32_t axis = 0; axis < 3; ++axis) {
        double volume = 0.0;
        for (uint32_t i = 0; i < proof.dims[axis]; ++i) {
            const auto lo = bandFirst[axis][i], hi = bandLast[axis][i];
            if (glm::any(glm::greaterThan(lo, hi))) continue;
            volume += double(hi.x - lo.x) * double(hi.y - lo.y);
        }
        volume /= proof.totalCells;
        if (volume < best) { best = volume; proof.supportAxis = axis; }
    }
    const uint32_t axis = proof.supportAxis, a = (axis + 1) % 3, b = (axis + 2) % 3;
    proof.supportBands.assign(proof.dims[axis], glm::vec4(1.0f, 1.0f, -1.0f, -1.0f));
    for (uint32_t i = 0; i < proof.dims[axis]; ++i) {
        const auto lo = bandFirst[axis][i], hi = bandLast[axis][i];
        if (glm::any(glm::greaterThan(lo, hi))) continue;
        // Use the existing proof margin; explicitly round the index envelope
        // outward as well. The GPU adds world/local arithmetic padding.
        const auto lower = [&](uint32_t k, uint32_t n) {
            const double roundoff = 8.0 * std::numeric_limits<float>::epsilon() * extent[k];
            return std::nextafter(float(-double(std::abs(halfExtent[k])) + double(n) * cellSize[k] -
                                       builder.margin[k] - roundoff), -INFINITY);
        };
        const auto upper = [&](uint32_t k, uint32_t n) {
            const double roundoff = 8.0 * std::numeric_limits<float>::epsilon() * extent[k];
            return std::nextafter(float(-double(std::abs(halfExtent[k])) + double(n) * cellSize[k] +
                                       builder.margin[k] + roundoff), INFINITY);
        };
        proof.supportBands[i] = {lower(a, lo.x), lower(b, lo.y),
                                upper(a, hi.x), upper(b, hi.y)};
    }
    return proof;
}

} // namespace Rendering
