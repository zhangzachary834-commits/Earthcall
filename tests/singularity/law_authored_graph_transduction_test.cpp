#include "Singularity/Language/GraphTransduction.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

using Singularity::Language::GraphTransduction;
using Singularity::Language::LanguageSystem;
using Singularity::Language::Lexeme;

int main() {
    std::cout << "Running law_authored_graph_transduction_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    {
        Zone zone("transduction-zone", "default");
        Object author;
        author.setObjectID("author.transduction");

        Formation request;
        assert(request.setIdentifier("request.semantic-to-html"));

        Lexeme page("page", "semantic.page");
        Lexeme heading("heading", "semantic.heading");
        Lexeme title("Welcome", "semantic.title");
        Lexeme cardA("card", "semantic.card.a");
        Lexeme cardB("card", "semantic.card.b");

        Lexeme protoSection("section", "prototype.html.section");
        Lexeme protoHeading("h1", "prototype.html.heading");
        Lexeme protoTitle("Welcome", "prototype.html.title");
        Lexeme protoCardA("article", "prototype.html.card.a");
        Lexeme protoCardB("article", "prototype.html.card.b");

        auto manifestsAs = language.intern(
            "manifests-as", "relation-kind.manifests-as");
        auto domChildOf = language.intern(
            "dom-child-of", "relation-kind.dom-child-of");
        assert(manifestsAs && domChildOf);

        std::vector<Singular*> beings{
            &zone, &author, &request,
            &page, &heading, &title, &cardA, &cardB,
            &protoSection, &protoHeading, &protoTitle, &protoCardA, &protoCardB,
            manifestsAs.get(), domChildOf.get()
        };

        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            out.insert(out.end(), beings.begin(), beings.end());
        });
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& relation : zone.getFormation().relations().getAll()) {
                if (relation) out.push_back(relation.get());
            }
        });
        Universe::instance().setRelationGenerationProvider(
            [&]() { return zone.getFormation().relations().generation(); });
        Universe::instance().setRelationRegistrar(
            [&](std::shared_ptr<Relation> relation) {
                assert(relation);
                const bool admitted = zone.getFormation().addRelation(relation);
                assert(admitted);
            });

        std::vector<ActionNode> actions;
        for (Singular* member : std::vector<Singular*>{
                 &page, &heading, &title, &cardA, &cardB,
                 &protoSection, &protoHeading, &protoTitle, &protoCardA, &protoCardB}) {
            actions.push_back(ActionNode::addElement("", member->getIdentifier()));
        }

        const std::string mappingKind = "@" + manifestsAs->getIdentifier();
        const std::string childKind = "@" + domChildOf->getIdentifier();

        actions.push_back(ActionNode::addRelation(
            page.getIdentifier(), protoSection.getIdentifier(), mappingKind, true));
        actions.push_back(ActionNode::addRelation(
            heading.getIdentifier(), protoHeading.getIdentifier(), mappingKind, true));
        actions.push_back(ActionNode::addRelation(
            title.getIdentifier(), protoTitle.getIdentifier(), mappingKind, true));
        actions.push_back(ActionNode::addRelation(
            cardA.getIdentifier(), protoCardA.getIdentifier(), mappingKind, true));
        actions.push_back(ActionNode::addRelation(
            cardB.getIdentifier(), protoCardB.getIdentifier(), mappingKind, true));

        actions.push_back(ActionNode::addRelation(
            protoHeading.getIdentifier(), protoSection.getIdentifier(), childKind, true));
        actions.push_back(ActionNode::addRelation(
            protoTitle.getIdentifier(), protoHeading.getIdentifier(), childKind, true));
        actions.push_back(ActionNode::addRelation(
            protoCardA.getIdentifier(), protoSection.getIdentifier(), childKind, true));
        actions.push_back(ActionNode::addRelation(
            protoCardB.getIdentifier(), protoSection.getIdentifier(), childKind, true));

        Law authorTemplate("author semantic-to-html template");
        authorTemplate.addAuthor(author);
        authorTemplate.setActionModel(ActionNode::sequence(std::move(actions)));

        // This is not merely an in-memory C++ executor witness. The Law is
        // serialized and restored first; persisted authored action text must
        // retain Formation membership, grounded relation-kind identity, and
        // direction.
        const nlohmann::json savedLaw = authorTemplate.toJson();
        auto restored = Law::fromJson(savedLaw);
        assert(restored);
        restored->addAuthor(author);
        assert(restored->applyTo(request) == Law::ApplicationResult::Applied);

        assert(request.hasMember(&page));
        assert(request.hasMember(&protoSection));
        assert(request.hasMember(&protoCardA));
        assert(request.hasMember(&protoCardB));

        std::size_t mappingEdges = 0;
        std::size_t childEdges = 0;
        for (const auto& relation : request.relations().getAll()) {
            assert(relation);
            assert(relation->directed);
            assert(relation->hasGroundedType());
            if (relation->getTypeLexeme() == manifestsAs.get()) ++mappingEdges;
            if (relation->getTypeLexeme() == domChildOf.get()) ++childEdges;
        }
        assert(mappingEdges == 5);
        assert(childEdges == 4);

        GraphTransduction::TemplateRequest authoredRequest;
        authoredRequest.requestFormation = &request;
        authoredRequest.mappingKind = manifestsAs.get();
        authoredRequest.targetFormationId = "html.from.authored.law";
        authoredRequest.targetIdPrefix = "live.";
        authoredRequest.rootPrototype = &protoSection;

        auto result = GraphTransduction::buildFromTemplate(authoredRequest);
        assert(result);
        assert(result.targetFormation);
        assert(result.targetFormation->root());
        assert(result.targetFormation->root()->getSymbol() == "section");

        auto liveA = result.findTarget("live.prototype.html.card.a");
        auto liveB = result.findTarget("live.prototype.html.card.b");
        assert(liveA && liveB);
        assert(liveA->getSymbol() == "article");
        assert(liveB->getSymbol() == "article");
        assert(liveA->getIdentifier() != liveB->getIdentifier());

        assert(result.targetRelations.size() == 4);
        assert(result.correspondenceRelations.size() == 9);

        // Exact Relation-kind identity survives the whole chain:
        // Law -> request Formation -> transduced target Formation.
        for (const auto& relation : result.targetRelations) {
            assert(relation);
            assert(relation->getTypeLexeme() == domChildOf.get());
            assert(relation->directed);
        }

        Universe::instance().setRelationRegistrar({});
        Universe::instance().setRelationGenerationProvider({});
        Universe::instance().setRelationProvider({});
        Universe::instance().setProvider({});
    }

    language.clear();

    std::cout << "law_authored_graph_transduction_test passed" << std::endl;
    return 0;
}
