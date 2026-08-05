#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Form {
public:
    enum class ShapeType {
        Cube,
        Sphere,
        Custom
    };

    Form() = default;
    Form(ShapeType type, const glm::vec3& dims) : shape(type), dimensions(dims) {}

    // Draw the form using OpenGL immediate mode (for prototype purposes)
    void draw() const;

    // Simple accessor
    ShapeType getShape() const { return shape; }
    const glm::vec3& getDimensions() const { return dimensions; }

    void setDimensions(const glm::vec3& dims) { dimensions = dims; }

private:
    ShapeType   shape       = ShapeType::Cube;
    glm::vec3   dimensions  {1.0f, 1.0f, 1.0f};
};
