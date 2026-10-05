#include "Person/Body/Body.hpp"
#include "Singularity/Storage/Serialization/Person/BodySerialization.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

bool near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

int main() {
    Body originalBody("Humanoid", "Voxel");
    originalBody.height = 1.75f;

    auto* head = new BodyPart("MyHead", BodyPart::Type::Head, ObjectTypes::ShapeKind::Cube, glm::vec3(1.5f, 2.0f, 1.0f));
    head->setColor(0.1f, 0.2f, 0.3f);
    originalBody.addPart(head);

    nlohmann::json j = bodyToJson(originalBody);

    Body restoredBody("Unknown", "Unknown");
    bodyFromJson(j, restoredBody);

    assert(restoredBody.shape == "Humanoid");
    assert(restoredBody.artStyle == "Voxel");
    assert(near(restoredBody.height, 1.75f));

    assert(restoredBody.parts.size() == 1);
    BodyPart* restoredHead = restoredBody.parts[0];

    assert(restoredHead->getName() == "MyHead");
    assert(restoredHead->getType() == BodyPart::Type::Head);
    assert(restoredHead->getPrimaryShape() == ObjectTypes::ShapeKind::Cube);

    assert(near(restoredHead->getDimensions().x, 1.5f));
    assert(near(restoredHead->getDimensions().y, 2.0f));
    assert(near(restoredHead->getDimensions().z, 1.0f));

    assert(near(restoredHead->getColor()[0], 0.1f));
    assert(near(restoredHead->getColor()[1], 0.2f));
    assert(near(restoredHead->getColor()[2], 0.3f));

    std::cout << "body_serialization_test: ALL OK\n";
    return 0;
}
