import sys

# Restore Law.hpp conditionMemory
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.hpp", "r") as f:
    hpp = f.read()
hpp = hpp.replace("std::unordered_map<const Singular*, bool> _conditionMemory;", "std::unordered_set<const Singular*> _conditionMemory;")
hpp = hpp.replace("""        auto it = _conditionMemory.find(subject);
        return it != _conditionMemory.end() && it->second;""", """        return _conditionMemory.count(subject) > 0;""")
hpp = hpp.replace("""    void rememberConditionState(const Singular* subject, bool state) {
        _conditionMemory[subject] = state;
    }""", """    void rememberConditionState(const Singular* subject, bool state) {
        if (state) _conditionMemory.insert(subject);
        else _conditionMemory.erase(subject);
    }""")
hpp = hpp.replace("const std::unordered_map<const Singular*, bool>& conditionMemory() const", "const std::unordered_set<const Singular*>& conditionMemory() const")
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.hpp", "w") as f:
    f.write(hpp)

# Restore Law.cpp loops
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    cpp = f.read()
cpp = cpp.replace("""            for (const auto& [subject, held] : law->conditionMemory()) {
                released.push_back(subject);
            }""", """            for (const Singular* subject : law->conditionMemory()) {
                released.push_back(subject);
            }""")
cpp = cpp.replace("""            for (const auto& [subject, held] : law->conditionMemory()) {
                if (held && matching.count(subject) == 0) released.push_back(subject);
            }""", """            for (const Singular* subject : law->conditionMemory()) {
                if (matching.count(subject) == 0) released.push_back(subject);
            }""")
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(cpp)
