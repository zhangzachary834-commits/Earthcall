import re

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

new_includes = """bool selectorIncludes(const OntoMath::Piecewise& selector, float u, float v,
                      const Object& subject) {
    const std::map<std::string, PropertyValue> vars{
        {"u", PropertyValue(static_cast<double>(u))},
        {"v", PropertyValue(static_cast<double>(v))},
    };
    auto val = selector.evaluate(vars, &subject);
    if (!val.has_value()) {
        static bool printed = false;
        if (!printed) {
            std::cerr << "selectorIncludes evaluate returned nullopt! u=" << u << " v=" << v << std::endl;
            printed = true;
        }
    }
    return val.has_value();
}"""

content = re.sub(
    r'bool selectorIncludes\(const OntoMath::Piecewise& selector, float u, float v,\n\s*const Object& subject\) \{.*?\n\}',
    new_includes,
    content,
    flags=re.DOTALL
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
