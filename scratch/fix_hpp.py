import re
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content = f.read()

content = content.replace(
    'bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const override;',
    'bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const;'
)
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content)
