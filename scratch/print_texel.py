with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'assert(result == PropertyPath::PathResult::Ok);',
    'PropertyValue temp;\nPropertyPath::parse("regionB").getValue(*obj, temp);\nauto tList = std::get<std::shared_ptr<PropertyList>>(temp);\nauto tv = std::get<glm::vec3>(tList->elements[0]);\nstd::cout << "regionB[0] is " << tv.x << "," << tv.y << "," << tv.z << "\\n";\nassert(result == PropertyPath::PathResult::Ok);'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
