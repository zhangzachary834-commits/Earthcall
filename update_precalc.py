import re

with open("tests/singularity/property_path_precalc_test.cpp", "r") as f:
    code = f.read()

# Update test resolving to Property*
code = re.sub(
    r"Property\* prop = path\.resolve\(obj\);",
    "auto slot = path.resolve(obj);\n    Property* prop = slot.prop;",
    code
)

code = re.sub(
    r"Property\* prop = path\.resolve\(obj, &component\);",
    "auto slot = path.resolve(obj);\n    Property* prop = slot.prop;\n    component = slot.trailingComponent;",
    code
)

with open("tests/singularity/property_path_precalc_test.cpp", "w") as f:
    f.write(code)
