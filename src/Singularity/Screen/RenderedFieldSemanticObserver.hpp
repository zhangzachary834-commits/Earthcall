#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/RadianceSource.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"

#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Rendering {

// Phase-B diagnostic seam for rendered-field semantic synthesis.
//
// IMPORTANT: this observer has no API that returns theorem authority to the
// renderer. It may classify and count conservative opportunities, but it cannot
// alter pixels, shader control flow, marching, accumulation, or visibility.
class RenderedFieldSemanticObserver {
public:
    enum class Channel : uint8_t { SourceRho, MediumDensity };
    enum class ProofKind : uint8_t {
        None,
        RadianceZeroContribution,
        DensityZeroSupport
    };

    struct Stats {
        uint64_t radianceSetRevisionHits = 0;
        uint64_t densitySetRevisionHits = 0;
        uint64_t vesselObservations = 0;
        uint64_t semanticBuilds = 0;
        uint64_t semanticCacheHits = 0;
        uint64_t canonicalMathBuilds = 0;
        uint64_t canonicalMathHits = 0;
        uint64_t theoremBuilds = 0;
        uint64_t theoremRefusals = 0;
        uint64_t hypotheticalRadianceBypasses = 0;
        uint64_t hypotheticalDensityBypasses = 0;
        uint64_t authorityBypassesApplied = 0;

        // Rung 7 successor: diagnostic artifacts aligned 1:1 with the already-
        // admitted renderer binding slots. Build/repair happens only at source-
        // set admission; no pixel path consumes these artifacts.
        uint64_t alignedSlotBuilds = 0;
        uint64_t alignedSlotRepairs = 0;
        uint64_t alignedSlotReuses = 0;
        uint64_t alignedSlotDrops = 0;
        size_t alignedSlotLogicalBytes = 0;
        uint64_t alignedHandlePublications = 0;
        uint64_t alignedHandleValidations = 0;
        uint64_t alignedHandleMetadataTests = 0;
        uint64_t alignedHandleFallbacks = 0;
    };

    struct AlignedSlotHandle {
        Channel channel = Channel::SourceRho;
        size_t slot = 0;
        uint64_t generation = 0;
    };

    void setEnabled(bool enabled) { _enabled = enabled; }
    bool enabled() const { return _enabled; }

    const Stats& stats() const { return _stats; }

    // Generation-only diagnostics let tests prove local slot repair without
    // exposing a theorem-consumption API to renderer control flow.
    size_t alignedRadianceSlotCount() const { return _radianceSlots.size(); }
    size_t alignedDensitySlotCount() const { return _densitySlots.size(); }
    uint64_t alignedRadianceSlotGeneration(size_t slot) const {
        return slot < _radianceSlots.size() ? _radianceSlots[slot].generation : 0;
    }
    uint64_t alignedDensitySlotGeneration(size_t slot) const {
        return slot < _densitySlots.size() ? _densitySlots[slot].generation : 0;
    }

    std::optional<AlignedSlotHandle> publishRadianceHandle(size_t slot) {
        return publishAlignedHandle(_radianceSlots, slot, Channel::SourceRho);
    }
    std::optional<AlignedSlotHandle> publishDensityHandle(size_t slot) {
        return publishAlignedHandle(_densitySlots, slot, Channel::MediumDensity);
    }

    bool validateRadianceHandle(
        const AlignedSlotHandle& handle,
        const RadianceSourceBinding& binding) {
        return validateAlignedHandle(
            _radianceSlots, handle, Channel::SourceRho,
            binding.producerId, binding.radianceRevision);
    }

    bool validateDensityHandle(
        const AlignedSlotHandle& handle,
        const VolumeDensityBinding& binding) {
        return validateAlignedHandle(
            _densitySlots, handle, Channel::MediumDensity,
            binding.producerId, binding.densityRevision);
    }

    void reset() {
        _stats = {};
        _vessels.clear();
        _canonicalLiteralMath.clear();
        _radianceSlots.clear();
        _densitySlots.clear();
        _nextAlignedGeneration = 0;
        _haveRadianceSetRevision = false;
        _haveDensitySetRevision = false;
        _lastRadianceSetRevision = 0;
        _lastDensitySetRevision = 0;
        _lastRadianceSetSize = 0;
        _lastDensitySetSize = 0;
    }

    void observeRadianceSources(
        const std::vector<RadianceSourceBinding>& sources,
        uint64_t setRevision) {
        if (!_enabled) return;
        if (_haveRadianceSetRevision &&
            _lastRadianceSetRevision == setRevision &&
            _lastRadianceSetSize == sources.size()) {
            ++_stats.radianceSetRevisionHits;
            return;
        }

        _haveRadianceSetRevision = true;
        _lastRadianceSetRevision = setRevision;
        _lastRadianceSetSize = sources.size();
        _stats.hypotheticalRadianceBypasses = 0;

        for (size_t slot = 0; slot < sources.size(); ++slot) {
            const auto& source = sources[slot];
            ProofKind proof = ProofKind::None;
            if (source.radianceExpr) {
                proof = observeVessel(
                    Channel::SourceRho, source.radianceExpr, source.radianceRevision);
                if (proof == ProofKind::RadianceZeroContribution)
                    ++_stats.hypotheticalRadianceBypasses;
            }
            updateAlignedSlot(
                _radianceSlots, slot, source.producerId, Channel::SourceRho,
                source.radianceRevision, proof);
        }
        shrinkAlignedSlots(_radianceSlots, sources.size());
        refreshAlignedSlotLogicalBytes();
    }

    void observeVolumeDensitySources(
        const std::vector<VolumeDensityBinding>& sources,
        uint64_t setRevision) {
        if (!_enabled) return;
        if (_haveDensitySetRevision &&
            _lastDensitySetRevision == setRevision &&
            _lastDensitySetSize == sources.size()) {
            ++_stats.densitySetRevisionHits;
            return;
        }

        _haveDensitySetRevision = true;
        _lastDensitySetRevision = setRevision;
        _lastDensitySetSize = sources.size();
        _stats.hypotheticalDensityBypasses = 0;

        for (size_t slot = 0; slot < sources.size(); ++slot) {
            const auto& medium = sources[slot];
            ProofKind proof = ProofKind::None;
            if (medium.densityExpr) {
                proof = observeVessel(
                    Channel::MediumDensity, medium.densityExpr, medium.densityRevision);
                if (proof == ProofKind::DensityZeroSupport)
                    ++_stats.hypotheticalDensityBypasses;
            }
            updateAlignedSlot(
                _densitySlots, slot, medium.producerId, Channel::MediumDensity,
                medium.densityRevision, proof);
        }
        shrinkAlignedSlots(_densitySlots, sources.size());
        refreshAlignedSlotLogicalBytes();
    }

private:
    struct AlignedSlotArtifact {
        std::string producerId;
        Channel channel = Channel::SourceRho;
        uint64_t authoredRevision = 0;
        uint64_t generation = 0;
        ProofKind proof = ProofKind::None;
    };

    void updateAlignedSlot(
        std::vector<AlignedSlotArtifact>& slots,
        size_t slot,
        const std::string& producerId,
        Channel channel,
        uint64_t authoredRevision,
        ProofKind proof) {
        if (slot < slots.size()) {
            auto& existing = slots[slot];
            if (existing.producerId == producerId &&
                existing.channel == channel &&
                existing.authoredRevision == authoredRevision) {
                ++_stats.alignedSlotReuses;
                return;
            }

            existing = AlignedSlotArtifact{
                producerId, channel, authoredRevision,
                ++_nextAlignedGeneration, proof};
            ++_stats.alignedSlotRepairs;
            return;
        }

        // The artifact vector is always grown in the same order as the admitted
        // binding vector. There is no identity lookup or theorem search here.
        slots.push_back(AlignedSlotArtifact{
            producerId, channel, authoredRevision,
            ++_nextAlignedGeneration, proof});
        ++_stats.alignedSlotBuilds;
    }

    void shrinkAlignedSlots(
        std::vector<AlignedSlotArtifact>& slots,
        size_t admittedSize) {
        if (slots.size() <= admittedSize) return;
        _stats.alignedSlotDrops += slots.size() - admittedSize;
        slots.resize(admittedSize);
    }

    void refreshAlignedSlotLogicalBytes() {
        size_t bytes =
            sizeof(AlignedSlotArtifact) *
            (_radianceSlots.size() + _densitySlots.size());
        for (const auto& slot : _radianceSlots) bytes += slot.producerId.size();
        for (const auto& slot : _densitySlots) bytes += slot.producerId.size();
        _stats.alignedSlotLogicalBytes = bytes;
    }
    struct VesselKey {
        Channel channel = Channel::SourceRho;
        const OntoMath::Piecewise* expr = nullptr;
        uint64_t revision = 0;

        bool operator==(const VesselKey& other) const {
            return channel == other.channel && expr == other.expr &&
                   revision == other.revision;
        }
    };

    struct VesselKeyHash {
        size_t operator()(const VesselKey& key) const {
            size_t h = std::hash<const void*>{}(key.expr);
            h ^= std::hash<uint64_t>{}(key.revision) +
                 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            h ^= static_cast<size_t>(key.channel) +
                 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            return h;
        }
    };

    // Deliberately tiny first production theorem surface. We prove only an
    // everywhere-defined scalar literal. Anything richer remains UNKNOWN and
    // therefore has zero optimization authority.
    static std::optional<double> exactEverywhereScalarLiteral(
        const OntoMath::Piecewise& model) {
        if (model.pieces.size() != 1) return std::nullopt;
        const auto& piece = model.pieces.front();
        if (piece.hasLo || piece.hasHi || piece.guard || piece.whereLEZero ||
            piece.call || piece.fold || !piece.mathNode)
            return std::nullopt;
        if (piece.mathNode->op != OntoMath::MathNode::Op::ScalarLeaf)
            return std::nullopt;

        const auto& terms = piece.mathNode->scalarForm.terms;
        if (terms.size() != 1) return std::nullopt;
        const auto& term = terms.front();
        if (!term.factors.empty() || !term.trans.empty()) return std::nullopt;
        return term.coefficient;
    }

    static uint64_t literalIdentity(double value) {
        // +0 and -0 are the same exact scalar truth for this theorem family.
        if (value == 0.0) return 0;
        uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }

    ProofKind observeVessel(
        Channel channel,
        const OntoMath::Piecewise* expr,
        uint64_t revision) {
        ++_stats.vesselObservations;

        const VesselKey key{channel, expr, revision};
        if (auto it = _vessels.find(key); it != _vessels.end()) {
            ++_stats.semanticCacheHits;
            return it->second;
        }

        ++_stats.semanticBuilds;
        ProofKind proof = ProofKind::None;
        const auto literal = exactEverywhereScalarLiteral(*expr);
        if (!literal.has_value()) {
            ++_stats.theoremRefusals;
        } else {
            const uint64_t canonical = literalIdentity(*literal);
            if (_canonicalLiteralMath.insert(canonical).second)
                ++_stats.canonicalMathBuilds;
            else
                ++_stats.canonicalMathHits;

            if (*literal == 0.0) {
                proof = channel == Channel::SourceRho
                    ? ProofKind::RadianceZeroContribution
                    : ProofKind::DensityZeroSupport;
                ++_stats.theoremBuilds;
            } else {
                ++_stats.theoremRefusals;
            }
        }

        _vessels.emplace(key, proof);
        return proof;
    }

    bool _enabled = false;
    bool _haveRadianceSetRevision = false;
    bool _haveDensitySetRevision = false;
    uint64_t _lastRadianceSetRevision = 0;
    uint64_t _lastDensitySetRevision = 0;
    size_t _lastRadianceSetSize = 0;
    size_t _lastDensitySetSize = 0;

    Stats _stats;
    std::unordered_map<VesselKey, ProofKind, VesselKeyHash> _vessels;
    std::unordered_set<uint64_t> _canonicalLiteralMath;
    std::vector<AlignedSlotArtifact> _radianceSlots;
    std::vector<AlignedSlotArtifact> _densitySlots;
    uint64_t _nextAlignedGeneration = 0;
};

} // namespace Rendering
