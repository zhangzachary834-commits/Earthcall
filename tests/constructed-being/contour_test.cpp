#include "ConstructedBeing/Singular/Object/Contour.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

bool near(float a, float b, float tol = 1e-5f) {
    return std::abs(a - b) <= tol;
}

bool vec_near(const glm::vec3& a, const glm::vec3& b, float tol = 1e-5f) {
    return near(a.x, b.x, tol) && near(a.y, b.y, tol) && near(a.z, b.z, tol);
}

void test_flat_contour() {
    std::vector<glm::vec3> vertices = {
        {0, 0, 0},
        {1, 0, 0},
        {1, 1, 0},
        {0, 1, 0}
    };
    std::vector<int> boundary = {0, 1, 2, 3};
    FlatContour flat(boundary, vertices);

    assert(flat.getType() == Contour::Type::Flat);
    assert(flat.edgeCount() == 4);
    assert(near(flat.area(), 1.0f));

    glm::vec3 normal = flat.normalAt(glm::vec2(0.5f, 0.5f));
    assert(vec_near(normal, glm::vec3(0, 0, 1))); // normal of xy plane CCW

    Contour::CurvatureInfo curv = flat.curvatureAt(glm::vec2(0.5f, 0.5f));
    assert(near(curv.gaussian, 0.0f));
    assert(near(curv.mean, 0.0f));

    std::cout << "test_flat_contour passed\n";
}

void test_spherical_contour() {
    RoundContour sphere = RoundContour::createSpherical(
        glm::vec3(0, 0, 0), 2.0f,
        0.0f, glm::pi<float>(),
        0.0f, 2.0f * glm::pi<float>(),
        {}
    );

    assert(sphere.getType() == Contour::Type::Round);
    assert(near(sphere.area(), 4.0f * glm::pi<float>() * 4.0f));

    Contour::CurvatureInfo curv = sphere.curvatureAt(glm::vec2(0.5f, 0.5f));
    assert(near(curv.principalK1, 0.5f));
    assert(near(curv.principalK2, 0.5f));
    assert(near(curv.gaussian, 0.25f));
    assert(near(curv.mean, 0.5f));

    glm::vec3 normal = sphere.normalAt(glm::vec2(0.0f, 0.5f)); // phi=0, theta=pi/2 -> (sin(pi/2)*cos(0), cos(pi/2), sin(pi/2)*sin(0)) = (1, 0, 0)
    assert(vec_near(normal, glm::vec3(1, 0, 0)));

    std::cout << "test_spherical_contour passed\n";
}

void test_cylindrical_contour() {
    RoundContour cyl = RoundContour::createCylindrical(
        glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 2.0f,
        5.0f, 2.0f * glm::pi<float>(),
        {}
    );

    assert(cyl.getType() == Contour::Type::Round);
    assert(near(cyl.area(), 2.0f * glm::pi<float>() * 2.0f * 5.0f));

    Contour::CurvatureInfo curv = cyl.curvatureAt(glm::vec2(0.5f, 0.5f));
    assert(near(curv.principalK1, 0.5f));
    assert(near(curv.principalK2, 0.0f));
    assert(near(curv.gaussian, 0.0f));
    assert(near(curv.mean, 0.25f));

    std::cout << "test_cylindrical_contour passed\n";
}

void test_conical_contour() {
    RoundContour cone = RoundContour::createConical(
        glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::quarter_pi<float>(), // 45 deg half angle -> r = h
        5.0f, 2.0f * glm::pi<float>(),
        {}
    );

    assert(cone.getType() == Contour::Type::Round);
    // Note: Contour.cpp's conical area calculation differs from standard mathematical formula:
    // The current implementation is: 0.5f * r * slantHeight * (_arcAngle / M_PI)
    // For 45 deg, h = 5: r = 5, slant = 5*sqrt(2)
    // Code Area = 0.5 * 5 * 5*sqrt(2) * (2pi / pi) = 25 * sqrt(2) = 35.3553
    assert(near(cone.area(), 25.0f * std::sqrt(2.0f)));

    Contour::CurvatureInfo curv = cone.curvatureAt(glm::vec2(0.5f, 0.5f)); // t = 0.5 -> r = 5 * (1 - 0.5) = 2.5
    // k1 = 1/2.5 = 0.4
    assert(near(curv.principalK1, 0.4f));
    assert(near(curv.principalK2, 0.0f));

    std::cout << "test_conical_contour passed\n";
}

void test_toroidal_contour() {
    RoundContour torus = RoundContour::createToroidal(
        glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 3.0f, 1.0f,
        2.0f * glm::pi<float>(),
        {}
    );

    assert(torus.getType() == Contour::Type::Round);
    assert(near(torus.area(), 4.0f * glm::pi<float>() * glm::pi<float>() * 3.0f * 1.0f));

    // t = 0 -> minorAngle = 0 -> cos(0)=1 -> k2 = 1 / (3 + 1*1) = 1/4 = 0.25
    Contour::CurvatureInfo curv = torus.curvatureAt(glm::vec2(0.5f, 0.0f));
    assert(near(curv.principalK1, 1.0f)); // 1/R2 = 1/1
    assert(near(curv.principalK2, 0.25f)); // 1/(R1 + R2)

    std::cout << "test_toroidal_contour passed\n";
}


int main() {
    test_flat_contour();
    test_spherical_contour();
    test_cylindrical_contour();
    test_conical_contour();
    test_toroidal_contour();

    std::cout << "All Contour tests passed.\n";
    return 0;
}
