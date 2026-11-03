import re

with open("src/ConstructedBeing/Singular/Property/PropertyValue.hpp", "r") as f:
    code = f.read()

# Update isValueComparable
code = re.sub(
    r"std::holds_alternative<glm::mat4>\(v\);\n\}",
    "std::holds_alternative<glm::mat4>(v) ||\n           std::holds_alternative<std::shared_ptr<PropertyDict>>(v) ||\n           std::holds_alternative<std::shared_ptr<PropertyList>>(v);\n}",
    code
)

# Move propertyValueUnchanged below PropertyDict
old_unchanged = """inline bool propertyValueUnchanged(const PropertyValue& a, const PropertyValue& b) {
    if (a.index() != b.index()) return false;
    if (!isValueComparable(a)) return false;
    return a == b;
}"""

new_unchanged = """inline bool propertyValueUnchanged(const PropertyValue& a, const PropertyValue& b) {
    if (a.index() != b.index()) return false;
    
    if (std::holds_alternative<std::shared_ptr<PropertyDict>>(a)) {
        auto pA = std::get<std::shared_ptr<PropertyDict>>(a);
        auto pB = std::get<std::shared_ptr<PropertyDict>>(b);
        if (pA == pB) return true;
        if (!pA || !pB) return false;
        if (pA->elements.size() != pB->elements.size()) return false;
        for (const auto& [k, vA] : pA->elements) {
            auto it = pB->elements.find(k);
            if (it == pB->elements.end()) return false;
            if (!propertyValueUnchanged(vA, it->second)) return false;
        }
        return true;
    }
    
    if (std::holds_alternative<std::shared_ptr<PropertyList>>(a)) {
        auto pA = std::get<std::shared_ptr<PropertyList>>(a);
        auto pB = std::get<std::shared_ptr<PropertyList>>(b);
        if (pA == pB) return true;
        if (!pA || !pB) return false;
        if (pA->elements.size() != pB->elements.size()) return false;
        for (size_t i = 0; i < pA->elements.size(); ++i) {
            if (!propertyValueUnchanged(pA->elements[i], pB->elements[i])) return false;
        }
        return true;
    }
    
    if (!isValueComparable(a)) return false;
    return a == b;
}"""

code = code.replace(old_unchanged, "")
code = re.sub(
    r"struct PropertyDict \{\n    std::map<std::string, PropertyValue> elements;\n\};",
    "struct PropertyDict {\n    std::map<std::string, PropertyValue> elements;\n};\n\n" + new_unchanged,
    code
)

# Also update propertyValuesEquivalent?
# Actually, propertyValuesEquivalent uses a == b. 
# We should update it to use propertyValueUnchanged for the fallback!
code = re.sub(
    r"return a.index\(\) == b.index\(\) && a == b;",
    "return propertyValueUnchanged(a, b);",
    code
)

with open("src/ConstructedBeing/Singular/Property/PropertyValue.hpp", "w") as f:
    f.write(code)
