#include "Singularity/OntoMath/CurveModel.hpp"

#include <cmath>

namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;
}

double CurveModel::evaluate(double x) const {
    switch (form) {
        case Form::Constant:
            return coeffs.empty() ? 0.0 : coeffs[0];
        case Form::Polynomial: {
            // Horner's rule.
            double y = 0.0;
            for (std::size_t i = coeffs.size(); i-- > 0;) {
                y = y * x + coeffs[i];
            }
            return y;
        }
        case Form::Sinusoid:
            return bias + amplitude * std::sin(kTwoPi * frequency * x + phase);
    }
    return 0.0;
}

nlohmann::json CurveModel::toJson() const {
    nlohmann::json j{{"form", static_cast<int>(form)}};
    if (!coeffs.empty()) j["coeffs"] = coeffs;
    if (form == Form::Sinusoid) {
        j["amplitude"] = amplitude;
        j["frequency"] = frequency;
        j["phase"] = phase;
        j["bias"] = bias;
    }
    return j;
}

CurveModel CurveModel::fromJson(const nlohmann::json& j) {
    CurveModel m;
    m.form = static_cast<Form>(j.value("form", 0));
    if (j.contains("coeffs")) m.coeffs = j["coeffs"].get<std::vector<double>>();
    m.amplitude = j.value("amplitude", 0.0);
    m.frequency = j.value("frequency", 1.0);
    m.phase = j.value("phase", 0.0);
    m.bias = j.value("bias", 0.0);
    return m;
}

CurveModel CurveModel::constant(double c) {
    CurveModel m;
    m.form = Form::Constant;
    m.coeffs = {c};
    return m;
}

CurveModel CurveModel::polynomial(std::vector<double> coefficients) {
    CurveModel m;
    m.form = Form::Polynomial;
    m.coeffs = std::move(coefficients);
    return m;
}

CurveModel CurveModel::sinusoid(double amplitude, double frequency, double phase, double bias) {
    CurveModel m;
    m.form = Form::Sinusoid;
    m.amplitude = amplitude;
    m.frequency = frequency;
    m.phase = phase;
    m.bias = bias;
    return m;
}
