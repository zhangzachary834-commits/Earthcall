import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# Add counters
content = content.replace("std::vector<Law::ApplicationRecord> LawManager::tick() {", """std::vector<Law::ApplicationRecord> LawManager::tick() {
    static int tick_counter = 0; tick_counter++;
    static double t_gates = 0, t_rete_col = 0, t_rete_cond = 0, t_rete_app = 0, t_swp_sub = 0, t_swp_cond = 0, t_swp_app = 0;
    auto t_tick_start = glfwGetTime();
""")

# gatesHold
content = content.replace("if (!gatesHold(*law)) {", """auto t1 = glfwGetTime();
        bool gh = gatesHold(*law);
        t_gates += glfwGetTime() - t1;
        if (!gh) {""")

# Rete collect
content = content.replace("std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);", """auto t2 = glfwGetTime();
            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);
            t_rete_col += glfwGetTime() - t2;
""")

# Rete conditionsSatisfied
content = content.replace("if (!law->conditionsSatisfied(*subject)) continue;", """auto t3 = glfwGetTime();
                bool holds = law->conditionsSatisfied(*subject);
                t_rete_cond += glfwGetTime() - t3;
                if (!holds) continue;
""")

# Rete apply
content = content.replace("applyAndMaybeDrive(*law, *subject, records);\n            }\n            continue;", """auto t4 = glfwGetTime();
                applyAndMaybeDrive(*law, *subject, records);
                t_rete_app += glfwGetTime() - t4;
            }
            continue;
""")

# Sweep subjects
content = content.replace("std::vector<Singular*> subjects = sweepSubjects(*law);", """auto t5 = glfwGetTime();
        std::vector<Singular*> subjects = sweepSubjects(*law);
        t_swp_sub += glfwGetTime() - t5;
""")

# Sweep conditions
content = content.replace("const bool holds = law->conditionsSatisfied(*subject);", """auto t6 = glfwGetTime();
            const bool holds = law->conditionsSatisfied(*subject);
            t_swp_cond += glfwGetTime() - t6;
""")

# Sweep apply
content = content.replace("applyAndMaybeDrive(*law, *subject, records);\n        }\n    }\n", """auto t7 = glfwGetTime();
            applyAndMaybeDrive(*law, *subject, records);
            t_swp_app += glfwGetTime() - t7;
        }
    }
    if (tick_counter == 10) {
        printf("--- PERF: gates:%.3f reteCol:%.3f reteCond:%.3f reteApp:%.3f swpSub:%.3f swpCond:%.3f swpApp:%.3f\\n",
            t_gates*1000, t_rete_col*1000, t_rete_cond*1000, t_rete_app*1000, t_swp_sub*1000, t_swp_cond*1000, t_swp_app*1000);
    }
""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
