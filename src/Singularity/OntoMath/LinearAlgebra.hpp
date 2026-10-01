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

} // namespace OntoMath
