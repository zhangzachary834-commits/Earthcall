import re

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content = f.read()

content = content.replace(
    'bool readAuthoredPropertyProjectionColors(Earthcall::StringId id, PropertyValue& out) const override;',
    ''
)

content = content.replace(
    '    std::string getTextString() const { return _textString; }\n',
    '    std::string getTextString() const { return _textString; }\n    bool readAuthoredPropertyProjectionColors(Earthcall::StringId id, PropertyValue& out) const override;\n'
)

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content)
