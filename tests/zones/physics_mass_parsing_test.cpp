#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <cmath>

int main() {
    std::cout << "Testing Physics::getObjectMass error paths..." << std::endl;

    auto obj = std::make_shared<Object>();

    // Case 1: No mass attribute
    assert(Physics::getObjectMass(obj.get(), 5.0f) == 5.0f);

    // Case 2: Valid mass attribute
    obj->setAttribute("mass", "10.5");
    assert(Physics::getObjectMass(obj.get(), 5.0f) == 10.5f);

    // Case 3: Invalid mass attribute (string that can't be parsed)
    // This will throw an exception in std::stof which is caught and handled
    obj->setAttribute("mass", "invalid_mass");
    assert(Physics::getObjectMass(obj.get(), 5.0f) == 5.0f);

    // Case 4: Negative mass attribute (should fallback to default since v > 0.0f is checked)
    obj->setAttribute("mass", "-10.0");
    assert(Physics::getObjectMass(obj.get(), 5.0f) == 5.0f);

    // Case 5: Infinity or NaN (should fallback to default since std::isfinite(v) is checked)
    obj->setAttribute("mass", "inf");
    assert(Physics::getObjectMass(obj.get(), 5.0f) == 5.0f);

    obj->setAttribute("mass", "nan");
    assert(Physics::getObjectMass(obj.get(), 5.0f) == 5.0f);

    // Null object test
    assert(Physics::getObjectMass(nullptr, 3.14f) == 3.14f);

    std::cout << "All Physics mass parsing tests passed!" << std::endl;
    return 0;
}
