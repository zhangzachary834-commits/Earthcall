#include "../../../src/Person/Body/BodyPart/Limb/Leg.hpp"
#include <iostream>

int main() {
    Leg leftLeg(Leg::Side::Left);
    if (leftLeg.getName() != "LeftLeg") return 1;
    if (leftLeg.getType() != BodyPart::Type::Leg) return 1;
    if (leftLeg.getDimensions() != glm::vec3(0.15f, 0.5f, 0.15f)) return 1;
    if (leftLeg.getPrimaryShape() != ObjectTypes::ShapeKind::Cube) return 1;

    Leg rightLeg(Leg::Side::Right);
    if (rightLeg.getName() != "RightLeg") return 1;
    if (rightLeg.getType() != BodyPart::Type::Leg) return 1;
    if (rightLeg.getDimensions() != glm::vec3(0.15f, 0.5f, 0.15f)) return 1;
    if (rightLeg.getPrimaryShape() != ObjectTypes::ShapeKind::Cube) return 1;

    std::cout << "leg_test passed\n";
    return 0;
}
