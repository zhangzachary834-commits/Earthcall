#include "Singularity/OntoMath/Function.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

bool neard(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

int main() {
    Function f;

    assert(neard(f.evaluate(5.0), 0.0));

    Function::Variable var{"x", 0.0};
    f.terms.push_back(Function::Term(2.0, {var}, 3.0));

    assert(neard(f.evaluate(2.0), 16.0));

    f.terms.push_back(Function::Term(5.0, {var}, 1.0));

    assert(neard(f.evaluate(2.0), 26.0));

    assert(neard(f.evaluate(0.0), 0.0));

    std::string printed = f.terms[0].print();
    assert(printed.find("2.000000") != std::string::npos);
    assert(printed.find("x") != std::string::npos);
    assert(printed.find("^3.000000") != std::string::npos);

    std::cout << "function_test: OK\n";
    return 0;
}
