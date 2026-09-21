#include "Person/Body/BodyPart/Limb.hpp"
#include "Person/Body/BodyPart/Limb/Arm.hpp"
#include "Person/Body/BodyPart/Limb/Finger.hpp"
#include "Person/Body/BodyPart/Limb/Foot.hpp"
#include "Person/Body/BodyPart/Limb/ForeArm.hpp"
#include "Person/Body/BodyPart/Limb/ForeLeg.hpp"
#include "Person/Body/BodyPart/Limb/Hand.hpp"
#include "Person/Body/BodyPart/Limb/Leg.hpp"
#include "Person/Body/BodyPart/Limb/Neck.hpp"
#include "Person/Body/BodyPart/Limb/Shoulder.hpp"
#include "Person/Body/BodyPart/Limb/Torso.hpp"

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

    Arm leftArm(Arm::Side::Left);
    assert(leftArm.getType() == BodyPart::Type::Arm);
    assert(leftArm.getName() == "LeftArm");

    Foot rightFoot(Foot::Side::Right);
    assert(rightFoot.getType() == BodyPart::Type::Foot);
    assert(rightFoot.getName() == "RightFoot");

    ForeLeg leftForeLeg(ForeLeg::Side::Left);
    assert(leftForeLeg.getType() == BodyPart::Type::ForeLeg);
    assert(leftForeLeg.getName() == "LeftForeLeg");

    Hand rightHand(Hand::Side::Right);
    assert(rightHand.getType() == BodyPart::Type::Hand);
    assert(rightHand.getName() == "RightHand");

    Leg leftLeg(Leg::Side::Left);
    assert(leftLeg.getType() == BodyPart::Type::Leg);
    assert(leftLeg.getName() == "LeftLeg");

    Neck neck;
    assert(neck.getType() == BodyPart::Type::Neck);
    assert(neck.getName() == "Neck");

    Shoulder rightShoulder(Shoulder::Side::Right);
    assert(rightShoulder.getType() == BodyPart::Type::Shoulder);
    assert(rightShoulder.getName() == "RightShoulder");

    Torso torso;
    assert(torso.getType() == BodyPart::Type::Torso);
    assert(torso.getName() == "Torso");

    std::cout << "Limb tests passed" << std::endl;
    return 0;
}
