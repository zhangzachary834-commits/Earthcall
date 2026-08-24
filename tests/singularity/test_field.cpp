#include "Singularity/OntoMath/Field.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include <iostream>
#include <cassert>

using namespace OntoMath;
using namespace geom;

void testFieldNodeProperties() {
    FieldNode node("test_field");
    
    // Ensure properties are built and retrievable
    node.listProperties();
    
    PropertyValue val;
    PropertyPath densityPath = PropertyPath::parse("field.baseDensity");
    auto readResult = densityPath.getValue(node, val);
    assert(readResult == PropertyPath::PathResult::Ok);
    
    float readDensity = std::get<float>(val);
    assert(readDensity == 1.0f); // Default value

    // Modify via PropertyPath
    PropertyValue newVal = 3.14f;
    auto writeResult = densityPath.setValue(node, newVal);
    assert(writeResult == PropertyPath::PathResult::Ok);
    
    assert(node.field->baseDensity == 3.14f);
    
    std::cout << "testFieldNodeProperties passed." << std::endl;
}

int main() {
    testFieldNodeProperties();
    std::cout << "All Field tests passed." << std::endl;
    return 0;
}
