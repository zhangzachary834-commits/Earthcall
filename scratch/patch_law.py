import re

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

new_func = """
#include "MathBinding.hpp"
void resolveSemanticTokenSlowPath(Singular* root, PropertyValue& out) {
    if (out.index() == 15) {
        const auto& dict = std::get<15>(out);
        if (dict) {
            auto itType = dict->elements.find("_type");
            if (itType != dict->elements.end() && itType->second.index() == 7 && std::get<7>(itType->second) == "projection") {
                auto itTarget = dict->elements.find("target");
                if (itTarget != dict->elements.end() && itTarget->second.index() == 7) {
                    root->readAuthoredPropertyProjectionColors(
                        Earthcall::StringInterner::intern(std::get<7>(itTarget->second)), out);
                }
            }
        }
    }
}
"""

# Append to the bottom of Law.cpp
content += new_func

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
