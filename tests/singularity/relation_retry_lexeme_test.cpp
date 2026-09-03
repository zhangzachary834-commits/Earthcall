#include "Relation/Relation.hpp"
#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Singularity/Language/Utterance.hpp"

#include <iostream>
#include <cassert>
#include <memory>

using namespace Singularity::Language;

void testPendingRelationRetry() {
    std::cout << "Testing Formation Pending Relation Retry..." << std::endl;

    Formation formation;
    auto objA = std::make_shared<Object>();
    objA->setObjectID("alpha_entity");

    auto objB = std::make_shared<Object>();
    objB->setObjectID("beta_entity");

    // Create a JSON-style unbound relation or relation referencing alpha & beta
    nlohmann::json relJson = {
        {"type", "instance-of"},
        {"entityA", "alpha_entity"},
        {"entityB", "beta_entity"},
        {"directed", true},
        {"weight", 1.0f}
    };

    // Relation loaded without resolver (unbound endpoints, stored in _savedA / _savedB)
    auto rel = std::make_shared<Relation>(Relation::fromJson(relJson));
    assert(!rel->hasEndpoints());
    assert(rel->aId() == "alpha_entity");
    assert(rel->bId() == "beta_entity");

    // Adding unbound relation to formation should place it in pendingRelations
    bool added = formation.addRelation(rel);
    assert(!added);
    assert(formation.getPendingRelations().size() == 1);
    assert(formation.relations().getAll().empty());

    // Add first member (objA) - rel should remain pending since objB is not in formation yet
    formation.addMember(objA.get());
    assert(formation.getPendingRelations().size() == 1);
    assert(formation.relations().getAll().empty());

    // Add second member (objB) - retryPendingRelations should bind rel and admit it
    formation.addMember(objB.get());
    assert(formation.getPendingRelations().empty());
    assert(formation.relations().getAll().size() == 1);
    assert(rel->hasEndpoints());
    assert(rel->a() == objA.get());
    assert(rel->b() == objB.get());

    std::cout << "✓ Formation Pending Relation Retry test passed." << std::endl;
}

void testLexemeTypedRelations() {
    std::cout << "Testing Lexeme-Typed Relations..." << std::endl;

    Lexeme categoryLexeme("instance-of", "lexeme.instance_of");
    Object sourceObj;
    sourceObj.setObjectID("source_node");
    Object targetObj;
    targetObj.setObjectID("target_node");

    // Construct Relation using Lexeme as type
    Relation rel(categoryLexeme, sourceObj, targetObj, true, 1.0f);

    assert(rel.type == "instance-of");
    assert(rel.getTypeLexeme() == &categoryLexeme);
    assert(rel.a() == &sourceObj);
    assert(rel.b() == &targetObj);

    // Test setTypeLexeme update
    Lexeme subcategoryLexeme("subcategory-of", "lexeme.subcategory_of");
    rel.setTypeLexeme(&subcategoryLexeme);
    assert(rel.type == "subcategory-of");
    assert(rel.getTypeLexeme() == &subcategoryLexeme);

    std::cout << "✓ Lexeme-Typed Relations test passed." << std::endl;
}

void testFirstMoverAsSingular() {
    std::cout << "Testing FirstMover as Singular and First Mover Relations..." << std::endl;

    Identity::FirstMover fm;
    fm.id = Identity::SingularId::mintOpaque();
    fm.displayName = "Claude";
    fm.kind = Identity::FirstMover::Kind::Model;

    // Verify Singular interface
    Singular* sFm = &fm;
    assert(sFm->getIdentifier() == fm.id.toString());
    assert(!sFm->getIdentifier().empty());

    assert(fm.findProperty("displayName") != nullptr);
    assert(fm.displayName == "Claude");

    // Relations between First Movers are Singulars
    Identity::FirstMover fm2;
    fm2.id = Identity::SingularId::mintOpaque();
    fm2.displayName = "Gemini";

    Relation authorRel("collaborates-with", fm, fm2, false);
    assert(authorRel.a() == &fm);
    assert(authorRel.b() == &fm2);
    assert(authorRel.aId() == fm.id.toString());
    assert(authorRel.bId() == fm2.id.toString());

    std::cout << "✓ FirstMover as Singular test passed." << std::endl;
}

void testUtteranceOccurrenceRelations() {
    std::cout << "Testing Utterance Occurrence-of Relations..." << std::endl;

    Utterance utt("The pawn moves forward", "utt-1", 100.0);
    Lexeme pawnLex("pawn", "lexeme.pawn");
    Lexeme moveLex("moves", "lexeme.move");

    utt.addLexeme(&pawnLex);
    utt.addLexeme(&moveLex);

    auto rels = utt.createOccurrenceRelations();
    assert(rels.size() == 2);
    assert(rels[0]->type == "occurrence-of");
    assert(rels[0]->a() == &utt);
    assert(rels[0]->b() == &pawnLex);
    assert(rels[1]->b() == &moveLex);

    std::cout << "✓ Utterance Occurrence-of Relations test passed." << std::endl;
}

int main() {
    std::cout << "--- Running Relation Retry & Lexeme-Type Tests ---" << std::endl;
    testPendingRelationRetry();
    testLexemeTypedRelations();
    testFirstMoverAsSingular();
    testUtteranceOccurrenceRelations();
    std::cout << "--- All Tests Passed ---" << std::endl;
    return 0;
}
