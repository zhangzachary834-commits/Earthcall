#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include <iostream>

int main() {
    PropertyPath p = PropertyPath::parse("a.b.c");
    if (p.segments.size() != 3) return 1;
    if (p.segments[0] != "a") return 1;
    if (p.segments[1] != "b") return 1;
    if (p.segments[2] != "c") return 1;
    if (p.toString() != "a.b.c") return 1;
    return 0;
}
