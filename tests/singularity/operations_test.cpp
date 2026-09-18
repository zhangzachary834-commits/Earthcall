#include "Singularity/OntoMath/Operations.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

bool neard(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

int main() {
    assert(neard(OntoMath::Operations::hyperop(0, 5, 2), 3.0));
    assert(neard(OntoMath::Operations::hyperop(1, 5, 2), 7.0));
    assert(neard(OntoMath::Operations::hyperop(2, 5, 2), 10.0));
    assert(neard(OntoMath::Operations::hyperop(3, 5, 2), 25.0));
    assert(neard(OntoMath::Operations::hyperop(4, 2, 3), 16.0));

    assert(neard(OntoMath::Operations::binom(4, -1), 0.0));
    assert(neard(OntoMath::Operations::binom(4, 5), 0.0));
    assert(neard(OntoMath::Operations::binom(4, 0), 1.0));
    assert(neard(OntoMath::Operations::binom(4, 1), 4.0));
    assert(neard(OntoMath::Operations::binom(4, 2), 6.0));
    assert(neard(OntoMath::Operations::binom(4, 3), 4.0));
    assert(neard(OntoMath::Operations::binom(4, 4), 1.0));

    assert(neard(OntoMath::Operations::bernstein(3, -1, 0.5), 0.0));
    assert(neard(OntoMath::Operations::bernstein(3, 4, 0.5), 0.0));

    assert(neard(OntoMath::Operations::bernstein(3, 0, 0.5), 0.125));
    assert(neard(OntoMath::Operations::bernstein(3, 1, 0.5), 0.375));
    assert(neard(OntoMath::Operations::bernstein(3, 2, 0.5), 0.375));
    assert(neard(OntoMath::Operations::bernstein(3, 3, 0.5), 0.125));

    std::cout << "operations_test: OK\n";
    return 0;
}
