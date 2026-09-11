import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        std::vector<Singular*> subjects = sweepSubjects(*law);""", """        auto t_sp2_0 = glfwGetTime();
        std::vector<Singular*> subjects = sweepSubjects(*law);
        auto t_sp2_1 = glfwGetTime();""")
        
content = content.replace("""        for (Singular* subject : subjects) {
            const bool holds = law->conditionsSatisfied(*subject);
            const bool wasHolding = law->lastConditionState(subject);
            law->rememberConditionState(subject, holds);""", """        auto t_sp2_2 = glfwGetTime();
        for (Singular* subject : subjects) {
            auto t_inner_0 = glfwGetTime();
            const bool holds = law->conditionsSatisfied(*subject);
            auto t_inner_1 = glfwGetTime();
            const bool wasHolding = law->lastConditionState(subject);
            law->rememberConditionState(subject, holds);
            auto t_inner_2 = glfwGetTime();
            static double IS1=0, IS2=0;
            IS1 += (t_inner_1 - t_inner_0);
            IS2 += (t_inner_2 - t_inner_1);""")
            
content = content.replace("""            applyAndMaybeDrive(*law, *subject, records);
        }
    }""", """            applyAndMaybeDrive(*law, *subject, records);
        }
        auto t_sp2_3 = glfwGetTime();
        static double S1=0, S2=0, S3=0;
        S1 += (t_sp2_1 - t_sp2_0);
        S2 += (t_sp2_2 - t_sp2_1);
        S3 += (t_sp2_3 - t_sp2_2);
        static int pp = 0;
        if (++pp == 3000) {
            printf("--- PATH 2 PERF ---\\n");
            printf("sweepSubjects: %.3f ms\\n", S1 * 1000.0);
            printf("between loop: %.3f ms\\n", S2 * 1000.0);
            printf("loop body: %.3f ms\\n", S3 * 1000.0);
            printf("  conditionsSatisfied: %.3f ms\\n", IS1 * 1000.0);
            printf("  rememberConditionState: %.3f ms\\n", IS2 * 1000.0);
            S1=S2=S3=IS1=IS2=pp=0;
        }
    }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
