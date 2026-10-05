#include "Singularity/OntoMath/LinearAlgebra.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace OntoMath {

namespace {

bool shapeProduct(std::size_t rows, std::size_t cols, std::size_t& out) {
    if (rows == 0 || cols == 0) return false;
    if (rows > std::numeric_limits<std::size_t>::max() / cols) return false;
    out = rows * cols;
    return true;
}

} // namespace

std::optional<MatrixValue> MatrixValue::create(std::size_t rows,
                                               std::size_t cols,
                                               std::vector<double> elements) {
    std::size_t expected = 0;
    if (!shapeProduct(rows, cols, expected) || elements.size() != expected) {
        return std::nullopt;
    }
    return MatrixValue(rows, cols, std::move(elements));
}

bool MatrixValue::valid() const {
    std::size_t expected = 0;
    return shapeProduct(_rows, _cols, expected) && _elements.size() == expected;
}

double MatrixValue::at(std::size_t row, std::size_t col) const {
    if (!valid() || row >= _rows || col >= _cols) {
        throw std::out_of_range("OntoMath::MatrixValue index out of range");
    }
    return _elements[row * _cols + col];
}

double& MatrixValue::at(std::size_t row, std::size_t col) {
    if (!valid() || row >= _rows || col >= _cols) {
        throw std::out_of_range("OntoMath::MatrixValue index out of range");
    }
    return _elements[row * _cols + col];
}

MatrixValue MatrixValue::fromGlmMat4(const glm::mat4& matrix) {
    std::vector<double> elements;
    elements.reserve(16);
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            elements.push_back(static_cast<double>(matrix[col][row]));
        }
    }
    return MatrixValue(4, 4, std::move(elements));
}

std::optional<glm::mat4> MatrixValue::toGlmMat4() const {
    if (!valid() || _rows != 4 || _cols != 4) return std::nullopt;

    glm::mat4 matrix(0.0f);
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            matrix[col][row] = static_cast<float>(at(row, col));
        }
    }
    return matrix;
}

namespace {

bool finiteMatrix(const MatrixValue& matrix, double& maxAbs) {
    if (!matrix.valid()) return false;
    maxAbs = 0.0;
    for (double v : matrix.elements()) {
        if (!std::isfinite(v)) return false;
        maxAbs = std::max(maxAbs, std::abs(v));
    }
    return true;
}

double pivotThreshold(double maxAbs) {
    return kMatrixRelativePivotEpsilon * maxAbs;
}

} // namespace

std::optional<MatrixValue> matrixIdentity(std::size_t dimension) {
    std::size_t count = 0;
    if (!shapeProduct(dimension, dimension, count)) return std::nullopt;
    std::vector<double> elements(count, 0.0);
    for (std::size_t i = 0; i < dimension; ++i) {
        elements[i * dimension + i] = 1.0;
    }
    return MatrixValue::create(dimension, dimension, std::move(elements));
}

std::optional<MatrixValue> matrixAdd(const MatrixValue& a, const MatrixValue& b) {
    if (!a.valid() || !b.valid() || a.rows() != b.rows() || a.cols() != b.cols()) {
        return std::nullopt;
    }
    std::vector<double> out(a.elements().size(), 0.0);
    for (std::size_t i = 0; i < out.size(); ++i) {
        const double v = a.elements()[i] + b.elements()[i];
        if (!std::isfinite(v)) return std::nullopt;
        out[i] = v;
    }
    return MatrixValue::create(a.rows(), a.cols(), std::move(out));
}

std::optional<MatrixValue> matrixSubtract(const MatrixValue& a, const MatrixValue& b) {
    if (!a.valid() || !b.valid() || a.rows() != b.rows() || a.cols() != b.cols()) {
        return std::nullopt;
    }
    std::vector<double> out(a.elements().size(), 0.0);
    for (std::size_t i = 0; i < out.size(); ++i) {
        const double v = a.elements()[i] - b.elements()[i];
        if (!std::isfinite(v)) return std::nullopt;
        out[i] = v;
    }
    return MatrixValue::create(a.rows(), a.cols(), std::move(out));
}

std::optional<MatrixValue> matrixScale(const MatrixValue& matrix, double scalar) {
    if (!matrix.valid() || !std::isfinite(scalar)) return std::nullopt;
    std::vector<double> out(matrix.elements().size(), 0.0);
    for (std::size_t i = 0; i < out.size(); ++i) {
        const double v = matrix.elements()[i] * scalar;
        if (!std::isfinite(v)) return std::nullopt;
        out[i] = v;
    }
    return MatrixValue::create(matrix.rows(), matrix.cols(), std::move(out));
}

std::optional<MatrixValue> matrixMultiply(const MatrixValue& a, const MatrixValue& b) {
    if (!a.valid() || !b.valid() || a.cols() != b.rows()) return std::nullopt;

    std::size_t count = 0;
    if (!shapeProduct(a.rows(), b.cols(), count)) return std::nullopt;
    std::vector<double> out(count, 0.0);
    for (std::size_t r = 0; r < a.rows(); ++r) {
        for (std::size_t c = 0; c < b.cols(); ++c) {
            double sum = 0.0;
            for (std::size_t k = 0; k < a.cols(); ++k) {
                sum += a.at(r, k) * b.at(k, c);
            }
            if (!std::isfinite(sum)) return std::nullopt;
            out[r * b.cols() + c] = sum;
        }
    }
    return MatrixValue::create(a.rows(), b.cols(), std::move(out));
}

std::optional<glm::vec3> matrixMultiplyVec3(const MatrixValue& matrix,
                                            const glm::vec3& vector) {
    // Existing OntoMath vectors are exactly vec3. A 4x4 transform acting on a
    // vec3 would have to guess point(w=1) vs direction(w=0), which Rung 0
    // explicitly forbids. Homogeneous affine semantics land later.
    if (!matrix.valid() || matrix.rows() != 3 || matrix.cols() != 3) {
        return std::nullopt;
    }

    glm::vec3 out(0.0f);
    for (std::size_t r = 0; r < 3; ++r) {
        double sum = 0.0;
        for (std::size_t c = 0; c < 3; ++c) {
            sum += matrix.at(r, c) * static_cast<double>(vector[c]);
        }
        if (!std::isfinite(sum)) return std::nullopt;
        out[static_cast<int>(r)] = static_cast<float>(sum);
    }
    return out;
}

std::optional<glm::vec3> unprojectNdcPoint(const MatrixValue& view,
                                                   const MatrixValue& projection,
                                                   const glm::vec3& ndc) {
    if (!view.valid() || !projection.valid() ||
        view.rows() != 4 || view.cols() != 4 ||
        projection.rows() != 4 || projection.cols() != 4) return std::nullopt;
    const auto vp = matrixMultiply(projection, view);
    if (!vp) return std::nullopt;
    const auto inverse = matrixInverse(*vp);
    if (!inverse) return std::nullopt;
    const double h[4] = {ndc.x, ndc.y, ndc.z, 1.0};
    double world[4] = {};
    for (std::size_t r = 0; r < 4; ++r)
        for (std::size_t col = 0; col < 4; ++col)
            world[r] += inverse->at(r, col) * h[col];
    if (!std::isfinite(world[0]) || !std::isfinite(world[1]) ||
        !std::isfinite(world[2]) || !std::isfinite(world[3]) ||
        std::abs(world[3]) <= kMatrixRelativePivotEpsilon) return std::nullopt;
    const double w = 1.0 / world[3];
    return glm::vec3(static_cast<float>(world[0] * w),
                     static_cast<float>(world[1] * w),
                     static_cast<float>(world[2] * w));
}

std::optional<MatrixValue> matrixTranspose(const MatrixValue& matrix) {
    double maxAbs = 0.0;
    if (!finiteMatrix(matrix, maxAbs)) return std::nullopt;
    std::vector<double> out(matrix.elements().size(), 0.0);
    for (std::size_t r = 0; r < matrix.rows(); ++r) {
        for (std::size_t c = 0; c < matrix.cols(); ++c) {
            out[c * matrix.rows() + r] = matrix.at(r, c);
        }
    }
    return MatrixValue::create(matrix.cols(), matrix.rows(), std::move(out));
}

std::optional<double> matrixDeterminant(const MatrixValue& matrix) {
    if (!matrix.valid() || matrix.rows() != matrix.cols()) return std::nullopt;

    double maxAbs = 0.0;
    if (!finiteMatrix(matrix, maxAbs)) return std::nullopt;
    if (maxAbs == 0.0) return 0.0;

    const std::size_t n = matrix.rows();
    std::vector<double> a = matrix.elements();
    double det = 1.0;
    int sign = 1;
    const double threshold = pivotThreshold(maxAbs);

    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        double pivotAbs = std::abs(a[col * n + col]);
        for (std::size_t r = col + 1; r < n; ++r) {
            const double candidate = std::abs(a[r * n + col]);
            if (candidate > pivotAbs) {
                pivotAbs = candidate;
                pivot = r;
            }
        }

        if (pivotAbs <= threshold) return 0.0;

        if (pivot != col) {
            for (std::size_t c = 0; c < n; ++c) {
                std::swap(a[col * n + c], a[pivot * n + c]);
            }
            sign = -sign;
        }

        const double pv = a[col * n + col];
        det *= pv;
        if (!std::isfinite(det)) return std::nullopt;

        for (std::size_t r = col + 1; r < n; ++r) {
            const double factor = a[r * n + col] / pv;
            for (std::size_t c = col + 1; c < n; ++c) {
                a[r * n + c] -= factor * a[col * n + c];
            }
        }
    }

    det *= static_cast<double>(sign);
    if (!std::isfinite(det)) return std::nullopt;
    return det;
}

std::optional<MatrixValue> matrixInverse(const MatrixValue& matrix) {
    if (!matrix.valid() || matrix.rows() != matrix.cols()) return std::nullopt;

    double maxAbs = 0.0;
    if (!finiteMatrix(matrix, maxAbs) || maxAbs == 0.0) return std::nullopt;

    const std::size_t n = matrix.rows();
    std::vector<double> left = matrix.elements();
    std::vector<double> right(n * n, 0.0);
    for (std::size_t i = 0; i < n; ++i) right[i * n + i] = 1.0;

    const double threshold = pivotThreshold(maxAbs);

    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        double pivotAbs = std::abs(left[col * n + col]);
        for (std::size_t r = col + 1; r < n; ++r) {
            const double candidate = std::abs(left[r * n + col]);
            if (candidate > pivotAbs) {
                pivotAbs = candidate;
                pivot = r;
            }
        }

        if (pivotAbs <= threshold) return std::nullopt;

        if (pivot != col) {
            for (std::size_t c = 0; c < n; ++c) {
                std::swap(left[col * n + c], left[pivot * n + c]);
                std::swap(right[col * n + c], right[pivot * n + c]);
            }
        }

        const double pv = left[col * n + col];
        for (std::size_t c = 0; c < n; ++c) {
            left[col * n + c] /= pv;
            right[col * n + c] /= pv;
            if (!std::isfinite(left[col * n + c]) ||
                !std::isfinite(right[col * n + c])) {
                return std::nullopt;
            }
        }

        for (std::size_t r = 0; r < n; ++r) {
            if (r == col) continue;
            const double factor = left[r * n + col];
            for (std::size_t c = 0; c < n; ++c) {
                left[r * n + c] -= factor * left[col * n + c];
                right[r * n + c] -= factor * right[col * n + c];
                if (!std::isfinite(left[r * n + c]) ||
                    !std::isfinite(right[r * n + c])) {
                    return std::nullopt;
                }
            }
        }
    }

    return MatrixValue::create(n, n, std::move(right));
}


std::optional<MatrixValue> cameraLookAt(const glm::vec3& eye,
                                         const glm::vec3& target,
                                         const glm::vec3& up) {
    if (!std::isfinite(eye.x) || !std::isfinite(eye.y) || !std::isfinite(eye.z) ||
        !std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(target.z) ||
        !std::isfinite(up.x) || !std::isfinite(up.y) || !std::isfinite(up.z)) {
        return std::nullopt;
    }
    const glm::vec3 forward = target - eye;
    if (glm::dot(forward, forward) <= std::numeric_limits<float>::epsilon() ||
        glm::dot(up, up) <= std::numeric_limits<float>::epsilon() ||
        glm::dot(glm::cross(forward, up), glm::cross(forward, up)) <=
            std::numeric_limits<float>::epsilon()) {
        return std::nullopt;
    }
    return MatrixValue::fromGlmMat4(glm::lookAt(eye, target, up));
}

std::optional<MatrixValue> cameraPerspective(double verticalFovRadians,
                                              double aspect,
                                              double nearPlane,
                                              double farPlane,
                                              bool zeroToOneDepth) {
    if (!std::isfinite(verticalFovRadians) || !std::isfinite(aspect) ||
        !std::isfinite(nearPlane) || !std::isfinite(farPlane) ||
        verticalFovRadians <= 0.0 || verticalFovRadians >= 3.14159265358979323846 ||
        aspect <= 0.0 || nearPlane <= 0.0 || farPlane <= nearPlane) {
        return std::nullopt;
    }
    const float fov = static_cast<float>(verticalFovRadians);
    const float a = static_cast<float>(aspect);
    const float n = static_cast<float>(nearPlane);
    const float f = static_cast<float>(farPlane);
    const glm::mat4 projection = zeroToOneDepth
        ? glm::perspectiveRH_ZO(fov, a, n, f)
        : glm::perspectiveRH_NO(fov, a, n, f);
    return MatrixValue::fromGlmMat4(projection);
}

} // namespace OntoMath
