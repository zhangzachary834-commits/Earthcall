#pragma once

#include "Singularity/Screen/RadianceSource.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Rendering {

// Observation-only accounting for semantic metadata aligned to renderer slots.
// It does not evaluate pixels or return rendering decisions.
class KnownExecutionSlotDiagnostics {
public:
    enum class Channel : uint8_t { SourceRho, MediumDensity };

    struct Stats {
        uint64_t builds = 0;
        uint64_t repairs = 0;
        uint64_t directChecks = 0;
        uint64_t metadataTests = 0;
        uint64_t fallbackChecks = 0;
        size_t residentBytes = 0;
    };

    const Stats& stats() const { return _stats; }

    void observeRadiance(const std::vector<RadianceSourceBinding>& sources) {
        refresh(_radiance, sources, Channel::SourceRho,
                [](const auto& v) -> const std::string& { return v.producerId; },
                [](const auto& v) { return v.radianceRevision; });
        refreshResidentBytes();
    }

    void observeDensity(const std::vector<VolumeDensityBinding>& media) {
        refresh(_density, media, Channel::MediumDensity,
                [](const auto& v) -> const std::string& { return v.producerId; },
                [](const auto& v) { return v.densityRevision; });
        refreshResidentBytes();
    }

    uint64_t radianceGeneration(size_t slot) const {
        return slot < _radiance.size() ? _radiance[slot].generation : 0;
    }

    uint64_t densityGeneration(size_t slot) const {
        return slot < _density.size() ? _density[slot].generation : 0;
    }

    void checkRadiance(size_t slot, const RadianceSourceBinding& source,
                       uint64_t expectedGeneration) {
        check(_radiance, slot, source.producerId, Channel::SourceRho,
              source.radianceRevision, expectedGeneration);
    }

    void checkDensity(size_t slot, const VolumeDensityBinding& medium,
                      uint64_t expectedGeneration) {
        check(_density, slot, medium.producerId, Channel::MediumDensity,
              medium.densityRevision, expectedGeneration);
    }

private:
    struct Entry {
        std::string producerId;
        Channel channel = Channel::SourceRho;
        uint64_t revision = 0;
        uint64_t generation = 0;
    };

    template <typename Binding, typename IdFn, typename RevisionFn>
    void refresh(std::vector<Entry>& entries,
                 const std::vector<Binding>& bindings,
                 Channel channel,
                 IdFn idOf,
                 RevisionFn revisionOf) {
        std::vector<Entry> next;
        next.reserve(bindings.size());

        for (size_t i = 0; i < bindings.size(); ++i) {
            const auto& binding = bindings[i];
            const std::string& producerId = idOf(binding);
            const uint64_t revision = revisionOf(binding);

            if (i < entries.size() &&
                entries[i].producerId == producerId &&
                entries[i].channel == channel &&
                entries[i].revision == revision) {
                next.push_back(entries[i]);
                continue;
            }

            Entry entry;
            entry.producerId = producerId;
            entry.channel = channel;
            entry.revision = revision;
            entry.generation = ++_nextGeneration;
            next.push_back(std::move(entry));

            if (entries.empty()) ++_stats.builds;
            else ++_stats.repairs;
        }

        entries = std::move(next);
    }

    void check(const std::vector<Entry>& entries,
               size_t slot,
               const std::string& producerId,
               Channel channel,
               uint64_t revision,
               uint64_t expectedGeneration) {
        ++_stats.directChecks;
        if (slot >= entries.size()) {
            ++_stats.fallbackChecks;
            return;
        }

        const auto& entry = entries[slot];
        const bool producerMatches = entry.producerId == producerId;
        const bool channelMatches = entry.channel == channel;
        const bool revisionMatches = entry.revision == revision;
        const bool generationMatches = entry.generation == expectedGeneration;
        _stats.metadataTests += 4;

        if (!producerMatches || !channelMatches || !revisionMatches ||
            !generationMatches)
            ++_stats.fallbackChecks;
    }

    void refreshResidentBytes() {
        size_t bytes = sizeof(Entry) * (_radiance.size() + _density.size());
        for (const auto& entry : _radiance) bytes += entry.producerId.size();
        for (const auto& entry : _density) bytes += entry.producerId.size();
        _stats.residentBytes = bytes;
    }

    Stats _stats;
    uint64_t _nextGeneration = 0;
    std::vector<Entry> _radiance;
    std::vector<Entry> _density;
};

} // namespace Rendering
