// Throwaway probe: what do Object::rotation and face.N.activeLayer actually
// read back after a write? Distinguishes "setter does nothing" from "setter
// legitimately clamps".
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* w = glfwCreateWindow(64, 64, "probe", nullptr, nullptr);
    glfwMakeContextCurrent(w);

    Object o;
    Property* rot = o.findProperty("rotation");
    glm::vec3 before = std::get<glm::vec3>(rot->value());
    std::printf("rotation before = (%.6f, %.6f, %.6f)\n", before.x, before.y, before.z);
    bool ok = rot->setValue(PropertyValue(glm::vec3(1.0f, 2.0f, 3.0f)));
    glm::vec3 after = std::get<glm::vec3>(rot->value());
    std::printf("setValue -> %d ; rotation after = (%.6f, %.6f, %.6f)\n",
                (int)ok, after.x, after.y, after.z);

    Property* al = o.findProperty("face.0.activeLayer");
    if (al) {
        PropertyValue v = al->value();
        std::printf("face.0.activeLayer before = %d\n", std::get<int>(v));
        bool ok2 = al->setValue(PropertyValue(1));
        std::printf("setValue(1) -> %d ; after = %d\n", (int)ok2, std::get<int>(al->value()));
        bool ok3 = al->setValue(PropertyValue(2));
        std::printf("setValue(2) -> %d ; after = %d\n", (int)ok3, std::get<int>(al->value()));
    }
    glfwDestroyWindow(w);
    glfwTerminate();
    return 0;
}
