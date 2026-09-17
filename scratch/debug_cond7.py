import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '                Singular& t = const_cast<Singular&>(target);\n                std::cout << "EVALUATING Condition. lhsPath: " << lhsPath.toString() << "\\n";',
    '''                Singular& t = const_cast<Singular&>(target);
                if (lhsPath.segments.size() > 0 && lhsPath.segments[0] == "regionB") {
                    std::cout << "EVALUATING Condition for regionB!\\n";
                    if (!lawGetValue(t, lhsPath, lhs)) {
                        std::cout << "lawGetValue returned FALSE!\\n";
                    } else {
                        std::cout << "lawGetValue returned TRUE! Is List: " << std::holds_alternative<std::shared_ptr<PropertyList>>(lhs) << "\\n";
                        if (std::holds_alternative<std::shared_ptr<PropertyList>>(lhs)) {
                            auto L = std::get<std::shared_ptr<PropertyList>>(lhs);
                            std::cout << "List size: " << L->elements.size() << "\\n";
                            if (L->elements.size() > 0) {
                                auto vec = std::get_if<glm::vec3>(&L->elements[0]);
                                if (vec) std::cout << "List[0]: " << vec->x << "," << vec->y << "," << vec->z << "\\n";
                            }
                            if (L->elements.size() > 1) {
                                auto vec = std::get_if<glm::vec3>(&L->elements[1]);
                                if (vec) std::cout << "List[1]: " << vec->x << "," << vec->y << "," << vec->z << "\\n";
                            }
                        }
                    }
                    std::cout << "propertyValueUnchanged: " << propertyValueUnchanged(lhs, rhs) << "\\n";
                }
'''
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
