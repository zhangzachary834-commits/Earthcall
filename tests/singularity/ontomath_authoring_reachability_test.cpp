#include "Singularity/Screen/MathEditors.hpp"
#include "Singularity/OntoMath/LinearAlgebra.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

using OntoMath::MathNode;
using OntoMath::MatrixValue;

MatrixValue makeMatrix(std::size_t rows, std::size_t cols,
                       std::initializer_list<double> elements) {
    auto matrix = MatrixValue::create(rows, cols, std::vector<double>(elements));
    assert(matrix);
    return std::move(*matrix);
}

class MatrixCarrier final : public Singular {
public:
    MatrixCarrier() : _matrix(makeMatrix(2, 2, {1, 2, 3, 4})) {}

    std::string getIdentifier() const override { return "matrix-carrier"; }

    const MatrixValue& matrix() const { return _matrix; }

protected:
    void buildProperties() override {
        registerProperty(std::make_unique<PropertyRef<MatrixCarrier, MatrixValue>>(
            "matrix", this, &_matrix));
    }

private:
    MatrixValue _matrix;
};

} // namespace

int main() {
    using Op = MathNode::Op;

    // "Editor-created" means the same pure initialization helper the ImGui
    // palette calls when a Person selects a mathematical form.
    MathNode authored;
    Rendering::MathEd::initializeMathNodeForEditor(authored, Op::MatrixConstruct);
    assert(authored.op == Op::MatrixConstruct);
    assert(authored.matrixRows == 2);
    assert(authored.matrixCols == 2);
    assert(authored.children.size() == 4);

    const double entries[4] = {4.0, 7.0, 2.0, 6.0};
    for (std::size_t i = 0; i < 4; ++i) {
        authored.children[i]->op = Op::ScalarLeaf;
        authored.children[i]->scalarForm = OntoMath::ScalarForm::constant(entries[i]);
    }

    const nlohmann::json authoredJson = authored.toJson();
    auto restored = MathNode::fromJson(authoredJson);
    assert(restored);
    assert(restored->op == Op::MatrixConstruct);
    assert(restored->matrixRows == 2 && restored->matrixCols == 2);
    assert(restored->children.size() == 4);
    assert(restored->print() == authored.print());

    auto value = restored->evaluate({});
    assert(value && std::holds_alternative<MatrixValue>(*value));
    const MatrixValue restoredMatrix = std::get<MatrixValue>(*value);
    assert(restoredMatrix == makeMatrix(2, 2, {4, 7, 2, 6}));

    // Identity has an authored dimension but no element children.
    MathNode identity;
    Rendering::MathEd::initializeMathNodeForEditor(identity, Op::MatrixIdentity);
    assert(identity.matrixRows == 4 && identity.matrixCols == 4);
    assert(identity.children.empty());

    auto identityRestored = MathNode::fromJson(identity.toJson());
    assert(identityRestored);
    assert(identityRestored->matrixRows == 4 && identityRestored->matrixCols == 4);
    assert(identityRestored->children.empty());

    // The editor gives every fixed-arity matrix operation the right structural
    // shape before the Person authors its children.
    MathNode multiply;
    Rendering::MathEd::initializeMathNodeForEditor(multiply, Op::MatrixMultiply);
    assert(multiply.children.size() == 2);

    MathNode inverse;
    Rendering::MathEd::initializeMathNodeForEditor(inverse, Op::MatrixInverse);
    assert(inverse.children.size() == 1);

    // Unknown future operations survive load/save byte-for-meaning unchanged.
    const nlohmann::json future = {
        {"op", 240},
        {"futureName", "matrix-exponential"},
        {"rows", 4},
        {"cols", 4},
        {"opaquePayload", {{"keep", true}, {"version", 7}}}
    };
    auto unknown = MathNode::fromJson(future);
    assert(unknown);
    assert(unknown->op == Op::Unsupported);
    assert(unknown->toJson() == future);

    // Range analysis has no Matrix interval domain yet. Refuse the proof rather
    // than returning a scalar infinity and lying about the expression's type.
    assert(!restored->evalRange({}).has_value());

    // Registered PropertyRef<MatrixValue> is legible through the SAME Law
    // get/set bridge as existing scalar/vector state.
    MatrixCarrier carrier;
    const PropertyPath path = PropertyPath::parse("matrix");

    PropertyValue before;
    assert(lawGetValue(carrier, path, before));
    assert(std::holds_alternative<MatrixValue>(before));
    assert(std::get<MatrixValue>(before) == makeMatrix(2, 2, {1, 2, 3, 4}));

    const MatrixValue replacement = makeMatrix(2, 2, {9, 8, 7, 6});
    const auto writeResult = lawSetValue(carrier, path, PropertyValue(replacement));
    assert(writeResult == PropertyPath::PathResult::Ok);

    PropertyValue after;
    assert(lawGetValue(carrier, path, after));
    assert(std::holds_alternative<MatrixValue>(after));
    assert(std::get<MatrixValue>(after) == replacement);
    assert(carrier.matrix() == replacement);

    // Re-writing the same authored value is recognized as unchanged.
    assert(lawSetValue(carrier, path, PropertyValue(replacement)) ==
           PropertyPath::PathResult::Unchanged);

    std::puts("ontomath_authoring_reachability_test: PASS");
    return 0;
}
