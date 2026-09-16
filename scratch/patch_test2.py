import re
with open("tests/law/synthesis_studio_living_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'const auto saved=scratch.path / "worlds/roundtrip.ecform";',
    'const auto saved=scratch.path / "worlds/roundtrip.json";'
)

with open("tests/law/synthesis_studio_living_test.cpp", "w") as f:
    f.write(content)
