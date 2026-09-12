import re

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    lines = f.readlines()

new_lines = []
in_tick = False
for line in lines:
    if "std::vector<Law::ApplicationRecord> LawManager::tick()" in line:
        new_lines.append(line)
        new_lines.append("""
    static int local_tick_count = 0;
    local_tick_count++;
    static double total_sweepSubjects = 0;
    static double total_condEval = 0;
    static double total_applyAndDrive = 0;
    static double total_reteCollect = 0;
    static double total_reteCond = 0;
    static double total_reteApply = 0;
    static double total_gatesHold = 0;
    static double total_other = 0;
    auto t_tick_start = glfwGetTime();
""")
        in_tick = True
        continue
        
    if in_tick and "return records;" in line:
        new_lines.append("""
    if (local_tick_count == 30) {
        printf("--- DETAILED TICK PERF ---\\n");
        printf("gatesHold: %.3f ms\\n", total_gatesHold * 1000.0);
        printf("reteCollect: %.3f ms\\n", total_reteCollect * 1000.0);
        printf("reteCond: %.3f ms\\n", total_reteCond * 1000.0);
        printf("reteApply: %.3f ms\\n", total_reteApply * 1000.0);
        printf("sweepSubjects: %.3f ms\\n", total_sweepSubjects * 1000.0);
        printf("condEval: %.3f ms\\n", total_condEval * 1000.0);
        printf("applyAndDrive: %.3f ms\\n", total_applyAndDrive * 1000.0);
        printf("total time: %.3f ms\\n", (glfwGetTime() - t_tick_start) * 1000.0);
    }
""")
    new_lines.append(line)

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp.new", "w") as f:
    f.writelines(new_lines)
