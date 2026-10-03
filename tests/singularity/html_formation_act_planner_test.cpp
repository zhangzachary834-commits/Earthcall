#include "Singularity/Foreign/Web/HtmlFormationActPlanner.hpp"
#include "Singularity/Language/LanguageSystem.hpp"

#include <cassert>
#include <iostream>
#include <memory>

using namespace Singularity::Foreign::Web;
using Singularity::Language::LanguageSystem;
using Singularity::Language::Lexeme;

namespace {

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
    std::cout << "Running html_formation_act_planner_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    {
        auto childOf = language.intern(
            "dom-child-of", "relation-kind.dom-child-of");
        auto nextSibling = language.intern(
            "dom-next-sibling", "relation-kind.dom-next-sibling");
        assert(childOf && nextSibling);

        Lexeme article("article", "desired.article");
        Lexeme list("ol", "desired.list");
        Lexeme first("li", "desired.first");
        Lexeme second("li", "desired.second");
        Lexeme text("Set theme", "desired.first.text");

        for (Lexeme* element : std::vector<Lexeme*>{
                 &article, &list, &first, &second}) {
            assert(element->setDynamicProperty(
                "html.nodeType", PropertyValue(std::string("element"))));
        }
        assert(text.setDynamicProperty(
            "html.nodeType", PropertyValue(std::string("text"))));

        Formation desired;
        assert(desired.setIdentifier("desired.html"));
        assert(desired.setRoot(&article));
        desired.addMember(&list);
        desired.addMember(&first);
        desired.addMember(&second);
        desired.addMember(&text);

        const auto edge = [&](Lexeme& a, Lexeme& b, Lexeme& kind) {
            auto relation = std::make_shared<Relation>(
                kind, a, b, true, 1.0f);
            assert(desired.addRelation(relation));
        };
        edge(list, article, *childOf);
        edge(first, list, *childOf);
        edge(second, list, *childOf);
        edge(text, first, *childOf);
        edge(first, second, *nextSibling);

        HtmlFormationActPlanner::Request request;
        request.desired = &desired;
        request.kinds = {childOf.get(), nextSibling.get()};
        request.pageSessionId = "page-session.planner";
        request.operationPrefix = "op.render";
        request.externalParentToken = "node.body";
        request.rootSiblingIndex = 0;

        HtmlFormationActPlanner::Bindings bindings;

        auto ready = HtmlFormationActPlanner::readyInsertions(request, bindings);
        assert(ready);
        assert(!ready.complete);
        assert(ready.acts.size() == 1);
        assert(ready.acts[0].desiredId == article.getIdentifier());
        assert(ready.acts[0].act.kind == DomActKind::InsertElement);
        assert(ready.acts[0].act.parentToken == "node.body");
        assert(ready.acts[0].act.siblingIndex == 0);
        assert(ready.acts[0].act.validate().valid);
        assert(ready.acts[0].act.toJson().contains("siblingIndex"));
        assert(ready.acts[0].act.toJson()["siblingIndex"] == 0);

        std::string error;
        assert(HtmlFormationActPlanner::confirmInsertion(
            ready.acts[0],
            confirmationFor(ready.acts[0], "node.article", 1),
            bindings,
            &error));
        assert(error.empty());

        ready = HtmlFormationActPlanner::readyInsertions(request, bindings);
        assert(ready);
        assert(ready.acts.size() == 1);
        assert(ready.acts[0].desiredId == list.getIdentifier());
        assert(ready.acts[0].act.parentToken == "node.article");
        assert(HtmlFormationActPlanner::confirmInsertion(
            ready.acts[0],
            confirmationFor(ready.acts[0], "node.list", 2),
            bindings,
            &error));

        ready = HtmlFormationActPlanner::readyInsertions(request, bindings);
        assert(ready);
        assert(ready.acts.size() == 2);
        assert(ready.acts[0].desiredId == first.getIdentifier());
        assert(ready.acts[0].act.siblingIndex == 0);
        assert(ready.acts[1].desiredId == second.getIdentifier());
        assert(ready.acts[1].act.siblingIndex == 1);

        assert(HtmlFormationActPlanner::confirmInsertion(
            ready.acts[0],
            confirmationFor(ready.acts[0], "node.first", 3),
            bindings,
            &error));
        assert(HtmlFormationActPlanner::confirmInsertion(
            ready.acts[1],
            confirmationFor(ready.acts[1], "node.second", 4),
            bindings,
            &error));

        ready = HtmlFormationActPlanner::readyInsertions(request, bindings);
        assert(ready);
        assert(ready.acts.size() == 1);
        assert(ready.acts[0].desiredId == text.getIdentifier());
        assert(ready.acts[0].act.kind == DomActKind::InsertText);
        assert(ready.acts[0].act.parentToken == "node.first");
        assert(ready.acts[0].act.text == "Set theme");
        assert(HtmlFormationActPlanner::confirmInsertion(
            ready.acts[0],
            confirmationFor(ready.acts[0], "node.text", 5),
            bindings,
            &error));

        ready = HtmlFormationActPlanner::readyInsertions(request, bindings);
        assert(ready);
        assert(ready.complete);
        assert(ready.acts.empty());
        assert(bindings.size() == 5);

        // A multi-child parent without explicit next-sibling structure is
        // under-specified. Never borrow vector insertion order as semantics.
        Formation unordered;
        assert(unordered.setIdentifier("desired.unordered"));
        assert(unordered.setRoot(&article));
        unordered.addMember(&first);
        unordered.addMember(&second);
        auto firstChild = std::make_shared<Relation>(
            *childOf, first, article, true, 1.0f);
        auto secondChild = std::make_shared<Relation>(
            *childOf, second, article, true, 1.0f);
        assert(unordered.addRelation(firstChild));
        assert(unordered.addRelation(secondChild));

        HtmlFormationActPlanner::Request unorderedRequest = request;
        unorderedRequest.desired = &unordered;
        auto refused = HtmlFormationActPlanner::readyInsertions(
            unorderedRequest, {});
        assert(!refused);
        assert(refused.refusal.find("complete dom-next-sibling chain") !=
               std::string::npos);
    }

    language.clear();

    std::cout << "html_formation_act_planner_test passed" << std::endl;
    return 0;
}
