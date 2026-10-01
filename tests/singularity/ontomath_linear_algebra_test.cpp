#include "Singularity/OntoMath/LinearAlgebra.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

using OntoMath::MathNode;
using OntoMath::MathType;
using OntoMath::MatrixValue;
using OntoMath::ValueKind;

bool near(double a, double b, double eps = 1e-8) {
    return std::fabs(a - b) <= eps;
}

bool nearMatrix(const MatrixValue& a, const MatrixValue& b, double eps = 1e-8) {
    if (a.rows() != b.rows() || a.cols() != b.cols()) return false;
    for (std::size_t r = 0; r < a.rows(); ++r) {
        for (std::size_t c = 0; c < a.cols(); ++c) {
            if (!near(a.at(r, c), b.at(r, c), eps)) return false;
        }
    }
    return true;
}

std::unique_ptr<MathNode> scalar(double value) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(value);
    return n;
}

std::unique_ptr<MathNode> value(const char* name) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

std::unique_ptr<MathNode> vector3(double x, double y, double z) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::VectorConstruct;
    n->children.push_back(scalar(x));
    n->children.push_back(scalar(y));
    n->children.push_back(scalar(z));
    return n;
}

std::unique_ptr<MathNode> matrixNode(std::size_t rows, std::size_t cols,
                                     std::initializer_list<double> elements) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::MatrixConstruct;
    n->matrixRows = rows;
    n->matrixCols = cols;
    for (double x : elements) n->children.push_back(scalar(x));
    return n;
}

MatrixValue mustMatrix(std::size_t rows, std::size_t cols,
                       std::initializer_list<double> elements) {
    auto m = MatrixValue::create(rows, cols, std::vector<double>(elements));
    assert(m);
    return std::move(*m);
}

} // namespace

int main() {
    // Serialization ABI: Rung 2 is append-only after Noise=29.
    static_assert(static_cast<int>(MathNode::Op::Noise) == 29);
    static_assert(static_cast<int>(MathNode::Op::MatrixConstruct) == 30);
    static_assert(static_cast<int>(MathNode::Op::MatrixIdentity) == 31);
    static_assert(static_cast<int>(MathNode::Op::MatrixAdd) == 32);
    static_assert(static_cast<int>(MathNode::Op::MatrixSub) == 33);
    static_assert(static_cast<int>(MathNode::Op::MatrixScale) == 34);
    static_assert(static_cast<int>(MathNode::Op::MatrixMultiply) == 35);
    static_assert(static_cast<int>(MathNode::Op::MatrixVectorMultiply) == 36);
    static_assert(static_cast<int>(MathNode::Op::MatrixTranspose) == 37);
    static_assert(static_cast<int>(MathNode::Op::MatrixDeterminant) == 38);
    static_assert(static_cast<int>(MathNode::Op::MatrixInverse) == 39);
    static_assert(static_cast<int>(MathNode::Op::Unsupported) == 255);

    // Hand-computable rectangular multiplication:
    // [1 2 3] [ 7  8]   [ 58  64]
    // [4 5 6] [ 9 10] = [139 154]
    //         [11 12]
    const MatrixValue a23 = mustMatrix(2, 3, {1,2,3, 4,5,6});
    const MatrixValue b32 = mustMatrix(3, 2, {7,8, 9,10, 11,12});
    auto product = OntoMath::matrixMultiply(a23, b32);
    assert(product);
    assert(nearMatrix(*product, mustMatrix(2, 2, {58,64, 139,154})));

    // Add/sub/scale preserve shape.
    auto aPlusA = OntoMath::matrixAdd(a23, a23);
    assert(aPlusA && nearMatrix(*aPlusA, mustMatrix(2,3,{2,4,6, 8,10,12})));
    auto backToA = OntoMath::matrixSubtract(*aPlusA, a23);
    assert(backToA && nearMatrix(*backToA, a23));
    auto scaled = OntoMath::matrixScale(a23, -0.5);
    assert(scaled && nearMatrix(*scaled, mustMatrix(2,3,{-0.5,-1,-1.5, -2,-2.5,-3})));

    // Transpose swaps the mathematical dimensions, not a storage convention.
    auto transposed = OntoMath::matrixTranspose(a23);
    assert(transposed);
    assert(nearMatrix(*transposed, mustMatrix(3,2,{1,4, 2,5, 3,6})));

    // 2x2 determinant/inverse, with singular inverse as explicit refusal.
    const MatrixValue a22 = mustMatrix(2,2,{4,7, 2,6});
    auto det = OntoMath::matrixDeterminant(a22);
    assert(det && near(*det, 10.0));

    auto inv = OntoMath::matrixInverse(a22);
    assert(inv);
    assert(nearMatrix(*inv, mustMatrix(2,2,{0.6,-0.7, -0.2,0.4}), 1e-10));

    auto identity2 = OntoMath::matrixIdentity(2);
    assert(identity2);
    auto aInv = OntoMath::matrixMultiply(a22, *inv);
    assert(aInv && nearMatrix(*aInv, *identity2, 1e-10));

    const MatrixValue singular = mustMatrix(2,2,{1,2, 2,4});
    auto singularDet = OntoMath::matrixDeterminant(singular);
    assert(singularDet && near(*singularDet, 0.0));
    assert(!OntoMath::matrixInverse(singular));

    // Hand-computable 3x3 with det=1, so its inverse is integral.
    const MatrixValue a33 = mustMatrix(3,3,{1,2,3, 0,1,4, 5,6,0});
    auto det33 = OntoMath::matrixDeterminant(a33);
    assert(det33 && near(*det33, 1.0));
    auto inv33 = OntoMath::matrixInverse(a33);
    assert(inv33);
    assert(nearMatrix(*inv33,
                      mustMatrix(3,3,{-24,18,5, 20,-15,-4, -5,4,1}),
                      1e-10));

    // The pivot policy is scale-relative: tiny-but-invertible is still
    // invertible, rather than being rejected by an absolute epsilon.
    const MatrixValue tiny = mustMatrix(2,2,{1e-20,0, 0,2e-20});
    auto tinyInv = OntoMath::matrixInverse(tiny);
    assert(tinyInv);
    assert(near(tinyInv->at(0,0), 1e20, 1e8));
    assert(near(tinyInv->at(1,1), 5e19, 1e8));

    // 3x3 times vec3 is the only matrix-vector seam in Rung 2. 4x4*vec3 is
    // deliberately refused because it would have to guess point(w=1) versus
    // direction(w=0), a distinction frozen by Rung 0.
    const MatrixValue diag3 = mustMatrix(3,3,{2,0,0, 0,3,0, 0,0,4});
    auto mv = OntoMath::matrixMultiplyVec3(diag3, glm::vec3(1,2,3));
    assert(mv);
    assert(glm::length(*mv - glm::vec3(2,6,12)) < 1e-6f);

    auto identity4 = OntoMath::matrixIdentity(4);
    assert(identity4);
    assert(!OntoMath::matrixMultiplyVec3(*identity4, glm::vec3(1,2,3)));

    // 4x4 affine witness: generic OntoMath inverse/determinant are already
    // sufficient for transform matrices without yet defining affine authoring.
    glm::mat4 glmAffine = glm::translate(glm::mat4(1.0f), glm::vec3(3,-2,5));
    glmAffine = glm::rotate(glmAffine, glm::radians(25.0f), glm::vec3(0,1,0));
    glmAffine = glm::scale(glmAffine, glm::vec3(2,3,4));
    const MatrixValue affine = MatrixValue::fromGlmMat4(glmAffine);

    auto affineDet = OntoMath::matrixDeterminant(affine);
    assert(affineDet && near(*affineDet, 24.0, 1e-4));
    auto affineInv = OntoMath::matrixInverse(affine);
    assert(affineInv);
    auto affineRoundTrip = OntoMath::matrixMultiply(affine, *affineInv);
    assert(affineRoundTrip && nearMatrix(*affineRoundTrip, *identity4, 2e-6));

    // Associativity witness within floating tolerance.
    const MatrixValue aa = mustMatrix(2,2,{1,2, 3,4});
    const MatrixValue bb = mustMatrix(2,2,{2,0, 1,2});
    const MatrixValue cc = mustMatrix(2,2,{0,1, 2,3});
    auto ab = OntoMath::matrixMultiply(aa, bb);
    auto bc = OntoMath::matrixMultiply(bb, cc);
    assert(ab && bc);
    auto left = OntoMath::matrixMultiply(*ab, cc);
    auto right = OntoMath::matrixMultiply(aa, *bc);
    assert(left && right && nearMatrix(*left, *right, 1e-10));

    // Dimension mismatch is a refusal in both pure algebra and the AST type
    // judgement — impossible mathematics never reaches execution as a guess.
    assert(!OntoMath::matrixAdd(a23, b32));
    assert(!OntoMath::matrixMultiply(a23, a23));

    OntoMath::TypeEnv env{
        {"A", MathType::matrix(2,3)},
        {"B", MathType::matrix(3,2)},
        {"bad", MathType::matrix(4,4)},
        {"M3", MathType::matrix(3,3)},
        {"v", MathType::vector(3)},
        {"s", ValueKind::Scalar}
    };

    MathNode mulType;
    mulType.op = MathNode::Op::MatrixMultiply;
    mulType.children.push_back(value("A"));
    mulType.children.push_back(value("B"));
    auto mt = mulType.typeOf(env);
    assert(mt && mt.kind == ValueKind::Matrix);
    assert(mt.type.rows == 2 && mt.type.cols == 2);

    MathNode badMul;
    badMul.op = MathNode::Op::MatrixMultiply;
    badMul.children.push_back(value("A"));
    badMul.children.push_back(value("bad"));
    assert(!badMul.typeOf(env));

    MathNode mvType;
    mvType.op = MathNode::Op::MatrixVectorMultiply;
    mvType.children.push_back(value("M3"));
    mvType.children.push_back(value("v"));
    auto mvt = mvType.typeOf(env);
    assert(mvt && mvt.kind == ValueKind::Vector && mvt.type.vectorDimension == 3);

    // Authored construction is a real AST node: dimensions survive copy,
    // JSON round-trip, printing, typing, and evaluation.
    auto authored = matrixNode(2,2,{4,7,2,6});
    const std::string authoredPrint = authored->print();
    assert(authoredPrint.rfind("matrix2x2(", 0) == 0);
    assert(authoredPrint.back() == ')');

    MathNode copied(*authored);
    assert(copied.matrixRows == 2 && copied.matrixCols == 2);
    assert(copied.print() == authoredPrint);

    const nlohmann::json authoredJson = authored->toJson();
    assert(authoredJson.at("op") == 30);
    assert(authoredJson.at("rows") == 2);
    assert(authoredJson.at("cols") == 2);

    auto restored = MathNode::fromJson(authoredJson);
    assert(restored);
    assert(restored->op == MathNode::Op::MatrixConstruct);
    assert(restored->matrixRows == 2 && restored->matrixCols == 2);
    assert(restored->print() == authored->print());

    auto authoredType = restored->typeOf(OntoMath::TypeEnv{});
    assert(authoredType && authoredType.kind == ValueKind::Matrix);
    assert(authoredType.type.rows == 2 && authoredType.type.cols == 2);

    auto authoredValue = restored->evaluate({});
    assert(authoredValue && std::holds_alternative<MatrixValue>(*authoredValue));
    assert(nearMatrix(std::get<MatrixValue>(*authoredValue), a22));

    // Identity is authored and serialized rather than smuggled in as a backend
    // convenience.
    MathNode identityAst;
    identityAst.op = MathNode::Op::MatrixIdentity;
    identityAst.matrixRows = identityAst.matrixCols = 3;
    assert(identityAst.print() == "identity(3)");
    auto identityAstType = identityAst.typeOf({});
    assert(identityAstType && identityAstType.type.rows == 3 && identityAstType.type.cols == 3);
    auto identityAstValue = identityAst.evaluate({});
    assert(identityAstValue && std::holds_alternative<MatrixValue>(*identityAstValue));

    auto identityRestored = MathNode::fromJson(identityAst.toJson());
    assert(identityRestored && identityRestored->matrixRows == 3 && identityRestored->matrixCols == 3);

    // Full AST evaluation for multiply, determinant, transpose and inverse.
    std::map<std::string, PropertyValue> vars{
        {"A", PropertyValue(a22)},
        {"B", PropertyValue(mustMatrix(2,2,{1,0, 0,2}))},
        {"S", PropertyValue(singular)}
    };

    MathNode astMul;
    astMul.op = MathNode::Op::MatrixMultiply;
    astMul.children.push_back(value("A"));
    astMul.children.push_back(value("B"));
    auto astMulValue = astMul.evaluate(vars);
    assert(astMulValue && std::holds_alternative<MatrixValue>(*astMulValue));
    assert(nearMatrix(std::get<MatrixValue>(*astMulValue),
                      mustMatrix(2,2,{4,14, 2,12})));

    MathNode astDet;
    astDet.op = MathNode::Op::MatrixDeterminant;
    astDet.children.push_back(value("A"));
    auto astDetValue = astDet.evaluate(vars);
    double astDetNumber = 0.0;
    assert(astDetValue && propertyValueToNumber(*astDetValue, astDetNumber));
    assert(near(astDetNumber, 10.0));

    MathNode astTranspose;
    astTranspose.op = MathNode::Op::MatrixTranspose;
    astTranspose.children.push_back(value("A"));
    auto astTransposeValue = astTranspose.evaluate(vars);
    assert(astTransposeValue && std::holds_alternative<MatrixValue>(*astTransposeValue));
    assert(nearMatrix(std::get<MatrixValue>(*astTransposeValue),
                      mustMatrix(2,2,{4,2, 7,6})));

    MathNode astInverse;
    astInverse.op = MathNode::Op::MatrixInverse;
    astInverse.children.push_back(value("A"));
    auto astInverseValue = astInverse.evaluate(vars);
    assert(astInverseValue && std::holds_alternative<MatrixValue>(*astInverseValue));

    MathNode singularInverse;
    singularInverse.op = MathNode::Op::MatrixInverse;
    singularInverse.children.push_back(value("S"));
    assert(!singularInverse.evaluate(vars));

    // Non-finite values are not silently propagated as mathematical answers.
    auto nonFinite = MatrixValue::create(2,2,{1.0, INFINITY, 0.0, 1.0});
    assert(nonFinite);
    assert(!OntoMath::matrixTranspose(*nonFinite));
    assert(!OntoMath::matrixInverse(*nonFinite));
    assert(!OntoMath::matrixDeterminant(*nonFinite));

    std::puts("ontomath_linear_algebra_test: PASS");
    return 0;
}
