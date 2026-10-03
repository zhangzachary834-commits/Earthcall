#include "Singularity/Terminal/LawSentenceGraph.hpp"
#include "Singularity/Language/GraphTransduction.hpp"
#include "Singularity/Foreign/Web/HtmlFormationActPlanner.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace Singularity::Terminal;
using Singularity::Language::GraphTransduction;
using Singularity::Language::LanguageSystem;
using Singularity::Language::Lexeme;
using Singularity::Foreign::Web::DomActKind;
using Singularity::Foreign::Web::HtmlFormationActPlanner;

namespace {

std::shared_ptr<Lexeme> semanticNode(
    const LawSentenceGraph::Result& graph,
    const std::string& nodeType,
    int kind,
    int occurrence = 0) {

    for (const auto& node : graph.semanticLexemes) {
        if (!node) continue;
        PropertyValue type;
        PropertyValue nodeKind;
        if (!node->getDynamicProperty("semantic.nodeType", type) ||
            !node->getDynamicProperty("semantic.kind", nodeKind) ||
            !std::holds_alternative<std::string>(type) ||
            !std::holds_alternative<int>(nodeKind) ||
            std::get<std::string>(type) != nodeType ||
            std::get<int>(nodeKind) != kind) {
            continue;
        }
        if (occurrence-- == 0) return node;
    }
    return nullptr;
}

} // namespace

int main() {
    std::cout << "Running language_to_html_transduction_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    {
        // -------------------- Language-side authored vocabulary --------------------
        auto crimson = language.intern("crimson", "lexeme.value.crimson");
        auto lexicalNext = language.intern("next", "relation-kind.lexical-next");
        auto denotes = language.intern("denotes", "relation-kind.occurrence-denotes");
        auto candidate = language.intern(
            "candidate-denotation", "relation-kind.candidate-denotation");
        auto semanticChild = language.intern(
            "semantic-child", "relation-kind.semantic-child");
        auto expresses = language.intern(
            "expresses", "relation-kind.lexical-expresses-semantic");

        // -------------------- Manifestation-side authored vocabulary ---------------
        auto manifestsAs = language.intern(
            "manifests-as", "relation-kind.manifests-as");
        auto domChildOf = language.intern(
            "dom-child-of", "relation-kind.dom-child-of");
        auto domNextSibling = language.intern(
            "dom-next-sibling", "relation-kind.dom-next-sibling");

        assert(crimson && lexicalNext && denotes && candidate &&
               semanticChild && expresses && manifestsAs &&
               domChildOf && domNextSibling);

        LawSentence::Vocabulary vocabulary;
        vocabulary.words = LawSentence::canonicalWords();
        vocabulary.words.push_back({
            "crimson",
            "value",
            crimson->getIdentifier(),
            "law.value.crimson",
            "crimson value",
            "authored Lexeme value"
        });
        LawSentence::Preset crimsonPreset;
        crimsonPreset.lawId = "law.value.crimson";
        crimsonPreset.value = PropertyValue(std::string("crimson"));
        vocabulary.presets.push_back(crimsonPreset);
        vocabulary.resolve = [](const LawSentence::Ambiguity&) {
            return LawSentence::Resolution{};
        };

        LawSentenceGraph::Kinds languageKinds{
            lexicalNext.get(),
            denotes.get(),
            candidate.get(),
            semanticChild.get(),
            expresses.get()
        };

        // One Terminal-modality utterance becomes three explicit graph layers.
        auto utterance = LawSentenceGraph::project(
            "then set theme to crimson, add emphasis by 2?",
            vocabulary,
            "utterance.html-witness",
            languageKinds);
        assert(utterance);
        assert(utterance.parse.ok);
        assert(utterance.semanticFormation);
        assert(utterance.semanticFormation->root());

        auto sequence = semanticNode(
            utterance,
            "action",
            static_cast<int>(ActionNode::Kind::Sequence));
        auto setAction = semanticNode(
            utterance,
            "action",
            static_cast<int>(ActionNode::Kind::Set));
        auto addAction = semanticNode(
            utterance,
            "action",
            static_cast<int>(ActionNode::Kind::Add));
        assert(sequence && setAction && addAction);

        auto* intent = dynamic_cast<Lexeme*>(utterance.semanticFormation->root());
        assert(intent);

        // Target prototypes are semantic choices, not live DOM nodes.
        Lexeme article("article", "prototype.html.law.article");
        Lexeme orderedList("ol", "prototype.html.actions.ol");
        Lexeme setItem("li", "prototype.html.action.set.li");
        Lexeme addItem("li", "prototype.html.action.add.li");

        // These are authored target-channel facts, not inferred from tag
        // spelling. GraphTransduction carries them onto fresh occurrences.
        for (Lexeme* element : std::vector<Lexeme*>{
                 &article, &orderedList, &setItem, &addItem}) {
            assert(element->setDynamicProperty(
                "html.nodeType", PropertyValue(std::string("element"))));
        }

        // ---------------- Law-authored semantic -> HTML template ----------------
        Formation request;
        assert(request.setIdentifier("request.language-to-html"));

        Zone zone("language-to-html-zone", "default");
        Object author;
        author.setObjectID("author.language-to-html");

        std::vector<Singular*> beings{
            &zone, &author, &request,
            intent, sequence.get(), setAction.get(), addAction.get(),
            &article, &orderedList, &setItem, &addItem,
            manifestsAs.get(), domChildOf.get(), domNextSibling.get()
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
                assert(zone.getFormation().addRelation(relation));
            });

        std::vector<ActionNode> actions;
        for (Singular* member : std::vector<Singular*>{
                 intent, sequence.get(), setAction.get(), addAction.get(),
                 &article, &orderedList, &setItem, &addItem}) {
            actions.push_back(ActionNode::addElement("", member->getIdentifier()));
        }

        const std::string mapping = "@" + manifestsAs->getIdentifier();
        const std::string child = "@" + domChildOf->getIdentifier();
        const std::string next = "@" + domNextSibling->getIdentifier();

        // Meaning and manifestation remain different beings.
        actions.push_back(ActionNode::addRelation(
            intent->getIdentifier(), article.getIdentifier(), mapping, true));
        actions.push_back(ActionNode::addRelation(
            sequence->getIdentifier(), orderedList.getIdentifier(), mapping, true));
        actions.push_back(ActionNode::addRelation(
            setAction->getIdentifier(), setItem.getIdentifier(), mapping, true));
        actions.push_back(ActionNode::addRelation(
            addAction->getIdentifier(), addItem.getIdentifier(), mapping, true));

        // The target topology is authored too. C++ does not decree that a Law
        // is an article or that action children are list items.
        actions.push_back(ActionNode::addRelation(
            orderedList.getIdentifier(), article.getIdentifier(), child, true));
        actions.push_back(ActionNode::addRelation(
            setItem.getIdentifier(), orderedList.getIdentifier(), child, true));
        actions.push_back(ActionNode::addRelation(
            addItem.getIdentifier(), orderedList.getIdentifier(), child, true));
        actions.push_back(ActionNode::addRelation(
            setItem.getIdentifier(), addItem.getIdentifier(), next, true));

        Law manifestationLaw("manifest Law semantics as HTML");
        manifestationLaw.addAuthor(author);
        manifestationLaw.setActionModel(ActionNode::sequence(std::move(actions)));

        // Persist and restore before execution: this is authored Law text, not
        // a privileged test-only callback.
        auto restored = Law::fromJson(manifestationLaw.toJson());
        assert(restored);
        restored->addAuthor(author);
        assert(restored->applyTo(request) == Law::ApplicationResult::Applied);

        GraphTransduction::TemplateRequest transduction;
        transduction.requestFormation = &request;
        transduction.mappingKind = manifestsAs.get();
        transduction.targetFormationId = "html.from.language";
        transduction.targetIdPrefix = "live.";
        transduction.rootPrototype = &article;

        auto html = GraphTransduction::buildFromTemplate(transduction);
        assert(html);
        assert(html.targetFormation);
        assert(html.targetFormation->root());
        assert(html.targetFormation->root()->getSymbol() == "article");

        auto liveSet = html.findTarget("live.prototype.html.action.set.li");
        auto liveAdd = html.findTarget("live.prototype.html.action.add.li");
        assert(liveSet && liveAdd);
        assert(liveSet->getSymbol() == "li");
        assert(liveAdd->getSymbol() == "li");
        assert(liveSet->getIdentifier() != liveAdd->getIdentifier());

        // Four semantic beings manifest as four HTML occurrences, and four
        // authored target Relations survive as exact target Relations.
        assert(html.targetLexemes.size() == 4);
        assert(html.targetRelations.size() == 4);
        assert(html.correspondenceRelations.size() == 8);

        bool sawSiblingOrder = false;
        for (const auto& relation : html.targetRelations) {
            assert(relation);
            assert(relation->directed);
            if (relation->getTypeLexeme() == domNextSibling.get()) {
                sawSiblingOrder = true;
                assert(relation->a() == liveSet.get());
                assert(relation->b() == liveAdd.get());
            }
        }
        assert(sawSiblingOrder);

        // The same generated HTML Formation is immediately consumable by the
        // structured browser Act planner. Because the desired root has no
        // browser identity yet, only that root is eligible in wave 1.
        HtmlFormationActPlanner::Request actRequest;
        actRequest.desired = html.targetFormation.get();
        actRequest.kinds = {domChildOf.get(), domNextSibling.get()};
        actRequest.pageSessionId = "page-session.language-witness";
        actRequest.operationPrefix = "op.language-render";
        actRequest.externalParentToken = "node.body";
        actRequest.rootSiblingIndex = 0;

        HtmlFormationActPlanner::Bindings browserBindings;
        auto readyActs = HtmlFormationActPlanner::readyInsertions(
            actRequest, browserBindings);
        assert(readyActs);
        assert(!readyActs.complete);
        assert(readyActs.acts.size() == 1);
        assert(readyActs.acts.front().desiredId ==
               html.targetFormation->root()->getIdentifier());
        assert(readyActs.acts.front().act.kind == DomActKind::InsertElement);
        assert(readyActs.acts.front().act.tagName == "article");
        assert(readyActs.acts.front().act.parentToken == "node.body");
        assert(readyActs.acts.front().act.targetNodeToken.empty());
        assert(readyActs.acts.front().act.validate().valid);

        assert(intent->getIdentifier() == "utterance.html-witness.semantic.intent");
        assert(html.targetFormation->root()->getIdentifier() ==
               "live.prototype.html.law.article");
        assert(intent != html.targetFormation->root());

        Universe::instance().setRelationRegistrar({});
        Universe::instance().setRelationGenerationProvider({});
        Universe::instance().setRelationProvider({});
        Universe::instance().setProvider({});
    }

    language.clear();

    std::cout << "language_to_html_transduction_test passed" << std::endl;
    return 0;
}
