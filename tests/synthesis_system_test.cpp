#include "ConstructedBeing/Singular/SynthesisSystem.hpp"
#include "ConstructedBeing/Singular/Concept.hpp"
#include "Singularity/Language/Lexeme.hpp"
#include "ConstructedBeing/Singular/SynthesisSystem.hpp"
#include "ConstructedBeing/Singular/Concept.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Language/Lexeme.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <iostream>
#include <cassert>
#include <memory>
#include <vector>

void testUniversalConceptRegistry() {
    auto concept = std::make_shared<Concept>("TestConcept");
    UniversalConceptRegistry::instance().registerConcept(concept);
    
    auto fetched = UniversalConceptRegistry::instance().findConcept(concept->getIdentifier());
    assert(fetched != nullptr && fetched->getIdentifier() == concept->getIdentifier());
    std::cout << "testUniversalConceptRegistry passed.\n";
}

void testSynthesisInstantiation() {
    auto concept = std::make_shared<Concept>("ConceptA");
    
    Concept::SingularTemplate lexemeTmpl;
    lexemeTmpl.classType = "Lexeme";
    lexemeTmpl.initialProperties["symbol"] = PropertyValue(std::string("HelloWorld"));
    concept->addMember(lexemeTmpl);
    
    Concept::SingularTemplate zoneTmpl;
    zoneTmpl.classType = "Zone";
    zoneTmpl.initialProperties["name"] = PropertyValue(std::string("NewZone"));
    concept->addMember(zoneTmpl);
    
    std::vector<Singular*> inputs;
    auto outputs = SynthesisSystem::instance().synthesize(inputs, *concept, {});
    
    assert(outputs.size() == 2);
    
    auto lexeme = std::dynamic_pointer_cast<Singularity::Language::Lexeme>(outputs[0]);
    assert(lexeme != nullptr);
    assert(lexeme->getSymbol() == "HelloWorld");
    
    auto zone = std::dynamic_pointer_cast<Zone>(outputs[1]);
    assert(zone != nullptr);
    assert(zone->name() == "NewZone");
    
    std::cout << "testSynthesisInstantiation passed.\n";
}

void testSynthesisMapping() {
    Object sourceObj;
    sourceObj.setPosition(glm::vec3(5.0f, 10.0f, 0.0f));
    sourceObj.setDynamicProperty("custom_str", PropertyValue(std::string("mapped_text")));
    
    auto concept = std::make_shared<Concept>("MappingConcept");
    Concept::SingularTemplate lexemeTmpl;
    lexemeTmpl.classType = "Lexeme";
    concept->addMember(lexemeTmpl);
    
    SynthesisMapping strMapping;
    strMapping.source = PropertyPath::parse("custom_str");
    strMapping.target = PropertyPath::parse("symbol");
    strMapping.hasExact = true;
    strMapping.exact.inputVariable = "x";
    
    OntoMath::Piecewise::Piece p1;
    auto node1 = std::make_shared<OntoMath::MathNode>();
    node1->op = OntoMath::MathNode::Op::ValueLeaf;
    node1->variableName = "x";
    p1.mathNode = node1;
    strMapping.exact.pieces.push_back(p1);
    
    std::vector<Singular*> inputs = { &sourceObj };
    auto outputs = SynthesisSystem::instance().synthesize(inputs, *concept, {strMapping});
    
    assert(outputs.size() == 1);
    auto lexeme = std::dynamic_pointer_cast<Singularity::Language::Lexeme>(outputs[0]);
    assert(lexeme != nullptr);
    
    // Since symbol maps custom_str
    assert(lexeme->getSymbol() == "mapped_text");
    
    std::cout << "testSynthesisMapping passed.\n";
}

void testLawSynthesisIntegration() {
    auto concept = std::make_shared<Concept>("LawTriggeredConcept");
    Concept::SingularTemplate objTmpl;
    objTmpl.classType = "Object";
    concept->addMember(objTmpl);
    UniversalConceptRegistry::instance().registerConcept(concept);
    
    ActionNode synthAction;
    synthAction.kind = ActionNode::Kind::Synthesize;
    synthAction.conceptId = concept->getIdentifier();
    
    auto executor = synthAction.compile();
    
    Object lawSubject;
    ECA::Event triggerEvent;
    triggerEvent.subject = &lawSubject;
    
    // Arm the trace
    ActionNode::TraceScope traceScope;
    
    executor(triggerEvent, lawSubject);
    
    auto trace = ActionNode::activeTrace();
    assert(trace != nullptr);
    assert(trace->fired());
    assert(trace->anyWrote() == true);
    
    std::cout << "testLawSynthesisIntegration passed.\n";
}

int main() {
    std::cout << "Running Universal Synthesis System Tests...\n";
    testUniversalConceptRegistry();
    testSynthesisInstantiation();
    testSynthesisMapping();
    testLawSynthesisIntegration();
    std::cout << "All Universal Synthesis tests passed!\n";
    return 0;
}
