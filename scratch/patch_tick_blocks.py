import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("std::vector<Law::ApplicationRecord> LawManager::tick() {", """std::vector<Law::ApplicationRecord> LawManager::tick() {
    static int p_tick_counter = 0; p_tick_counter++;
    auto b1_start = glfwGetTime();
""")

content = content.replace("std::vector<Law::ApplicationRecord> records;", """auto b2_start = glfwGetTime();
    std::vector<Law::ApplicationRecord> records;""")

content = content.replace("std::vector<Law*> continuousLaws;", """auto b3_start = glfwGetTime();
    std::vector<Law*> continuousLaws;""")

content = content.replace("runDriveSessions(records);", """auto b4_start = glfwGetTime();
    runDriveSessions(records);""")

content = content.replace("return records;", """
    auto b4_end = glfwGetTime();
    static double S_B1 = 0, S_B2 = 0, S_B3 = 0, S_B4 = 0;
    S_B1 += (b2_start - b1_start);
    S_B2 += (b3_start - b2_start);
    S_B3 += (b4_start - b3_start);
    S_B4 += (b4_end - b4_start);
    
    if (p_tick_counter == 24) {
        printf("--- BLOCK PERF ---\\n");
        printf("Block 1 (Sync+Eval): %.3f ms\\n", S_B1 * 1000.0);
        printf("Block 2 (Rete Agenda): %.3f ms\\n", S_B2 * 1000.0);
        printf("Block 3 (Sweep Loop): %.3f ms\\n", S_B3 * 1000.0);
        printf("Block 4 (Drives+Reap): %.3f ms\\n", S_B4 * 1000.0);
    }
    return records;""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
