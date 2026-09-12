import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""    if (_rete.hasDirtyFacts()) {
        _rete.evaluateDirty();
        _dirty = true;
    }""", """    if (_rete.hasDirtyFacts()) {
        _rete.evaluateDirty();
        _dirty = true;
    }
    auto t_post_dirty = glfwGetTime();""")

content = content.replace("""    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;""", """    auto t_post_agenda = glfwGetTime();
    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;""")

content = content.replace("""    _tickTiming.evalMs  = static_cast<float>((T3 - T2) * 1000.0);""", """    _tickTiming.evalMs  = static_cast<float>((T3 - T2) * 1000.0);
    
    static double s_dirty = 0, s_agenda = 0, s_sweep = 0;
    s_dirty += (t_post_dirty - T2);
    s_agenda += (t_post_agenda - t_post_dirty);
    s_sweep += (T3 - t_post_agenda);
    
    static int p_tick = 0;
    if (++p_tick == 24) {
        printf("--- EVAL BREAKDOWN ---\\n");
        printf("dirty:  %.3f ms\\n", s_dirty * 1000.0);
        printf("agenda: %.3f ms\\n", s_agenda * 1000.0);
        printf("sweep:  %.3f ms\\n", s_sweep * 1000.0);
    }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
