#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace OntoMath {

// A mathematical matrix value, independent of any renderer/physics storage
// convention. Elements are stored in canonical logical ROW-MAJOR order:
//   elements[row * cols + col]
// Access is always expressed as (row, col). The glm bridge performs the explicit
// [column][row] conversion at the substrate boundary.
class MatrixValue {
public:
    MatrixValue() = delete;

    static std::optional<MatrixValue> create(std::size_t rows,
                                             std::size_t cols,
                                             std::vector<double> elements);

    static MatrixValue fromGlmMat4(const glm::mat4& matrix);
    std::optional<glm::mat4> toGlmMat4() const;

    std::size_t rows() const { return _rows; }
    std::size_t cols() const { return _cols; }
    const std::vector<double>& elements() const { return _elements; }

    bool valid() const;

    double at(std::size_t row, std::size_t col) const;
    double& at(std::size_t row, std::size_t col);

    friend bool operator==(const MatrixValue& a, const MatrixValue& b) {
        return a._rows == b._rows && a._cols == b._cols &&
               a._elements == b._elements;
    }
    friend bool operator!=(const MatrixValue& a, const MatrixValue& b) {
        return !(a == b);
    }

private:
    MatrixValue(std::size_t rows, std::size_t cols, std::vector<double> elements)
        : _rows(rows), _cols(cols), _elements(std::move(elements)) {}

    std::size_t _rows = 0;
    std::size_t _cols = 0;
    std::vector<double> _elements;
};

// Shared numerical refusal threshold for pivot-based operations. The threshold
// is relative to the largest finite coefficient in the matrix, so uniformly
// scaling an invertible matrix does not by itself make it "singular".
inline constexpr double kMatrixRelativePivotEpsilon = 1e-12;

std::optional<MatrixValue> matrixIdentity(std::size_t dimension);
std::optional<MatrixValue> matrixAdd(const MatrixValue& a, const MatrixValue& b);
std::optional<MatrixValue> matrixSubtract(const MatrixValue& a, const MatrixValue& b);
std::optional<MatrixValue> matrixScale(const MatrixValue& matrix, double scalar);
std::optional<MatrixValue> matrixMultiply(const MatrixValue& a, const MatrixValue& b);
std::optional<glm::vec3> matrixMultiplyVec3(const MatrixValue& matrix,
                                            const glm::vec3& vector);
std::optional<MatrixValue> matrixTranspose(const MatrixValue& matrix);
std::optional<double> matrixDeterminant(const MatrixValue& matrix);
std::optional<MatrixValue> matrixInverse(const MatrixValue& matrix);
// Project an NDC point through inverse(projection * view), including the
// homogeneous divide. This owns general picking/unprojection mathematics;
 // callers still own viewport/pointer policy.
std::optional<glm::vec3> unprojectNdcPoint(const MatrixValue& view,
                                           const MatrixValue& projection,
                                           const glm::vec3& ndc);


std::optional<MatrixValue> cameraLookAt(const glm::vec3& eye,
                                        const glm::vec3& target,
                                        const glm::vec3& up);
std::optional<MatrixValue> cameraPerspective(double verticalFovRadians,
                                             double aspect,
                                             double nearPlane,
                                             double farPlane,
                                             bool zeroToOneDepth);
std::optional<MatrixValue> cameraOrthographic(double left, double right,
                                              double bottom, double top,
                                              double nearPlane, double farPlane,
                                              bool zeroToOneDepth);

// Canonical affine mathematics. These functions own transform meaning; GLM is
// only a representation/execution boundary. All affine transforms are 4x4
// homogeneous matrices acting on column vectors.
std::optional<MatrixValue> affineIdentity();
std::optional<MatrixValue> affineTranslation(const glm::vec3& translation);
std::optional<MatrixValue> affineScale(const glm::vec3& scale);
std::optional<MatrixValue> affineAxisAngle(const glm::vec3& axis, double radians);
std::optional<MatrixValue> affineEulerXYZDegrees(const glm::vec3& degrees);
std::optional<MatrixValue> affineCompose(const MatrixValue& first,
                                         const MatrixValue& second);
std::optional<MatrixValue> affineTRS(const glm::vec3& translation,
                                     const glm::vec3& eulerDegrees,
                                     const glm::vec3& scale);
std::optional<glm::vec3> transformPoint(const MatrixValue& affine,
                                        const glm::vec3& point);
std::optional<glm::vec3> transformDirection(const MatrixValue& affine,
                                            const glm::vec3& direction);
std::optional<glm::vec3> transformNormal(const MatrixValue& affine,
                                         const glm::vec3& normal);
std::optional<MatrixValue> inverseAffine(const MatrixValue& affine);
std::optional<glm::vec3> affineExtractTranslation(const MatrixValue& affine);
std::optional<MatrixValue> affineExtractRotationBasis(const MatrixValue& affine);
std::optional<glm::vec3> affineExtractScale(const MatrixValue& affine);
std::optional<glm::vec3> affineExtractEulerXYZDegrees(const MatrixValue& affine);
std::optional<MatrixValue> affineSelectTRS(const MatrixValue& parent,
                                           const MatrixValue& child,
                                           const MatrixValue& localOffset,
                                           bool inheritTranslation,
                                           bool inheritRotation,
                                           bool inheritScale);

} // namespace OntoMath