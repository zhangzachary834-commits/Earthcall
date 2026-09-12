import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

target = """                std::vector<Singular*> subjects = sweepSubjects(*law);
                for (Singular* being : subjects) {
                    if (!being || Universe::instance().isUnmade(being)) continue;
                    if (!law->conditionsSatisfied(*being)) continue;
                    // A live drive session OWNS the process: one process,
                    // one clock, per law-and-subject. What a re-firing
                    // event means is the AUTHOR'S choice — Absorb (a block
                    // resting in constant collision cannot stack or reset
                    // the process) or Restart (the new trigger is a new t=0).
                    if (law->drives() &&
                        hasDriveSession(law->getIdentifier(), being->getIdentifier())) {
                        if (law->retrigger() == Law::Retrigger::Absorb) continue;
                        restartDriveSession(*law, being->getIdentifier());
                    }
                    applyAndMaybeDrive(*law, *being, records);
                }
                continue;"""

replacement = """                auto tA = glfwGetTime();
                std::vector<Singular*> subjects = sweepSubjects(*law);
                auto tB = glfwGetTime();
                int holds = 0;
                for (Singular* being : subjects) {
                    if (!being || Universe::instance().isUnmade(being)) continue;
                    if (!law->conditionsSatisfied(*being)) continue;
                    holds++;
                    if (law->drives() &&
                        hasDriveSession(law->getIdentifier(), being->getIdentifier())) {
                        if (law->retrigger() == Law::Retrigger::Absorb) continue;
                        restartDriveSession(*law, being->getIdentifier());
                    }
                    applyAndMaybeDrive(*law, *being, records);
                }
                auto tC = glfwGetTime();
                if ((tC - tA) * 1000.0 > 0.1) {
                    printf("[EVENT_SWEEP] Law '%s' took %.2f ms to evaluate %zu subjects, %d held\\n", law->getIdentifier().c_str(), (tC - tA)*1000.0, subjects.size(), holds);
                }
                continue;"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched event sweep!")
else:
    print("Could not find event sweep block")
