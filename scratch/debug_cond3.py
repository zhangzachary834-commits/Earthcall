import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '                Singular& t = const_cast<Singular&>(target);',
    '                Singular& t = const_cast<Singular&>(target);\n                if (lhsPath.segments.size() == 1 && lhsPath.segments[0] == "regionB") { std::cout << "EVALUATING regionB!\\n"; }'
)

content = content.replace(
    '                    if (ECA::LawAuditLogger::instance().wouldLog("CONDITION")) {',
    '                    if (lhsPath.segments.size() == 1 && lhsPath.segments[0] == "regionB") { std::cout << "EVAL regionB FAIL - Prop Not Found!\\n"; }\n                    if (ECA::LawAuditLogger::instance().wouldLog("CONDITION")) {'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
