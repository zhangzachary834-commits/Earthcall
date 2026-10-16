#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

// Zero-density proof for a participating medium.
//
// A medium's authored density D(p) is evaluated at every ray-march sample
// inside its box, and a sample whose D <= 0 contributes nothing at all (no
// extinction, no scattering, no emission). This module proves, ahead of time
// and from the authored mathematics alone, which cells of the medium's box can
// only ever produce D <= 0, so the shader can skip evaluating D there. Sample
// positions and counts are untouched; only provably-null work disappears.
// Prism Sun: "Keep the aurora. Kill only the work that reality cannot observe."
//
// Nothing here knows what any medium is. The proof is OntoMath's own interval
// arithmetic (MathNode::evalRange, outward-rounded) over boxes of local space.
// Anything it cannot bound -- an unbound variable, time, an unsupported op --
// is an unproven cell, and an unproven cell is evaluated exactly as before.
//
// Claude Opus 5.5 · Claude Code · 2026-10-09. Zach: "MAKE SURE UR PROOF
// STRUCTURE IS AS GENERALIZABLE AS POSSIBLE".
namespace Rendering {

struct VolumeZeroProof {
    // Kernel-only derived acceleration data, not authored medium state.
    // Cells along local x, y, z, tiling [-halfExtent, +halfExtent]. Each axis
    // is at most kMaxAxisCells so the shader can unpack it from 10 bits.
    glm::uvec3 dims{0u};
    // One bit per cell, x fastest, then y, then z: set = D <= 0 everywhere in
    // that cell (slightly enlarged, see buildVolumeZeroProof). Packed 24 bits
    // per float so every word is an exact f32 integer in the params buffer.
    std::vector<float> words;
    uint32_t provenCells = 0;
    uint32_t totalCells = 0;
    uint32_t rangeEvaluations = 0;
    // Conservative local envelope of all unproven cells, enlarged like the
    // cell proof. Reversed bounds mean every cell is proven empty. The GPU
    // uses this only to skip null samples on the original chord lattice;
    // these bounds must never replace the authored box or its quadrature.
    glm::vec3 supportMin{0.0f};
    glm::vec3 supportMax{0.0f};
    // Kernel-derived column index over the SAME cell proof. In each slab on
    // supportAxis, xy/zw enclose every unproven cell on the other two axes
    // (cyclic order); reversed bounds denote an empty slab. Chosen by minimum
    // enclosed volume, never by a medium's identity. No additional theorems,
    // denser grid, authored fields or changing sample positions.
    uint32_t supportAxis = 0u;
    std::vector<glm::vec4> supportBands;

    // Interval-Certified Adaptive Volumetric Quadrature (Aetherion, 2026-10-16):
    // 2-bit per-cell stride certificate packed 12 cells per 24-bit f32 integer
    // word (0 = 1x fine, 1 = 2x medium, 2 = 4x coarse, 3 = proven empty), derived
    // from OntoMath's outward-rounded interval enclosure and Lipschitz derivative
    // ball at each leaf cell during proof construction.
    std::vector<float> adaptiveWords;
    uint32_t certified2xCells = 0;
    uint32_t certified4xCells = 0;
    float maxDensityBound = 0.0f;
    float maxVariationBound = 0.0f;

    static constexpr uint32_t kBitsPerWord = 24u;
    static constexpr uint32_t kAdaptiveCellsPerWord = 12u;
    static constexpr uint32_t kMaxAxisCells = 1023u;
    bool any() const { return provenCells > 0; }
    bool proven(uint32_t x, uint32_t y, uint32_t z) const;
    uint32_t adaptiveStrideCode(uint32_t x, uint32_t y, uint32_t z) const;
    uint32_t adaptiveStride(uint32_t x, uint32_t y, uint32_t z) const;
};

struct VolumeCertifiedBounds {
    bool certified = false;
    float maxDensity = 0.0f;
    float maxExtinction = 0.0f;
    float maxSourceRate = 0.0f;
    float maxVariationRate = 0.0f;
};

// Computes Kernel-certified upper bounds on density, extinction, source
// radiance per unit length, and variation rate over the medium's certified
// support envelope using OntoMath interval arithmetic. Any unbounded or
// unprovable channel leaves certified = false so the shader refuses early
// tail termination and adaptive coarsening.
VolumeCertifiedBounds computeVolumeCertifiedBounds(
    const VolumeZeroProof& proof,
    const OntoMath::Piecewise* densityExpr,
    const OntoMath::Piecewise* extinctionExpr,
    const OntoMath::Piecewise* scatteringExpr,
    const OntoMath::Piecewise* volumeChromaExpr,
    const OntoMath::Piecewise* phaseExpr,
    const OntoMath::Piecewise* emissionExpr,
    double mediumTime = 0.0,
    float incidentRadianceBound = 1.0f);

// True only if every value the Piecewise can produce over the local box
// [lo, hi] is <= 0, mirroring the WGSL emitter: first matching piece wins,
// no match yields 0. Point variables p/x/y/z are bound to the box; every other
// variable (time included) is left unbounded, so it can only defeat a proof.
bool densityProvenNonPositive(const OntoMath::Piecewise& density,
                              const glm::vec3& lo, const glm::vec3& hi,
                              uint32_t* rangeEvaluations = nullptr);

// Builds the cell proof for a medium box of the given half extent. The grid
// is sized from the box's own proportions (about targetCells near-cubic
// cells); cells are proved coarse-to-fine so an empty region costs one range
// evaluation, not one per cell. Each cell is tested enlarged by
// marginFraction of its size so float rounding of the shader's sample point
// cannot land a sample just outside the region that was actually proved.
VolumeZeroProof buildVolumeZeroProof(const OntoMath::Piecewise& density,
                                     const glm::vec3& halfExtent,
                                     uint32_t targetCells = 32768u,
                                     float marginFraction = 0.01f);

} // namespace Rendering
