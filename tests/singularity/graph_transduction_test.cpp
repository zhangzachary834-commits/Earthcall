#include "Singularity/Language/GraphTransduction.hpp"

#include <cassert>
#include <iostream>
#include <memory>

using Singularity::Language::GraphTransduction;
using Singularity::Language::Lexeme;

int main() {
    std::cout << "Running graph_transduction_test..." << std::endl;

    // Source meaning: an already-individuated semantic Formation. It is not
    // HTML and remains untouched by the manifestation.
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
        {&page,    "html.node.section", "section"},
        {&heading, "html.node.heading", "h1"},
        {&title,   "html.node.title",   "Welcome"},
        {&cardA,   "html.node.card.a",  "article"},
        {&cardB,   "html.node.card.b",  "article"},
    };

    plan.relations = {
        {nullptr, 1, 0, &domChildOf, {}, true, 1.0f},
        {nullptr, 2, 1, &domChildOf, {}, true, 1.0f},
        {nullptr, 3, 0, &domChildOf, {}, true, 1.0f},
        {nullptr, 4, 0, &domChildOf, {}, true, 1.0f},
        {nullptr, 1, 3, &domNextSibling, {}, true, 1.0f},
        {nullptr, 3, 4, &domNextSibling, {}, true, 1.0f},
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

    // Every sourced occurrence has an explicit source -> target provenance
    // Relation; the target structure itself is a separate graph.
    assert(result.correspondenceRelations.size() == plan.occurrences.size());
    assert(result.targetRelations.size() == plan.relations.size());

    for (const auto& relation : result.correspondenceRelations) {
        assert(relation);
        assert(relation->directed);
        assert(relation->typeLabel() == "manifests-as");
    }

    // The HTML-shaped target uses the same Relation spelling the DOM mirror
    // already projects from browser state, proving the two directions can meet
    // in one native graph vocabulary without making HTML an Earthcall kind.
    assert(result.targetRelations.front()->typeLabel() == "dom-child-of");
    assert(result.targetRelations.front()->aId() == "html.node.heading");
    assert(result.targetRelations.front()->bId() == "html.node.section");

    // Transactional refusal: duplicate target identity is rejected before any
    // target Formation exists.
    GraphTransduction::Plan bad = plan;
    bad.occurrences[4].targetId = bad.occurrences[3].targetId;
    auto refused = GraphTransduction::build(bad);
    assert(!refused);
    assert(!refused.targetFormation);
    assert(refused.targetLexemes.empty());
    assert(refused.refusal.find("duplicate target occurrence identity") != std::string::npos);

    std::cout << "graph_transduction_test passed" << std::endl;
    return 0;
}
