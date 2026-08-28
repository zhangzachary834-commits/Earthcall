#include "Singularity/OntoMath/ScalarForm.hpp"
#include <iostream>

using namespace OntoMath;

int main() {
    auto y = MathNode::fromValueLeaf("y");
    auto p = MathNode::fromValueLeaf("p");
    auto c02 = MathNode::fromNumber(0.2);
    auto c20 = MathNode::fromNumber(2.0);
    
    auto scale1 = MathNode::fromOp2(MathNode::Op::Scale, std::move(c02), std::move(p));
    auto noise = MathNode::fromOp1(MathNode::Op::Noise, std::move(scale1));
    auto scale2 = MathNode::fromOp2(MathNode::Op::Scale, std::move(c20), std::move(noise));
    auto sub = MathNode::fromOp2(MathNode::Op::Sub, std::move(y), std::move(scale2));

    std::cout << sub->toJson().dump(2) << std::endl;
    return 0;
}
