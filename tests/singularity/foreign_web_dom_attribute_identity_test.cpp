#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include <cassert>
#include <iostream>

using namespace Singularity::Foreign::Web;

int main() {
    std::cout << "Running foreign_web_dom_attribute_identity_test..." << std::endl;

    auto& language = Singularity::Language::LanguageSystem::instance();
    language.clear();

    DomMirrorTranslator translator;

    // 1. Create snapshot with a node having multiple attributes where a value shares spelling with an attribute name
    DomSnapshot snapshot;
    snapshot.pageSessionId = "page-session.attrtest1";
    snapshot.url = "http://localhost/test";
    snapshot.rootNodeToken = "node.1";
    snapshot.sequenceBase = 0;

    DomNodeRecord node;
    node.nodeToken = "node.1";
    node.nodeType = "element";
    node.symbol = "button";
    node.attributes = {
        {"class", "btn-primary"},
        {"data-target", "class"} // Value is "class", sharing spelling with the attribute name "class"
    };

    snapshot.nodes = {node};

    std::string err;
    bool admitted = translator.admitSnapshot(snapshot, &err);
    assert(admitted);
    assert(err.empty());

    auto nodeLexeme = translator.findNodeLexeme("node.1");
    assert(nodeLexeme != nullptr);

    auto nodeForm = translator.findNodeFormation("node.1");
    assert(nodeForm != nullptr);

    // Verify initial relation graph for node.1
    auto initialHasAttrRels = nodeForm->relations().getRelationsOfType(DomRelationType::kHasAttribute);
    assert(initialHasAttrRels.size() == 2);

    auto initialHasValRels = nodeForm->relations().getRelationsOfType(DomRelationType::kHasValue);
    assert(initialHasValRels.size() == 2);

    // 2. Apply delta removing attribute "class"
    DomDelta delta;
    delta.pageSessionId = "page-session.attrtest1";
    delta.sequence = 1;

    DomDeltaRecord removeRecord;
    removeRecord.kind = DomDeltaKind::AttributeRemove;
    removeRecord.targetNodeToken = "node.1";
    removeRecord.attributeName = "class";

    delta.records = {removeRecord};

    bool deltaApplied = translator.applyDelta(delta, &err);
    assert(deltaApplied);
    assert(err.empty());

    // 3. Verify relation graph after attribute "class" is removed
    auto remainingHasAttrRels = nodeForm->relations().getRelationsOfType(DomRelationType::kHasAttribute);
    assert(remainingHasAttrRels.size() == 1);

    // The remaining attribute relation MUST be for "data-target"
    auto remainingAttrLex = dynamic_cast<Singularity::Language::Lexeme*>(remainingHasAttrRels[0]->b());
    assert(remainingAttrLex != nullptr);
    assert(remainingAttrLex->getSymbol() == "data-target");

    auto remainingHasValRels = nodeForm->relations().getRelationsOfType(DomRelationType::kHasValue);
    assert(remainingHasValRels.size() == 1);

    // The remaining value relation MUST be from "data-target" -> "class"
    auto remainingValLex = dynamic_cast<Singularity::Language::Lexeme*>(remainingHasValRels[0]->b());
    assert(remainingValLex != nullptr);
    assert(remainingValLex->getSymbol() == "class");

    // The attribute Lexeme for "class" should no longer be in LanguageSystem
    std::string removedAttrId = DomMirrorTranslator::makeAttrLexemeId("page-session.attrtest1", "node.1", 0);
    assert(language.findById(removedAttrId) == nullptr);

    // The attribute Lexeme for "data-target" MUST still exist
    std::string keptAttrId = DomMirrorTranslator::makeAttrLexemeId("page-session.attrtest1", "node.1", 1);
    assert(language.findById(keptAttrId) != nullptr);

    // The value Lexeme "class" for "data-target" MUST still exist in LanguageSystem
    std::string keptValId = DomMirrorTranslator::makeAttrValLexemeId("page-session.attrtest1", "node.1", 1);
    assert(language.findById(keptValId) != nullptr);

    std::cout << "foreign_web_dom_attribute_identity_test PASSED!" << std::endl;
    return 0;
}
