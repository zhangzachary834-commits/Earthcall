#include "Singularity/Language/GraphTransduction.hpp"

#include <cassert>
#include <iostream>
#include <memory>

using Singularity::Language::GraphTransduction;
using Singularity::Language::Lexeme;

int main() {
    std::cout << "Running graph_transduction_test..." << std::endl;

    // Source meaning: already-individuated semantic beings.
    Lexeme page("page", "semantic.page");
    Lexeme heading("heading", "semantic.heading");
    Lexeme title("Welcome", "semantic.title");
    Lexeme cardA("card", "semantic.card.a");
    Lexeme cardB("card", "semantic.card.b");

    Formation source;
    assert(source.setIdentifier("semantic.page.formation"));
    assert(source.setRoot(&page));
    source.addMember(&heading);
    source.addMember(&title);
    source.addMember(&cardA);
    source.addMember(&cardB);

    // Authored relation-kind beings. The mechanism does not own the semantics
    // of these spellings.
    Lexeme manifestsAs("manifests-as", "relation-kind.manifests-as");
    Lexeme domChildOf("dom-child-of", "relation-kind.dom-child-of");
    Lexeme domNextSibling("dom-next-sibling", "relation-kind.dom-next-sibling");

    GraphTransduction::Plan plan;
    plan.sourceFormation = &source;
    plan.targetFormationId = "html.page.formation";
    plan.correspondenceKind = &manifestsAs;
    plan.hasRoot = true;
    plan.rootOccurrence = 0;

    plan.occurrences = {
        {{&page},    "html.node.section", "section"},
        {{&heading}, "html.node.heading", "h1"},
        {{&title},   "html.node.title",   "Welcome"},
        {{&cardA},   "html.node.card.a",  "article"},
        {{&cardB},   "html.node.card.b",  "article"},
    };

    plan.relations = {
        {{}, 1, 0, &domChildOf, {}, true, 1.0f},
        {{}, 2, 1, &domChildOf, {}, true, 1.0f},
        {{}, 3, 0, &domChildOf, {}, true, 1.0f},
        {{}, 4, 0, &domChildOf, {}, true, 1.0f},
        {{}, 1, 3, &domNextSibling, {}, true, 1.0f},
        {{}, 3, 4, &domNextSibling, {}, true, 1.0f},
    };

    auto result = GraphTransduction::build(plan);
    assert(result);
    assert(result.refusal.empty());
    assert(result.targetFormation);
    assert(result.targetFormation->getIdentifier() == "html.page.formation");
    assert(result.targetFormation->root());
    assert(result.targetFormation->root()->getIdentifier() == "html.node.section");

    // Same spelling is not identity. Two card meanings can manifest as two
    // separate <article> occurrences without collapsing.
    auto a = result.findTarget("html.node.card.a");
    auto b = result.findTarget("html.node.card.b");
    assert(a && b);
    assert(a.get() != b.get());
    assert(a->getSymbol() == "article");
    assert(b->getSymbol() == "article");
    assert(a->getIdentifier() != b->getIdentifier());

    // Source and target remain different beings.
    assert(cardA.getIdentifier() == "semantic.card.a");
    assert(a->getIdentifier() == "html.node.card.a");
    assert(&cardA != static_cast<Singular*>(a.get()));

    assert(result.correspondenceRelations.size() == plan.occurrences.size());
    assert(result.targetRelations.size() == plan.relations.size());

    for (const auto& relation : result.correspondenceRelations) {
        assert(relation);
        assert(relation->directed);
        assert(relation->typeLabel() == "manifests-as");
    }

    assert(result.targetRelations.front()->typeLabel() == "dom-child-of");
    assert(result.targetRelations.front()->aId() == "html.node.heading");
    assert(result.targetRelations.front()->bId() == "html.node.section");

    // Many source meanings may lawfully converge into one target occurrence,
    // while provenance remains one edge per source.
    GraphTransduction::Plan composite;
    composite.sourceFormation = &source;
    composite.targetFormationId = "html.composite.formation";
    composite.correspondenceKind = &manifestsAs;
    composite.occurrences = {
        {{&heading, &title}, "html.composite.heading", "h1"}
    };
    auto compositeResult = GraphTransduction::build(composite);
    assert(compositeResult);
    assert(compositeResult.correspondenceRelations.size() == 2);

    // Transactional refusal: duplicate target identity is rejected before any
    // target Formation exists.
    GraphTransduction::Plan bad = plan;
    bad.occurrences[4].targetId = bad.occurrences[3].targetId;
    auto refused = GraphTransduction::build(bad);
    assert(!refused);
    assert(!refused.targetFormation);
    assert(refused.targetLexemes.empty());
    assert(refused.refusal.find("duplicate target occurrence identity") != std::string::npos);

    // Rung 1: the mapping itself can be an ordinary authored Formation.
    // Source beings point to target prototype Lexemes; structural Relations
    // among prototypes become target topology.
    Lexeme protoSection("section", "prototype.html.section");
    Lexeme protoHeading("h1", "prototype.html.heading");
    Lexeme protoTitle("Welcome", "prototype.html.title");
    Lexeme protoCardA("article", "prototype.html.card.a");
    Lexeme protoCardB("article", "prototype.html.card.b");

    Formation authoredRequest;
    assert(authoredRequest.setIdentifier("request.semantic-to-html"));
    for (Singular* s : std::vector<Singular*>{
             &page, &heading, &title, &cardA, &cardB,
             &protoSection, &protoHeading, &protoTitle, &protoCardA, &protoCardB}) {
        authoredRequest.addMember(s);
    }

    const auto addMapping = [&](Singular& sourceBeing, Lexeme& prototype) {
        auto relation = std::make_shared<Relation>(
            manifestsAs, sourceBeing, prototype, true, 1.0f);
        assert(authoredRequest.addRelation(relation));
    };
    addMapping(page, protoSection);
    addMapping(heading, protoHeading);
    addMapping(title, protoTitle);
    addMapping(cardA, protoCardA);
    addMapping(cardB, protoCardB);

    const auto addTargetEdge = [&](Lexeme& from, Lexeme& to, Lexeme& kind) {
        auto relation = std::make_shared<Relation>(
            kind, from, to, true, 1.0f);
        assert(authoredRequest.addRelation(relation));
    };
    addTargetEdge(protoHeading, protoSection, domChildOf);
    addTargetEdge(protoTitle, protoHeading, domChildOf);
    addTargetEdge(protoCardA, protoSection, domChildOf);
    addTargetEdge(protoCardB, protoSection, domChildOf);
    addTargetEdge(protoHeading, protoCardA, domNextSibling);
    addTargetEdge(protoCardA, protoCardB, domNextSibling);

    GraphTransduction::TemplateRequest templateRequest;
    templateRequest.requestFormation = &authoredRequest;
    templateRequest.mappingKind = &manifestsAs;
    templateRequest.targetFormationId = "html.authored.formation";
    templateRequest.targetIdPrefix = "live.";
    templateRequest.rootPrototype = &protoSection;

    auto authored = GraphTransduction::buildFromTemplate(templateRequest);
    assert(authored);
    assert(authored.targetFormation);
    assert(authored.targetFormation->root());
    assert(authored.targetFormation->root()->getIdentifier() ==
           "live.prototype.html.section");

    auto authoredA = authored.findTarget("live.prototype.html.card.a");
    auto authoredB = authored.findTarget("live.prototype.html.card.b");
    assert(authoredA && authoredB);
    assert(authoredA->getSymbol() == "article");
    assert(authoredB->getSymbol() == "article");
    assert(authoredA->getIdentifier() != authoredB->getIdentifier());

    assert(authored.targetRelations.size() == 6);
    // 5 source->target occurrence correspondences + 6 source-Relation ->
    // target-Relation correspondences.
    assert(authored.correspondenceRelations.size() == 11);

    std::cout << "graph_transduction_test passed" << std::endl;
    return 0;
}
