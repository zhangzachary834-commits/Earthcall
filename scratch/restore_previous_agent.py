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
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.hpp", "w") as f:
    f.write(hpp)

# Restore Law.cpp loops and retractFirst
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

# Restore retractFact pop_back
cpp = cpp.replace("""            auto it = std::find(_facts.begin(), _facts.end(), fact);
            if (it != _facts.end()) _facts.erase(it);""", """            auto it = std::find(_facts.begin(), _facts.end(), fact);
            if (it != _facts.end()) {
                *it = std::move(_facts.back());
                _facts.pop_back();
            }""")

cpp = cpp.replace("""            auto ait = std::find(alpha->memory.begin(), alpha->memory.end(), fact);
            if (ait != alpha->memory.end()) alpha->memory.erase(ait);""", """            auto ait = std::find(alpha->memory.begin(), alpha->memory.end(), fact);
            if (ait != alpha->memory.end()) {
                *ait = std::move(alpha->memory.back());
                alpha->memory.pop_back();
            }""")
            
cpp = cpp.replace("""            auto bit = std::find(beta->memory.begin(), beta->memory.end(), fact);
            if (bit != beta->memory.end()) beta->memory.erase(bit);""", """            auto bit = std::find(beta->memory.begin(), beta->memory.end(), fact);
            if (bit != beta->memory.end()) {
                *bit = std::move(beta->memory.back());
                beta->memory.pop_back();
            }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(cpp)
