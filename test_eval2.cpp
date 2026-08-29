#include <iostream>
#include <cmath>

struct vec3 { float x, y, z; };
struct ivec3 { int x, y, z; };

float length(vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

int main() {
    vec3 extent = {500, 15, 500};
    ivec3 res = {64, 16, 64};
    vec3 step = { (extent.x * 2.0f) / res.x, (extent.y * 2.0f) / res.y, (extent.z * 2.0f) / res.z };
    vec3 halfStep = { step.x * 0.5f, step.y * 0.5f, step.z * 0.5f };
    float radius = length(halfStep);
    
    std::cout << "Extent: " << extent.x << ", " << extent.y << ", " << extent.z << "\n";
    std::cout << "Step: " << step.x << ", " << step.y << ", " << step.z << "\n";
    std::cout << "Radius: " << radius << "\n";
    std::cout << "Radius * 4.0: " << radius * 4.0f << "\n";
    
    std::cout << "d for top voxel: " << 14.0f << "\n";
    std::cout << "Is it <= radius*4? " << (14.0f <= radius * 4.0f ? "YES (OVERDRAW!)" : "NO") << "\n";
}
