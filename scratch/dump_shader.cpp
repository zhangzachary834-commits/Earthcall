#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include <iostream>

int main() {
    auto node = std::make_shared<geom::SdfNode>(geom::makeImplicit("x*x + y*y + z*z - 0.3"));
    auto prog = sdfwgsl::compile(*node, nullptr);
    std::cout << prog.wgsl << "\n";
    return 0;
}
