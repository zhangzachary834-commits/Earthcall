import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'auto law = std::make_shared<Law>("witness.law");',
    'auto law = std::make_shared<Law>("witness.law");\n    law->addAuthor(Earthcall::StringInterner::intern("person.test"));'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
