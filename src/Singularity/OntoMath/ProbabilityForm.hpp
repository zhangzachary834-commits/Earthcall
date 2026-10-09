#pragma once

#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
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
        Exponential = 3,
        Gamma = 4,
        Weibull = 5,
        ExtremeValue = 6,
        Poisson = 7,
        Binomial = 8,
        NegativeBinomial = 9,
        Geometric = 10,
        LogNormal = 11,
        ChiSquared = 12,
        Cauchy = 13,
        FisherF = 14,
        StudentT = 15,
        Symbolic = 16
    };

    Kind kind = Kind::Uniform;
    
    // Distribution parameters (e.g., [min, max] for Uniform, [mean, stddev] for Gaussian)
    std::vector<double> params;

    // The Probability Density Function (or PMF for discrete) for Symbolic distributions.
    // Must integrate to 1 over its domain.
    std::optional<Piecewise> pdf;

    // The name of the random variable (e.g., "x") used in the pdf.
    std::string variableName = "x";

    Distribution() = default;
    Distribution(Kind k, std::vector<double> p) : kind(k), params(std::move(p)) {}
    Distribution(Piecewise pdfModel, std::string varName) 
        : kind(Kind::Symbolic), pdf(std::move(pdfModel)), variableName(std::move(varName)) {}

    // Draw a random sample from this distribution
    double sample() const;

    // The mathematical expectation (mean) of the distribution, used when a deterministic
    // projection is required (e.g., when previewing an average outcome).
    double expectedValue() const;

    // The mathematical variance of the distribution.
    double variance() const;
};

// Evaluate a Stochastic node during MathNode traversal
std::optional<PropertyValue> evaluateStochastic(const std::string& distType, const std::vector<std::optional<PropertyValue>>& evaluatedArgs);

} // namespace Probability
} // namespace OntoMath
