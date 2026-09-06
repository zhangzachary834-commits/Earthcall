#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include <iostream>
#include <chrono>

int main() {
    auto node = geom::makeImplicit("y - 40.0 * noise(0.008 * (p + vec3(100,0,100)))");
    geom::SdfNode sdfNode(node);
    
    auto start = std::chrono::high_resolution_clock::now();
    int hitCount = 0;
    for (int iz = 0; iz < 64; ++iz) {
        for (int ix = 0; ix < 64; ++ix) {
            for (int iy = 0; iy < 32; ++iy) {
                glm::vec3 boxMin(ix, iy, iz);
                glm::vec3 boxMax(ix+1, iy+1, iz+1);
                OntoMath::Interval range = geom::evalRange(sdfNode, boxMin, boxMax);
                if (range.lo <= 0.0f && range.hi >= 0.0f) hitCount++;
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " us, hits: " << hitCount << "\n";
    return 0;
}
