import os

test_code = """
#include "support/test_harness.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ImageCodecChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include <cassert>
#include <iostream>

extern MaterialManager materials;

int main() {
    TestSupport::BootedEngineHarness harness;
    auto activeZone = harness.zones.zones()[harness.zones.currentIndex()];
    
    const unsigned char png_data[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x14, 0x49, 0x44, 0x41, 0x54, 0x08, 0x5b, 0x63, 0xfc, 0xcf, 0xc0, 0x00, 0x44, 0x0c, 0x0c, 0x8c, 0x30, 0x06, 0xb2, 0x00, 0x00, 0x39, 0x01, 0x01, 0xfd, 0xef, 0x3a, 0x63, 0x93, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    auto [obj, mat] = ImageCodecChannel::ingestPngFromMemory(png_data, sizeof(png_data), "witness_img", *activeZone);
    assert(obj != nullptr && mat != nullptr);

    obj->setDynamicProperty("witness_passed", PropertyValue(false));

    auto law = std::make_shared<Law>("witness.law", "test_author");
    
    glm::vec3 redVal(1.0f, 0.0f, 0.0f);
    auto expectedRedList = std::make_shared<PropertyList>();
    for (int i = 0; i < 2; i++) expectedRedList->elements.push_back(PropertyValue(redVal));
    
    ConditionNode cond = ConditionNode::compare("regionB", Op::Eq, PropertyValue(expectedRedList));
    law->setConditionModel(cond);

    ActionNode act = ActionNode::set("witness_passed", PropertyValue(true));
    law->setActionModel(act);

    harness.lawManager.add(law);

    std::string reason;

    OntoMath::Piecewise selectorA;
    selectorA.inputVariable = "x";
    OntoMath::Piecewise::Piece p1;
    auto yNode = std::make_unique<OntoMath::MathNode>(); yNode->op = OntoMath::MathNode::Op::ValueLeaf; yNode->variableName = "v";
    auto twoNode = std::make_unique<OntoMath::MathNode>(); twoNode->op = OntoMath::MathNode::Op::ScalarLeaf; twoNode->scalarForm = OntoMath::ScalarForm::constant(0.5);
    auto subNode = std::make_unique<OntoMath::MathNode>(); subNode->op = OntoMath::MathNode::Op::Sub;
    subNode->children.push_back(std::move(yNode)); subNode->children.push_back(std::move(twoNode));
    p1.whereNode = std::make_unique<OntoMath::MathNode>();
    p1.whereNode->op = OntoMath::MathNode::Op::LessEqZero;
    p1.whereNode->children.push_back(std::move(subNode));
    OntoMath::Piecewise::Piece p2;
    p2.guardNode = std::make_unique<ConditionNode>();
    p2.guardNode->kind = ConditionNode::Kind::All;
    selectorA.pieces.push_back(std::move(p1));
    selectorA.pieces.push_back(std::move(p2));

    bool successA = obj->elevateSurfaceRegionProperty("regionA", 0, selectorA, reason);
    assert(successA);

    OntoMath::Piecewise selectorB;
    selectorB.inputVariable = "x";
    OntoMath::Piecewise::Piece p3;
    auto xNode = std::make_unique<OntoMath::MathNode>(); xNode->op = OntoMath::MathNode::Op::ValueLeaf; xNode->variableName = "u";
    auto twoNodeB = std::make_unique<OntoMath::MathNode>(); twoNodeB->op = OntoMath::MathNode::Op::ScalarLeaf; twoNodeB->scalarForm = OntoMath::ScalarForm::constant(0.5);
    auto subNodeB = std::make_unique<OntoMath::MathNode>(); subNodeB->op = OntoMath::MathNode::Op::Sub;
    subNodeB->children.push_back(std::move(xNode)); subNodeB->children.push_back(std::move(twoNodeB));
    p3.whereNode = std::make_unique<OntoMath::MathNode>();
    p3.whereNode->op = OntoMath::MathNode::Op::LessEqZero;
    p3.whereNode->children.push_back(std::move(subNodeB));
    OntoMath::Piecewise::Piece p4;
    p4.guardNode = std::make_unique<ConditionNode>();
    p4.guardNode->kind = ConditionNode::Kind::All;
    selectorB.pieces.push_back(std::move(p3));
    selectorB.pieces.push_back(std::move(p4));

    bool successB = obj->elevateSurfaceRegionProperty("regionB", 0, selectorB, reason);
    assert(successB);

    auto redList = std::make_shared<PropertyList>();
    for (int i = 0; i < 2; i++) redList->elements.push_back(PropertyValue(redVal));
    
    auto result = PropertyPath::parse("regionA").setValue(*obj, PropertyValue(redList));
    assert(result == PropertyPath::PathResult::Ok);

    harness.lawManager.tick();

    PropertyValue val;
    assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);
    assert(std::get<bool>(val) == false);

    obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.25f), redVal);
    obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.75f), redVal);

    PropertyValue readB;
    assert(PropertyPath::parse("regionB").getValue(*obj, readB) == PropertyPath::PathResult::Ok);
    auto readBList = std::get<std::shared_ptr<PropertyList>>(readB);
    assert(readBList->elements.size() == 2);
    
    obj->notifyPropertyChanged(obj.get(), "regionB");
    harness.lawManager.tick();

    assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);
    assert(std::get<bool>(val) == true);
    
    std::cout << "TEST PASSED" << std::endl;
    return 0;
}
"""
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(test_code)
