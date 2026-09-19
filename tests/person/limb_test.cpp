#include "Person/Body/BodyPart/Limb.hpp"
#include "Person/Body/BodyPart/Limb/Finger.hpp"
#include "Person/Body/BodyPart/Limb/ForeArm.hpp"
#include <iostream>
#include <cassert>

int main() {
    Limb l("TestLimb", BodyPart::Type::Arm);
    assert(l.getName() == "TestLimb");
    assert(l.getType() == BodyPart::Type::Arm);

    Finger f(Finger::Side::Left, "Index");
    assert(f.getType() == BodyPart::Type::Finger);

    ForeArm arm(ForeArm::Side::Right);
    assert(arm.getType() == BodyPart::Type::ForeArm);

    std::cout << "Limb tests passed" << std::endl;
    return 0;
}
