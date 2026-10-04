#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Singularity/Language/Utterance.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"

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

    // Create a JSON-style unbound legacy relation referencing alpha & beta.
    nlohmann::json relJson = {
        {"type", "instance-of"},
        {"entityA", "alpha_entity"},
        {"entityB", "beta_entity"},
        {"directed", true},
        {"weight", 1.0f}
    };

    // Relation loaded without resolver (unbound endpoints, stored in saved ids)
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

void testLexemeTypedRelationsUseSemanticIdentity() {
    std::cout << "Testing Lexeme-Typed Relations use semantic identity..." << std::endl;

    Lexeme categoryLexeme("instance-of", "lexeme.instance_of");
    Object sourceObj;
    sourceObj.setObjectID("source_node");
    Object targetObj;
    targetObj.setObjectID("target_node");

    // Construct Relation using Lexeme as type. The stable Lexeme ID, not the
    // spelling, is the Relation kind identity.
    Relation rel(categoryLexeme, sourceObj, targetObj, true, 1.0f);

    assert(rel.type == "lexeme.instance_of");
    assert(rel.typeLabel() == "instance-of");
    assert(rel.getTypeLexeme() == &categoryLexeme);
    assert(rel.a() == &sourceObj);
    assert(rel.b() == &targetObj);

    // Test setTypeLexeme update
    Lexeme subcategoryLexeme("subcategory-of", "lexeme.subcategory_of");
    rel.setTypeLexeme(&subcategoryLexeme);
    assert(rel.type == "lexeme.subcategory_of");
    assert(rel.typeLabel() == "subcategory-of");
    assert(rel.getTypeLexeme() == &subcategoryLexeme);

    // Two authored Relation kinds may have the same spelling without becoming
    // the same semantic relation. This is the language_meaning_probe collision
    // that used to merge both edges in RelationManager.
    Lexeme ownsA("owns", "relation-kind.owns.a");
    Lexeme ownsB("owns", "relation-kind.owns.b");
    auto edgeA = std::make_shared<Relation>(ownsA, sourceObj, targetObj, true, 0.5f);
    auto edgeB = std::make_shared<Relation>(ownsB, sourceObj, targetObj, true, 0.5f);
    RelationManager manager;
    manager.add(edgeA);
    manager.add(edgeB);
    assert(edgeA->typeLabel() == edgeB->typeLabel());
    assert(edgeA->type != edgeB->type);
    assert(edgeA->getIdentifier() != edgeB->getIdentifier());
    assert(manager.getAll().size() == 2);

    // Serialization keeps the human label for compatibility while persisting
    // the true semantic type identity separately and restoring the Lexeme.
    const auto saved = edgeA->toJson();
    assert(saved.at("type") == "owns");
    assert(saved.at("typeId") == "relation-kind.owns.a");
    Relation rebound = Relation::fromJson(saved, [&](const std::string& id) -> Singular* {
        if (id == ownsA.getIdentifier()) return &ownsA;
        if (id == sourceObj.getIdentifier()) return &sourceObj;
        if (id == targetObj.getIdentifier()) return &targetObj;
        return nullptr;
    });
    assert(rebound.getTypeLexeme() == &ownsA);
    assert(rebound.type == ownsA.getIdentifier());
    assert(rebound.typeLabel() == "owns");

    std::cout << "✓ Lexeme-Typed semantic identity test passed." << std::endl;
}

void testAuthoredCppInheritanceConstitutiveOpcode() {
    std::cout << "Testing authored C++-inheritance Relation substance..." << std::endl;

    // The opcode is engine substrate; which authored Relation kind carries it
    // is data on the Relation-kind Lexeme.
    Lexeme cppInstanceOf("instance-of", "relation-kind.cpp-instance-of");
    cppInstanceOf.setDynamicProperty(
        Relation::kConstitutiveOpcodeProperty,
        static_cast<int>(Relation::ConstitutiveOpcode::CppInheritance));

    // Endpoint B is an authored descriptor saying which irreducible C++
    // ontology kind it denotes. This does not make "Object" an authored
    // domain category; it exposes the existing C++ inheritance checker as an
    // operation Persons may use as constitutive Relation substance.
    Object objectKindDescriptor;
    objectKindDescriptor.setObjectID("descriptor.cpp.object");
    objectKindDescriptor.setPhysicalObject(0);
    objectKindDescriptor.setDynamicProperty(
        Relation::kCppBeingKindProperty,
        static_cast<int>(ConditionNode::BeingKind::Object));

    Object actualObject;
    actualObject.setObjectID("actual.object");
    Relation holds(cppInstanceOf, actualObject, objectKindDescriptor, true, 1.0f);
    assert(holds.evaluateConstitutive() == Relation::ConstitutiveStatus::Holds);

    Lexeme notAnObject("not-an-object", "lexeme.not-an-object");
    Relation violated(cppInstanceOf, notAnObject, objectKindDescriptor, true, 1.0f);
    assert(violated.evaluateConstitutive() == Relation::ConstitutiveStatus::Violated);

    // Same visible label, different Relation-kind identity, no opcode: this is
    // an ordinary authored semantic relation and is not hijacked by the C++
    // inheritance meaning of the first kind.
    Lexeme authoredInstanceOf("instance-of", "relation-kind.authored-instance-of");
    Relation independent(authoredInstanceOf, notAnObject, objectKindDescriptor, true, 1.0f);
    assert(independent.evaluateConstitutive() == Relation::ConstitutiveStatus::NotApplicable);

    std::cout << "✓ Authored C++ inheritance opcode test passed." << std::endl;
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

    assert(fm.findProperty("displayName") != nullptr);

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
    assert(rels[0]->typeLabel() == "occurrence-of");
    assert(rels[0]->a() == &utt);
    assert(rels[0]->b() == &pawnLex);
    assert(rels[1]->b() == &moveLex);

    std::cout << "✓ Utterance Occurrence-of Relations test passed." << std::endl;
}

int main() {
    std::cout << "--- Running Relation Retry & Lexeme-Type Tests ---" << std::endl;
    testPendingRelationRetry();
    testLexemeTypedRelationsUseSemanticIdentity();
    testAuthoredCppInheritanceConstitutiveOpcode();
    testFirstMoverAsSingular();
    testUtteranceOccurrenceRelations();
    std::cout << "--- All Tests Passed ---" << std::endl;
    return 0;
}
