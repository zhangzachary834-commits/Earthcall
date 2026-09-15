#include "ConstructedBeing/Singular/Object/Object/FaceTexture.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

bool nearf(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

int main() {
    FaceTexture tex;
    tex.create(2, 2, 0xFFFFFFFF);

    assert(tex.width == 2);
    assert(tex.height == 2);
    assert(tex.pixels.size() == 16);

    // Test base operations
    assert(tex.layers.size() == 1);

    // Add layers
    tex.addLayer();
    assert(tex.layers.size() == 2);

    // Blend mode setting
    tex.setBlendMode(1, 1);
    assert(tex.blendModes[1] == 1);

    // Test edge cases in pixel write
    assert(!tex.writePixel(glm::vec2(-0.1f, 0.5f), glm::vec3(1.0f)));
    assert(!tex.writePixel(glm::vec2(1.1f, 0.5f), glm::vec3(1.0f)));
    assert(!tex.writePixel(glm::vec2(0.5f, -0.1f), glm::vec3(1.0f)));
    assert(!tex.writePixel(glm::vec2(0.5f, 1.1f), glm::vec3(1.0f)));

    // Pixel write
    assert(tex.writePixel(glm::vec2(0.25f, 0.25f), glm::vec3(1.0f, 0.0f, 0.0f)));
    assert(tex.pixels[0] == 255);
    assert(tex.pixels[1] == 0);
    assert(tex.pixels[2] == 0);
    assert(tex.pixels[3] == 255);

    // Test edge cases in region write
    std::vector<glm::vec3> bad_colors = { glm::vec3(1.0f) };
    assert(!tex.writeRegion(-1, 0, 1, 1, bad_colors));
    assert(!tex.writeRegion(0, -1, 1, 1, bad_colors));
    assert(!tex.writeRegion(0, 0, 3, 1, bad_colors));
    assert(!tex.writeRegion(0, 0, 1, 3, bad_colors));
    assert(!tex.writeRegion(0, 0, 1, 1, {}));

    // Write region
    std::vector<glm::vec3> colors = {
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)
    };
    assert(tex.writeRegion(0, 0, 2, 2, colors));

    for (int i = 0; i < 4; ++i) {
        assert(tex.pixels[i*4 + 0] == 0);
        assert(tex.pixels[i*4 + 1] == 255);
        assert(tex.pixels[i*4 + 2] == 0);
        assert(tex.pixels[i*4 + 3] == 255);
    }

    // Delete layer
    tex.deleteLayer(1);
    assert(tex.layers.size() == 1);

    std::cout << "OK\n";
    return 0;
}
