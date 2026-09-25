#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/RadianceSource.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using OntoMath::MathNode;
using OntoMath::Piecewise;
using OntoMath::ScalarForm;

enum class Channel : uint8_t {
    SourceRho,
    MediumDensity,
};

enum class Action : uint8_t {
    Exact,
    SkipZeroRadianceContribution,
    SkipZeroDensitySupport,
};

struct Admission {
    std::string producerId;
    Channel channel = Channel::SourceRho;
    uint64_t authoredRevision = 0;
    const Piecewise* expr = nullptr;
    uint64_t expectedArtifactGeneration = 0;
};

struct ArtifactEntry {
    std::string producerId;
    Channel channel = Channel::SourceRho;
    uint64_t authoredRevision = 0;
    uint64_t generation = 0;
    bool exactLiteralZero = false;
};

struct Accounting {
    uint64_t dispatchLookups = 0;
    uint64_t metadataTests = 0;
    uint64_t exactEvaluationsAvoided = 0;
    uint64_t exactFallbacks = 0;

    // These remain explicit zeroes. The witness has no code path that can
    // perform relevance discovery before indexing artifact[knownSlot].
    uint64_t recordScans = 0;
    uint64_t hierarchyWalks = 0;
    uint64_t hashProbes = 0;
    uint64_t spatialSearches = 0;

    uint64_t fullBuilds = 0;
    uint64_t localRepairs = 0;
    uint64_t buildNanoseconds = 0;
    uint64_t repairNanoseconds = 0;
    size_t artifactBytes = 0;
};

std::shared_ptr<MathNode> scalarNode(double value) {
    auto n = std::make_shared<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = ScalarForm::constant(value);
    return n;
}

Piecewise scalarPiecewise(double value) {
    return Piecewise::continuous(scalarNode(value));
}

// Deliberately tiny theorem family. Build-time inspection is allowed; the hot
// dispatch path below never re-opens Piecewise and never classifies position.
bool exactEverywhereLiteralZero(const Piecewise& model) {
    if (model.pieces.size() != 1) return false;
    const auto& piece = model.pieces.front();
    if (piece.hasLo || piece.hasHi || piece.guard || piece.whereLEZero ||
        piece.call || piece.fold || !piece.mathNode) {
        return false;
    }
    if (piece.mathNode->op != MathNode::Op::ScalarLeaf) return false;

    const auto& terms = piece.mathNode->scalarForm.terms;
    if (terms.size() != 1) return false;
    const auto& term = terms.front();
    if (!term.factors.empty() || !term.trans.empty()) return false;
    return term.coefficient == 0.0;
}

class DirectDispatchTable {
public:
    explicit DirectDispatchTable(Accounting& accounting)
        : _accounting(accounting) {}

    std::vector<uint64_t> build(const std::vector<Admission>& admissions) {
        const auto t0 = std::chrono::steady_clock::now();
        _entries.clear();
        _entries.reserve(admissions.size());

        std::vector<uint64_t> generations;
        generations.reserve(admissions.size());

        for (const auto& admission : admissions) {
            ArtifactEntry entry;
            entry.producerId = admission.producerId;
            entry.channel = admission.channel;
            entry.authoredRevision = admission.authoredRevision;
            entry.generation = ++_nextGeneration;
            entry.exactLiteralZero =
                admission.expr && exactEverywhereLiteralZero(*admission.expr);
            generations.push_back(entry.generation);
            _entries.push_back(std::move(entry));
        }

        const auto t1 = std::chrono::steady_clock::now();
        ++_accounting.fullBuilds;
        _accounting.buildNanoseconds +=
            static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                    .count());
        refreshByteAccounting();
        return generations;
    }

    uint64_t repair(size_t knownSlot, const Admission& admission) {
        assert(knownSlot < _entries.size());
        const auto t0 = std::chrono::steady_clock::now();

        ArtifactEntry replacement;
        replacement.producerId = admission.producerId;
        replacement.channel = admission.channel;
        replacement.authoredRevision = admission.authoredRevision;
        replacement.generation = ++_nextGeneration;
        replacement.exactLiteralZero =
            admission.expr && exactEverywhereLiteralZero(*admission.expr);
        _entries[knownSlot] = std::move(replacement);

        const auto t1 = std::chrono::steady_clock::now();
        ++_accounting.localRepairs;
        _accounting.repairNanoseconds +=
            static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                    .count());
        refreshByteAccounting();
        return _entries[knownSlot].generation;
    }

    uint64_t generation(size_t knownSlot) const {
        assert(knownSlot < _entries.size());
        return _entries[knownSlot].generation;
    }

    Action dispatch(size_t knownSlot, const Admission& admission) {
        ++_accounting.dispatchLookups;

        if (knownSlot >= _entries.size()) {
            ++_accounting.exactFallbacks;
            return Action::Exact;
        }

        // Fixed provenance gate. Exactly four comparisons are performed for a
        // valid slot, independent of theorem count, scene size, or world size.
        const auto& entry = _entries[knownSlot];
        const bool producerMatches = entry.producerId == admission.producerId;
        const bool channelMatches = entry.channel == admission.channel;
        const bool revisionMatches =
            entry.authoredRevision == admission.authoredRevision;
        const bool generationMatches =
            entry.generation == admission.expectedArtifactGeneration;
        _accounting.metadataTests += 4;

        if (!producerMatches || !channelMatches || !revisionMatches ||
            !generationMatches || !entry.exactLiteralZero) {
            ++_accounting.exactFallbacks;
            return Action::Exact;
        }

        ++_accounting.exactEvaluationsAvoided;
        return entry.channel == Channel::SourceRho
            ? Action::SkipZeroRadianceContribution
            : Action::SkipZeroDensitySupport;
    }

private:
    void refreshByteAccounting() {
        size_t bytes = sizeof(ArtifactEntry) * _entries.size();
        for (const auto& entry : _entries) bytes += entry.producerId.size();
        _accounting.artifactBytes = bytes;
    }

    Accounting& _accounting;
    std::vector<ArtifactEntry> _entries;
    uint64_t _nextGeneration = 0;
};

Admission admitted(std::string producerId, Channel channel, uint64_t revision,
                   const Piecewise* expr) {
    Admission a;
    a.producerId = std::move(producerId);
    a.channel = channel;
    a.authoredRevision = revision;
    a.expr = expr;
    return a;
}

Admission admitted(const Rendering::RadianceSourceBinding& source) {
    return admitted(source.producerId, Channel::SourceRho,
                    source.radianceRevision, source.radianceExpr);
}

Admission admitted(const Rendering::VolumeDensityBinding& medium) {
    return admitted(medium.producerId, Channel::MediumDensity,
                    medium.densityRevision, medium.densityExpr);
}

} // namespace

int main() {
    Piecewise zeroA = scalarPiecewise(0.0);
    Piecewise zeroB = scalarPiecewise(0.0); // byte-identical math, distinct vessel
    Piecewise nonZero = scalarPiecewise(3.0);
    Piecewise emission = scalarPiecewise(7.0);

    Accounting accounting;
    DirectDispatchTable table(accounting);

    Rendering::RadianceSourceBinding sourceBinding;
    sourceBinding.producerId = "field/source-A";
    sourceBinding.radianceExpr = &zeroA;
    sourceBinding.radianceRevision = 101;

    Rendering::VolumeDensityBinding mediumBinding;
    mediumBinding.producerId = "field/medium-A";
    mediumBinding.densityExpr = &zeroB;
    mediumBinding.densityRevision = 202;
    mediumBinding.emissionExpr = &emission;
    mediumBinding.emissionRevision = 404;
    geom::SdfNode occluder;
    mediumBinding.occluderSdf = &occluder;
    mediumBinding.occluderRevision = 505;

    Rendering::RadianceSourceBinding neighborBinding;
    neighborBinding.producerId = "field/source-neighbor";
    neighborBinding.radianceExpr = &nonZero;
    neighborBinding.radianceRevision = 303;

    Admission source = admitted(sourceBinding);
    Admission medium = admitted(mediumBinding);
    Admission neighbor = admitted(neighborBinding);

    auto generations = table.build({source, medium, neighbor});
    source.expectedArtifactGeneration = generations[0];
    medium.expectedArtifactGeneration = generations[1];
    neighbor.expectedArtifactGeneration = generations[2];

    // Byte-identical rho and density math MUST retain channel-specific action.
    assert(table.dispatch(0, source) == Action::SkipZeroRadianceContribution);
    assert(table.dispatch(1, medium) == Action::SkipZeroDensitySupport);
    assert(table.dispatch(2, neighbor) == Action::Exact);

    // Same numeric slot, different producer: slot identity is not lifetime
    // identity. The stale artifact must fail open.
    Admission producerReplacement = source;
    producerReplacement.producerId = "field/source-B";
    assert(table.dispatch(0, producerReplacement) == Action::Exact);

    // Authored revision mutation must fail open until local repair publishes a
    // new artifact generation.
    Admission revisedSource = source;
    revisedSource.authoredRevision = 102;
    assert(table.dispatch(0, revisedSource) == Action::Exact);

    // Deliberately stale artifact generation must fail open.
    Admission staleGeneration = source;
    staleGeneration.expectedArtifactGeneration += 999;
    assert(table.dispatch(0, staleGeneration) == Action::Exact);

    // A theorem from another semantic channel is never authority.
    Admission channelMismatch = source;
    channelMismatch.channel = Channel::MediumDensity;
    assert(table.dispatch(0, channelMismatch) == Action::Exact);

    // Removal/re-addition with a reused numeric slot but a new producer identity
    // must not inherit the removed producer's theorem.
    Admission readded =
        admitted("field/source-A/readded", Channel::SourceRho, 101, &zeroA);
    readded.expectedArtifactGeneration = source.expectedArtifactGeneration;
    assert(table.dispatch(0, readded) == Action::Exact);

    // Local repair mutates exactly one slot. The unaffected neighbor retains its
    // artifact identity and therefore its cached state.
    const uint64_t neighborGenerationBefore = table.generation(2);
    revisedSource.expectedArtifactGeneration = table.repair(0, revisedSource);
    assert(table.generation(2) == neighborGenerationBefore);
    assert(table.dispatch(0, revisedSource) ==
           Action::SkipZeroRadianceContribution);
    assert(table.dispatch(2, neighbor) == Action::Exact);

    // Density-zero authority is intentionally narrow. V4 self-emission and the
    // independent participating-medium occluder lane remain present and
    // untouched even when the density action says exact density evaluation can
    // be skipped.
    const auto* emissionBefore = mediumBinding.emissionExpr;
    const uint64_t emissionRevisionBefore = mediumBinding.emissionRevision;
    const auto* occluderBefore = mediumBinding.occluderSdf;
    const uint64_t occluderRevisionBefore = mediumBinding.occluderRevision;

    assert(table.dispatch(1, medium) == Action::SkipZeroDensitySupport);
    assert(mediumBinding.emissionExpr == emissionBefore);
    assert(mediumBinding.emissionRevision == emissionRevisionBefore);
    assert(mediumBinding.occluderSdf == occluderBefore);
    assert(mediumBinding.occluderRevision == occluderRevisionBefore);

    // The hot-path claim is structural and explicit: no relevance-search
    // counters exist behind dispatch[knownSlot].
    assert(accounting.recordScans == 0);
    assert(accounting.hierarchyWalks == 0);
    assert(accounting.hashProbes == 0);
    assert(accounting.spatialSearches == 0);

    assert(accounting.fullBuilds == 1);
    assert(accounting.localRepairs == 1);
    assert(accounting.dispatchLookups == 11);
    assert(accounting.metadataTests == 44);
    assert(accounting.exactEvaluationsAvoided == 4);
    assert(accounting.exactFallbacks == 7);
    assert(accounting.artifactBytes > 0);

    std::printf(
        "DIRECT_DISPATCH PASS lookups=%llu metadata_tests=%llu "
        "exact_avoided=%llu fallbacks=%llu record_scans=%llu "
        "hierarchy_walks=%llu hash_probes=%llu spatial_searches=%llu "
        "build_ns=%llu repair_ns=%llu artifact_bytes=%zu\n",
        static_cast<unsigned long long>(accounting.dispatchLookups),
        static_cast<unsigned long long>(accounting.metadataTests),
        static_cast<unsigned long long>(accounting.exactEvaluationsAvoided),
        static_cast<unsigned long long>(accounting.exactFallbacks),
        static_cast<unsigned long long>(accounting.recordScans),
        static_cast<unsigned long long>(accounting.hierarchyWalks),
        static_cast<unsigned long long>(accounting.hashProbes),
        static_cast<unsigned long long>(accounting.spatialSearches),
        static_cast<unsigned long long>(accounting.buildNanoseconds),
        static_cast<unsigned long long>(accounting.repairNanoseconds),
        accounting.artifactBytes);

    return 0;
}
