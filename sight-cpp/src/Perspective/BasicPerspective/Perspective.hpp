#pragma once

#include <string>
#include <GLFW/glfw3.h>
#include "Form/Singular/Singular.hpp"

class Perspective : public Singular {
public:
    Perspective();
    ~Perspective();

    void update(float deltaTime);
    void render();
    void handleInput(GLFWwindow* window);

    std::string getIdentifier() const override { return "Perspective"; }

private:
    void buildProperties() override {}
};
