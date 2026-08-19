#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Language/Lexeme.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/OntoMath/ProbabilityForm.hpp"

#include <iostream>
#include <cassert>

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

int main() {
    std::cout << "--- Running Logos Modality Tests ---" << std::endl;
    testLanguageSystem();
    testStochasticOntoMath();
    std::cout << "--- All Tests Passed ---" << std::endl;
    return 0;
}
