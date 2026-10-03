#include "Singularity/Language/GraphTransduction.hpp"

#include <unordered_set>

namespace Singularity {
namespace Language {

std::shared_ptr<Lexeme> GraphTransduction::Result::findTarget(const std::string& id) const {
    for (const auto& lexeme : targetLexemes) {
        if (lexeme && lexeme->getIdentifier() == id) return lexeme;
    }
    return nullptr;
}

std::shared_ptr<Relation> GraphTransduction::makeRelation(
    Lexeme* kind,
    const std::string& legacyType,
    const Singular& a,
    const Singular& b,
    bool directed,
    float weight,
    std::string& refusal) {

    if (kind) {
        return std::make_shared<Relation>(*kind, a, b, directed, weight);
    }
    if (legacyType.empty()) {
        refusal = "relation kind is empty (supply an authored kind Lexeme or legacy type)";
        return nullptr;
    }
    return std::make_shared<Relation>(legacyType, a, b, directed, weight);
}

GraphTransduction::Result GraphTransduction::build(const Plan& plan) {
    Result out;

    if (plan.targetFormationId.empty()) {
        out.refusal = "target Formation identity is empty";
        return out;
    }
    if (plan.occurrences.empty()) {
        out.refusal = "a transduction needs at least one target occurrence";
        return out;
    }
    if (plan.hasRoot && plan.rootOccurrence >= plan.occurrences.size()) {
        out.refusal = "target root occurrence is out of range";
        return out;
    }

    const bool hasCorrespondenceKind =
        plan.correspondenceKind || !plan.correspondenceLegacyType.empty();

    std::unordered_set<std::string> ids;
    ids.reserve(plan.occurrences.size());

    for (const auto& occurrence : plan.occurrences) {
        if (occurrence.targetId.empty()) {
            out.refusal = "target occurrence identity is empty";
            return out;
        }
        if (!ids.insert(occurrence.targetId).second) {
            out.refusal = "duplicate target occurrence identity: " + occurrence.targetId;
            return out;
        }
        if (plan.sourceFormation && occurrence.source &&
            !plan.sourceFormation->hasMember(occurrence.source)) {
            out.refusal = "source being is outside the declared source Formation: " +
                          occurrence.source->getIdentifier();
            return out;
        }
        if (occurrence.source && !hasCorrespondenceKind) {
            out.refusal =
                "source/target correspondence has no Relation kind; provenance would be lost";
            return out;
        }
    }

    for (const auto& relation : plan.relations) {
        if (relation.from >= plan.occurrences.size() ||
            relation.to >= plan.occurrences.size()) {
            out.refusal = "target Relation endpoint index is out of range";
            return out;
        }
        if (relation.from == relation.to) {
            out.refusal = "target Relation would self-ground one occurrence";
            return out;
        }
        if (!relation.kind && relation.legacyType.empty()) {
            out.refusal = "target Relation has no authored kind";
            return out;
        }
        if (relation.source && !hasCorrespondenceKind) {
            out.refusal =
                "source/target Relation correspondence has no Relation kind; provenance would be lost";
            return out;
        }
    }

    // Construct the whole target while detached. No global registry can see a
    // half-built manifestation.
    out.targetLexemes.reserve(plan.occurrences.size());
    for (const auto& occurrence : plan.occurrences) {
        out.targetLexemes.push_back(
            std::make_shared<Lexeme>(occurrence.symbol, occurrence.targetId));
    }

    out.targetFormation = std::make_shared<Formation>();
    if (!out.targetFormation->setIdentifier(plan.targetFormationId)) {
        out.refusal = "target Formation refused its identity";
        out.targetFormation.reset();
        out.targetLexemes.clear();
        return out;
    }

    for (const auto& lexeme : out.targetLexemes) {
        out.targetFormation->addMember(lexeme.get());
    }

    if (plan.hasRoot &&
        !out.targetFormation->setRoot(out.targetLexemes[plan.rootOccurrence].get())) {
        out.refusal = "target Formation refused its root";
        out.targetFormation.reset();
        out.targetLexemes.clear();
        return out;
    }

    out.targetRelations.reserve(plan.relations.size());
    for (const auto& spec : plan.relations) {
        std::string refusal;
        auto relation = makeRelation(
            spec.kind,
            spec.legacyType,
            *out.targetLexemes[spec.from],
            *out.targetLexemes[spec.to],
            spec.directed,
            spec.weight,
            refusal);
        if (!relation || !out.targetFormation->addRelation(relation)) {
            out.refusal = !refusal.empty()
                ? refusal
                : "target Formation refused Relation " +
                  out.targetLexemes[spec.from]->getIdentifier() + " -> " +
                  out.targetLexemes[spec.to]->getIdentifier();
            out.targetFormation.reset();
            out.targetRelations.clear();
            out.targetLexemes.clear();
            out.correspondenceRelations.clear();
            return out;
        }
        out.targetRelations.push_back(relation);
    }

    // Build source -> target provenance only after the target graph itself is
    // coherent. These Relations remain separate from the target Formation:
    // correspondence connects two ontological layers; it is not a DOM/member
    // edge inside the target manifestation.
    for (std::size_t i = 0; i < plan.occurrences.size(); ++i) {
        const auto& spec = plan.occurrences[i];
        if (!spec.source) continue;

        std::string refusal;
        auto relation = makeRelation(
            plan.correspondenceKind,
            plan.correspondenceLegacyType,
            *spec.source,
            *out.targetLexemes[i],
            true,
            1.0f,
            refusal);
        if (!relation) {
            out.refusal = refusal;
            out.targetFormation.reset();
            out.targetRelations.clear();
            out.targetLexemes.clear();
            out.correspondenceRelations.clear();
            return out;
        }
        out.correspondenceRelations.push_back(relation);
    }

    for (std::size_t i = 0; i < plan.relations.size(); ++i) {
        const auto& spec = plan.relations[i];
        if (!spec.source) continue;

        std::string refusal;
        auto relation = makeRelation(
            plan.correspondenceKind,
            plan.correspondenceLegacyType,
            *spec.source,
            *out.targetRelations[i],
            true,
            1.0f,
            refusal);
        if (!relation) {
            out.refusal = refusal;
            out.targetFormation.reset();
            out.targetRelations.clear();
            out.targetLexemes.clear();
            out.correspondenceRelations.clear();
            return out;
        }
        out.correspondenceRelations.push_back(relation);
    }

    out.ok = true;
    return out;
}

} // namespace Language
} // namespace Singularity
