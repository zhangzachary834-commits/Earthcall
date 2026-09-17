import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'law->setActionModel(act);',
    'law->setActionModel(act);\n    law->setActivation(Law::Activation::WhileTrue);'
)
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
