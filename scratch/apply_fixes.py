import re

# 1. Law.hpp
with open('src/ZonesOfEarth/AuthorsOfLaw/Law.hpp', 'r') as f:
    hpp = f.read()

hpp = hpp.replace('bool lastConditionState(const std::string& subjectId) const {', 'bool lastConditionState(const Singular* subject) const {')
hpp = hpp.replace('auto it = _conditionMemory.find(subjectId);', 'auto it = _conditionMemory.find(subject);')
hpp = hpp.replace('void rememberConditionState(const std::string& subjectId, bool state) {', 'void rememberConditionState(const Singular* subject, bool state) {')
hpp = hpp.replace('_conditionMemory[subjectId] = state;', '_conditionMemory[subject] = state;')
hpp = hpp.replace('const std::unordered_map<std::string, bool>& conditionMemory() const {', 'const std::unordered_map<const Singular*, bool>& conditionMemory() const {')

hpp = hpp.replace('bool hasOnset(const std::string& subjectId) const {', 'bool hasOnset(const Singular* subject) const {')
hpp = hpp.replace('return _onsetMemory.count(subjectId) != 0;', 'return _onsetMemory.count(subject) != 0;')
hpp = hpp.replace('double onsetFor(const std::string& subjectId) const {', 'double onsetFor(const Singular* subject) const {')
hpp = hpp.replace('auto it = _onsetMemory.find(subjectId);', 'auto it = _onsetMemory.find(subject);')
hpp = hpp.replace('void rememberOnset(const std::string& subjectId, double worldTime) {', 'void rememberOnset(const Singular* subject, double worldTime) {')
hpp = hpp.replace('_onsetMemory[subjectId] = worldTime;', '_onsetMemory[subject] = worldTime;')
hpp = hpp.replace('void forgetOnset(const std::string& subjectId) { _onsetMemory.erase(subjectId); }', 'void forgetOnset(const Singular* subject) { _onsetMemory.erase(subject); }\n    void forgetSubject(const Singular* subject) {\n        _conditionMemory.erase(subject);\n        _onsetMemory.erase(subject);\n    }')

hpp = hpp.replace('std::unordered_map<std::string, bool> _conditionMemory;', 'std::unordered_map<const Singular*, bool> _conditionMemory;')
hpp = hpp.replace('std::unordered_map<std::string, double> _onsetMemory;', 'std::unordered_map<const Singular*, double> _onsetMemory;')

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.hpp', 'w') as f:
    f.write(hpp)

# 2. Law.cpp
with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'r') as f:
    cpp = f.read()

# applyTo removes timestamp
cpp = cpp.replace('event.timestamp = std::time(nullptr);', '')

cpp = cpp.replace('''        if (Universe::instance().hasClock()) {
            const std::string subjectId = target.getIdentifier();
            onsetScope.emplace(hasOnset(subjectId) ? onsetFor(subjectId)
                                                   : Universe::instance().now());
        }''', '''        if (Universe::instance().hasClock()) {
            onsetScope.emplace(hasOnset(&target) ? onsetFor(&target)
                                                 : Universe::instance().now());
        }''')

cpp = cpp.replace('''        for (const std::string& subjectId : _rete.retractFactsAbout(being)) {
            _seededSubjects.erase(subjectId);
        }
        // …and every RELATION''', '''        for (const std::string& subjectId : _rete.retractFactsAbout(being)) {
            _seededSubjects.erase(subjectId);
        }
        for (auto& law : _laws) {
            law->forgetSubject(being);
        }
        // …and every RELATION''')

cpp = cpp.replace('''            std::unordered_set<std::string> matching;
            matching.reserve(subjects.size());
            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                const std::string subjectId = subject->getIdentifier();
                matching.insert(subjectId);
                const bool wasHolding = law->lastConditionState(subjectId);
                law->rememberConditionState(subjectId, true);
                if (!wasHolding && Universe::instance().hasClock()) {
                    law->rememberOnset(subjectId, Universe::instance().now());
                }
            }''', '''            std::unordered_set<const Singular*> matching;
            matching.reserve(subjects.size());
            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                matching.insert(subject);
                const bool wasHolding = law->lastConditionState(subject);
                law->rememberConditionState(subject, true);
                if (!wasHolding && Universe::instance().hasClock()) {
                    law->rememberOnset(subject, Universe::instance().now());
                }
            }''')

cpp = cpp.replace('''            std::vector<std::string> released;
            for (const auto& [subjectId, held] : law->conditionMemory()) {
                if (held && matching.count(subjectId) == 0) released.push_back(subjectId);
            }
            for (const auto& subjectId : released) {
                law->rememberConditionState(subjectId, false);
                law->forgetOnset(subjectId);
            }''', '''            std::vector<const Singular*> released;
            for (const auto& [subject, held] : law->conditionMemory()) {
                if (held && matching.count(subject) == 0) released.push_back(subject);
            }
            for (const auto* subject : released) {
                law->rememberConditionState(subject, false);
                law->forgetOnset(subject);
            }''')

cpp = cpp.replace('''            std::vector<std::string> released;
            for (const auto& [subjectId, held] : law->conditionMemory()) {
                if (held) released.push_back(subjectId);
            }
            for (const std::string& subjectId : released) {
                law->rememberConditionState(subjectId, false);
                law->forgetOnset(subjectId);
            }''', '''            std::vector<const Singular*> released;
            for (const auto& [subject, held] : law->conditionMemory()) {
                if (held) released.push_back(subject);
            }
            for (const Singular* subject : released) {
                law->rememberConditionState(subject, false);
                law->forgetOnset(subject);
            }''')

cpp = cpp.replace('''        for (Singular* subject : subjects) {
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
            }
            applyAndMaybeDrive(*law, *subject, records);
        }''', '''        for (Singular* subject : subjects) {
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
            }
            applyAndMaybeDrive(*law, *subject, records);
        }''')


# Fix drive session onsets
cpp = cpp.replace('''        law.rememberOnset(subjectId, now);''', '''        if (Singular* subject = Universe::instance().findBeing(subjectId)) {
            law.rememberOnset(subject, now);
        }''')
cpp = cpp.replace('''    law.rememberOnset(subjectId, onset);''', '''    if (Singular* subject = Universe::instance().findBeing(subjectId)) {
        law.rememberOnset(subject, onset);
    }''')
cpp = cpp.replace('if (law) law->forgetOnset(it->subjectId);', 'if (law && subject) law->forgetOnset(subject);')
cpp = cpp.replace('law->forgetOnset(it->subjectId);', 'law->forgetOnset(subject);')
cpp = cpp.replace('law->rememberOnset(it->subjectId, it->onset);', 'law->rememberOnset(subject, it->onset);')

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'w') as f:
    f.write(cpp)
