import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {""", """        if (law->activation() == Law::Activation::WhileTrue) {
            if (!hasTerminals) {
                static std::unordered_set<std::string> printed;
                if (printed.insert(law->getIdentifier()).second) {
                    fprintf(stderr, "--- NO TERMINALS FOR LAW: %s ---\\n", law->getIdentifier().c_str());
                    fflush(stderr);
                }
            }
        }
        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
