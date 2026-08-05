#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "json.hpp"
#include <memory>
#include <vector>

namespace OntoMath {

// Represents a mathematical continuous scalar density field.
// Driven by either a hardcoded equation or an OntoMath Piecewise AST.
class ScalarField {
public:
    enum class EvaluationMode { Procedural, AST };

    ScalarField() = default;
    ~ScalarField() = default;

    EvaluationMode mode = EvaluationMode::Procedural;

    // The underlying mathematical definition represented as a piecewise function
    // (AST) for Law integration and Path B WGSL compilation.
    Piecewise astDefinition;

    // Hardcoded path configuration (Path A).
    // If not using AST, we can configure procedural noise parameters.
    float baseDensity = 1.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;

    nlohmann::json toJson() const;
    static std::shared_ptr<ScalarField> fromJson(const nlohmann::json& j);
};

// Represents a mathematical continuous vector flow/force field.
class VectorField {
public:
    enum class EvaluationMode { Procedural, AST };

    VectorField() = default;
    ~VectorField() = default;

    EvaluationMode mode = EvaluationMode::Procedural;

    Piecewise astDefinition;

    float baseFlowX = 0.0f;
    float baseFlowY = 0.0f;
    float baseFlowZ = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;

    nlohmann::json toJson() const;
    static std::shared_ptr<VectorField> fromJson(const nlohmann::json& j);
};

} // namespace OntoMath
