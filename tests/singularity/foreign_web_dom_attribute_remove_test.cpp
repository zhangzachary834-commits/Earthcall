#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include <cassert>
#include <iostream>

using namespace Singularity::Foreign::Web;

int main() {
    std::cout << "Running foreign_web_dom_attribute_remove_test..." << std::endl;

    auto& language = Singularity::Language::LanguageSystem::instance();
    language.clear();

    DomMirrorTranslator translator;

    // 1. Create Snapshot with two nodes (node.1 and node.2) both carrying attribute "class"
    DomSnapshot snapshot;
    snapshot.protocolVersion = 1;
    snapshot.pageSessionId = "page-session.attrtest123";
    snapshot.rootNodeToken = "node.1";
    snapshot.sequenceBase = 0;

    DomNodeRecord node1;
    node1.nodeToken = "node.1";
    node1.nodeType = "element";
    node1.symbol = "div";
    node1.attributes.push_back({"class", "container"});

    DomNodeRecord node2;
    node2.nodeToken = "node.2";
    node2.nodeType = "element";
    node2.symbol = "span";
    node2.parentToken = "node.1";
    node2.attributes.push_back({"class", "container"});

    snapshot.nodes.push_back(node1);
    snapshot.nodes.push_back(node2);

    std::string err;
    bool admitted = translator.admitSnapshot(snapshot, &err);
    assert(admitted);
    assert(err.empty());

    // Verify initial graph state for node.1 and node.2
    auto node1Form = translator.findNodeFormation("node.1");
    auto node2Form = translator.findNodeFormation("node.2");
    assert(node1Form != nullptr);
    assert(node2Form != nullptr);

    // Both node formations should contain 3 members (nodeLexeme, attrLexeme, valLexeme)
    assert(node1Form->getMembers().size() == 3);
    assert(node2Form->getMembers().size() == 3);

    // Both node formations should contain 2 relations (kHasAttribute, kHasValue)
    assert(node1Form->relations().getAll().size() == 2);
    assert(node2Form->relations().getAll().size() == 2);

    // Store node.2's attribute lexeme ID to prove it remains untouched
    std::string node2AttrLexemeId;
    for (const auto& rel : node2Form->relations().getAll()) {
        if (rel && rel->typeLabel() == DomRelationType::kHasAttribute) {
            if (auto* lex = dynamic_cast<Singularity::Language::Lexeme*>(rel->b())) {
                node2AttrLexemeId = lex->getIdentifier();
            }
        }
    }
    assert(!node2AttrLexemeId.empty());
    assert(language.findById(node2AttrLexemeId) != nullptr);

    // 2. Apply AttributeRemove delta targeting node.1 and attributeName "class"
    DomDelta delta;
    delta.protocolVersion = 1;
    delta.pageSessionId = "page-session.attrtest123";
    delta.sequence = 1;

    DomDeltaRecord removeRecord;
    removeRecord.kind = DomDeltaKind::AttributeRemove;
    removeRecord.targetNodeToken = "node.1";
    removeRecord.attributeName = "class";
    delta.records.push_back(removeRecord);

    bool deltaApplied = translator.applyDelta(delta, &err);
    assert(deltaApplied);
    assert(err.empty());

    // 3. Verify node.1 attribute relation edges and member lexemes were removed
    assert(node1Form->getMembers().size() == 1); // Only nodeLexeme remains
    assert(node1Form->relations().getAll().empty()); // kHasAttribute and kHasValue removed

    // 4. Verify node.2 attribute relation edges, members, and LanguageSystem registration remain intact
    assert(node2Form->getMembers().size() == 3);
    assert(node2Form->relations().getAll().size() == 2);
    assert(language.findById(node2AttrLexemeId) != nullptr);

    std::cout << "foreign_web_dom_attribute_remove_test PASSED!" << std::endl;
    return 0;
}
