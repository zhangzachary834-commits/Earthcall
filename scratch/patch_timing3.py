import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_1 = """    auto T1 = glfwGetTime();

    // Introduce any being the network has not met."""

replace_1 = """    auto T1 = glfwGetTime();
    double totalReteTime = 0.0;
    double totalApplyTime = 0.0;
    double totalSweepTime = 0.0;
    int reteCount = 0;
    int sweepCount = 0;

    // Introduce any being the network has not met."""

find_2 = """        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
            std::vector<std::size_t> termIds;
            termIds.reserve(termIt->second.size());
            for (const auto& info : termIt->second) termIds.push_back(info.nodeId);

            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);"""

replace_2 = """        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
            reteCount++;
            auto tR1 = glfwGetTime();
            std::vector<std::size_t> termIds;
            termIds.reserve(termIt->second.size());
            for (const auto& info : termIt->second) termIds.push_back(info.nodeId);

            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);
            auto tR2 = glfwGetTime();
            totalReteTime += (tR2 - tR1);"""

find_3 = """            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                const std::string subjectId = subject->getIdentifier();
                if (law->drives() &&
                    hasDriveSession(lawId, subjectId)) {
                    if (law->retrigger() == Law::Retrigger::Absorb) continue;
                    restartDriveSession(*law, subjectId);
                }
                applyAndMaybeDrive(*law, *subject, records);
            }
            continue;
        }"""

replace_3 = """            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                const std::string subjectId = subject->getIdentifier();
                if (law->drives() &&
                    hasDriveSession(lawId, subjectId)) {
                    if (law->retrigger() == Law::Retrigger::Absorb) continue;
                    restartDriveSession(*law, subjectId);
                }
                applyAndMaybeDrive(*law, *subject, records);
            }
            totalApplyTime += (glfwGetTime() - tR2);
            continue;
        }"""

find_4 = """        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        std::vector<Singular*> subjects = sweepSubjects(*law);"""

replace_4 = """        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        sweepCount++;
        auto tS1 = glfwGetTime();
        std::vector<Singular*> subjects = sweepSubjects(*law);"""

find_5 = """            // An OnBecomeTrue law that drives launches its process at the
            // edge and runs it to the end of its authored bounds.
            applyAndMaybeDrive(*law, *subject, records);
        }
    }

    auto T3 = glfwGetTime();"""

replace_5 = """            // An OnBecomeTrue law that drives launches its process at the
            // edge and runs it to the end of its authored bounds.
            applyAndMaybeDrive(*law, *subject, records);
        }
        totalSweepTime += (glfwGetTime() - tS1);
    }
    static int tickCount = 0;
    if (tickCount++ % 24 == 0) {
        printf("TICK: reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep)\\n",
            reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0);
    }

    auto T3 = glfwGetTime();"""

content = content.replace(find_1, replace_1)
content = content.replace(find_2, replace_2)
content = content.replace(find_3, replace_3)
content = content.replace(find_4, replace_4)
content = content.replace(find_5, replace_5)

with open(file_path, "w") as f:
    f.write(content)
print("Patched timing 3!")
