import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# Add counters
content = content.replace("std::vector<Law::ApplicationRecord> LawManager::tick() {", """std::vector<Law::ApplicationRecord> LawManager::tick() {
    static int tick_counter = 0; tick_counter++;
    auto t_start = glfwGetTime();
""")

content = content.replace("syncProphetic();", """syncProphetic();
    auto t_prop = glfwGetTime();""")

content = content.replace("if (_connected) {", """auto t_syn = glfwGetTime();
    if (_connected) {""")

content = content.replace("refreshVocabularyIndex();", """auto t_sed = glfwGetTime();
    refreshVocabularyIndex();
    auto t_voc = glfwGetTime();""")

content = content.replace("if (_rete.hasDirtyFacts()) {", """if (_rete.hasDirtyFacts()) {""")

content = content.replace("std::vector<Law*> continuousLaws;", """auto t_eva = glfwGetTime();
    std::vector<Law*> continuousLaws;""")

content = content.replace("runDriveSessions(records);", """auto t_law = glfwGetTime();
    runDriveSessions(records);
    auto t_drv = glfwGetTime();""")

content = content.replace("reapUnmade();", """reapUnmade();
    auto t_rep = glfwGetTime();""")

content = content.replace("return records;", """
    static double s_prop = 0, s_syn = 0, s_sed = 0, s_voc = 0, s_eva = 0, s_law = 0, s_drv = 0, s_rep = 0;
    s_prop += t_prop - t_start;
    s_syn += t_syn - t_prop;
    s_sed += t_sed - t_syn;
    s_voc += t_voc - t_sed;
    s_eva += t_eva - t_voc;
    s_law += t_law - t_eva;
    s_drv += t_drv - t_law;
    s_rep += t_rep - t_drv;
    
    if (tick_counter == 24) {
        printf("--- TICK TOP PERF ---\\n");
        printf("prophetic: %.3f ms\\n", s_prop * 1000.0);
        printf("syncRete: %.3f ms\\n", s_syn * 1000.0);
        printf("seed: %.3f ms\\n", s_sed * 1000.0);
        printf("vocab: %.3f ms\\n", s_voc * 1000.0);
        printf("eval+rete: %.3f ms\\n", s_eva * 1000.0);
        printf("laws_loop: %.3f ms\\n", s_law * 1000.0);
        printf("drives: %.3f ms\\n", s_drv * 1000.0);
        printf("reap: %.3f ms\\n", s_rep * 1000.0);
    }
    return records;""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
