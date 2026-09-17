import re
with open("src/Singularity/OntoMath/ScalarForm.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '        const auto g = whereLEZero->evaluate(vars, subject);\n        double val = 0.0;\n        if (!g || !propertyValueToNumber(*g, val) || val > 0.0) return false;',
    '        const auto g = whereLEZero->evaluate(vars, subject);\n        double val = 0.0;\n        if (!g || !propertyValueToNumber(*g, val) || val > 0.0) {\n            std::cout << "whereLEZero REJECTED: has_g=" << g.has_value() << " val=" << val << " vars[v]=" << (vars.count("v") ? std::get<double>(vars.at("v")) : -1) << "\\n";\n            return false;\n        }'
)

with open("src/Singularity/OntoMath/ScalarForm.cpp", "w") as f:
    f.write(content)
