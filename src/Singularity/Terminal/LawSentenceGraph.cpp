#include "Singularity/Terminal/LawSentenceGraph.hpp"

#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace Singularity {
namespace Terminal {
namespace LawSentenceGraph {

namespace {

using Singularity::Language::Lexeme;
using Singularity::Language::LanguageSystem;

struct OccurrenceSeed {
    std::size_t start = 0;
    std::size_t end = 0;
    std::string role;
    std::string opcode;
    std::string lexemeId;
    std::string lawId;
    const LawSentence::Ambiguity* ambiguity = nullptr;
};

std::shared_ptr<Relation> groundedRelation(
    Lexeme* kind,
    Singular& a,
    Singular& b,
    bool directed,
    std::string& refusal) {

    if (!kind) {
        refusal = "LawSentenceGraph requires an authored Relation-kind Lexeme";
        return nullptr;
    }
    return std::make_shared<Relation>(*kind, a, b, directed, 1.0f);
}

void writeOccurrenceProperties(
    Lexeme& occurrence,
    const OccurrenceSeed& seed) {

    occurrence.setDynamicProperty("language.start", PropertyValue(static_cast<int>(seed.start)));
    occurrence.setDynamicProperty("language.end", PropertyValue(static_cast<int>(seed.end)));
    occurrence.setDynamicProperty("language.role", PropertyValue(seed.role));

    if (!seed.opcode.empty()) {
        occurrence.setDynamicProperty("language.opcode", PropertyValue(seed.opcode));
    }
    if (!seed.lexemeId.empty()) {
        occurrence.setDynamicProperty("language.lexemeId", PropertyValue(seed.lexemeId));
    }
    if (!seed.lawId.empty()) {
        occurrence.setDynamicProperty("language.lawId", PropertyValue(seed.lawId));
    }

    if (seed.ambiguity) {
        occurrence.setDynamicProperty("language.ambiguous", PropertyValue(true));
        occurrence.setDynamicProperty(
            "language.ambiguitySlot", PropertyValue(seed.ambiguity->slot));

        auto list = std::make_shared<PropertyList>();
        for (const auto& candidate : seed.ambiguity->candidates) {
            list->elements.emplace_back(candidate.individual());
        }
        occurrence.setDynamicProperty("language.candidates", PropertyValue(list));
    }
}

} // namespace

Result project(const std::string& text,
               const LawSentence::Vocabulary& vocabulary,
               const std::string& utteranceId,
               const Kinds& kinds) {
    Result out;
    if (utteranceId.empty()) {
        out.refusal = "utterance identity is empty";
        return out;
    }
    if (!kinds.next || !kinds.denotes) {
        out.refusal = "lexical next/denotes Relation-kind Lexemes are required";
        return out;
    }

    out.parse = LawSentence::parse(text, vocabulary);

    std::vector<OccurrenceSeed> seeds;
    seeds.reserve(out.parse.spans.size() + out.parse.ambiguities.size());

    // Every recognized interval becomes its own occurrence. Error colour is a
    // diagnostic overlay, not a semantic occurrence.
    for (const auto& span : out.parse.spans) {
        if (span.end <= span.start || span.end > text.size() || span.role == "error") continue;
        seeds.push_back({
            span.start, span.end, span.role,
            span.opcode, span.lexemeId, span.lawId, nullptr
        });
    }

    // Attach every ambiguity to the occurrence covering its exact interval.
    // An unresolved ambiguity throws before LawSentence can mark a chosen
    // Span, so create the missing occurrence rather than losing the plurality.
    for (const auto& ambiguity : out.parse.ambiguities) {
        auto it = std::find_if(seeds.begin(), seeds.end(), [&](const OccurrenceSeed& seed) {
            return seed.start == ambiguity.start && seed.end == ambiguity.end;
        });
        if (it == seeds.end()) {
            seeds.push_back({
                ambiguity.start,
                ambiguity.end,
                ambiguity.slot,
                {},
                {},
                {},
                &ambiguity
            });
        } else {
            it->ambiguity = &ambiguity;
        }
    }

    std::stable_sort(seeds.begin(), seeds.end(), [](const OccurrenceSeed& a, const OccurrenceSeed& b) {
        if (a.start != b.start) return a.start < b.start;
        if (a.end != b.end) return a.end < b.end;
        return a.role < b.role;
    });

    // Do not mint two occurrence beings for the same parser interval/role.
    seeds.erase(std::unique(seeds.begin(), seeds.end(), [](const OccurrenceSeed& a, const OccurrenceSeed& b) {
        return a.start == b.start && a.end == b.end && a.role == b.role;
    }), seeds.end());

    if (seeds.empty()) {
        out.refusal = "the utterance produced no lexical occurrences";
        return out;
    }

    out.lexicalFormation = std::make_shared<Formation>();
    out.denotationFormation = std::make_shared<Formation>();
    if (!out.lexicalFormation->setIdentifier(utteranceId + ".lexical") ||
        !out.denotationFormation->setIdentifier(utteranceId + ".denotation")) {
        out.refusal = "could not name utterance graph layers";
        return out;
    }

    for (std::size_t i = 0; i < seeds.size(); ++i) {
        const auto& seed = seeds[i];
        if (seed.end > text.size() || seed.start >= seed.end) {
            out.refusal = "parser returned an invalid occurrence interval";
            return out;
        }

        auto occurrence = std::make_shared<Lexeme>(
            text.substr(seed.start, seed.end - seed.start),
            utteranceId + ".occ." + std::to_string(i));
        writeOccurrenceProperties(*occurrence, seed);

        out.lexicalFormation->addMember(occurrence.get());
        out.denotationFormation->addMember(occurrence.get());
        out.occurrences.push_back(std::move(occurrence));
    }

    // Exact lexical order is graph structure, not implied by vector position.
    for (std::size_t i = 1; i < out.occurrences.size(); ++i) {
        std::string refusal;
        auto relation = groundedRelation(
            kinds.next,
            *out.occurrences[i - 1],
            *out.occurrences[i],
            true,
            refusal);
        if (!relation || !out.lexicalFormation->addRelation(relation)) {
            out.refusal = refusal.empty() ? "lexical Formation refused sequence Relation" : refusal;
            return out;
        }
        out.lexicalRelations.push_back(std::move(relation));
    }

    std::map<std::string, std::shared_ptr<Lexeme>> pinned;
    auto pinMeaning = [&](const std::string& id) -> std::shared_ptr<Lexeme> {
        if (id.empty()) return nullptr;
        const auto existing = pinned.find(id);
        if (existing != pinned.end()) return existing->second;
        auto meaning = LanguageSystem::instance().findById(id);
        if (meaning) {
            pinned[id] = meaning;
            out.meaningLexemes.push_back(meaning);
            out.denotationFormation->addMember(meaning.get());
        }
        return meaning;
    };

    // Chosen denotations from ordinary parser spans.
    for (std::size_t i = 0; i < seeds.size(); ++i) {
        const auto& seed = seeds[i];
        if (seed.lexemeId.empty()) continue;

        auto meaning = pinMeaning(seed.lexemeId);
        if (!meaning) {
            out.refusal = "chosen Lexeme is no longer live: " + seed.lexemeId;
            return out;
        }

        std::string refusal;
        auto relation = groundedRelation(
            kinds.denotes, *out.occurrences[i], *meaning, true, refusal);
        if (!relation || !out.denotationFormation->addRelation(relation)) {
            out.refusal = refusal.empty() ? "denotation Formation refused chosen meaning" : refusal;
            return out;
        }
        out.denotationRelations.push_back(std::move(relation));
    }

    // Plural denotations. Even when a Metalaw chose one, keep the alternatives
    // legible; resolution narrows action, it does not erase what was ambiguous.
    for (std::size_t i = 0; i < seeds.size(); ++i) {
        const auto* ambiguity = seeds[i].ambiguity;
        if (!ambiguity) continue;
        if (!kinds.candidateDenotation) {
            out.refusal = "ambiguity exists but no candidate-denotation Relation kind was supplied";
            return out;
        }

        std::set<std::string> emitted;
        for (const auto& candidate : ambiguity->candidates) {
            if (candidate.lexemeId.empty() || !emitted.insert(candidate.lexemeId).second) continue;
            auto meaning = pinMeaning(candidate.lexemeId);
            if (!meaning) {
                out.refusal = "ambiguous Lexeme candidate is no longer live: " + candidate.lexemeId;
                return out;
            }

            std::string refusal;
            auto relation = groundedRelation(
                kinds.candidateDenotation,
                *out.occurrences[i],
                *meaning,
                true,
                refusal);
            if (!relation || !out.denotationFormation->addRelation(relation)) {
                out.refusal = refusal.empty() ? "denotation Formation refused ambiguity candidate" : refusal;
                return out;
            }
            out.denotationRelations.push_back(std::move(relation));
        }
    }

    out.ok = true;
    return out;
}

} // namespace LawSentenceGraph
} // namespace Terminal
} // namespace Singularity
