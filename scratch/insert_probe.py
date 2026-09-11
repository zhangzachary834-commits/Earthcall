import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

target = """        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        std::vector<Singular*> subjects = sweepSubjects(*law);

        for (Singular* subject : subjects) {"""

replacement = """        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        auto tA = glfwGetTime();
        std::vector<Singular*> subjects = sweepSubjects(*law);
        auto tB = glfwGetTime();

        int holdsCount = 0;
        for (Singular* subject : subjects) {
            if (!subject || Universe::instance().isUnmade(subject)) continue;
            
            auto tC = glfwGetTime();
            const bool holds = law->conditionsSatisfied(*subject);
            auto tD = glfwGetTime();
            if (holds) holdsCount++;
            
            if ((tD - tC) * 1000.0 > 0.5) {
                printf("[EVAL] Law '%s' conditionsSatisfied on '%s' took %.2f ms\\n", law->getIdentifier().c_str(), subject->getIdentifier().c_str(), (tD - tC) * 1000.0);
            }"""

if target in content:
    content = content.replace(target, replacement)
    
    # Also add a log after the loop
    target2 = """            // An OnBecomeTrue law that drives launches its process at the
            // edge and runs it to the end of its authored bounds.
            applyAndMaybeDrive(*law, *subject, records);
        }
    }

    auto T3 = glfwGetTime();"""
    
    replacement2 = """            // An OnBecomeTrue law that drives launches its process at the
            // edge and runs it to the end of its authored bounds.
            applyAndMaybeDrive(*law, *subject, records);
        }
        auto tE = glfwGetTime();
        if ((tE - tA) * 1000.0 > 2.0) {
            printf("[SWEEP] Law '%s' (hasTerm=%d, act=%d) took %.2f ms (sweep=%.2f, evals=%.2f). Evaluated %zu subjects, %d held.\\n", 
                law->getIdentifier().c_str(), hasTerminals, (int)law->activation(), (tE - tA)*1000.0, (tB - tA)*1000.0, (tE - tB)*1000.0, subjects.size(), holdsCount);
        }
    }

    auto T3 = glfwGetTime();"""
    
    content = content.replace(target2, replacement2)
    
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched!")
else:
    print("Could not find target block")

