#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <iostream>

int main() {
    for (float x = 0.0f; x < 1.0f; x += 0.2f) {
        std::cout << glm::perlin(glm::vec3(x, 0.5f, 0.1f)) << std::endl;
    }
    return 0;
}
