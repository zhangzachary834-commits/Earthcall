import re

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'r') as f:
    cpp = f.read()

# Fix the fallback sweep loop
bad_sweep = """        for (Singular* subject : subjects) {
            if (!subject || Universe::instance().isUnmade(subject)) continue;
            const bool holds = law->conditionsSatisfied(*subject);
            const std::string subjectId = subject->getIdentifier();
            const bool wasHolding = law->lastConditionState(subjectId);
            law->rememberConditionState(subjectId, holds);

            // The false->true edge is t=0 for this subject's change-over-time
            // clock (time.sinceApplied); release re-arms it.
            if (holds && !wasHolding && Universe::instance().hasClock()) {
                law->rememberOnset(subjectId, Universe::instance().now());
            } else if (!holds && wasHolding) {
                law->forgetOnset(subjectId);
            }

            const bool fire = law->activation() == Law::Activation::WhileTrue
                                  ? holds
                                  : (holds && !wasHolding);   // the false->true edge
            if (!fire) continue;
            if (law->drives() &&
                hasDriveSession(law->getIdentifier(), subjectId)) {
                if (law->retrigger() == Law::Retrigger::Absorb) {
                    continue;   // a re-edge while the launched process still
                                // runs is absorbed — the session owns it
                }
                restartDriveSession(*law, subjectId);   // a re-edge = new t=0
            }"""

good_sweep = """        for (Singular* subject : subjects) {
            if (!subject || Universe::instance().isUnmade(subject)) continue;
            const bool holds = law->conditionsSatisfied(*subject);
            const bool wasHolding = law->lastConditionState(subject);
            law->rememberConditionState(subject, holds);

            // The false->true edge is t=0 for this subject's change-over-time
            // clock (time.sinceApplied); release re-arms it.
            if (holds && !wasHolding && Universe::instance().hasClock()) {
                law->rememberOnset(subject, Universe::instance().now());
            } else if (!holds && wasHolding) {
                law->forgetOnset(subject);
            }

            const bool fire = law->activation() == Law::Activation::WhileTrue
                                  ? holds
                                  : (holds && !wasHolding);   // the false->true edge
            if (!fire) continue;
            if (law->drives() &&
                hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {
                if (law->retrigger() == Law::Retrigger::Absorb) {
                    continue;   // a re-edge while the launched process still
                                // runs is absorbed — the session owns it
                }
                restartDriveSession(*law, subject->getIdentifier());   // a re-edge = new t=0
            }"""

cpp = cpp.replace(bad_sweep, good_sweep)

# Fix Universe::instance().findBeing
# I need to implement a local findBeing using Universe::instance().beings()
find_being_macro = """        Singular* subject = nullptr;
        for (Singular* being : Universe::instance().beings()) {
            if (being && being->getIdentifier() == subjectId) { subject = being; break; }
        }
        if (subject) {"""

bad_find_1 = """        if (Singular* subject = Universe::instance().findBeing(subjectId)) {
            law.rememberOnset(subject, now);
        }"""
good_find_1 = find_being_macro + """
            law.rememberOnset(subject, now);
        }"""

bad_find_2 = """    if (Singular* subject = Universe::instance().findBeing(subjectId)) {
        law.rememberOnset(subject, onset);
    }"""
good_find_2 = find_being_macro + """
        law.rememberOnset(subject, onset);
    }"""

cpp = cpp.replace(bad_find_1, good_find_1)
cpp = cpp.replace(bad_find_2, good_find_2)

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'w') as f:
    f.write(cpp)
