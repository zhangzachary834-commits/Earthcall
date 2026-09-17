#include "Singularity/OntoMath/ProbabilityForm.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>

bool neard(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

int main() {
    using namespace OntoMath::Probability;

    Distribution uniform(Distribution::Kind::Uniform, {0.0, 10.0});
    assert(uniform.kind == Distribution::Kind::Uniform);
    assert(neard(uniform.expectedValue(), 5.0));
    double s1 = uniform.sample();
    assert(s1 >= 0.0 && s1 <= 10.0);

    Distribution gaussian(Distribution::Kind::Gaussian, {5.0, 2.0});
    assert(gaussian.kind == Distribution::Kind::Gaussian);
    assert(neard(gaussian.expectedValue(), 5.0));

    Distribution bernoulli(Distribution::Kind::Bernoulli, {0.75});
    assert(bernoulli.kind == Distribution::Kind::Bernoulli);
    assert(neard(bernoulli.expectedValue(), 0.75));
    double s3 = bernoulli.sample();
    assert(s3 == 0.0 || s3 == 1.0);

    Distribution uniform_def(Distribution::Kind::Uniform, {5.0});
    assert(neard(uniform_def.expectedValue(), 3.0));

    std::vector<std::optional<PropertyValue>> uniformArgs = { PropertyValue(2.0), PropertyValue(8.0) };
    auto result1 = evaluateStochastic("uniform", uniformArgs);
    assert(result1.has_value());
    double n1;
    assert(propertyValueToNumber(*result1, n1));
    assert(n1 >= 2.0 && n1 <= 8.0);

    std::vector<std::optional<PropertyValue>> badArgs = { std::nullopt, PropertyValue(8.0) };
    auto result2 = evaluateStochastic("uniform", badArgs);
    assert(!result2.has_value());

    std::cout << "probability_form_test: OK\n";
    return 0;
}
