import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("if (_rete.hasDirtyFacts()) {", """auto t_eval_start = glfwGetTime();
    if (_rete.hasDirtyFacts()) {""")
content = content.replace("_rete.evaluateDirty();\n    }", """_rete.evaluateDirty();
    }
    auto t_eval_end = glfwGetTime();
    static double s_eval_total = 0;
    static int s_eval_count = 0;
    s_eval_total += (t_eval_end - t_eval_start);
    s_eval_count++;
    if (s_eval_count == 24) {
        printf("--- EVAL DIRTY TOOK: %.3f ms ---\\n", s_eval_total * 1000.0);
    }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
