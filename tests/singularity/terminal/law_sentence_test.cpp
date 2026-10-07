#include "Singularity/Terminal/LawSentence.hpp"

#include <cassert>
#include <iostream>

using namespace Singularity::Terminal::LawSentence;

#include "Singularity/Terminal/TerminalChannel.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "support/test_harness.hpp"

// WITNESS DOCUMENTATION:
// What the old test could prove:
//   That LawSentence::parse() functions when supplied with a synthetic,
//   manually-constructed Vocabulary struct containing hardcoded word/being strings.
//
// What the old test could NOT prove:
//   That the real application path -- where TerminalChannel dynamically constructs
//   Vocabulary from Universe, LawManager, EventBus, and ZoneManager -- populates
//   a valid Vocabulary and successfully parses/searches sentences (`??` queries,
//   candidate listings, and status messages) through TerminalChannel::liveParse()
//   and TerminalChannel::statusOf().

static void testRealTerminalChannelPath() {
    std::cout << "Testing LawSentence via real TerminalChannel application path...\n";

    TestSupport::BootedEngineHarness harness("Tester");
    LawManager& laws = harness.lawManager;

    Singularity::Terminal::TerminalChannel::syncRegister(laws);
    auto* channel = Singularity::Terminal::TerminalChannel::find(laws);
    assert(channel != nullptr && "TerminalChannel must be registered on real LawManager");

    auto testObj = std::make_unique<Object>();
    testObj->setObjectID("quantum_cube");
    testObj->setDynamicProperty("glow", PropertyValue(1.0));
    testObj->setDynamicProperty("displayName", PropertyValue(std::string("Quantum Cube")));
    harness.zones.active().addObject(std::move(testObj));

    // Sense / tick to populate Universe and dynamic vocabulary
    channel->sense(laws);

    // Build real dynamic Vocabulary from live application state
    auto liveVocab = channel->vocabulary(laws);
    assert(!liveVocab.beings.empty() && "Real Vocabulary must contain beings from Universe");

    // Test LawSentence::parse on real dynamically constructed Vocabulary
    const auto parseResult = parse("?? quantum_cube", liveVocab);
    assert(parseResult.ok);
    assert(parseResult.search);

    bool foundObj = false;
    for (const auto& c : parseResult.candidates) {
        if (c.find("quantum_cube") != std::string::npos || c.find("Quantum Cube") != std::string::npos) {
            foundObj = true;
            break;
        }
    }
    assert(foundObj && "Real TerminalChannel Vocabulary path must find the object in Universe");

    // Test foreign sentence authoring through TerminalChannel
    Object author("test_author");
    auto foreignRes = channel->authorForeign(laws, "on \"tick\" then set glow 1?", {&author}, "law_test_glow");
    assert(foreignRes.status == "preview");

    std::cout << "  ✓ Real TerminalChannel path tests passed successfully!\n";
}

int main() {
    std::cout << "Testing LawSentence parsing...\n";

    Vocabulary vocab;
    vocab.words = canonicalWords();
    vocab.beings = {"alice", "bob"};
    vocab.events = {"zone-entered"};

    {
        Parse p = parse("?? bob", vocab);
        assert(p.ok);
        assert(p.search);
        bool foundBob = false;
        for (const auto& c : p.candidates) {
            if (c.find("bob") != std::string::npos) {
                foundBob = true;
                break;
            }
        }
        assert(foundBob);
    }

    {
        Parse p = parse("when something", vocab);
        assert(!p.ok || !p.error.empty());
    }

    testRealTerminalChannelPath();

    std::cout << "LawSentence tests passed successfully!\n";
    return 0;
}
