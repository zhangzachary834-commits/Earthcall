import re

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

new_func = """static inline void resolveProjectionToken(Singular& target, const PropertyValue& in, PropertyValue& out) {
    if (in.index() != 15) {
        out = in;
        return;
    }
    const auto& dict = std::get<15>(in);
    if (!dict) {
        out = in;
        return;
    }
    auto itType = dict->elements.find("_type");
    if (itType == dict->elements.end() || itType->second.index() != 7 || std::get<7>(itType->second) != "projection") {
        out = in;
        return;
    }
    
    auto itTarget = dict->elements.find("target");
    if (itTarget == dict->elements.end() || itTarget->second.index() != 7) {
        out = in;
        return;
    }
    std::string targetName = std::get<7>(itTarget->second);
    
    target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);
}"""

content = re.sub(
    r'static void resolveProjectionToken.*?\}\n',
    new_func + '\n',
    content,
    flags=re.DOTALL
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
