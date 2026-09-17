import re

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "r") as f:
    content = f.read()

new_getvalue = """PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out, std::size_t startIndex) const {
    ResolvedSlot slot = resolve(root, startIndex);
    
    auto resolveToken = [&](PropertyValue& val, Singular* target) {
        if (val.index() == 15) {
            const auto& dict = std::get<15>(val);
            if (dict) {
                auto itType = dict->elements.find("_type");
                if (itType != dict->elements.end() && itType->second.index() == 7 && std::get<7>(itType->second) == "projection") {
                    auto itTarget = dict->elements.find("target");
                    if (itTarget != dict->elements.end() && itTarget->second.index() == 7) {
                        target->readAuthoredPropertyProjectionColors(
                            Earthcall::StringInterner::intern(std::get<7>(itTarget->second)), val);
                    }
                }
            }
        }
    };

    if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            if (root.getDynamicProperty(segments[startIndex], out)) {
                resolveToken(out, &root);
                return PathResult::Ok;
            }
        }
        return PathResult::NoSuchProperty;
    }

    PropertyValue v;
    if (slot.prop) v = slot.prop->value();
    else if (slot.dynamicSlot) v = *slot.dynamicSlot;
    
    resolveToken(v, slot.owner ? slot.owner : &root);

    if (slot.trailingComponent.empty()) {
        out = std::move(v);
        return !std::holds_alternative<std::monostate>(out) ? PathResult::Ok : PathResult::NoSuchProperty;
    }

    glm::vec3* vec = std::get_if<glm::vec3>(&v);
    if (!vec) return PathResult::BadComponent;
    out = PropertyValue(*componentOf(*vec, slot.trailingComponent));
    return PathResult::Ok;
}"""

content = re.sub(
    r'PropertyPath::PathResult PropertyPath::getValue.*?return PathResult::Ok;\n\}',
    new_getvalue,
    content,
    flags=re.DOTALL
)

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
    f.write(content)
