import sys

# Restore Law.cpp and Law.hpp to original state
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    cpp = f.read()

# Replace the hasTerminals check
cpp = cpp.replace("""        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {""", """        if (hasTerminals && (law->activation() == Law::Activation::WhileTrue || law->activation() == Law::Activation::OnBecomeTrue)) {""")

# Replace the first loop
cpp = cpp.replace("""            std::unordered_set<const Singular*> matching;
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

# Replace the second loop
cpp = cpp.replace("""            for (Singular* subject : subjects) {
                if (law->drives() &&
                    hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {""", """            const std::vector<Singular*>& fireSubjects = 
                (law->activation() == Law::Activation::OnBecomeTrue) ? newlyTrue : subjects;

            for (Singular* subject : fireSubjects) {
                if (law->drives() &&
                    hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(cpp)
