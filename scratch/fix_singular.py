import re
with open("src/ConstructedBeing/Singular/Singular.hpp", "r") as f:
    content = f.read()

content = content.replace(
    'virtual bool readAuthoredPropertyProjectionColors(Earthcall::StringId,\n                                                      PropertyValue&) const {\n        return false;\n    }',
    ''
)
content = content.replace(
    'public:',
    'public:\n    virtual bool readAuthoredPropertyProjectionColors(Earthcall::StringId,\n                                                      PropertyValue&) const {\n        return false;\n    }'
)

with open("src/ConstructedBeing/Singular/Singular.hpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content2 = f.read()

content2 = content2.replace(
    'bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const override;',
    ''
)
content2 = content2.replace(
    'public:',
    'public:\n    bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const override;'
)
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content2)

