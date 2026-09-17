import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'law->addAuthor(Earthcall::StringInterner::intern("person.test"));',
    'law->addAuthor(*harness.me);'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
