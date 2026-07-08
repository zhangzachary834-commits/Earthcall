#include "Singularity/OntoMath/Operations.hpp"

double Operations::hyperop(int level, double a, double b) {
    if (level == 0) { return b + 1; }             // succession
    if (level == 1) { return a + b; }             // addition

    if (b == 0) {
        if (level == 2) { return 0; }             // multiplication base case
        return 1;                                 // exp/tet/... base case
    }

    // Pure-recursion modeling stub
    for (int i = 0; i < level; i++) {

        // recursively call hyperop on the lower level to simulate how hyper operation works
    }

    return hyperop(level - 1, a, hyperop(level, a, b - 1));
}
