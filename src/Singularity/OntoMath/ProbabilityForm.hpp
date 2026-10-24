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

    // The names of the random variables (e.g., {"x", "y", "z"}) used in the pdf.
    std::vector<std::string> variableNames = {"x"};

    Distribution() = default;
    Distribution(Kind k, std::vector<double> p) : kind(k), params(std::move(p)) {}
    Distribution(Piecewise pdfModel, std::vector<std::string> varNames) 
        : kind(Kind::Symbolic), pdf(std::move(pdfModel)), variableNames(std::move(varNames)) {}
    Distribution(Piecewise pdfModel, std::string varName) 
        : kind(Kind::Symbolic), pdf(std::move(pdfModel)), variableNames({std::move(varName)}) {}

    // Draw a random sample from this distribution. For N-D it returns N elements.
    std::vector<double> sample() const;

    // The mathematical expectation vector (mean) of the distribution.
    std::vector<double> expectedVector() const;

    // The mathematical covariance matrix of the distribution. 
    // Returns a flattened N x N matrix (row-major).
    std::vector<double> covarianceMatrix() const;

    // 1D Convenience wrappers for backwards compatibility and exact math
    double expectedValue() const { return expectedVector().empty() ? 0.0 : expectedVector()[0]; }
    double variance() const { return covarianceMatrix().empty() ? 0.0 : covarianceMatrix()[0]; }
};

// Evaluate a Stochastic node during MathNode traversal
std::optional<PropertyValue> evaluateStochastic(const std::string& distType, const std::vector<std::optional<PropertyValue>>& evaluatedArgs);

} // namespace Probability
} // namespace OntoMath
