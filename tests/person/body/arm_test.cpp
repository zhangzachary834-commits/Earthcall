#include "../../../src/Person/Body/BodyPart/Limb/Arm.hpp"
#include <iostream>

int main() {
    Arm leftArm(Arm::Side::Left);
    if (leftArm.getName() != "LeftArm") return 1;
    if (leftArm.getType() != BodyPart::Type::Arm) return 1;
    if (leftArm.getDimensions() != glm::vec3(0.1f, 0.5f, 0.1f)) return 1;
    if (leftArm.getPrimaryShape() != ObjectTypes::ShapeKind::Cube) return 1;

    Arm rightArm(Arm::Side::Right);
    if (rightArm.getName() != "RightArm") return 1;
    if (rightArm.getType() != BodyPart::Type::Arm) return 1;
    if (rightArm.getDimensions() != glm::vec3(0.1f, 0.5f, 0.1f)) return 1;
    if (rightArm.getPrimaryShape() != ObjectTypes::ShapeKind::Cube) return 1;

    std::cout << "arm_test passed\n";
    return 0;
}
