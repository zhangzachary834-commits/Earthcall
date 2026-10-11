#include "Singularity/Terminal/LawSentence.hpp"

#include <cassert>
#include <iostream>

using namespace Singularity::Terminal::LawSentence;

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

    std::cout << "LawSentence tests passed successfully!\n";
    return 0;
}
