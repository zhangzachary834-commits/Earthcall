#include <iostream>
#include <chrono>
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
int main() {
    auto t0 = std::chrono::high_resolution_clock::now();
    geom::SdfNode n = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f));
    geom::tessellateSdf(n, glm::vec3(1.0f), glm::ivec3(32));
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "32x32x32 Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";

    t0 = std::chrono::high_resolution_clock::now();
    geom::tessellateSdf(n, glm::vec3(1.0f), glm::ivec3(128, 24, 128));
    t1 = std::chrono::high_resolution_clock::now();
    std::cout << "128x24x128 Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
    return 0;
}
