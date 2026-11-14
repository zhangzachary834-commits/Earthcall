#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace Singularity::Foreign::Web;
using Singularity::Language::LanguageSystem;

static std::string readFixture(const std::string& path) {
    const std::vector<std::string> prefixes = {"", "../", "../../"};
    std::string resolvedPath = path;

    for (const auto& prefix : prefixes) {
        std::string candidate = prefix + path;
        if (std::filesystem::exists(candidate)) {
            resolvedPath = candidate;
            break;
        }
    }

    std::ifstream file(resolvedPath);
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

    // 5.5 Attribute Remove Lifecycle Gap
    {
        nlohmann::json j = {
            {"protocolVersion", 1},
            {"pageSessionId", "page-session.a91f4b2c"},
            {"sequence", 2},
            {"records", {{
                {"kind", "attribute-remove"},
                {"targetNodeToken", "node.6"},
                {"attributeName", "id"}
            }}}
        };
        DomDelta delta = DomDelta::fromJson(j);
        assert(delta.validate().valid);

        std::string err;
        bool ok = translator.applyDelta(delta, &err);
        assert(ok);
        assert(err.empty());
        assert(translator.getCurrentSequence() == 2);

        // Prove attribute and value lexemes were destroyed from language system
        assert(language.findById("page-session.a91f4b2c.attr.node.6.0") == nullptr);
        assert(language.findById("page-session.a91f4b2c.attrval.node.6.0") == nullptr);

        auto div1Form = translator.findNodeFormation("node.6");
        assert(div1Form != nullptr);
        // Both links and both members must disappear, not just the attribute binding.
        bool foundAttrRel = false;
        bool foundValueRel = false;
        bool preservedClassAttr = false;
        for (const auto& rel : div1Form->relations().getAll()) {
            if (rel->typeLabel() == DomRelationType::kHasAttribute) {
                auto* target = dynamic_cast<Singularity::Language::Lexeme*>(rel->b());
                if (target && target->getSymbol() == "id") foundAttrRel = true;
                if (target && target->getSymbol() == "class") preservedClassAttr = true;
            }
            if (rel->typeLabel() == DomRelationType::kHasValue) {
                auto* source = dynamic_cast<Singularity::Language::Lexeme*>(rel->a());
                if (source && source->getSymbol() == "id") foundValueRel = true;
            }
        }
        assert(!foundAttrRel);
        assert(!foundValueRel);
        assert(preservedClassAttr);
        assert(language.findById("page-session.a91f4b2c.attr.node.6.1") != nullptr);
        assert(language.findById("page-session.a91f4b2c.attrval.node.6.1") != nullptr);
    }

    // 5.6 Remove-set-remove must preserve the surviving class attribute identity.
    {
        const std::string prefix = "page-session.a91f4b2c.";
        nlohmann::json setJson = {
            {"protocolVersion", 1}, {"pageSessionId", "page-session.a91f4b2c"},
            {"sequence", 3}, {"records", {{
                {"kind", "attribute-set"}, {"targetNodeToken", "node.6"},
                {"attributeName", "data-test"}, {"attributeValue", "fresh"}
            }}}
        };
        auto setDelta = DomDelta::fromJson(setJson);
        assert(setDelta.validate().valid);
        std::string err;
        const bool setApplied = translator.applyDelta(setDelta, &err);
        assert(setApplied);
        assert(err.empty());
        assert(language.findById(prefix + "attr.node.6.2") != nullptr);
        assert(language.findById(prefix + "attrval.node.6.2") != nullptr);
        assert(language.findById(prefix + "attr.node.6.1") != nullptr);

        nlohmann::json removeJson = {
            {"protocolVersion", 1}, {"pageSessionId", "page-session.a91f4b2c"},
            {"sequence", 4}, {"records", {{
                {"kind", "attribute-remove"}, {"targetNodeToken", "node.6"},
                {"attributeName", "data-test"}
            }}}
        };
        auto removeDelta = DomDelta::fromJson(removeJson);
        assert(removeDelta.validate().valid);
        const bool removeApplied = translator.applyDelta(removeDelta, &err);
        assert(removeApplied);
        assert(err.empty());
        assert(translator.getCurrentSequence() == 4);
        assert(language.findById(prefix + "attr.node.6.2") == nullptr);
        assert(language.findById(prefix + "attrval.node.6.2") == nullptr);
        assert(language.findById(prefix + "attr.node.6.1") != nullptr);
        assert(language.findById(prefix + "attrval.node.6.1") != nullptr);
        auto form = translator.findNodeFormation("node.6");
        assert(form != nullptr);
        bool classLinked = false;
        bool removedLinked = false;
        for (const auto& rel : form->relations().getAll()) {
            if (rel->typeLabel() != DomRelationType::kHasAttribute) continue;
            auto* attr = dynamic_cast<Singularity::Language::Lexeme*>(rel->b());
            if (attr && attr->getSymbol() == "class") classLinked = true;
            if (attr && attr->getSymbol() == "data-test") removedLinked = true;
        }
        assert(classLinked);
        assert(!removedLinked);
    }

    // 5.7 Repeated set replaces the previous occurrence and remove clears the final one.
    {
        const std::string prefix = "page-session.a91f4b2c.";
        for (int sequence = 5; sequence <= 6; ++sequence) {
            nlohmann::json j = {
                {"protocolVersion", 1}, {"pageSessionId", "page-session.a91f4b2c"},
                {"sequence", sequence}, {"records", {{
                    {"kind", "attribute-set"}, {"targetNodeToken", "node.6"},
                    {"attributeName", "data-repeat"},
                    {"attributeValue", sequence == 5 ? "first" : "second"}
                }}}
            };
            auto delta = DomDelta::fromJson(j);
            assert(delta.validate().valid);
            std::string err;
            assert(translator.applyDelta(delta, &err));
            assert(err.empty());
        }
        auto form = translator.findNodeFormation("node.6");
        assert(form != nullptr);
        assert(language.findById(prefix + "attr.node.6.3") == nullptr);
        assert(language.findById(prefix + "attrval.node.6.3") == nullptr);
        assert(language.findById(prefix + "attr.node.6.4") != nullptr);
        assert(language.findById(prefix + "attrval.node.6.4") != nullptr);
        int repeatedLinks = 0;
        for (const auto& rel : form->relations().getAll()) {
            if (rel->typeLabel() != DomRelationType::kHasAttribute) continue;
            auto* attr = dynamic_cast<Singularity::Language::Lexeme*>(rel->b());
            if (attr && attr->getSymbol() == "data-repeat") ++repeatedLinks;
        }
        assert(repeatedLinks == 1);
        nlohmann::json j = {
            {"protocolVersion", 1}, {"pageSessionId", "page-session.a91f4b2c"},
            {"sequence", 7}, {"records", {{
                {"kind", "attribute-remove"}, {"targetNodeToken", "node.6"},
                {"attributeName", "data-repeat"}
            }}}
        };
        auto delta = DomDelta::fromJson(j);
        assert(delta.validate().valid);
        std::string err;
        assert(translator.applyDelta(delta, &err));
        assert(err.empty());
        assert(language.findById(prefix + "attr.node.6.4") == nullptr);
        assert(language.findById(prefix + "attrval.node.6.4") == nullptr);
        assert(language.findById(prefix + "attr.node.6.1") != nullptr);
        assert(language.findById(prefix + "attrval.node.6.1") != nullptr);
        for (const auto& rel : form->relations().getAll()) {
            if (rel->typeLabel() != DomRelationType::kHasAttribute) continue;
            auto* attr = dynamic_cast<Singularity::Language::Lexeme*>(rel->b());
            assert(!attr || attr->getSymbol() != "data-repeat");
        }
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
