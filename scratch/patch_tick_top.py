import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# Add counters
content = content.replace("std::vector<Law::ApplicationRecord> LawManager::tick() {", """std::vector<Law::ApplicationRecord> LawManager::tick() {
    static int tick_counter = 0; tick_counter++;
    auto t_start = glfwGetTime();
""")

content = content.replace("syncProphetic();", """syncProphetic();
    auto t_prophetic = glfwGetTime();""")

content = content.replace("if (_connected) {", """auto t_sync = glfwGetTime();
    if (_connected) {""")

content = content.replace("refreshVocabularyIndex();", """auto t_seed = glfwGetTime();
    refreshVocabularyIndex();
    auto t_vocab = glfwGetTime();""")

content = content.replace("if (_rete.hasDirtyFacts()) {", """if (_rete.hasDirtyFacts()) {""")

content = content.replace("std::vector<Law*> continuousLaws;", """auto t_eval = glfwGetTime();
    std::vector<Law*> continuousLaws;""")

content = content.replace("runDriveSessions(records);", """auto t_laws = glfwGetTime();
    runDriveSessions(records);
    auto t_drives = glfwGetTime();""")

content = content.replace("reapUnmade();", """reapUnmade();
    auto t_reap = glfwGetTime();""")

content = content.replace("return records;", """
    static double s_prophetic = 0, s_sync = 0, s_seed = 0, s_vocab = 0, s_eval = 0, s_laws = 0, s_drives = 0, s_reap = 0;
    s_prophetic += t_prophetic - t_start;
    s_sync += t_sync - t_prophetic;
    s_seed += t_seed - t_sync;
    s_vocab += t_vocab - t_seed;
    s_eval += t_eval - t_vocab;
    s_laws += t_laws - t_eval;
    s_drives += t_drives - t_laws;
    s_reap += t_reap - t_drives;
    
    if (tick_counter == 24) {
        printf("--- TICK TOP PERF ---\\n");
        printf("prophetic: %.3f ms\\n", s_prophetic * 1000.0);
        printf("sync: %.3f ms\\n", s_sync * 1000.0);
        printf("seed: %.3f ms\\n", s_seed * 1000.0);
        printf("vocab: %.3f ms\\n", s_vocab * 1000.0);
        printf("eval: %.3f ms\\n", s_eval * 1000.0);
        printf("laws: %.3f ms\\n", s_laws * 1000.0);
        printf("drives: %.3f ms\\n", s_drives * 1000.0);
        printf("reap: %.3f ms\\n", s_reap * 1000.0);
    }
    return records;""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
