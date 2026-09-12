import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# Add counters
content = content.replace("std::vector<Law::ApplicationRecord> LawManager::tick() {", """std::vector<Law::ApplicationRecord> LawManager::tick() {
    static int tick_counter = 0; tick_counter++;
    static double t_rete_book = 0, t_rete_col = 0, t_rete_cond = 0;
""")

# Rete collect
content = content.replace("std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);", """auto t1 = glfwGetTime();
            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);
            t_rete_col += glfwGetTime() - t1;
""")

# Rete bookkeeping
content = content.replace("const auto& targets = law->targets().getMembers();", """auto t2 = glfwGetTime();
            const auto& targets = law->targets().getMembers();""")
content = content.replace("law->clearOnset(subject);\n            }", """law->clearOnset(subject);
            }
            t_rete_book += glfwGetTime() - t2;""")


content = content.replace("return records;", """
    if (tick_counter == 24) {
        printf("--- PERF BOOK: col:%.3f book:%.3f\\n",
            t_rete_col*1000, t_rete_book*1000);
    }
    return records;""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
