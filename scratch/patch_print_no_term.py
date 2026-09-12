import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""        if (law->activation() == Law::Activation::WhileTrue) {
            static int no_term = 0, yes_term = 0;
            if (hasTerminals) yes_term++; else no_term++;
            static int pp = 0;
            if (++pp == 500) {
                printf("--- WHILE TRUE TERMINALS: Yes %d, No %d ---\\n", yes_term, no_term);
                yes_term = 0; no_term = 0; pp = 0;
            }
        }""", """        if (law->activation() == Law::Activation::WhileTrue) {
            if (!hasTerminals) {
                static std::unordered_set<std::string> printed;
                if (printed.insert(law->getIdentifier()).second) {
                    printf("--- NO TERMINALS FOR LAW: %s ---\\n", law->getIdentifier().c_str());
                }
            }
        }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
