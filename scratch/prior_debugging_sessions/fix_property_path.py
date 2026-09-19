import re

header_path = "src/ConstructedBeing/Singular/Property/PropertyPath.hpp"
with open(header_path, "r") as f:
    content = f.read()

content = content.replace(
    "Property* resolve(Singular& root, std::string* trailingComponent = nullptr,\n                      Singular** owner = nullptr) const;",
    "Property* resolve(Singular& root, std::string* trailingComponent = nullptr,\n                      Singular** owner = nullptr, std::size_t startIndex = 0) const;"
)
content = content.replace(
    "PathResult getValue(Singular& root, PropertyValue& out) const;",
    "PathResult getValue(Singular& root, PropertyValue& out, std::size_t startIndex = 0) const;"
)
content = content.replace(
    "PathResult setValue(Singular& root, const PropertyValue& v) const;",
    "PathResult setValue(Singular& root, const PropertyValue& v, std::size_t startIndex = 0) const;"
)

with open(header_path, "w") as f:
    f.write(content)

cpp_path = "src/ConstructedBeing/Singular/Property/PropertyPath.cpp"
with open(cpp_path, "r") as f:
    content = f.read()

content = content.replace(
    "Property* PropertyPath::resolve(Singular& root, std::string* trailingComponent,\n                                Singular** owner) const {",
    "Property* PropertyPath::resolve(Singular& root, std::string* trailingComponent,\n                                Singular** owner, std::size_t startIndex) const {"
)
content = content.replace(
    "std::size_t i = 0;",
    "std::size_t i = startIndex;"
)
content = content.replace(
    "PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out) const {",
    "PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out, std::size_t startIndex) const {"
)
content = content.replace(
    "Property* property = resolve(root, &component);",
    "Property* property = resolve(root, &component, nullptr, startIndex);"
)
content = content.replace(
    "if (segments.size() == 1) {",
    "if (segments.size() - startIndex == 1) {"
)
content = content.replace(
    "if (root.getDynamicProperty(segments[0], out)) {",
    "if (root.getDynamicProperty(segments[startIndex], out)) {"
)

content = content.replace(
    "PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v) const {",
    "PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v, std::size_t startIndex) const {"
)
content = content.replace(
    "Property* property = resolve(root, &component, &owner);",
    "Property* property = resolve(root, &component, &owner, startIndex);"
)
content = content.replace(
    "if (segments.size() == 1) {",
    "if (segments.size() - startIndex == 1) {"
)
content = content.replace(
    "if (root.getDynamicProperty(segments[0], cur) &&",
    "if (root.getDynamicProperty(segments[startIndex], cur) &&"
)
content = content.replace(
    "root.setDynamicProperty(segments[0], v);",
    "root.setDynamicProperty(segments[startIndex], v);"
)

with open(cpp_path, "w") as f:
    f.write(content)
