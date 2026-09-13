import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    cpp = f.read()

cpp = cpp.replace("""            for (const auto& [subject, held] : law->conditionMemory()) {
                if (held) released.push_back(subject);
            }""", """            for (const Singular* subject : law->conditionMemory()) {
                released.push_back(subject);
            }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(cpp)
