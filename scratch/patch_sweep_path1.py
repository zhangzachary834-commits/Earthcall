import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {""", """        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
            auto t_sp1_0 = glfwGetTime();""")
content = content.replace("""            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);""", """            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);
            auto t_sp1_1 = glfwGetTime();""")
content = content.replace("""            for (Singular* subject : subjects) {""", """            auto t_sp1_2 = glfwGetTime();
            for (Singular* subject : subjects) {""")
content = content.replace("""            continue;
        }

        // OnBecomeTrue""", """            auto t_sp1_3 = glfwGetTime();
            
            static double S1=0, S2=0, S3=0;
            S1 += (t_sp1_1 - t_sp1_0);
            S2 += (t_sp1_2 - t_sp1_1);
            S3 += (t_sp1_3 - t_sp1_2);
            static int pp = 0;
            if (++pp == 3000) {
                printf("--- PATH 1 PERF ---\\n");
                printf("collectTerminalSubjects: %.3f ms\\n", S1 * 1000.0);
                printf("released memory loop: %.3f ms\\n", S2 * 1000.0);
                printf("apply loop: %.3f ms\\n", S3 * 1000.0);
            }
            continue;
        }

        // OnBecomeTrue""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
