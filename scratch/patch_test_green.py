with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'for (int i = 0; i < 2; i++) redList->elements.push_back(PropertyValue(red));',
    'for (int i = 0; i < 2; i++) redList->elements.push_back(PropertyValue(glm::vec3(0.0f, 1.0f, 0.0f)));'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
