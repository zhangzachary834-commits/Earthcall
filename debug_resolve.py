import re
cpp_path = "src/ConstructedBeing/Singular/Property/PropertyPath.cpp"
with open(cpp_path, "r") as f:
    content = f.read()

content = content.replace(
    "if (i == segments.size() - 1 && trailingComponent &&\n            std::holds_alternative<glm::vec3>(found->value())) {",
    "printf(\"i=%zu segments.size()=%zu trailingComponent=%p isVec3=%d\\n\", i, segments.size(), trailingComponent, (int)std::holds_alternative<glm::vec3>(found->value()));\n        if (i == segments.size() - 1 && trailingComponent &&\n            std::holds_alternative<glm::vec3>(found->value())) {"
)

with open(cpp_path, "w") as f:
    f.write(content)
