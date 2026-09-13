import sys

# Restore everything to HEAD first
# git restore src/ZonesOfEarth/AuthorsOfLaw/Law.cpp src/ZonesOfEarth/AuthorsOfLaw/Law.hpp src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp

# 1. swap_and_pop for _facts in PropheticRete.cpp
with open("src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp", "r") as f:
    rete_cpp = f.read()

rete_cpp = rete_cpp.replace("""        if (it != _facts.end()) {
            _facts.erase(it);""", """        if (it != _facts.end()) {
            if (it != _facts.end() - 1) *it = std::move(_facts.back());
            _facts.pop_back();""")

with open("src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp", "w") as f:
    f.write(rete_cpp)

# 2. OnBecomeTrue fast path + clean Zach's debug prints in Law.cpp
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    law_cpp = f.read()

# Remove Zach's committed debug prints
import re
law_cpp = re.sub(r"        if \(law->activation\(\) == Law::Activation::WhileTrue\) \{\s*if \(\!hasTerminals\) \{\s*static std::unordered_set<std::string> printed;.*?\}\s*\}\s*\}", "", law_cpp, flags=re.DOTALL)
law_cpp = re.sub(r"        if \(law->activation\(\) == Law::Activation::WhileTrue\) \{\s*static int no_term = 0, yes_term = 0;.*?\}\s*\}", "", law_cpp, flags=re.DOTALL)

# Add OnBecomeTrue to fast path check
law_cpp = law_cpp.replace("""        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {""", """        if (hasTerminals && (law->activation() == Law::Activation::WhileTrue || law->activation() == Law::Activation::OnBecomeTrue)) {""")

# Change first loop to remember newlyTrue
law_cpp = law_cpp.replace("""            std::unordered_set<const Singular*> matching;
            matching.reserve(subjects.size());
            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                matching.insert(subject);
                const bool wasHolding = law->lastConditionState(subject);
                law->rememberConditionState(subject, true);
                if (!wasHolding && Universe::instance().hasClock()) {
                    law->rememberOnset(subject, Universe::instance().now());
                }
            }""", """            std::unordered_set<const Singular*> matching;
            std::vector<Singular*> newlyTrue;
            matching.reserve(subjects.size());
            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                matching.insert(subject);
                const bool wasHolding = law->lastConditionState(subject);
                law->rememberConditionState(subject, true);
                if (!wasHolding) {
                    newlyTrue.push_back(subject);
                    if (Universe::instance().hasClock()) {
                        law->rememberOnset(subject, Universe::instance().now());
                    }
                }
            }""")

# Change second loop to iterate over newlyTrue for OBT
law_cpp = law_cpp.replace("""            for (Singular* subject : subjects) {
                if (law->drives() &&
                    hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {""", """            const std::vector<Singular*>& fireSubjects = 
                (law->activation() == Law::Activation::OnBecomeTrue) ? newlyTrue : subjects;

            for (Singular* subject : fireSubjects) {
                if (law->drives() &&
                    hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(law_cpp)
