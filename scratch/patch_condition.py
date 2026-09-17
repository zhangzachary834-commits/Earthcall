import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

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
    
    // It's a projection token! Let's get the colors.
    // We have to cast to Object.
    if (target.isKind(BeingKind::Object)) {
        Object& obj = static_cast<Object&>(target);
        obj.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);
    }
}
"""

if "resolveProjectionToken" not in content:
    content = content.replace(
        'namespace Earthcall {\n\n',
        'namespace Earthcall {\n\n#include "ConstructedBeing/Singular/Object/Object.hpp"\n' + helper
    )

content = content.replace(
    'if (!lawGetValue(t, lhsPath, lhs)) {',
    'if (!lawGetValue(t, lhsPath, lhs)) {'
) # Just ensuring it exists

content = content.replace(
    'PropertyValue rhs = rhsLiteral;',
    'resolveProjectionToken(t, lhs, lhs);\n                PropertyValue rhs = rhsLiteral;'
)

content = content.replace(
    'if (!rhsPath.empty() && !lawGetValue(t, rhsPath, rhs)) {',
    'if (!rhsPath.empty() && !lawGetValue(t, rhsPath, rhs)) {'
)

content = content.replace(
    'return [evaluate](const ECA::Event&, const Singular&) {\n                    return evaluate();\n                };',
    'return [evaluate](const ECA::Event&, const Singular&) {\n                    return evaluate();\n                };'
) # Wait, evaluating rhs

content = content.replace(
    '                        });\n                    }\n                    return false;\n                }\n                return [evaluate](lhs, rhs);',
    '                        });\n                    }\n                    return false;\n                }\n                resolveProjectionToken(t, rhs, rhs);\n                return [evaluate](lhs, rhs);'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
