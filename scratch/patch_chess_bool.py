import re

with open("tests/law/chess_extended_rules_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'if (!being.getDynamicProperty(name, v)) return false;',
    'if (!lawGetValue(being, PropertyPath::parse(name), v)) return false;'
)

with open("tests/law/chess_extended_rules_test.cpp", "w") as f:
    f.write(content)
