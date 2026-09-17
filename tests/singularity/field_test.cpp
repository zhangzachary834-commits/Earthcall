#include "Singularity/OntoMath/Field.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

bool neard(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

int main() {
    using namespace OntoMath;

    ScalarField field;
    field.mode = ScalarField::EvaluationMode::Procedural;
    field.baseDensity = 2.0f;
    field.frequency = 0.5f;
    field.amplitude = 1.5f;

    nlohmann::json jField = field.toJson();
    std::shared_ptr<ScalarField> restored = ScalarField::fromJson(jField);

    assert(restored->mode == ScalarField::EvaluationMode::Procedural);
    assert(neard(restored->baseDensity, 2.0f));
    assert(neard(restored->frequency, 0.5f));
    assert(neard(restored->amplitude, 1.5f));

    ScalarField astField;
    astField.mode = ScalarField::EvaluationMode::AST;
    astField.astDefinition = Piecewise::continuous(MathNode::fromLegacyExpression(ScalarForm::constant(5.0)));

    nlohmann::json jAst = astField.toJson();
    std::shared_ptr<ScalarField> restoredAst = ScalarField::fromJson(jAst);

    assert(restoredAst->mode == ScalarField::EvaluationMode::AST);

    std::cout << "field_test: OK\n";
    return 0;
}
