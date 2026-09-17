import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '                Singular& t = const_cast<Singular&>(target);',
    '''                Singular& t = const_cast<Singular&>(target);
                if (lhsPath.segments.size() > 0 && lhsPath.segments[0] == "regionB") {
                    PropertyValue test_lhs;
                    if (!lawGetValue(t, lhsPath, test_lhs)) {
                        std::cout << "DEBUG: lawGetValue returned FALSE!\\n";
                    } else {
                        std::cout << "DEBUG: lawGetValue returned TRUE! Is List: " << std::holds_alternative<std::shared_ptr<PropertyList>>(test_lhs) << "\\n";
                        if (std::holds_alternative<std::shared_ptr<PropertyList>>(test_lhs)) {
                            auto L = std::get<std::shared_ptr<PropertyList>>(test_lhs);
                            std::cout << "DEBUG: List size: " << L->elements.size() << "\\n";
                        }
                    }
                }
'''
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
