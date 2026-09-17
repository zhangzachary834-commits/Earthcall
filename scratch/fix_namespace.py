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
    
    if (target.isKind(BeingKind::Object)) {
        Object& obj = static_cast<Object&>(target);
        obj.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);
    }
}
"""

content = content.replace(
    '#include <utility>\n',
    '#include <utility>\n' + helper
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
