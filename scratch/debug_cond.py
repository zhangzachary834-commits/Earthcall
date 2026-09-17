import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '                    case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); break;',
    '''                    case Op::Eq: {
                        res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs));
                        if (lhsPath.components.size() == 1 && lhsPath.components[0] == "regionB") {
                            std::cout << "Condition Evaluated regionB == rhs. Res: " << res << "\\n";
                        }
                        break;
                    }'''
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
