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

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content2 = f.read()

if "bool readAuthoredPropertyProjectionColors" not in content2:
    content2 = content2.replace(
        'bool readAuthoredPropertyProjection(Earthcall::StringId id,\n                                        PropertyValue& out) const override;',
        'bool readAuthoredPropertyProjection(Earthcall::StringId id,\n                                        PropertyValue& out) const override;\n    bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,\n                                        PropertyValue& out) const override;'
    )
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content2)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content3 = f.read()

helper = """
static void resolveProjectionToken(Singular& target, const PropertyValue& in, PropertyValue& out) {
    out = in;
    const auto* dict = std::get_if<std::shared_ptr<PropertyDict>>(&in);
    if (!dict || !*dict) return;
    auto itType = (*dict)->elements.find("_type");
    if (itType == (*dict)->elements.end() || !std::holds_alternative<std::string>(itType->second) || std::get<std::string>(itType->second) != "projection") return;
    
    auto itTarget = (*dict)->elements.find("target");
    if (itTarget == (*dict)->elements.end() || !std::holds_alternative<std::string>(itTarget->second)) return;
    std::string targetName = std::get<std::string>(itTarget->second);
    
    target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);
}
"""

if "resolveProjectionToken" not in content3:
    content3 = content3.replace(
        '#include <utility>\n',
        '#include <utility>\n' + helper
    )

content3 = content3.replace(
    'PropertyValue rhs = rhsLiteral;',
    'resolveProjectionToken(t, lhs, lhs);\n                PropertyValue rhs = rhsLiteral;'
)

content3 = content3.replace(
    '                        });\n                    }\n                    return false;\n                }\n                return [evaluate](lhs, rhs);',
    '                        });\n                    }\n                    return false;\n                }\n                resolveProjectionToken(t, rhs, rhs);\n                return [evaluate](lhs, rhs);'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content3)
