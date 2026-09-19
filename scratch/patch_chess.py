import re

with open("tests/law/chess_extended_rules_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'if (!being.getDynamicProperty(name, v)) return fallback;',
    'if (!lawGetValue(being, PropertyPath::parse(name), v)) return fallback;'
)

if 'ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp' not in content:
    content = content.replace(
        '#include "support/test_harness.hpp"\n',
        '#include "support/test_harness.hpp"\n#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"\n'
    )

with open("tests/law/chess_extended_rules_test.cpp", "w") as f:
    f.write(content)
