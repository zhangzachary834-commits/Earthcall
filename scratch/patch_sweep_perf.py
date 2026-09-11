import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("std::vector<Singular*> subjects = sweepSubjects(*law);", """std::vector<Singular*> subjects = sweepSubjects(*law);
        auto ts_0 = glfwGetTime();""")

content = content.replace("for (Singular* subject : subjects) {", """for (Singular* subject : subjects) {
            auto ts_1 = glfwGetTime();""")

content = content.replace("const bool holds = law->conditionsSatisfied(*subject);", """const bool holds = law->conditionsSatisfied(*subject);
            auto ts_2 = glfwGetTime();""")

content = content.replace("law->rememberConditionState(subject, holds);", """law->rememberConditionState(subject, holds);
            auto ts_3 = glfwGetTime();""")

content = content.replace("const bool fire = law->activation() == Law::Activation::WhileTrue", """auto ts_4 = glfwGetTime();
            const bool fire = law->activation() == Law::Activation::WhileTrue""")

content = content.replace("applyAndMaybeDrive(*law, *subject, records);", """auto ts_5 = glfwGetTime();
            applyAndMaybeDrive(*law, *subject, records);
            auto ts_6 = glfwGetTime();
            
            static double S1=0, S2=0, S3=0, S4=0, S5=0, S6=0;
            S1 += (ts_1 - ts_0);
            S2 += (ts_2 - ts_1);
            S3 += (ts_3 - ts_2);
            S4 += (ts_4 - ts_3);
            S5 += (ts_5 - ts_4);
            S6 += (ts_6 - ts_5);
            static int pp = 0;
            if (++pp == 50000) {
                printf("--- SWEEP PERF ---\\n");
                printf("Loop overhead: %.3f ms\\n", S1 * 1000.0);
                printf("condSatisfied: %.3f ms\\n", S2 * 1000.0);
                printf("rememberState: %.3f ms\\n", S3 * 1000.0);
                printf("onset: %.3f ms\\n", S4 * 1000.0);
                printf("drives: %.3f ms\\n", S5 * 1000.0);
                printf("apply: %.3f ms\\n", S6 * 1000.0);
            }
""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
