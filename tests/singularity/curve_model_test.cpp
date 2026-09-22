#include "Singularity/OntoMath/CurveModel.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

bool neard(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

int main() {
    CurveModel constant = CurveModel::constant(5.0);
    assert(constant.form == CurveModel::Form::Constant);
    assert(neard(constant.evaluate(0.0), 5.0));
    assert(neard(constant.evaluate(10.0), 5.0));

    CurveModel poly = CurveModel::polynomial({2.0, 3.0, 4.0});
    assert(poly.form == CurveModel::Form::Polynomial);
    assert(neard(poly.evaluate(0.0), 2.0));
    assert(neard(poly.evaluate(1.0), 9.0));
    assert(neard(poly.evaluate(2.0), 24.0));

    CurveModel sine = CurveModel::sinusoid(2.0, 0.25, 0.0, 1.0);
    assert(sine.form == CurveModel::Form::Sinusoid);
    assert(neard(sine.evaluate(0.0), 1.0));
    assert(neard(sine.evaluate(1.0), 3.0));
    assert(neard(sine.evaluate(2.0), 1.0));

    nlohmann::json jConst = constant.toJson();
    CurveModel cConst = CurveModel::fromJson(jConst);
    assert(cConst.form == CurveModel::Form::Constant);
    assert(neard(cConst.evaluate(0.0), 5.0));

    nlohmann::json jPoly = poly.toJson();
    CurveModel cPoly = CurveModel::fromJson(jPoly);
    assert(cPoly.form == CurveModel::Form::Polynomial);
    assert(neard(cPoly.evaluate(1.0), 9.0));

    nlohmann::json jSine = sine.toJson();
    CurveModel cSine = CurveModel::fromJson(jSine);
    assert(cSine.form == CurveModel::Form::Sinusoid);
    assert(neard(cSine.evaluate(1.0), 3.0));

    CurveModel empty;
    assert(empty.form == CurveModel::Form::Constant);
    assert(neard(empty.evaluate(0.0), 0.0));

    CurveModel emptyPoly = CurveModel::polynomial({});
    assert(neard(emptyPoly.evaluate(10.0), 0.0));

    std::cout << "curve_model_test: OK\n";
    return 0;
}
