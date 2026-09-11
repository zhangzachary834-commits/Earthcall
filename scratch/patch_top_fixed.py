import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# Make sure we don't duplicate variables, use unique names
content = content.replace("std::vector<Law::ApplicationRecord> LawManager::tick() {", """std::vector<Law::ApplicationRecord> LawManager::tick() {
    static int p_tick_counter = 0; p_tick_counter++;
    auto T_START = glfwGetTime();
""")

content = content.replace("syncProphetic();", """syncProphetic();
    auto T_PROP = glfwGetTime();""")

content = content.replace("if (_connected) {", """auto T_SYNC = glfwGetTime();
    if (_connected) {""")

content = content.replace("refreshVocabularyIndex();", """auto T_SEED = glfwGetTime();
    refreshVocabularyIndex();
    auto T_VOCAB = glfwGetTime();""")

content = content.replace("if (_rete.hasDirtyFacts()) {", """if (_rete.hasDirtyFacts()) {""")

content = content.replace("std::vector<Law*> continuousLaws;", """auto T_EVAL = glfwGetTime();
    std::vector<Law*> continuousLaws;""")

content = content.replace("runDriveSessions(records);", """auto T_LAWS = glfwGetTime();
    runDriveSessions(records);
    auto T_DRIVES = glfwGetTime();""")

content = content.replace("reapUnmade();", """reapUnmade();
    auto T_REAP = glfwGetTime();""")

content = content.replace("return records;", """
    static double S_PROP = 0, S_SYNC = 0, S_SEED = 0, S_VOCAB = 0, S_EVAL = 0, S_LAWS = 0, S_DRIVES = 0, S_REAP = 0;
    S_PROP += T_PROP - T_START;
    S_SYNC += T_SYNC - T_PROP;
    S_SEED += T_SEED - T_SYNC;
    S_VOCAB += T_VOCAB - T_SEED;
    S_EVAL += T_EVAL - T_VOCAB;
    S_LAWS += T_LAWS - T_EVAL;
    S_DRIVES += T_DRIVES - T_LAWS;
    S_REAP += T_REAP - T_DRIVES;
    
    if (p_tick_counter == 24) {
        printf("--- TICK TOP PERF ---\\n");
        printf("prophetic: %.3f ms\\n", S_PROP * 1000.0);
        printf("syncRete: %.3f ms\\n", S_SYNC * 1000.0);
        printf("seed: %.3f ms\\n", S_SEED * 1000.0);
        printf("vocab: %.3f ms\\n", S_VOCAB * 1000.0);
        printf("eval+rete: %.3f ms\\n", S_EVAL * 1000.0);
        printf("laws_loop: %.3f ms\\n", S_LAWS * 1000.0);
        printf("drives: %.3f ms\\n", S_DRIVES * 1000.0);
        printf("reap: %.3f ms\\n", S_REAP * 1000.0);
    }
    return records;""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
