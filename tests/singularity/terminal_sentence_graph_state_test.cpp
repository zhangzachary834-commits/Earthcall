#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cassert>
#include <iostream>

using Singularity::Terminal::TerminalChannel;
using Singularity::Language::LanguageSystem;

int main() {
    std::cout << "Running terminal_sentence_graph_state_test..." << std::endl;

    auto& language = LanguageSystem::instance();
    language.clear();

    {
        LawManager laws;
        TerminalChannel channel;

        // Preview is read-only, so this test needs no Person author or Zone.
        // It still passes through authorForeign's real vocabulary and parse
        // path, then retains the exact Parse without asking a Metalaw twice.
        const auto response = channel.authorForeign(
            laws,
            "then set x to 1?",
            {},
            "");
        assert(response.status == "preview");

        const auto* graph = channel.lastSentenceGraph();
        assert(graph);
        assert(graph->parse.ok);
        assert(graph->parse.previewOnly);
        assert(graph->lexicalFormation);
        assert(graph->denotationFormation);
        assert(graph->semanticFormation);

        assert(graph->lexicalFormation->getIdentifier() ==
               "terminal-channel.utterance.1.lexical");
        assert(graph->denotationFormation->getIdentifier() ==
               "terminal-channel.utterance.1.denotation");
        assert(graph->semanticFormation->getIdentifier() ==
               "terminal-channel.utterance.1.semantic");

        PropertyValue value;
        assert(lawGetValue(
            channel,
            PropertyPath::parse("graph.utteranceId"),
            value));
        assert(std::get<std::string>(value) == "terminal-channel.utterance.1");

        assert(lawGetValue(
            channel,
            PropertyPath::parse("graph.semanticFormationId"),
            value));
        assert(std::get<std::string>(value) ==
               "terminal-channel.utterance.1.semantic");

        assert(lawGetValue(
            channel,
            PropertyPath::parse("graph.status"),
            value));
        assert(std::get<std::string>(value).find("semantic") != std::string::npos);

        // A second utterance is a new occurrence world, not a rewrite of the
        // first one's identities.
        const auto second = channel.authorForeign(
            laws,
            "then add x by 2?",
            {},
            "");
        assert(second.status == "preview");
        graph = channel.lastSentenceGraph();
        assert(graph);
        assert(graph->lexicalFormation->getIdentifier() ==
               "terminal-channel.utterance.2.lexical");

        assert(lawGetValue(
            channel,
            PropertyPath::parse("graph.utteranceId"),
            value));
        assert(std::get<std::string>(value) == "terminal-channel.utterance.2");
    }

    language.clear();

    std::cout << "terminal_sentence_graph_state_test passed" << std::endl;
    return 0;
}
