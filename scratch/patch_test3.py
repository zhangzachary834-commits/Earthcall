import re
with open("tests/law/synthesis_studio_living_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'const auto saved=scratch.path / "worlds/roundtrip.json";',
    'const auto saved=scratch.path / "worlds/roundtrip.ecform";'
)
content = content.replace(
    'std::ifstream savedFile(saved);',
    'std::ifstream savedFile(scratch.path / "worlds/roundtrip.ecform");'
)


with open("tests/law/synthesis_studio_living_test.cpp", "w") as f:
    f.write(content)
