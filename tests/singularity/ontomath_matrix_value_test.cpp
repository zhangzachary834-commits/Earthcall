#include "Singularity/OntoMath/LinearAlgebra.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <glm/glm.hpp>

#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

namespace {

bool near(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}

std::unique_ptr<OntoMath::MathNode> valueLeaf(const char* name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

} // namespace

int main() {
    using OntoMath::MathNode;
    using OntoMath::MathType;
    using OntoMath::MatrixValue;
    using OntoMath::ValueKind;

    // Shape is intrinsic and malformed storage cannot become a MatrixValue.
    auto matrix = MatrixValue::create(2, 3, {1.0, 2.0, 3.0,
                                             4.0, 5.0, 6.0});
    assert(matrix.has_value());
    assert(matrix->valid());
    assert(matrix->rows() == 2);
    assert(matrix->cols() == 3);
    assert(near(matrix->at(0, 0), 1.0));
    assert(near(matrix->at(0, 2), 3.0));
    assert(near(matrix->at(1, 0), 4.0));
    assert(near(matrix->at(1, 2), 6.0));
    assert(!MatrixValue::create(2, 3, {1.0, 2.0}).has_value());
    assert(!MatrixValue::create(0, 3, {}).has_value());

    // PropertyValue now carries OntoMath's matrix value directly and preserves
    // dimensions + canonical row-major logical elements through JSON.
    PropertyValue pv(*matrix);
    assert(std::holds_alternative<MatrixValue>(pv));
    const nlohmann::json j = propertyValueToJson(pv);
    assert(j.at("t") == "matrix");
    assert(j.at("rows") == 2);
    assert(j.at("cols") == 3);

    const PropertyValue roundTrip = propertyValueFromJson(j);
    assert(std::holds_alternative<MatrixValue>(roundTrip));
    assert(std::get<MatrixValue>(roundTrip) == *matrix);

    // The Zone persistence format is msgpack over the same JSON structure.
    // Prove dimensions and elements survive that binary boundary too.
    const std::vector<std::uint8_t> packed = nlohmann::json::to_msgpack(j);
    const nlohmann::json unpacked = nlohmann::json::from_msgpack(packed);
    const PropertyValue msgpackRoundTrip = propertyValueFromJson(unpacked);
    assert(std::holds_alternative<MatrixValue>(msgpackRoundTrip));
    assert(std::get<MatrixValue>(msgpackRoundTrip) == *matrix);

    // Malformed serialized matrix shape REFUSES instead of padding/truncating.
    const nlohmann::json malformed = {
        {"t", "matrix"},
        {"rows", 2},
        {"cols", 3},
        {"elements", nlohmann::json::array({1.0, 2.0})}
    };
    const PropertyValue malformedValue = propertyValueFromJson(malformed);
    assert(std::holds_alternative<std::monostate>(malformedValue));

    // The GLM bridge is explicit and lossless for legacy 4x4 transform storage.
    glm::mat4 legacy(0.0f);
    double seed = 1.0;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            legacy[c][r] = static_cast<float>(seed);
            seed += 0.25;
        }
    }
    const MatrixValue bridged = MatrixValue::fromGlmMat4(legacy);
    assert(bridged.rows() == 4 && bridged.cols() == 4);
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            assert(near(bridged.at(row, col), legacy[col][row]));
        }
    }
    const auto back = bridged.toGlmMat4();
    assert(back.has_value());
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            assert(near((*back)[c][r], legacy[c][r]));
        }
    }
    assert(!matrix->toGlmMat4().has_value());

    // Matrix is now a first-class compile-time OntoMath type and dimensions
    // survive the ValueLeaf judgement.
    OntoMath::TypeEnv env{
        {"M", MathType::matrix(2, 3)},
        // Existing callers remain source-compatible: ValueKind implicitly
        // constructs a MathType.
        {"s", ValueKind::Scalar}
    };

    MathNode leaf;
    leaf.op = MathNode::Op::ValueLeaf;
    leaf.variableName = "M";
    const auto leafType = leaf.typeOf(env);
    assert(leafType);
    assert(leafType.kind == ValueKind::Matrix);
    assert(leafType.type.rows == 2);
    assert(leafType.type.cols == 3);
    assert(leafType.type.hasValidMatrixShape());

    OntoMath::TypeEnv malformedEnv{{"bad", MathType::matrix(0, 3)}};
    MathNode malformedLeaf;
    malformedLeaf.op = MathNode::Op::ValueLeaf;
    malformedLeaf.variableName = "bad";
    assert(!malformedLeaf.typeOf(malformedEnv));

    // Rung 1 is intentionally NOT Rung 2: old scalar/vector operations do not
    // acquire matrix meaning by accident. Matrix algebra gets dedicated,
    // append-only authored operations next.
    MathNode add;
    add.op = MathNode::Op::Add;
    add.children.push_back(valueLeaf("M"));
    add.children.push_back(valueLeaf("M"));
    const auto addType = add.typeOf(env);
    assert(!addType);

    std::map<std::string, PropertyValue> values{{"M", PropertyValue(*matrix)}};
    assert(!add.evaluate(values).has_value());

    // Legacy glm::mat4 serialization remains exactly available for old saves.
    const nlohmann::json legacyJson = propertyValueToJson(PropertyValue(legacy));
    assert(legacyJson.at("t") == "mat4");
    const PropertyValue legacyRoundTrip = propertyValueFromJson(legacyJson);
    assert(std::holds_alternative<glm::mat4>(legacyRoundTrip));

    std::puts("ontomath_matrix_value_test: PASS");
    return 0;
}
