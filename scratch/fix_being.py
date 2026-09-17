import re
with open("src/ConstructedBeing/Singular/Singular.hpp", "r") as f:
    content = f.read()

if "virtual bool readAuthoredPropertyProjectionColors(" not in content:
    content = content.replace(
        'virtual bool readAuthoredPropertyProjection(Earthcall::StringId,\n                                                PropertyValue&) const {\n        return false;\n    }',
        'virtual bool readAuthoredPropertyProjection(Earthcall::StringId,\n                                                PropertyValue&) const {\n        return false;\n    }\n    virtual bool readAuthoredPropertyProjectionColors(Earthcall::StringId,\n                                                      PropertyValue&) const {\n        return false;\n    }'
    )
with open("src/ConstructedBeing/Singular/Singular.hpp", "w") as f:
    f.write(content)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content2 = f.read()

content2 = content2.replace(
    'if (target.isKind(BeingKind::Object)) {\n        Object& obj = static_cast<Object&>(target);\n        obj.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);\n    }',
    'target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content2)

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content3 = f.read()

content3 = content3.replace(
    'bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const;',
    'bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const override;'
)
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content3)
