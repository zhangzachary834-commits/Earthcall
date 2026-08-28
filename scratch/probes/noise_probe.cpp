#include "Singularity/OntoMath/ScalarForm.hpp"
#include <iostream>
#include <map>
#include <glm/glm.hpp>

int main() {
    using namespace OntoMath;
    auto node = std::make_unique<MathNode>();
    node->op = MathNode::Op::Noise;
    
    auto varNode = std::make_unique<MathNode>();
    varNode->op = MathNode::Op::ValueLeaf;
    varNode->variableName = "p";
    
    node->children.push_back(std::move(varNode));
    
    std::map<std::string, PropertyValue> vars;
    vars["p"] = PropertyValue(glm::vec3(0.5f, 0.0f, 0.5f));
    
    auto result = node->evaluate(vars, nullptr);
    if (result) {
        double d = 0;
        if (propertyValueToNumber(*result, d)) {
            std::cout << "Noise output: " << d << std::endl;
        } else {
            std::cout << "Failed to get double from property value" << std::endl;
        }
    } else {
        std::cout << "Evaluation failed" << std::endl;
    }
    
    return 0;
}
