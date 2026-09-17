import re

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    assert(std::get<bool>(val) == true);',
    '''
    if (std::get<bool>(val) != true) {
        std::cout << "regionB actual:\\n";
        for (auto& e : readBList->elements) {
            auto v = std::get<glm::vec3>(e);
            std::cout << v.x << ", " << v.y << ", " << v.z << "\\n";
        }
        std::cout << "expected:\\n";
        for (auto& e : expectedRedList->elements) {
            auto v = std::get<glm::vec3>(e);
            std::cout << v.x << ", " << v.y << ", " << v.z << "\\n";
        }
        assert(std::get<bool>(val) == true);
    }
'''
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
