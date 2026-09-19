import re

file_path = "src/ConstructedBeing/Singular/Object/Object/ObjectProperties.cpp"
with open(file_path, "r") as f:
    content = f.read()

# For ShapeKindBridge which has inline `std::string name() const override { return "shape.kind"; }`
content = re.sub(
    r'(std::string name\(\) const override \{[^\}]+\})',
    r'\1\n    Earthcall::StringId nameId() const override { return Earthcall::StringInterner::intern(name()); }',
    content
)

with open(file_path, "w") as f:
    f.write(content)
