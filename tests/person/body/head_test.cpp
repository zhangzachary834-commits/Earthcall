#include "Person/Body/Head/Head.hpp"
#include <iostream>

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "PASSED: " << message << std::endl;
}

int main() {
    Head head;
    check(head.getType() == BodyPart::Type::Head, "Type is Head");
    check(head.getName() == "Head", "Name is Head");
    return 0;
}
