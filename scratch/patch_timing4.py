import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """std::vector<Law::ApplicationRecord> LawManager::tick() {
    auto T0 = glfwGetTime();

    // Bring the possibility-space index up to date with the law text before
    // anything consults it. Cheap when nothing moved: one integer compare.
    syncProphetic();

    // Bring compiled terminals up to date with the conditions they were
    // compiled from, before anything reads either. Conditions are edited from
    // the graph window, from tools, and from loaded worlds; asking here means
    // no editing path has to remember to recompile, and the reactive and
    // sweep evaluations cannot be looking at different conditions.
    for (const auto& law : _laws) {
        if (law) syncReteCompilation(*law);
    }
    auto T1 = glfwGetTime();"""

replace_str = """std::vector<Law::ApplicationRecord> LawManager::tick() {
    auto T0 = glfwGetTime();

    syncProphetic();
    auto Tp = glfwGetTime();

    for (const auto& law : _laws) {
        if (law) syncReteCompilation(*law);
    }
    auto T1 = glfwGetTime();
    
    static int tickCount = 0;
    if (tickCount++ % 24 == 0) {
        printf("SYNC: syncProphetic=%.3f ms, syncReteCompilation=%.3f ms\\n",
            (Tp - T0) * 1000.0, (T1 - Tp) * 1000.0);
    }"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched timing 4!")
else:
    print("Could not find string in Law.cpp!")
