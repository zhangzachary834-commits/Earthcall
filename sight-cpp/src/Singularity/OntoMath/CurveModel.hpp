#pragma once

#include "json.hpp"

#include <vector>

// A one-variable model of change: y = f(x). The mathematical text of a law's
// Drive action — and later the output of ChangeRecorder fits and the transform
// of PropertyMapping (LAW_AND_CREATION_SYSTEM.md §2c). Plain data:
// serializable, introspectable, evaluated on demand.
//
// The Sinusoid fields deliberately mirror Automation::Track — Automation was
// the first pattern-of-change-as-data; CurveModel is its generalization off
// the transform channels onto arbitrary PropertyPaths.
struct CurveModel {
    enum class Form { Constant = 0, Polynomial = 1, Sinusoid = 2 };
    Form form = Form::Constant;

    // Constant: coeffs[0] (0.0 when empty).
    // Polynomial: coeffs[0] + coeffs[1]*x + coeffs[2]*x^2 + ...
    std::vector<double> coeffs;

    // Sinusoid: bias + amplitude * sin(2*pi*frequency*x + phase)
    double amplitude = 0.0;
    double frequency = 1.0;
    double phase = 0.0;
    double bias = 0.0;

    double evaluate(double x) const;

    nlohmann::json toJson() const;
    static CurveModel fromJson(const nlohmann::json& j);

    static CurveModel constant(double c);
    static CurveModel polynomial(std::vector<double> coefficients);
    static CurveModel sinusoid(double amplitude, double frequency,
                               double phase = 0.0, double bias = 0.0);
};
