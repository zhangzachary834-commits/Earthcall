import os
import re

singular_hpp = "src/ConstructedBeing/Singular/Singular.hpp"
with open(singular_hpp, "r") as f:
    content = f.read()

if "void registerProperty(" not in content:
    content = content.replace(
        "virtual void buildProperties() = 0;",
        "virtual void buildProperties() = 0;\n\n    void registerProperty(std::unique_ptr<Property> prop) {\n        if (!prop) return;\n        _propertyNames.push_back(prop->nameId());\n        _propertyRegistry.push_back(std::move(prop));\n    }"
    )
    with open(singular_hpp, "w") as f:
        f.write(content)

# Now find all files that use _propertyRegistry.push_back
def process_file(path):
    with open(path, "r") as f:
        content = f.read()
    
    new_content = content.replace("_propertyRegistry.push_back", "registerProperty")
    
    if new_content != content:
        with open(path, "w") as f:
            f.write(new_content)
        print(f"Patched {path}")

for root, _, files in os.walk("src"):
    for file in files:
        if file.endswith((".cpp", ".hpp")):
            process_file(os.path.join(root, file))

