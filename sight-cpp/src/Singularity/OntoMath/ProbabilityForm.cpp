#include "Singularity/OntoMath/ProbabilityForm.hpp"
#include <random>

namespace OntoMath {
namespace Probability {

// Thread-local random number generator to avoid lock contention during parallel evaluations
static thread_local std::mt19937 generator(std::random_device{}());

double Distribution::sample() const {
    if (params.empty()) return 0.0;

    switch (kind) {
        case Kind::Uniform: {
            double min = params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            std::uniform_real_distribution<double> dist(min, max);
            return dist(generator);
        }
        case Kind::Gaussian: {
            double mean = params[0];
            double stddev = (params.size() > 1) ? params[1] : 1.0;
            std::normal_distribution<double> dist(mean, stddev);
            return dist(generator);
        }
        case Kind::Bernoulli: {
            double p = params[0];
            std::bernoulli_distribution dist(p);
            return dist(generator) ? 1.0 : 0.0;
        }
    }
    return 0.0;
}

double Distribution::expectedValue() const {
    if (params.empty()) return 0.0;

    switch (kind) {
        case Kind::Uniform: {
            double min = params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            return (min + max) / 2.0;
        }
        case Kind::Gaussian: {
            return params[0];
        }
        case Kind::Bernoulli: {
            return params[0]; // E[X] = p for Bernoulli
        }
    }
    return 0.0;
}

std::optional<PropertyValue> evaluateStochastic(const std::string& distType, const std::vector<std::optional<PropertyValue>>& evaluatedArgs) {
    std::vector<double> numericParams;
    for (const auto& arg : evaluatedArgs) {
        if (!arg) return std::nullopt; // Undefined inputs propagate to undefined output
        double n = 0.0;
        if (!propertyValueToNumber(*arg, n)) return std::nullopt;
        numericParams.push_back(n);
    }

    Distribution::Kind kind = Distribution::Kind::Uniform;
    if (distType == "gaussian" || distType == "normal") {
        kind = Distribution::Kind::Gaussian;
    } else if (distType == "bernoulli") {
        kind = Distribution::Kind::Bernoulli;
    }

    Distribution dist(kind, numericParams);
    return PropertyValue(dist.sample());
}

} // namespace Probability
} // namespace OntoMath
