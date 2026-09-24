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
    };

    void setEnabled(bool enabled) { _enabled = enabled; }
    bool enabled() const { return _enabled; }

    const Stats& stats() const { return _stats; }

    void reset() {
        _stats = {};
        _vessels.clear();
        _canonicalLiteralMath.clear();
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

        for (const auto& source : sources) {
            if (!source.radianceExpr) continue;
            const ProofKind proof = observeVessel(
                Channel::SourceRho, source.radianceExpr, source.radianceRevision);
            if (proof == ProofKind::RadianceZeroContribution)
                ++_stats.hypotheticalRadianceBypasses;
        }
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

        for (const auto& medium : sources) {
            if (!medium.densityExpr) continue;
            const ProofKind proof = observeVessel(
                Channel::MediumDensity, medium.densityExpr, medium.densityRevision);
            if (proof == ProofKind::DensityZeroSupport)
                ++_stats.hypotheticalDensityBypasses;
        }
    }

private:
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
};

} // namespace Rendering
