#include "Field.hpp"

namespace OntoMath {

ScalarField::ScalarField() {
    // Default to a continuous scalar variable mapping, ensuring it is a valid AST
    astDefinition = Piecewise::continuous(
        MathNode::fromLegacyExpression(ScalarForm::constant(0.0))
    );
}

} // namespace OntoMath
