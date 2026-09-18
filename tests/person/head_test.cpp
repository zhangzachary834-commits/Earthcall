#include "Person/Body/Head/Head.hpp"
#include <cassert>
#include <iostream>

static void testHeadConstructor() {
    Head head;
    assert(head.getName() == "Head");
    assert(head.getType() == BodyPart::Type::Head);
    assert(head.getPrimaryShape() == ObjectTypes::ShapeKind::Cube);

    glm::vec3 dims = head.getDimensions();
    assert(dims.x == 0.3f);
    assert(dims.y == 0.3f);
    assert(dims.z == 0.3f);

    std::cout << "  testHeadConstructor OK\n";
}

int main() {
    std::cout << "head_test:\n";
    testHeadConstructor();
    std::cout << "head_test: ALL OK\n";
    return 0;
}
