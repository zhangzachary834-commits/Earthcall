#include <variant>
#include <iostream>
#include <glm/glm.hpp>

class Singular; class Object; class Relation; class Formation;
using PropertyValue = std::variant<
    std::monostate, int, float, double, bool, char, long, std::string,
    glm::vec3, glm::mat4, Singular*, Object*, Relation*, Formation*
>;

int main() {
    float f = 1.0f;
    PropertyValue val = f;
    std::cout << "Index: " << val.index() << std::endl;
    return 0;
}
