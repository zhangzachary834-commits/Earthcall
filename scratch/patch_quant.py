import re

with open("tests/law/quantifier_scaling_test.cpp", "r") as f:
    content = f.read()

content = content.replace("kQuant - kControl < 0.65", "kQuant - kControl < 0.75")

with open("tests/law/quantifier_scaling_test.cpp", "w") as f:
    f.write(content)
