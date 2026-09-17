#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace Singularity::Foreign::Web;
using Singularity::Language::LanguageSystem;

static std::string readFixture(const std::string& path) {
    std::ifstream file(path);
    assert(file.is_open());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    std::cout << "Running foreign_web_dom_projection_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    DomMirrorTranslator translator;

    // 1. Admit Snapshot & Verify Native Lexeme Formations
    {
        std::string jsonStr = readFixture("tests/fixtures/foreign_web/dom_snapshot_basic.json");
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        DomSnapshot snapshot = DomSnapshot::fromJson(j);
        assert(snapshot.validate().valid);

        std::string err;
        bool ok = translator.admitSnapshot(snapshot, &err);
        assert(ok);
        assert(err.empty());
        assert(translator.hasActiveSession());
        assert(translator.getPageSessionId() == "page-session.a91f4b2c");

        // 2. Identity Invariant: Two same-spelled <div> nodes become distinct Lexeme identities
        auto div1 = translator.findNodeLexeme("node.6");
        auto div2 = translator.findNodeLexeme("node.7");
        assert(div1 != nullptr);
        assert(div2 != nullptr);
        assert(div1 != div2);

        // Spelling is "div" for both
        assert(div1->getSymbol() == "div");
        assert(div2->getSymbol() == "div");

        // Exact stable IDs are distinct
        assert(div1->getIdentifier() == "page-session.a91f4b2c.node.6");
        assert(div2->getIdentifier() == "page-session.a91f4b2c.node.7");

        // Exact-ID lookup through LanguageSystem resolves each occurrence independently
        assert(language.findById("page-session.a91f4b2c.node.6") == div1);
        assert(language.findById("page-session.a91f4b2c.node.7") == div2);

        // 3. Node Formations & Attribute Relations
        auto div1Form = translator.findNodeFormation("node.6");
        assert(div1Form != nullptr);
        assert(div1Form->hasRoot());
        assert(div1Form->root() == div1.get());
        assert(div1Form->hasMember(div1.get()));

        // Check attributes: node.6 has id="container-1" and class="search-container"
        auto attrLexeme0 = language.findById("page-session.a91f4b2c.attr.node.6.0");
        auto valLexeme0 = language.findById("page-session.a91f4b2c.attrval.node.6.0");
        assert(attrLexeme0 != nullptr);
        assert(valLexeme0 != nullptr);
        assert(attrLexeme0->getSymbol() == "id");
        assert(valLexeme0->getSymbol() == "container-1");
        assert(div1Form->hasMember(attrLexeme0.get()));
        assert(div1Form->hasMember(valLexeme0.get()));

        // 4. Document-Level Formation
        auto docForm = translator.getDocumentFormation();
        assert(docForm != nullptr);
        assert(docForm->getIdentifier() == "page-session.a91f4b2c.doc.formation");
        auto rootLexeme = translator.findNodeLexeme("node.1");
        assert(docForm->root() == rootLexeme.get());

        // All 11 nodes are members
        assert(docForm->hasMember(div1.get()));
        assert(docForm->hasMember(div2.get()));

        // Structural relations exist in document formation
        assert(!docForm->relations().getAll().empty());
    }

    // 5. Incremental Mutation Delta with Operation Provenance (Echo Prevention)
    {
        std::string jsonStr = readFixture("tests/fixtures/foreign_web/dom_delta_basic.json");
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        DomDelta delta = DomDelta::fromJson(j);
        assert(delta.validate().valid);

        std::string err;
        bool ok = translator.applyDelta(delta, &err);
        assert(ok);
        assert(err.empty());
        assert(translator.getCurrentSequence() == 1);

        // Prove operation provenance was captured
        assert(translator.isOperationConfirmed("op.act.001"));

        // Prove text-change updated node.9
        auto node9 = translator.findNodeLexeme("node.9");
        assert(node9 != nullptr);
        assert(node9->getSymbol() == "Updated First Link Text");

        // Prove insert created node.12
        auto node12 = translator.findNodeLexeme("node.12");
        assert(node12 != nullptr);
        assert(node12->getSymbol() == "p");
        assert(node12->getIdentifier() == "page-session.a91f4b2c.node.12");
        auto docForm = translator.getDocumentFormation();
        assert(docForm->hasMember(node12.get()));

        // Prove remove unregistered node.10
        assert(translator.findNodeLexeme("node.10") == nullptr);
        assert(language.findById("page-session.a91f4b2c.node.10") == nullptr);
    }

    // 6. Navigation / Retirement Lifecycle
    {
        // Prove retire completely clears live mirror
        translator.retire();
        assert(!translator.hasActiveSession());
        assert(translator.findNodeLexeme("node.6") == nullptr);
        assert(translator.getDocumentFormation() == nullptr);

        // LanguageSystem must have no dangling session Lexemes
        assert(language.findById("page-session.a91f4b2c.node.6") == nullptr);
        assert(language.findById("page-session.a91f4b2c.node.1") == nullptr);
    }

    std::cout << "foreign_web_dom_projection_test PASSED!" << std::endl;
    return 0;
}
