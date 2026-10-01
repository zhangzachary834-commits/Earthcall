#include "Singularity/OntoMath/LinearAlgebra.hpp"

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

} // namespace OntoMath
