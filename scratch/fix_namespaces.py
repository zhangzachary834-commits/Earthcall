import re

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    code = f.read()

code = code.replace("OntoMath::ConditionNode", "ConditionNode")
code = code.replace("OntoMath::ActionNode", "ActionNode")

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(code)

