import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/LawManager.cpp"
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# First, revert my previous patch
content = content.replace("""            auto tStart = glfwGetTime();
            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);
            auto tRete = glfwGetTime();

            // WHOM the law is about is the author's answer, not the network's.""", """            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);

            // WHOM the law is about is the author's answer, not the network's.""")

content = content.replace("""                applyAndMaybeDrive(*law, *subject, records);
            }
            auto tEnd = glfwGetTime();
            if (tEnd - tStart > 0.001) {
                 printf("Law %s took %.3f ms (rete: %.3f, apply: %.3f)\\n", law->getIdentifier().c_str(), (tEnd - tStart) * 1000.0, (tRete - tStart) * 1000.0, (tEnd - tRete) * 1000.0);
            }
            continue;
        }""", """                applyAndMaybeDrive(*law, *subject, records);
            }
            continue;
        }""")

# Now add per-tick accumulation
find_str = """    auto T1 = glfwGetTime();

    // ------------------------------------------------------------------
    // Event pass"""

replace_str = """    auto T1 = glfwGetTime();
    double totalReteTime = 0.0;
    double totalApplyTime = 0.0;
    double totalSweepTime = 0.0;
    int reteCount = 0;
    int sweepCount = 0;
    
    // ------------------------------------------------------------------
    // Event pass"""

find_str2 = """        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
            std::vector<std::size_t> termIds;
            termIds.reserve(termIt->second.size());
            for (const auto& info : termIt->second) termIds.push_back(info.nodeId);

            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);"""

replace_str2 = """        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
            reteCount++;
            auto tR1 = glfwGetTime();
            std::vector<std::size_t> termIds;
            termIds.reserve(termIt->second.size());
            for (const auto& info : termIt->second) termIds.push_back(info.nodeId);

            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);
            auto tR2 = glfwGetTime();
            totalReteTime += (tR2 - tR1);"""

find_str3 = """            for (Singular* subject : subjects) {
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

replace_str3 = """            for (Singular* subject : subjects) {
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

find_str4 = """        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        std::vector<Singular*> subjects = sweepSubjects(*law);"""

replace_str4 = """        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        sweepCount++;
        auto tS1 = glfwGetTime();
        std::vector<Singular*> subjects = sweepSubjects(*law);"""

find_str5 = """            // An OnBecomeTrue law that drives launches its process at the
            // edge and runs it to the end of its authored bounds.
            applyAndMaybeDrive(*law, *subject, records);
        }
    }

    auto T3 = glfwGetTime();"""

replace_str5 = """            // An OnBecomeTrue law that drives launches its process at the
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

if find_str in content and find_str2 in content and find_str3 in content and find_str4 in content and find_str5 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    content = content.replace(find_str3, replace_str3)
    content = content.replace(find_str4, replace_str4)
    content = content.replace(find_str5, replace_str5)
    with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
        f.write(content)
    print("Patched Law.cpp!")
else:
    print("Could not find blocks!")
