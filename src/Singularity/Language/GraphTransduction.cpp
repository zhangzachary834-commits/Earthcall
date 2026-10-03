#include "Singularity/Language/GraphTransduction.hpp"

#include <unordered_map>
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

bool GraphTransduction::sameKind(
    const Relation& relation,
    const Lexeme* kind,
    const std::string& legacyType) {

    if (kind) {
        return relation.type == kind->getIdentifier();
    }
    return !legacyType.empty() && relation.type == legacyType;
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

    const auto validateSources = [&](const std::vector<const Singular*>& sources,
                                     const char* what) -> std::string {
        std::unordered_set<const Singular*> seen;
        for (const Singular* source : sources) {
            if (!source) return std::string(what) + " contains a null source";
            if (!seen.insert(source).second) {
                return std::string(what) + " repeats source " + source->getIdentifier();
            }
            if (plan.sourceFormation && !plan.sourceFormation->hasMember(source)) {
                return std::string("source being is outside the declared source Formation: ") +
                       source->getIdentifier();
            }
        }
        if (!sources.empty() && !hasCorrespondenceKind) {
            return std::string(what) +
                   " has source meaning but no correspondence Relation kind; provenance would be lost";
        }
        return {};
    };

    for (const auto& occurrence : plan.occurrences) {
        if (occurrence.targetId.empty()) {
            out.refusal = "target occurrence identity is empty";
            return out;
        }
        if (!ids.insert(occurrence.targetId).second) {
            out.refusal = "duplicate target occurrence identity: " + occurrence.targetId;
            return out;
        }
        out.refusal = validateSources(occurrence.sources, "target occurrence");
        if (!out.refusal.empty()) return out;
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
        out.refusal = validateSources(relation.sources, "target Relation");
        if (!out.refusal.empty()) return out;
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
        for (const Singular* source : plan.occurrences[i].sources) {
            std::string refusal;
            auto relation = makeRelation(
                plan.correspondenceKind,
                plan.correspondenceLegacyType,
                *source,
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
    }

    for (std::size_t i = 0; i < plan.relations.size(); ++i) {
        for (const Singular* source : plan.relations[i].sources) {
            std::string refusal;
            auto relation = makeRelation(
                plan.correspondenceKind,
                plan.correspondenceLegacyType,
                *source,
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
    }

    out.ok = true;
    return out;
}

GraphTransduction::Result GraphTransduction::buildFromTemplate(
    const TemplateRequest& request) {

    Result refused;
    if (!request.requestFormation) {
        refused.refusal = "template request has no Formation";
        return refused;
    }
    if (!request.mappingKind && request.mappingLegacyType.empty()) {
        refused.refusal = "template request has no mapping Relation kind";
        return refused;
    }
    if (request.targetFormationId.empty()) {
        refused.refusal = "template target Formation identity is empty";
        return refused;
    }
    if (request.targetIdPrefix.empty()) {
        refused.refusal = "template target identity prefix is empty";
        return refused;
    }

    // Find source -> target-prototype edges. The target prototypes are ordinary
    // Lexemes inside the authored request graph; nothing here asks their
    // spelling what role they play.
    std::vector<const Lexeme*> prototypes;
    std::unordered_map<const Lexeme*, std::vector<const Singular*>> sourcesByPrototype;

    for (const auto& relation : request.requestFormation->relations().getAll()) {
        if (!relation || !sameKind(*relation, request.mappingKind, request.mappingLegacyType)) {
            continue;
        }
        if (!relation->directed) {
            refused.refusal = "template mapping Relation must be directed";
            return refused;
        }
        const Singular* source = relation->a();
        const auto* prototype = dynamic_cast<const Lexeme*>(relation->b());
        if (!source || !prototype) {
            refused.refusal = "template mapping must point from a source being to a target Lexeme prototype";
            return refused;
        }
        if (!request.requestFormation->hasMember(source) ||
            !request.requestFormation->hasMember(prototype)) {
            refused.refusal = "template mapping endpoint is outside the request Formation";
            return refused;
        }

        auto& sources = sourcesByPrototype[prototype];
        if (sources.empty()) prototypes.push_back(prototype);
        sources.push_back(source);
    }

    if (prototypes.empty()) {
        refused.refusal = "template request contains no source -> target prototype mappings";
        return refused;
    }

    Plan plan;
    plan.sourceFormation = request.requestFormation;
    plan.targetFormationId = request.targetFormationId;
    plan.correspondenceKind = request.mappingKind;
    plan.correspondenceLegacyType = request.mappingLegacyType;

    std::unordered_map<const Singular*, std::size_t> indexByPrototype;
    for (const Lexeme* prototype : prototypes) {
        const std::size_t index = plan.occurrences.size();
        indexByPrototype[prototype] = index;
        plan.occurrences.push_back({
            sourcesByPrototype[prototype],
            request.targetIdPrefix + prototype->getIdentifier(),
            prototype->getSymbol()
        });

        if (request.rootPrototype == prototype) {
            plan.hasRoot = true;
            plan.rootOccurrence = index;
        }
    }

    if (request.rootPrototype && !plan.hasRoot) {
        refused.refusal = "template root prototype is not mapped by the request";
        return refused;
    }

    // Every non-mapping Relation whose two endpoints are target prototypes is
    // itself target structure. Preserve its authored kind, direction, weight,
    // and provenance by mapping the source Relation being to the cloned target
    // Relation.
    for (const auto& relation : request.requestFormation->relations().getAll()) {
        if (!relation || sameKind(*relation, request.mappingKind, request.mappingLegacyType)) {
            continue;
        }
        const auto from = indexByPrototype.find(relation->a());
        const auto to = indexByPrototype.find(relation->b());
        if (from == indexByPrototype.end() || to == indexByPrototype.end()) {
            continue;
        }

        RelationSpec spec;
        spec.sources.push_back(relation.get());
        spec.from = from->second;
        spec.to = to->second;
        spec.kind = relation->getTypeLexeme();
        if (!spec.kind) spec.legacyType = relation->type;
        spec.directed = relation->directed;
        spec.weight = relation->getWeight();
        plan.relations.push_back(std::move(spec));
    }

    return build(plan);
}

} // namespace Language
} // namespace Singularity
