#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/AdvancedFacePaint.hpp"

#include <cassert>
#include <iostream>
#include <cmath>

int main() {
    std::cout << "Testing AdvancedFacePainter...\n";
    using namespace AdvancedFacePaint;

    AdvancedFacePainter painter;

    // 1. Linear Gradient Test
    {
        GradientSettings linear;
        linear.type = GradientType::Linear;
        linear.startColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        linear.endColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        linear.startPoint = glm::vec2(0.0f, 0.0f);
        linear.endPoint = glm::vec2(1.0f, 0.0f);
        linear.useAlpha = false;

        // At start point
        glm::vec4 c1 = painter.calculateGradientColor(glm::vec2(0.0f, 0.0f), linear);
        assert(std::abs(c1.r - 1.0f) < 0.01f);
        assert(std::abs(c1.b - 0.0f) < 0.01f);

        // At mid point
        glm::vec4 c2 = painter.calculateGradientColor(glm::vec2(0.5f, 0.0f), linear);
        assert(std::abs(c2.r - 0.5f) < 0.01f);
        assert(std::abs(c2.b - 0.5f) < 0.01f);

        // At end point
        glm::vec4 c3 = painter.calculateGradientColor(glm::vec2(1.0f, 0.0f), linear);
        assert(std::abs(c3.r - 0.0f) < 0.01f);
        assert(std::abs(c3.b - 1.0f) < 0.01f);
    }

    // 2. Radial Gradient Test
    {
        GradientSettings radial;
        radial.type = GradientType::Radial;
        radial.startColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        radial.endColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        radial.startPoint = glm::vec2(0.5f, 0.5f);
        radial.endPoint = glm::vec2(1.0f, 0.5f); // Distance 0.5
        radial.useAlpha = false;

        // At center
        glm::vec4 c1 = painter.calculateGradientColor(glm::vec2(0.5f, 0.5f), radial);
        assert(std::abs(c1.r - 1.0f) < 0.01f);

        // At edge
        glm::vec4 c2 = painter.calculateGradientColor(glm::vec2(1.0f, 0.5f), radial);
        assert(std::abs(c2.r - 0.0f) < 0.01f);

        // At mid distance
        glm::vec4 c3 = painter.calculateGradientColor(glm::vec2(0.75f, 0.5f), radial);
        assert(std::abs(c3.r - 0.5f) < 0.01f);
    }

    // 3. Normal Smudge Test
    {
        SmudgeSettings smudge;
        smudge.type = SmudgeType::Normal;
        smudge.radius = 0.5f;
        smudge.strength = 1.0f;
        smudge.softness = 1.0f;

        glm::vec4 baseColor(1.0f, 1.0f, 1.0f, 1.0f);

        // At center
        glm::vec4 c1 = painter.calculateSmudgeColor(glm::vec2(0.5f, 0.5f), smudge, baseColor);
        // It smudges towards 0 based on implementation
        assert(c1.r < 1.0f);
    }

    // 4. Initialize and Cleanup
    {
        initializeAdvancedPainter();
        assert(g_advancedPainter != nullptr);

        // Test high-level paint (with null object)
        bool success = paintFaceAdvanced(nullptr, 0, glm::vec2(0.5f), nullptr, nullptr);
        assert(!success);

        cleanupAdvancedPainter();
        assert(g_advancedPainter == nullptr);
    }

    std::cout << "AdvancedFacePaint tests passed!\n";
    return 0;
}
