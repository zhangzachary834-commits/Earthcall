import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""        if (law->activation() == Law::Activation::WhileTrue && !hasTerminals) {""", """        if (law->activation() == Law::Activation::OnBecomeTrue) {
            static std::unordered_set<std::string> printed;
            if (printed.insert(law->getIdentifier()).second) {
                fprintf(stderr, ">>> OBT LAW: %s, hasTerminals=%d\\n", law->getIdentifier().c_str(), hasTerminals);
            }
        }
        if (law->activation() == Law::Activation::WhileTrue && !hasTerminals) {""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
