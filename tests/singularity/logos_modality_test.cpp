#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Language/Lexeme.hpp"
#include "Singularity/Language/SyntacticParser.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/OntoMath/ProbabilityForm.hpp"
#include "Relation/Relation.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <iostream>
#include <cassert>
#include <memory>

using namespace Singularity::Language;
using namespace OntoMath;

void testLanguageSystem() {
    std::cout << "Testing LanguageSystem..." << std::endl;
    auto sys = &LanguageSystem::instance();
    sys->clear();

    auto joy = sys->resolve("Joy");
    assert(joy->getSymbol() == "Joy");
    assert(sys->getAll().size() == 1);

    auto peace = sys->resolve("Peace");
    assert(peace->getSymbol() == "Peace");
    assert(sys->getAll().size() == 2);

    auto joyAgain = sys->resolve("Joy");
    assert(joy == joyAgain); // Should resolve to same Singular entity

    std::cout << "✓ LanguageSystem tests passed." << std::endl;
}

void testStochasticOntoMath() {
    std::cout << "Testing Stochastic OntoMath..." << std::endl;
    
    // Create a MathNode of Op::Stochastic, distType "uniform", args [0.0, 10.0]
    auto stochasticNode = std::make_unique<MathNode>();
    stochasticNode->op = MathNode::Op::Stochastic;
    stochasticNode->stringArg = "uniform";

    auto minNode = std::make_unique<MathNode>();
    minNode->op = MathNode::Op::ScalarLeaf;
    minNode->scalarForm = ScalarForm::constant(0.0);

    auto maxNode = std::make_unique<MathNode>();
    maxNode->op = MathNode::Op::ScalarLeaf;
    maxNode->scalarForm = ScalarForm::constant(10.0);

    stochasticNode->children.push_back(std::move(minNode));
    stochasticNode->children.push_back(std::move(maxNode));

    std::map<std::string, PropertyValue> emptyVars;

    // Sample it a few times
    for (int i = 0; i < 5; ++i) {
        auto val = stochasticNode->evaluate(emptyVars, nullptr);
        assert(val.has_value());
        double result = std::get<double>(val.value());
        std::cout << "Uniform Sample: " << result << std::endl;
        assert(result >= 0.0 && result <= 10.0);
    }

    std::cout << "✓ Stochastic OntoMath tests passed." << std::endl;
}

void testParserBindsLexemes() {
    std::cout << "Testing SyntacticParser binds Lexemes, not strings..." << std::endl;
    auto& lang = LanguageSystem::instance();
    lang.clear();

    Zone zone("lexicon-test", "default");

    auto noun = lang.intern("noun", "lexeme.pos.noun");
    auto verb = lang.intern("verb", "lexeme.pos.verb");
    auto determiner = lang.intern("determiner", "lexeme.pos.determiner");
    auto preposition = lang.intern("preposition", "lexeme.pos.preposition");
    auto the = lang.intern("the", "lexeme.the");
    auto arthur = lang.intern("arthur", "lexeme.arthur");
    auto sword = lang.intern("sword", "lexeme.sword");
    auto picked = lang.intern("picked", "lexeme.picked");
    auto up = lang.intern("up", "lexeme.up");
    auto pickedUp = lang.intern("picked_up", "lexeme.picked_up");
    auto hasInventory = lang.intern("has_inventory", "lexeme.has_inventory");

    zone.addToFormation(noun.get());
    zone.addToFormation(verb.get());
    zone.addToFormation(determiner.get());
    zone.addToFormation(preposition.get());
    zone.addToFormation(the.get());
    zone.addToFormation(arthur.get());
    zone.addToFormation(sword.get());
    zone.addToFormation(picked.get());
    zone.addToFormation(up.get());
    zone.addToFormation(pickedUp.get());
    zone.addToFormation(hasInventory.get());

    assert(zone.formation().addRelation(std::make_shared<Relation>("is_pos", *the, *determiner, true, 1.0f)));
    assert(zone.formation().addRelation(std::make_shared<Relation>("is_pos", *arthur, *noun, true, 1.0f)));
    assert(zone.formation().addRelation(std::make_shared<Relation>("is_pos", *sword, *noun, true, 1.0f)));
    assert(zone.formation().addRelation(std::make_shared<Relation>("is_pos", *picked, *verb, true, 1.0f)));
    assert(zone.formation().addRelation(std::make_shared<Relation>("is_pos", *up, *preposition, true, 1.0f)));
    assert(zone.formation().addRelation(std::make_shared<Relation>("resolves_to", *pickedUp, *hasInventory, true, 1.0f)));

    auto parsed = SyntacticParser::parse("Arthur picked up the sword", zone);
    assert(parsed.size() == 1);
    assert(parsed[0]);
    assert(parsed[0]->type == "has_inventory");
    assert(parsed[0]->a() == arthur.get());
    assert(parsed[0]->b() == sword.get());
    assert(dynamic_cast<Lexeme*>(parsed[0]->a()) != nullptr);
    assert(dynamic_cast<Lexeme*>(parsed[0]->b()) != nullptr);

    // A Relation cannot be constructed from leftover name-strings; identity
    // is the two Singulars. JSON still writes those identifiers.
    auto saved = parsed[0]->toJson();
    assert(saved["entityA"] == "lexeme.arthur");
    assert(saved["entityB"] == "lexeme.sword");
    auto rebound = Relation::fromJson(saved, [&](const std::string& id) -> Singular* {
        return zone.formation().findMemberByIdentifier(id);
    });
    assert(rebound.a() == arthur.get());
    assert(rebound.b() == sword.get());

    std::cout << "✓ SyntacticParser Lexeme-binding tests passed." << std::endl;
}

int main() {
    std::cout << "--- Running Logos Modality Tests ---" << std::endl;
    testLanguageSystem();
    testStochasticOntoMath();
    testParserBindsLexemes();
    std::cout << "--- All Tests Passed ---" << std::endl;
    return 0;
}
