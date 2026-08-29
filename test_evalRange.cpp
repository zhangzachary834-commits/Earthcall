#include <iostream>
#include <memory>
#include "src/ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

using namespace OntoMath;

int main() {
    SdfNode node;
    node.kind = NodeKind::Noise;
    node.a = 20.0f; // amplitude
    node.b = 0.1f;  // frequency
    
    // Evaluate evalRange
    Interval r = geom::evalRange(node, glm::vec3(0), glm::vec3(15.625, 1.875, 15.625));
    std::cout << "evalRange: [" << r.lo << ", " << r.hi << "]\n";
}
