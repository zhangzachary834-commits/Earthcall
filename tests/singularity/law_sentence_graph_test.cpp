#include "Singularity/Terminal/LawSentenceGraph.hpp"
#include "Singularity/Language/LanguageSystem.hpp"

#include <cassert>
#include <iostream>

using namespace Singularity::Terminal;
using Singularity::Language::LanguageSystem;

namespace {

std::shared_ptr<Singularity::Language::Lexeme> occurrenceBySymbol(
    const LawSentenceGraph::Result& graph,
    const std::string& symbol) {
    for (const auto& occurrence : graph.occurrences) {
        if (occurrence && occurrence->getSymbol() == symbol) return occurrence;
    }
    return nullptr;
}

} // namespace

int main() {
    std::cout << "Running law_sentence_graph_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    auto doSet = language.intern("do", "lexeme.do.set");
    auto doAdd = language.intern("do", "lexeme.do.add");
    auto nextKind = language.intern("next", "relation-kind.lexical-next");
    auto denotesKind = language.intern("denotes", "relation-kind.occurrence-denotes");
    auto candidateKind = language.intern(
        "candidate-denotation", "relation-kind.candidate-denotation");
    auto semanticChildKind = language.intern(
        "semantic-child", "relation-kind.semantic-child");
    auto expressesKind = language.intern(
        "expresses", "relation-kind.lexical-expresses-semantic");
    assert(doSet && doAdd && nextKind && denotesKind && candidateKind &&
           semanticChildKind && expressesKind);

    LawSentence::Vocabulary vocab;
    vocab.words = LawSentence::canonicalWords();
    vocab.words.push_back({
        "do", "action.Set", doSet->getIdentifier(), "law.do.set",
        "set a value", {}
    });
    vocab.words.push_back({
        "do", "action.Add", doAdd->getIdentifier(), "law.do.add",
        "add a value", {}
    });
    vocab.resolve = [](const LawSentence::Ambiguity&) {
        return LawSentence::Resolution{"", "test keeps plurality open"};
    };

    LawSentenceGraph::Kinds kinds{
        nextKind.get(),
        denotesKind.get(),
        candidateKind.get(),
        semanticChildKind.get(),
        expressesKind.get()
    };

    // Unresolved plurality remains a graph instead of collapsing into a
    // guessed meaning or disappearing behind an error string.
    auto open = LawSentenceGraph::project("do", vocab, "utterance.open", kinds);
    assert(open);
    assert(!open.parse.ok);
    assert(open.parse.ambiguities.size() == 1);
    assert(open.occurrences.size() == 1);

    auto ambiguous = occurrenceBySymbol(open, "do");
    assert(ambiguous);

    PropertyValue isAmbiguous;
    assert(ambiguous->getDynamicProperty("language.ambiguous", isAmbiguous));
    assert(std::holds_alternative<bool>(isAmbiguous));
    assert(std::get<bool>(isAmbiguous));

    PropertyValue candidates;
    assert(ambiguous->getDynamicProperty("language.candidates", candidates));
    auto list = std::get<std::shared_ptr<PropertyList>>(candidates);
    assert(list);
    assert(list->elements.size() == 2);

    std::size_t candidateEdges = 0;
    for (const auto& relation : open.denotationRelations) {
        if (relation && relation->getTypeLexeme() == candidateKind.get()) {
            ++candidateEdges;
        }
    }
    assert(candidateEdges == 2);

    // Now let a Metalaw seam choose one meaning. The graph keeps BOTH
    // candidates while adding an exact chosen-denotation edge.
    LawSentence::Vocabulary resolvedVocab = vocab;
    resolvedVocab.resolve = [&](const LawSentence::Ambiguity& ambiguity) {
        assert(ambiguity.start == 0);
        assert(ambiguity.end == 2);
        return LawSentence::Resolution{
            doSet->getIdentifier() + "->law.do.set",
            "test Metalaw chooses Set"
        };
    };

    auto resolved = LawSentenceGraph::project(
        "do x to 1?", resolvedVocab, "utterance.resolved", kinds);
    assert(resolved);
    assert(resolved.parse.ok);
    assert(resolved.parse.previewOnly);
    assert(resolved.parse.ambiguities.size() == 1);

    auto chosenOccurrence = occurrenceBySymbol(resolved, "do");
    assert(chosenOccurrence);

    PropertyValue chosenId;
    assert(chosenOccurrence->getDynamicProperty("language.lexemeId", chosenId));
    assert(std::get<std::string>(chosenId) == doSet->getIdentifier());

    std::size_t chosenEdges = 0;
    candidateEdges = 0;
    for (const auto& relation : resolved.denotationRelations) {
        if (!relation) continue;
        if (relation->getTypeLexeme() == denotesKind.get() &&
            relation->b() == doSet.get()) {
            ++chosenEdges;
        }
        if (relation->getTypeLexeme() == candidateKind.get()) {
            ++candidateEdges;
        }
    }
    assert(chosenEdges == 1);
    assert(candidateEdges == 2);

    // Rung 3b: a successful sentence now has a rooted semantic Formation.
    assert(resolved.semanticFormation);
    assert(resolved.semanticFormation->root());
    assert(resolved.semanticFormation->root()->getIdentifier() ==
           "utterance.resolved.semantic.intent");

    std::size_t actionNodes = 0;
    for (const auto& node : resolved.semanticLexemes) {
        if (!node) continue;
        PropertyValue type;
        if (!node->getDynamicProperty("semantic.nodeType", type) ||
            !std::holds_alternative<std::string>(type)) {
            continue;
        }
        if (std::get<std::string>(type) == "action") ++actionNodes;
    }
    assert(actionNodes == 1);
    assert(!resolved.semanticRelations.empty());
    for (const auto& relation : resolved.semanticRelations) {
        assert(relation);
        assert(relation->directed);
        assert(relation->getTypeLexeme() == semanticChildKind.get());
    }

    bool doExpressesAction = false;
    for (const auto& relation : resolved.semanticProvenanceRelations) {
        assert(relation);
        assert(relation->getTypeLexeme() == expressesKind.get());
        if (relation->a() == chosenOccurrence.get()) {
            PropertyValue nodeType;
            if (relation->b() &&
                relation->b()->getDynamicProperty("semantic.nodeType", nodeType) &&
                std::holds_alternative<std::string>(nodeType) &&
                std::get<std::string>(nodeType) == "action") {
                doExpressesAction = true;
            }
        }
    }
    assert(doExpressesAction);

    // Lexical order is explicit Relation structure.
    assert(resolved.occurrences.size() >= 4);
    assert(resolved.lexicalRelations.size() == resolved.occurrences.size() - 1);
    for (const auto& relation : resolved.lexicalRelations) {
        assert(relation);
        assert(relation->directed);
        assert(relation->getTypeLexeme() == nextKind.get());
    }

    // Compound structure is preserved rather than flattened.
    auto red = language.intern("red", "lexeme.value.red");
    assert(red);
    LawSentence::Vocabulary compoundVocab;
    compoundVocab.words = LawSentence::canonicalWords();
    compoundVocab.words.push_back({
        "red", "value", red->getIdentifier(), "law.value.red",
        "red value", {}
    });
    LawSentence::Preset redPreset;
    redPreset.lawId = "law.value.red";
    redPreset.value = PropertyValue(std::string("red"));
    compoundVocab.presets.push_back(redPreset);
    compoundVocab.resolve = [](const LawSentence::Ambiguity&) {
        return LawSentence::Resolution{};
    };

    auto compound = LawSentenceGraph::project(
        "then set color to red, add size by 2?",
        compoundVocab,
        "utterance.compound",
        kinds);
    assert(compound);
    assert(compound.parse.ok);
    assert(compound.parse.action);
    assert(compound.parse.action->kind == ActionNode::Kind::Sequence);

    actionNodes = 0;
    for (const auto& node : compound.semanticLexemes) {
        if (!node) continue;
        PropertyValue type;
        if (node->getDynamicProperty("semantic.nodeType", type) &&
            std::holds_alternative<std::string>(type) &&
            std::get<std::string>(type) == "action") {
            ++actionNodes;
        }
    }
    assert(actionNodes == 3);

    std::size_t actionTreeEdges = 0;
    for (const auto& relation : compound.semanticRelations) {
        if (!relation || relation->getTypeLexeme() != semanticChildKind.get()) continue;
        PropertyValue aType;
        PropertyValue bType;
        if (relation->a() && relation->b() &&
            relation->a()->getDynamicProperty("semantic.nodeType", aType) &&
            relation->b()->getDynamicProperty("semantic.nodeType", bType) &&
            std::holds_alternative<std::string>(aType) &&
            std::holds_alternative<std::string>(bType) &&
            std::get<std::string>(aType) == "action" &&
            std::get<std::string>(bType) == "action") {
            ++actionTreeEdges;
        }
    }
    assert(actionTreeEdges == 2);

    language.clear();

    std::cout << "law_sentence_graph_test passed" << std::endl;
    return 0;
}
