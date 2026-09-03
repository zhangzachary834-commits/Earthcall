#include <iostream>
#include <cmath>
#include "src/ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

// We can just manually emulate the SDF evaluation for Perlin terrain
// Actually, no need to link the whole engine, I can just do the math:
int main() {
    glm::vec3 extent(500, 15, 500);
    glm::ivec3 res(64, 16, 64);
    glm::vec3 step = (extent * 2.0f) / glm::vec3(res);
    glm::vec3 c = glm::vec3(0, 0, 0); // center of some voxel
    glm::vec3 minBox = -extent;
    glm::vec3 maxBox = minBox + step;
    float radius = glm::length(step * 0.5f);
    
    std::cout << "Extent: " << extent.x << ", " << extent.y << ", " << extent.z << "\n";
    std::cout << "Step: " << step.x << ", " << step.y << ", " << step.z << "\n";
    std::cout << "Radius: " << radius << "\n";
    std::cout << "Radius * 4.0: " << radius * 4.0f << "\n";
    
    // For a voxel in the top layer (y = 14)
    // The surface is at y = 0
    // d = 14
    // abs(14) <= 44.3 -> TRUE!
    std::cout << "d for top voxel: " << 14.0f << "\n";
    std::cout << "Is it <= radius*4? " << (14.0f <= radius * 4.0f ? "YES (OVERDRAW!)" : "NO") << "\n";
}
