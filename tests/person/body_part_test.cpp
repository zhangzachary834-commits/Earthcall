#include "Person/Body/BodyPart/BodyPart.hpp"
#include <cassert>
#include <iostream>

static void testInitialState() {
    BodyPart bodyPart;
    assert(bodyPart.getName() == "");
    assert(bodyPart.getType() == BodyPart::Type::Undefined);
    assert(bodyPart.getPrimaryShape() == ObjectTypes::ShapeKind::Cube);

    // Check default color
    const float* color = bodyPart.getColor();
    assert(color[0] == 1.0f);
    assert(color[1] == 1.0f);
    assert(color[2] == 1.0f);

    std::cout << "  testInitialState OK\n";
}

static void testCustomInitialization() {
    BodyPart bodyPart("RightArm", BodyPart::Type::Arm, ObjectTypes::ShapeKind::Sphere, glm::vec3(2.0f, 3.0f, 4.0f));
    assert(bodyPart.getName() == "RightArm");
    assert(bodyPart.getType() == BodyPart::Type::Arm);
    assert(bodyPart.getPrimaryShape() == ObjectTypes::ShapeKind::Sphere);

    glm::vec3 dims = bodyPart.getDimensions();
    assert(dims.x == 2.0f);
    assert(dims.y == 3.0f);
    assert(dims.z == 4.0f);

    std::cout << "  testCustomInitialization OK\n";
}

static void testSubObjects() {
    BodyPart bodyPart("Leg", BodyPart::Type::Leg, ObjectTypes::ShapeKind::Cylinder, glm::vec3(1.0f));
    assert(bodyPart.getSubObjectCount() == 0);

    bodyPart.addSubObject(ObjectTypes::ShapeKind::Sphere);
    assert(bodyPart.getSubObjectCount() == 1);

    // Check local offset
    glm::mat4 offset(1.0f);
    offset = glm::translate(offset, glm::vec3(0.0f, -1.0f, 0.0f));
    bodyPart.addSubObject(ObjectTypes::ShapeKind::Cube, offset);
    assert(bodyPart.getSubObjectCount() == 2);

    const glm::mat4& fetchedOffset = bodyPart.getSubObjectLocalOffset(1);
    assert(fetchedOffset[3][1] == -1.0f); // Check translation Y

    bodyPart.removeSubObject(0);
    assert(bodyPart.getSubObjectCount() == 1);
    const glm::mat4& fetchedOffset2 = bodyPart.getSubObjectLocalOffset(0);
    assert(fetchedOffset2[3][1] == -1.0f); // Index shifts down

    std::cout << "  testSubObjects OK\n";
}

static void testTransformPropagation() {
    BodyPart bodyPart("Torso", BodyPart::Type::Torso, ObjectTypes::ShapeKind::Cube, glm::vec3(1.0f));
    bodyPart.addSubObject(ObjectTypes::ShapeKind::Sphere);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
    bodyPart.setTransform(transform);

    // Ensure primary object inherited transform
    Object* primary = bodyPart.getPrimaryObject();
    assert(primary != nullptr);
    assert(primary->getTransform()[3][0] == 5.0f);

    // Ensure sub-object inherited transform
    Object* sub = bodyPart.getSubObject(0);
    assert(sub != nullptr);
    assert(sub->getTransform()[3][0] == 5.0f);

    std::cout << "  testTransformPropagation OK\n";
}

int main() {
    std::cout << "body_part_test:\n";
    testInitialState();
    testCustomInitialization();
    testSubObjects();
    testTransformPropagation();
    std::cout << "body_part_test: ALL OK\n";
    return 0;
}
