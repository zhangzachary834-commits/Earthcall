#include "Singularity/Screen/VolumeZeroProof.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <variant>

namespace Rendering {

bool VolumeZeroProof::proven(uint32_t x, uint32_t y, uint32_t z) const {
    if (x >= dims.x || y >= dims.y || z >= dims.z) return false;
    const uint32_t index = x + dims.x * (y + dims.y * z);
    const uint32_t word = static_cast<uint32_t>(words[index / kBitsPerWord]);
    return ((word >> (index % kBitsPerWord)) & 1u) != 0u;
}

uint32_t VolumeZeroProof::adaptiveStrideCode(uint32_t x, uint32_t y, uint32_t z) const {
    if (x >= dims.x || y >= dims.y || z >= dims.z) return 0u;
    if (proven(x, y, z)) return 3u;
    const uint32_t index = x + dims.x * (y + dims.y * z);
    const uint32_t wordIdx = index / kAdaptiveCellsPerWord;
    if (wordIdx >= adaptiveWords.size()) return 0u;
    const uint32_t shift = (index % kAdaptiveCellsPerWord) * 2u;
    const uint32_t word = static_cast<uint32_t>(adaptiveWords[wordIdx]);
    return (word >> shift) & 3u;
}

uint32_t VolumeZeroProof::adaptiveStride(uint32_t x, uint32_t y, uint32_t z) const {
    const uint32_t code = adaptiveStrideCode(x, y, z);
    if (code == 3u) return 0u;
    if (code == 2u) return 4u;
    if (code == 1u) return 2u;
    return 1u;
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
    const glm::vec3 mid = 0.5f * (lo + hi);
    const std::map<std::string, PropertyValue> midVars = {
        {"x", static_cast<double>(mid.x)},
        {"y", static_cast<double>(mid.y)},
        {"z", static_cast<double>(mid.z)},
        {OntoMath::kAmbientPointVar, mid},
    };

    for (const auto& piece : density.pieces) {
        if (!piece.mathNode) continue;   // the emitter skips these too
        // Midpoint witness: any sound interval enclosure over [lo, hi] must
        // enclose the midpoint value D(mid). If D(mid) > 1e-6, range->scalar.hi
        // cannot be <= 0, so we can reject before running 3D Perlin lattice
        // range sweeps.
        if (!piece.hasLo && !piece.hasHi && !piece.guard && !piece.whereLEZero &&
            !piece.call && !piece.fold) {
            const auto midVal = piece.mathNode->evaluate(midVars, nullptr);
            if (midVal && std::holds_alternative<double>(*midVal) &&
                std::get<double>(*midVal) > 1e-6) {
                return false;
            }
        }
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

// Fast O(1)-noise interval enclosure of an occupied leaf cell [lo, hi] using
// OntoMath::MathNode::evalFastRange (which bounds 3D Perlin noise via the O(1)
// global Lipschitz derivative ball around the cell midpoint).
OntoMath::Interval evalLeafDensityFastRange(const OntoMath::Piecewise& density,
                                            const glm::vec3& lo,
                                            const glm::vec3& hi) {
    using RV = OntoMath::MathNode::RangeValue;
    const OntoMath::Interval ix(lo.x, hi.x), iy(lo.y, hi.y), iz(lo.z, hi.z);
    const std::map<std::string, RV> vars = {
        {"x", RV::makeScalar(ix)},
        {"y", RV::makeScalar(iy)},
        {"z", RV::makeScalar(iz)},
        {OntoMath::kAmbientPointVar, RV::makeVector(ix, iy, iz)},
    };
    bool anyPiece = false;
    OntoMath::Interval hull(0.0f, 0.0f);
    for (const auto& piece : density.pieces) {
        if (!piece.mathNode) continue;
        if (piece.call || piece.fold) return OntoMath::Interval::infinite();
        const auto range = piece.mathNode->evalFastRange(vars);
        if (!range || range->kind != OntoMath::ValueKind::Scalar ||
            std::isnan(range->scalar.lo) || std::isnan(range->scalar.hi)) {
            return OntoMath::Interval::infinite();
        }
        if (!anyPiece) {
            hull = range->scalar;
            anyPiece = true;
        } else {
            hull = OntoMath::Interval(std::min(hull.lo, range->scalar.lo),
                                      std::max(hull.hi, range->scalar.hi));
        }
        if (!piece.hasLo && !piece.hasHi && !piece.guard && !piece.whereLEZero) {
            return hull;
        }
    }
    return hull;
}

struct Builder {
    const OntoMath::Piecewise& density;
    glm::vec3 halfExtent;
    glm::vec3 cellSize;
    glm::vec3 margin;
    VolumeZeroProof& out;

    void setAdaptiveCode(uint32_t index, uint32_t code) {
        const uint32_t wordIdx = index / VolumeZeroProof::kAdaptiveCellsPerWord;
        const uint32_t shift = (index % VolumeZeroProof::kAdaptiveCellsPerWord) * 2u;
        auto& word = out.adaptiveWords[wordIdx];
        const uint32_t bits =
            (static_cast<uint32_t>(word) & ~(3u << shift)) | ((code & 3u) << shift);
        word = static_cast<float>(bits);
    }

    void mark(glm::uvec3 a, glm::uvec3 b) {
        for (uint32_t z = a.z; z < b.z; ++z)
            for (uint32_t y = a.y; y < b.y; ++y)
                for (uint32_t x = a.x; x < b.x; ++x) {
                    const uint32_t index = x + out.dims.x * (y + out.dims.y * z);
                    auto& word = out.words[index / VolumeZeroProof::kBitsPerWord];
                    const uint32_t bits = static_cast<uint32_t>(word) |
                        (1u << (index % VolumeZeroProof::kBitsPerWord));
                    word = static_cast<float>(bits);
                    setAdaptiveCode(index, 3u);
                    ++out.provenCells;
                }
    }

    void certifyOccupiedLeaf(glm::uvec3 cell, const glm::vec3& lo, const glm::vec3& hi) {
        const uint32_t index = cell.x + out.dims.x * (cell.y + out.dims.y * cell.z);
        const OntoMath::Interval r = evalLeafDensityFastRange(density, lo, hi);
        if (!r.bounded() || std::isnan(r.lo) || std::isnan(r.hi)) {
            out.maxDensityBound = std::numeric_limits<float>::infinity();
            out.maxVariationBound = std::numeric_limits<float>::infinity();
            setAdaptiveCode(index, 0u);
            return;
        }
        const float dLo = std::max(0.0f, r.lo);
        const float dHi = std::max(0.0f, r.hi);
        const float variation = OntoMath::Interval::roundUp(dHi - dLo);
        out.maxDensityBound = std::max(out.maxDensityBound, dHi);
        out.maxVariationBound = std::max(out.maxVariationBound, variation);
        if (variation <= 0.025f) {
            setAdaptiveCode(index, 2u);
            ++out.certified4xCells;
        } else if (variation <= 0.10f) {
            setAdaptiveCode(index, 1u);
            ++out.certified2xCells;
        } else {
            setAdaptiveCode(index, 0u);
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
        if (span[axis] <= 1u) {
            certifyOccupiedLeaf(a, lo, hi);
            return;   // a single unproven cell, now variation-certified
        }
        glm::uvec3 mid = b;
        mid[axis] = a[axis] + span[axis] / 2u;
        glm::uvec3 start = a;
        start[axis] = mid[axis];
        prove(a, mid);
        prove(start, b);
    }
};

std::optional<OntoMath::MathNode::RangeValue> evalPiecewiseChannelRange(
    const OntoMath::Piecewise& expr,
    const std::map<std::string, OntoMath::MathNode::RangeValue>& vars,
    OntoMath::ValueKind expectedKind) {
    using RV = OntoMath::MathNode::RangeValue;
    bool anyPiece = false;
    RV hull = expectedKind == OntoMath::ValueKind::Vector
                  ? RV::makeVector(OntoMath::Interval(0.0f), OntoMath::Interval(0.0f), OntoMath::Interval(0.0f))
                  : RV::makeScalar(OntoMath::Interval(0.0f));
    for (const auto& piece : expr.pieces) {
        if (!piece.mathNode) continue;
        if (piece.call || piece.fold) return std::nullopt;
        const auto r = piece.mathNode->evalFastRange(vars);
        if (!r || r->kind != expectedKind) return std::nullopt;
        if (expectedKind == OntoMath::ValueKind::Scalar) {
            if (!r->scalar.bounded() || std::isnan(r->scalar.lo) || std::isnan(r->scalar.hi))
                return std::nullopt;
            if (!anyPiece) {
                hull = *r;
                anyPiece = true;
            } else {
                hull.scalar = OntoMath::Interval(std::min(hull.scalar.lo, r->scalar.lo),
                                                 std::max(hull.scalar.hi, r->scalar.hi));
            }
        } else {
            for (int c = 0; c < 3; ++c) {
                if (!r->vec[c].bounded() || std::isnan(r->vec[c].lo) || std::isnan(r->vec[c].hi))
                    return std::nullopt;
            }
            if (!anyPiece) {
                hull = *r;
                anyPiece = true;
            } else {
                for (int c = 0; c < 3; ++c) {
                    hull.vec[c] = OntoMath::Interval(std::min(hull.vec[c].lo, r->vec[c].lo),
                                                     std::max(hull.vec[c].hi, r->vec[c].hi));
                }
            }
        }
        if (!piece.hasLo && !piece.hasHi && !piece.guard && !piece.whereLEZero) {
            return hull;
        }
    }
    return hull;
}

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
    proof.adaptiveWords.assign(
        (proof.totalCells + VolumeZeroProof::kAdaptiveCellsPerWord - 1u) /
            VolumeZeroProof::kAdaptiveCellsPerWord,
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

VolumeCertifiedBounds computeVolumeCertifiedBounds(
    const VolumeZeroProof& proof,
    const OntoMath::Piecewise* densityExpr,
    const OntoMath::Piecewise* extinctionExpr,
    const OntoMath::Piecewise* scatteringExpr,
    const OntoMath::Piecewise* volumeChromaExpr,
    const OntoMath::Piecewise* phaseExpr,
    const OntoMath::Piecewise* emissionExpr,
    double mediumTime,
    float incidentRadianceBound) {
    VolumeCertifiedBounds out;
    if (proof.totalCells > 0u && proof.provenCells == proof.totalCells) {
        out.certified = true;
        return out;
    }
    if (!std::isfinite(incidentRadianceBound) || incidentRadianceBound < 0.0f) {
        return out;
    }
    const glm::vec3 lo = proof.supportMin;
    const glm::vec3 hi = proof.supportMax;
    if (glm::any(glm::greaterThan(lo, hi)) ||
        !std::isfinite(lo.x + lo.y + lo.z + hi.x + hi.y + hi.z)) {
        return out;
    }

    using RV = OntoMath::MathNode::RangeValue;
    const OntoMath::Interval ix(lo.x, hi.x), iy(lo.y, hi.y), iz(lo.z, hi.z);
    const float tFloat = static_cast<float>(mediumTime);
    const OntoMath::Interval it = std::isfinite(tFloat)
                                      ? OntoMath::Interval(tFloat, tFloat)
                                      : OntoMath::Interval::infinite();
    const OntoMath::Interval unitDir(-1.0f, 1.0f);
    std::map<std::string, RV> vars = {
        {"x", RV::makeScalar(ix)},
        {"y", RV::makeScalar(iy)},
        {"z", RV::makeScalar(iz)},
        {OntoMath::kAmbientPointVar, RV::makeVector(ix, iy, iz)},
        {OntoMath::kTimeVar, RV::makeScalar(it)},
        {OntoMath::kOmegaXVar, RV::makeScalar(unitDir)},
        {OntoMath::kOmegaYVar, RV::makeScalar(unitDir)},
        {OntoMath::kOmegaZVar, RV::makeScalar(unitDir)},
        {OntoMath::kWiXVar, RV::makeScalar(unitDir)},
        {OntoMath::kWiYVar, RV::makeScalar(unitDir)},
        {OntoMath::kWiZVar, RV::makeScalar(unitDir)},
        {OntoMath::kWoXVar, RV::makeScalar(unitDir)},
        {OntoMath::kWoYVar, RV::makeScalar(unitDir)},
        {OntoMath::kWoZVar, RV::makeScalar(unitDir)},
        {"omega", RV::makeVector(unitDir, unitDir, unitDir)},
        {"wi", RV::makeVector(unitDir, unitDir, unitDir)},
        {"wo", RV::makeVector(unitDir, unitDir, unitDir)},
    };

    float maxD = proof.maxDensityBound;
    if (!(maxD > 0.0f) || !std::isfinite(maxD)) {
        if (!densityExpr || densityExpr->pieces.empty()) return out;
        const auto dRange = evalPiecewiseChannelRange(*densityExpr, vars, OntoMath::ValueKind::Scalar);
        if (!dRange) return out;
        maxD = std::max(0.0f, dRange->scalar.hi);
    }
    if (!std::isfinite(maxD)) return out;
    out.maxDensity = maxD;
    vars["D"] = RV::makeScalar(OntoMath::Interval(0.0f, maxD));

    float maxExt = maxD;
    if (extinctionExpr && !extinctionExpr->pieces.empty()) {
        const auto extRange = evalPiecewiseChannelRange(*extinctionExpr, vars, OntoMath::ValueKind::Scalar);
        if (!extRange) return out;
        maxExt = std::max(0.0f, extRange->scalar.hi);
    }
    out.maxExtinction = maxExt;

    float maxScat = maxD;
    if (scatteringExpr && !scatteringExpr->pieces.empty()) {
        const auto scatRange = evalPiecewiseChannelRange(*scatteringExpr, vars, OntoMath::ValueKind::Scalar);
        if (!scatRange) return out;
        maxScat = std::max(0.0f, scatRange->scalar.hi);
    }

    float maxChroma = 1.0f;
    if (volumeChromaExpr && !volumeChromaExpr->pieces.empty()) {
        const auto chromaRange = evalPiecewiseChannelRange(*volumeChromaExpr, vars, OntoMath::ValueKind::Vector);
        if (!chromaRange) return out;
        maxChroma = std::max({0.0f, chromaRange->vec[0].hi, chromaRange->vec[1].hi, chromaRange->vec[2].hi});
    }

    float maxPhase = 1.0f;
    if (phaseExpr && !phaseExpr->pieces.empty()) {
        const auto phaseRange = evalPiecewiseChannelRange(*phaseExpr, vars, OntoMath::ValueKind::Scalar);
        if (!phaseRange) return out;
        maxPhase = std::max(0.0f, phaseRange->scalar.hi);
    }

    float maxEmit = 0.0f;
    if (emissionExpr && !emissionExpr->pieces.empty()) {
        const auto emitRange = evalPiecewiseChannelRange(*emissionExpr, vars, OntoMath::ValueKind::Vector);
        if (!emitRange) return out;
        maxEmit = std::max({0.0f, emitRange->vec[0].hi, emitRange->vec[1].hi, emitRange->vec[2].hi});
    }

    const OntoMath::Interval scatterRate =
        OntoMath::Interval(0.0f, incidentRadianceBound) *
        OntoMath::Interval(0.0f, maxScat) *
        OntoMath::Interval(0.0f, maxChroma) *
        OntoMath::Interval(0.0f, maxPhase);
    const OntoMath::Interval totalRate = scatterRate + OntoMath::Interval(0.0f, maxEmit);
    if (!totalRate.bounded() || std::isnan(totalRate.hi)) return out;

    out.maxSourceRate = OntoMath::Interval::roundUp(std::max(0.0f, totalRate.hi));
    const float varFactor = std::isfinite(proof.maxVariationBound) && maxD > 1e-6f
                                ? std::min(1.0f, proof.maxVariationBound / maxD)
                                : 1.0f;
    out.maxVariationRate = OntoMath::Interval::roundUp(out.maxSourceRate * std::max(varFactor, 0.05f));
    out.certified = std::isfinite(out.maxSourceRate) && std::isfinite(out.maxVariationRate);
    return out;
}

} // namespace Rendering
