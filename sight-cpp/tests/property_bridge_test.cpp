// Property bridge milestone test (LAW_AND_CREATION_SYSTEM.md, commit 2):
//   "set position.y by path → the cube actually moves."
//
// Exercises: PropertyPath resolution (dotted names, vec3 components),
// ComputedProperty (position via transform, rotation via euler accessors),
// PropertyRef (center, shape.* params), arithmetic coercion, and clean
// failure on unknown paths.

#include "Form/Object/Object.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    // Object's constructor initializes face textures (GL calls), so give the
    // test a real — but hidden — context.
    if (!glfwInit()) {
        std::fprintf(stderr, "property_bridge_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "property_bridge_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "property_bridge_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object obj;

        // 1. The milestone: position.y by path moves the object.
        assert(PropertyPath::parse("position.y").setValue(obj, PropertyValue(3.0f)));
        assert(std::fabs(obj.getTransform()[3].y - 3.0f) < 1e-5f);

        // Whole-vector set.
        assert(PropertyPath::parse("position")
                   .setValue(obj, PropertyValue(glm::vec3(1.0f, 2.0f, 3.0f))));
        assert(std::fabs(obj.getTransform()[3].x - 1.0f) < 1e-5f);
        assert(std::fabs(obj.getTransform()[3].z - 3.0f) < 1e-5f);

        // 2. Rotation round-trips through the euler accessors.
        assert(PropertyPath::parse("rotation")
                   .setValue(obj, PropertyValue(glm::vec3(0.0f, 90.0f, 0.0f))));
        PropertyValue out;
        assert(PropertyPath::parse("rotation.y").getValue(obj, out));
        assert(std::fabs(std::get<float>(out) - 90.0f) < 1e-3f);

        // 3. Shape params registered under dotted names.
        assert(PropertyPath::parse("shape.majorR").setValue(obj, PropertyValue(0.5f)));
        assert(PropertyPath::parse("shape.majorR").getValue(obj, out));
        assert(std::fabs(std::get<float>(out) - 0.5f) < 1e-6f);

        // Arithmetic coercion: an int arriving for a float slot still lands.
        assert(PropertyPath::parse("shape.minorR").setValue(obj, PropertyValue(1)));
        assert(PropertyPath::parse("shape.minorR").getValue(obj, out));
        assert(std::fabs(std::get<float>(out) - 1.0f) < 1e-6f);

        // center via PropertyRef.
        assert(PropertyPath::parse("center.z").setValue(obj, PropertyValue(0.25f)));
        assert(std::fabs(obj.getCenter().z - 0.25f) < 1e-6f);

        // 4. Unknown paths fail cleanly — no crash, nullptr/false.
        assert(PropertyPath::parse("nonexistent.thing").resolve(obj) == nullptr);
        assert(!PropertyPath::parse("nonexistent").setValue(obj, PropertyValue(1)));
        PropertyValue unused;
        assert(!PropertyPath::parse("position.w").getValue(obj, unused));
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("property_bridge_test: ALL OK");
    return 0;
}
