#pragma once

#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include <string>
#include <vector>
#include <optional>

namespace OntoMath {
namespace Probability {

// Represents a stochastic distribution in OntoMath.
// Laws can use this to apply uncertain conceptual changes, such as modifying 
// a property based on a Gaussian distribution representing semantic ambiguity.
class Distribution {
public:
    enum class Kind {
        Uniform = 0,
        Gaussian = 1,
        Bernoulli = 2,
    };

    Kind kind = Kind::Uniform;
    
    // Distribution parameters (e.g., [min, max] for Uniform, [mean, stddev] for Gaussian)
    std::vector<double> params;

    Distribution() = default;
    Distribution(Kind k, std::vector<double> p) : kind(k), params(std::move(p)) {}

    // Draw a random sample from this distribution
    double sample() const;

    // The mathematical expectation (mean) of the distribution, used when a deterministic
    // projection is required (e.g., when previewing an average outcome).
    double expectedValue() const;
};

// Evaluate a Stochastic node during MathNode traversal
std::optional<PropertyValue> evaluateStochastic(const std::string& distType, const std::vector<std::optional<PropertyValue>>& evaluatedArgs);

} // namespace Probability
} // namespace OntoMath
