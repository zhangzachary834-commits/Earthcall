import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""    bool compileResult = true;
    if (law->activation() != Law::Activation::OnBecomeTrue) {
        compileResult = _rete.compileLaw(law->getIdentifier(),
                                         law->conditions(),
                                         law->activation() == Law::Activation::OnEvent);
    }""", """    bool compileResult = _rete.compileLaw(law->getIdentifier(),
                                     law->conditions(),
                                     law->activation() == Law::Activation::OnEvent);""")

content = content.replace("""        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {""", """        if (hasTerminals && (law->activation() == Law::Activation::WhileTrue || law->activation() == Law::Activation::OnBecomeTrue)) {""")

content = content.replace("""            for (Singular* subject : subjects) {
                const bool wasHolding = law->lastConditionState(subject);
                law->rememberConditionState(subject, true);
                if (!wasHolding && Universe::instance().hasClock()) {
                    law->rememberOnset(subject, Universe::instance().now());
                }
                
                if (law->drives() &&
                    hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {
                    if (law->retrigger() == Law::Retrigger::Absorb) {
                        continue;
                    }
                    restartDriveSession(*law, subject->getIdentifier());
                }
                applyAndMaybeDrive(*law, *subject, records);
            }
            continue;
        }""", """            for (Singular* subject : subjects) {
                const bool wasHolding = law->lastConditionState(subject);
                law->rememberConditionState(subject, true);
                if (!wasHolding && Universe::instance().hasClock()) {
                    law->rememberOnset(subject, Universe::instance().now());
                }
                
                bool fire = false;
                if (law->activation() == Law::Activation::WhileTrue) {
                    fire = true;
                } else if (law->activation() == Law::Activation::OnBecomeTrue) {
                    fire = !wasHolding;
                }
                if (!fire) continue;
                
                if (law->drives() &&
                    hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {
                    if (law->retrigger() == Law::Retrigger::Absorb) {
                        continue;
                    }
                    restartDriveSession(*law, subject->getIdentifier());
                }
                applyAndMaybeDrive(*law, *subject, records);
            }
            continue;
        }""")


with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
