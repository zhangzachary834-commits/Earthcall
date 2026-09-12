import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""bool ReteNetwork::retractFact(const std::string& factId) {""", """bool ReteNetwork::retractFact(const std::string& factId) {
    auto t_start = glfwGetTime();""")

content = content.replace("""    return true;
}""", """    auto t_end = glfwGetTime();
    static double s_ret = 0;
    static int c_ret = 0;
    s_ret += (t_end - t_start);
    c_ret++;
    if (c_ret == 10000) {
        printf("--- RETRACT FACT PERF: %.3f ms over 10000 calls ---\\n", s_ret * 1000.0);
        c_ret = 0;
        s_ret = 0;
    }
    return true;
}""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
