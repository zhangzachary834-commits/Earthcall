#pragma once

namespace OntoMath {

// OntoMath primitive operations. hyperop generalizes the arithmetic ladder:
// level 0 = succession, 1 = addition, 2 = multiplication, 3 = exponentiation,
// 4 = tetration, ... (the hyperoperation sequence).
class Operations {
public:
    static double hyperop(int level, double a, double b);
    static double binom(int n, int k);
    static double bernstein(int n, int i, double t);
};

} // namespace OntoMath

using Operations = OntoMath::Operations;
