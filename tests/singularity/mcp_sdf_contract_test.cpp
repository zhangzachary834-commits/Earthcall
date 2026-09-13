#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAILED: " << message << '\n';
    } else {
        std::cout << "  ok: " << message << '\n';
    }
}
} // namespace

int main() {
    std::cout << "Running mcp_sdf_contract_test...\n";

    // The MCP tool currently advertises function-like examples such as
    // sphere(0.5), box(...), smoothUnion(...), and morph(...). The engine's
    // string path is the implicit f(x,y,z)=0 expression compiler, so this test
    // guards the actual expressions an agent can safely send through that path.
    const auto sphere = geom::makeImplicit("sqrt(x*x+y*y+z*z)-0.5");
    check(!sphere.rpn.empty(), "implicit sphere equation compiles");

    const auto wave = geom::makeImplicit("sqrt(x*x+y*y+z*z)-0.5+0.05*sin(x)");
    check(!wave.rpn.empty(), "composed implicit equation compiles");

    const auto advertisedSphere = geom::makeImplicit("sphere(0.5)");
    check(advertisedSphere.rpn.empty(),
          "function-like sphere shorthand is not silently accepted as an implicit equation");

    const auto advertisedCsg = geom::makeImplicit("smoothUnion(sphere(0.5),box(0.4),0.1)");
    check(advertisedCsg.rpn.empty(),
          "function-like CSG shorthand is not silently accepted as an implicit equation");

    if (failures != 0) {
        std::cerr << "mcp_sdf_contract_test: " << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "mcp_sdf_contract_test: ALL OK\n";
    return EXIT_SUCCESS;
}
