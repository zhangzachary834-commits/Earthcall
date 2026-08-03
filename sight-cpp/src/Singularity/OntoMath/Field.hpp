#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"
#include <memory>
#include <vector>

namespace OntoMath {

// Base interface for a mathematical Field
class Field {
public:
    virtual ~Field() = default;
};

// Represents a mathematical continuous scalar density field.
// Driven by either a hardcoded equation or an OntoMath Piecewise AST.
class ScalarField : public Field {
public:
    ScalarField();
    ~ScalarField() override = default;

    // The underlying mathematical definition represented as a piecewise function
    // (AST) for Law integration and Path B WGSL compilation.
    Piecewise astDefinition;

    // Hardcoded path configuration (Path A).
    // If not using AST, we can configure procedural noise parameters.
    float baseDensity = 1.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
};

} // namespace OntoMath
