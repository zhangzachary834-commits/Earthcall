#include "Singularity/Language/SyntacticParser.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Relation/RelationManager.hpp"
#include <cassert>
#include <iostream>

using namespace Singularity;
using namespace Singularity::Language;

int main() {
    auto& language = LanguageSystem::instance();
    language.clear();

    Zone zone("test_zone", "Test Zone");

    // We expect tokenization to handle punctuation and case
    auto the = language.intern("the", "the");
    auto dog = language.intern("dog", "dog");
    auto chase = language.intern("chase", "chase");
    auto cat = language.intern("cat", "cat");

    // Nouns/verbs/determiners
    auto noun = language.intern("noun", "noun");
    auto verb = language.intern("verb", "verb");
    auto determiner = language.intern("determiner", "determiner");
    auto preposition = language.intern("preposition", "preposition");

    zone.addToFormation(the.get());
    zone.addToFormation(dog.get());
    zone.addToFormation(chase.get());
    zone.addToFormation(cat.get());
    zone.addToFormation(noun.get());
    zone.addToFormation(verb.get());
    zone.addToFormation(determiner.get());
    zone.addToFormation(preposition.get());

    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *dog, *noun, false, 1.0f));
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *chase, *verb, false, 1.0f));
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *the, *determiner, false, 1.0f));
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *cat, *noun, false, 1.0f));

    auto meaning_chase = language.intern("meaning_chase", "meaning_chase");
    zone.addToFormation(meaning_chase.get());
    zone.formation().relations().add(std::make_shared<Relation>("resolves_to", *chase, *meaning_chase, false, 1.0f));

    // Case 1: Simple sentence, correct POS, tokenization validation
    auto relations1 = SyntacticParser::parse("The dog, chase the cat!", zone);
    assert(relations1.size() == 1);
    auto& rel1 = relations1[0];
    assert(rel1->a()->getIdentifier() == dog->getIdentifier());
    assert(rel1->b()->getIdentifier() == cat->getIdentifier());
    assert(rel1->typeLabel() == "meaning_chase");

    // Case 2: Multi-word preposition relation phrase
    auto run = language.intern("run", "run");
    auto away = language.intern("away", "away");
    auto from = language.intern("from", "from");
    auto bear = language.intern("bear", "bear");
    zone.addToFormation(run.get());
    zone.addToFormation(away.get());
    zone.addToFormation(from.get());
    zone.addToFormation(bear.get());
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *run, *verb, false, 1.0f));
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *away, *preposition, false, 1.0f));
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *from, *preposition, false, 1.0f));
    zone.formation().relations().add(std::make_shared<Relation>("is_pos", *bear, *noun, false, 1.0f));

    auto meaning_run_away_from = language.intern("meaning_run_away_from", "meaning_run_away_from");
    zone.addToFormation(meaning_run_away_from.get());
    auto run_away_from = language.intern("run_away_from", "run_away_from");
    zone.addToFormation(run_away_from.get());
    zone.formation().relations().add(std::make_shared<Relation>("resolves_to", *run_away_from, *meaning_run_away_from, false, 1.0f));

    auto relations2 = SyntacticParser::parse("The dog run away from the bear.", zone);
    assert(relations2.size() == 1);
    assert(relations2[0]->a()->getIdentifier() == dog->getIdentifier());
    assert(relations2[0]->b()->getIdentifier() == bear->getIdentifier());
    assert(relations2[0]->typeLabel() == "meaning_run_away_from");

    // Case 3: Unknown words default to noun and get interned
    auto relations3 = SyntacticParser::parse("florp chase glorp", zone);
    assert(relations3.size() == 1);
    auto florp = language.findBySymbol("florp");
    auto glorp = language.findBySymbol("glorp");
    assert(florp);
    assert(glorp);
    assert(relations3[0]->a()->getIdentifier() == florp->getIdentifier());
    assert(relations3[0]->b()->getIdentifier() == glorp->getIdentifier());
    assert(relations3[0]->typeLabel() == "meaning_chase");

    std::cout << "syntactic_parser_test passed\n";
    return 0;
}
