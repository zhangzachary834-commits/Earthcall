import re
with open("src/Singularity/OntoMath/ScalarForm.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '        if (!piece.mathNode) return std::nullopt;\n        return piece.mathNode->evaluate(vars, subject);',
    '        if (!piece.mathNode) { std::cout << "piece.mathNode is NULL!\\n"; return std::nullopt; }\n        auto ret = piece.mathNode->evaluate(vars, subject);\n        std::cout << "piece.mathNode->evaluate returned: " << ret.has_value() << "\\n";\n        return ret;'
)

with open("src/Singularity/OntoMath/ScalarForm.cpp", "w") as f:
    f.write(content)
