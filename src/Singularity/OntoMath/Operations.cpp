#include "Singularity/OntoMath/Operations.hpp"
#include <cmath>
#include <algorithm>

namespace OntoMath {

double Operations::hyperop(int level, double a, double b) {
    if (level == 0) { return b + 1; }             // succession
    if (level == 1) { return a + b; }             // addition

    if (b == 0) {
        if (level == 2) { return 0; }             // multiplication base case
        return 1;                                 // exp/tet/... base case
    }

    return hyperop(level - 1, a, hyperop(level, a, b - 1));
}

double Operations::binom(int n, int k) {
    if (k < 0 || k > n) return 0.0;
    double r = 1.0;
    for (int i = 0; i < k; ++i) r = r * (n - i) / (i + 1);
    return r;
}

double Operations::bernstein(int n, int i, double t) {
    if (i < 0 || i > n) return 0.0;
    return binom(n, i) * std::pow(t, i) * std::pow(1.0 - t, n - i);
}

} // namespace OntoMath
