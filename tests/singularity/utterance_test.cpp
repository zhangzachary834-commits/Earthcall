#include "Singularity/Language/Utterance.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Relation/RelationManager.hpp"
#include <cassert>
#include <iostream>

using namespace Singularity;
using namespace Singularity::Language;

int main() {
    auto& language = LanguageSystem::instance();
    language.clear();

    auto hello = language.intern("hello", "hello");
    auto world = language.intern("world", "world");

    Utterance utterance("hello world", "u-123", 1.5);

    assert(utterance.getRawText() == "hello world");
    assert(utterance.getIdentifier() == "u-123");
    assert(utterance.getTimestamp() == 1.5);

    utterance.addLexeme(hello.get());
    utterance.addLexeme(world.get());

    assert(utterance.getLexemes().size() == 2);
    assert(utterance.getLexemes()[0] == hello.get());

    auto relations = utterance.createOccurrenceRelations();
    assert(relations.size() == 2);

    assert(relations[0]->typeLabel() == "occurrence-of");
    assert(relations[0]->a() == &utterance);
    assert(relations[0]->b() == hello.get());

    assert(relations[1]->typeLabel() == "occurrence-of");
    assert(relations[1]->a() == &utterance);
    assert(relations[1]->b() == world.get());

    std::cout << "utterance_test passed\n";
    return 0;
}
