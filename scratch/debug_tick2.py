import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    std::cout << "DEBUG: beings count: " << Universe::instance().beings().size() << "\\n";\n    harness.lawManager.tick();',
    '''    bool found = false;
    for (auto* b : Universe::instance().beings()) {
        if (b == obj.get()) found = true;
    }
    std::cout << "DEBUG: obj in beings? " << found << "\\n";
    harness.lawManager.tick();'''
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
