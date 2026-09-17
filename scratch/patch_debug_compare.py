with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'std::cout << "setValue result: " << (int)result << "\\n";',
    'std::cout << "beforeVal is null? " << !std::holds_alternative<std::shared_ptr<PropertyList>>(beforeVal) << "\\n";\n    if (auto p = std::get_if<std::shared_ptr<PropertyList>>(&beforeVal)) {\n        if (*p && !(*p)->elements.empty()) {\n            if (auto v = std::get_if<glm::vec3>(&(*p)->elements[0])) {\n                std::cout << "beforeVal[0]: " << v->x << ", " << v->y << ", " << v->z << "\\n";\n            }\n        }\n    }\n    std::cout << "setValue result: " << (int)result << "\\n";'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
