#include "Singularity/Foreign/Web/DomMirrorBridge.hpp"
#include "Singularity/Foreign/Web/HtmlFormationActPlanner.hpp"
#include "Singularity/Language/LanguageSystem.hpp"

#include <cassert>
#include <iostream>
#include <memory>

using namespace Singularity::Foreign::Web;
using Singularity::Language::LanguageSystem;
using Singularity::Language::Lexeme;

namespace {

std::string envelope(const char* type, const nlohmann::json& payload) {
    return nlohmann::json{{"type", type}, {"payload", payload}}.dump();
}

DomDelta confirmationFor(
    const HtmlFormationActPlanner::PlannedAct& planned,
    const std::string& browserToken,
    std::uint64_t sequence) {

    DomDelta delta;
    delta.protocolVersion = kDomProtocolVersion;
    delta.pageSessionId = planned.act.pageSessionId;
    delta.sequence = sequence;
    delta.originOperationId = planned.act.operationId;

    DomNodeRecord node;
    node.nodeToken = browserToken;
    node.parentToken = planned.act.parentToken;
    node.siblingIndex = planned.act.siblingIndex;
    if (planned.act.kind == DomActKind::InsertElement) {
        node.nodeType = "element";
        node.symbol = planned.act.tagName;
    } else {
        node.nodeType = "text";
        node.symbol = "#text";
        node.textContent = planned.act.text;
    }

    DomDeltaRecord record;
    record.kind = DomDeltaKind::Insert;
    record.targetNodeToken = browserToken;
    record.parentToken = node.parentToken;
    record.siblingIndex = node.siblingIndex;
    record.node = node;
    delta.records.push_back(record);
    return delta;
}

} // namespace

int main() {
    std::cout << "Running html_formation_roundtrip_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    {
        auto childOf = language.intern(
            "dom-child-of", "relation-kind.dom-child-of");
        auto nextSibling = language.intern(
            "dom-next-sibling", "relation-kind.dom-next-sibling");
        assert(childOf && nextSibling);

        // Browser truth begins with a tiny existing document whose body is
        // already addressable by an exact foreign token.
        DomSnapshot snapshot;
        snapshot.protocolVersion = kDomProtocolVersion;
        snapshot.pageSessionId = "page-session.roundtrip";
        snapshot.sequenceBase = 0;
        snapshot.url = "https://example.invalid/roundtrip";
        snapshot.rootNodeToken = "node.html";

        DomNodeRecord html;
        html.nodeToken = "node.html";
        html.nodeType = "element";
        html.symbol = "html";
        html.siblingIndex = 0;

        DomNodeRecord body;
        body.nodeToken = "node.body";
        body.nodeType = "element";
        body.symbol = "body";
        body.parentToken = "node.html";
        body.siblingIndex = 0;

        snapshot.nodes = {html, body};
        assert(snapshot.validate().valid);

        DomMirrorBridge bridge;
        std::string error;
        assert(bridge.handleWireMessage(
            envelope("snapshot", snapshot.toJson()),
            &error));
        assert(error.empty());
        assert(bridge.hasActiveSession());
        assert(bridge.getPageSessionId() == snapshot.pageSessionId);
        assert(bridge.translator().findNodeLexeme("node.body"));

        int confirmations = 0;
        std::string confirmedOperation;
        bridge.onActConfirmed([&](const std::string& op) {
            ++confirmations;
            confirmedOperation = op;
        });

        // Desired native HTML graph: <article><span>hello</span></article>.
        // Nothing here carries browser identity yet.
        Lexeme article("article", "desired.article");
        Lexeme span("span", "desired.span");
        Lexeme text("hello", "desired.text");
        for (Lexeme* element : std::vector<Lexeme*>{&article, &span}) {
            assert(element->setDynamicProperty(
                "html.nodeType", PropertyValue(std::string("element"))));
        }
        assert(text.setDynamicProperty(
            "html.nodeType", PropertyValue(std::string("text"))));

        Formation desired;
        assert(desired.setIdentifier("desired.roundtrip.html"));
        assert(desired.setRoot(&article));
        desired.addMember(&span);
        desired.addMember(&text);

        auto spanChild = std::make_shared<Relation>(
            *childOf, span, article, true, 1.0f);
        auto textChild = std::make_shared<Relation>(
            *childOf, text, span, true, 1.0f);
        assert(desired.addRelation(spanChild));
        assert(desired.addRelation(textChild));

        HtmlFormationActPlanner::Request request;
        request.desired = &desired;
        request.kinds = {childOf.get(), nextSibling.get()};
        request.pageSessionId = snapshot.pageSessionId;
        request.operationPrefix = "op.roundtrip";
        request.externalParentToken = "node.body";
        request.rootSiblingIndex = 0;

        HtmlFormationActPlanner::Bindings bindings;

        // Wave 1: article can be issued, but has no target token yet.
        auto ready = HtmlFormationActPlanner::readyInsertions(
            request, bindings);
        assert(ready);
        assert(ready.acts.size() == 1);
        auto articleAct = ready.acts.front();
        assert(articleAct.desiredId == article.getIdentifier());
        assert(articleAct.act.targetNodeToken.empty());
        assert(bridge.issueAct(articleAct.act, &error));
        assert(error.empty());

        // The browser mints node.article and reports it through the SAME
        // mutation channel used for all sensed foreign changes.
        DomDelta articleDelta = confirmationFor(
            articleAct, "node.article", 1);
        assert(bridge.handleWireMessage(
            envelope("delta", articleDelta.toJson()),
            &error));
        assert(error.empty());
        assert(confirmations == 1);
        assert(confirmedOperation == articleAct.act.operationId);
        assert(bridge.translator().isOperationConfirmed(
            articleAct.act.operationId));
        auto sensedArticle =
            bridge.translator().findNodeLexeme("node.article");
        assert(sensedArticle);
        assert(sensedArticle->getSymbol() == "article");

        assert(HtmlFormationActPlanner::confirmInsertion(
            articleAct, articleDelta, bindings, &error));
        assert(bindings.at(article.getIdentifier()) == "node.article");

        // The exact sensed token unlocks the next wave. There is no guessed
        // "future" token hidden in the desired graph.
        ready = HtmlFormationActPlanner::readyInsertions(
            request, bindings);
        assert(ready);
        assert(ready.acts.size() == 1);
        auto spanAct = ready.acts.front();
        assert(spanAct.desiredId == span.getIdentifier());
        assert(spanAct.act.parentToken == "node.article");
        assert(bridge.issueAct(spanAct.act, &error));

        DomDelta spanDelta = confirmationFor(
            spanAct, "node.span", 2);
        assert(bridge.handleWireMessage(
            envelope("delta", spanDelta.toJson()),
            &error));
        assert(HtmlFormationActPlanner::confirmInsertion(
            spanAct, spanDelta, bindings, &error));
        assert(bridge.translator().findNodeLexeme("node.span"));
        assert(confirmations == 2);

        // Then text is unlocked by the sensed span token.
        ready = HtmlFormationActPlanner::readyInsertions(
            request, bindings);
        assert(ready);
        assert(ready.acts.size() == 1);
        auto textAct = ready.acts.front();
        assert(textAct.desiredId == text.getIdentifier());
        assert(textAct.act.kind == DomActKind::InsertText);
        assert(textAct.act.parentToken == "node.span");
        assert(textAct.act.text == "hello");
        assert(bridge.issueAct(textAct.act, &error));

        DomDelta textDelta = confirmationFor(
            textAct, "node.text", 3);
        assert(bridge.handleWireMessage(
            envelope("delta", textDelta.toJson()),
            &error));
        assert(HtmlFormationActPlanner::confirmInsertion(
            textAct, textDelta, bindings, &error));

        auto sensedText =
            bridge.translator().findNodeLexeme("node.text");
        assert(sensedText);
        assert(sensedText->getSymbol() == "hello");
        assert(confirmations == 3);

        ready = HtmlFormationActPlanner::readyInsertions(
            request, bindings);
        assert(ready);
        assert(ready.complete);
        assert(ready.acts.empty());

        // Desired identity remains native and distinct from foreign identity.
        assert(article.getIdentifier() == "desired.article");
        assert(sensedArticle->getIdentifier() ==
               "page-session.roundtrip.node.article");
        assert(article.getIdentifier() != sensedArticle->getIdentifier());
    }

    language.clear();

    std::cout << "html_formation_roundtrip_test passed" << std::endl;
    return 0;
}
