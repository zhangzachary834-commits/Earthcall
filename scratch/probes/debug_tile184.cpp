#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include "Singularity/OntoMath/ClampedWaveSupport.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/Renderer.hpp"

using Json = nlohmann::json;

static std::pair<glm::vec3, glm::vec3> canonicalPose(int i) {
    const double a = 6.2831853071795864769 * i / 64.0;
    glm::vec3 eye(float(8 * std::sin(a)), float(50 + 2 * std::sin(a * .5)), float(75 + 8 * (1 - std::cos(a))));
    glm::vec3 aim(float(40 + 8 * std::sin(a * .5)), float(62 + 2 * std::cos(a)), 110);
    return {eye, aim};
}

int main() {
    std::cout << "Debug tile 184" << std::endl;
    return 0;
}
