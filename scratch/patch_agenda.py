import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("std::vector<ReteActivation> agenda = _rete.drainAgenda();", """std::vector<ReteActivation> agenda = _rete.drainAgenda();
        if (agenda.size() > 0) {
            static int print_count = 0;
            if (print_count++ < 20) {
                printf("--- AGENDA SIZE: %zu ---\\n", agenda.size());
            }
        }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
