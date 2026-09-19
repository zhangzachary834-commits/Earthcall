#include "ConstructedBeing/Singular/Object/AngleTools.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

bool near(float a, float b, float tol = 1e-5f) {
    return std::abs(a - b) <= tol;
}

int main() {
    glm::vec3 origin(0.0f, 0.0f, 0.0f);
    glm::vec3 x_axis(1.0f, 0.0f, 0.0f);
    glm::vec3 y_axis(0.0f, 1.0f, 0.0f);

    float angle = AngleTools::computeVertexAngle(origin, x_axis, y_axis);
    if (!near(angle, glm::half_pi<float>())) {
        std::cerr << "computeVertexAngle failed! expected " << glm::half_pi<float>() << " got " << angle << std::endl;
        return 1;
    }

    int chi = AngleTools::eulerCharacteristic(8, 12, 6);
    if (chi != 2) {
        std::cerr << "eulerCharacteristic failed! expected 2 got " << chi << std::endl;
        return 1;
    }

    std::vector<float> angles1 = {glm::half_pi<float>(), glm::half_pi<float>(), glm::half_pi<float>()};
    if (!AngleTools::canFormConvexVertex(angles1)) {
        std::cerr << "canFormConvexVertex failed for valid angles" << std::endl;
        return 1;
    }

    std::vector<float> angles2 = {glm::pi<float>(), glm::pi<float>(), 0.1f};
    if (AngleTools::canFormConvexVertex(angles2)) {
        std::cerr << "canFormConvexVertex failed for invalid angles" << std::endl;
        return 1;
    }

    std::cout << "All AngleTools tests passed.\n";
    return 0;
}
