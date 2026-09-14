with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

timing_code = """
    std::vector<glm::ivec2> selected;
    double t_start = glfwGetTime();
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }
    double t_cache = glfwGetTime();
"""

content = content.replace("""    std::vector<glm::ivec2> selected;
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }""", timing_code)

write_code = """    }
    double t_write_start = glfwGetTime();
    bool res = ft.writeSamples(selected, colors);
    double t_end = glfwGetTime();
    std::printf("Cache lookup/compute: %.3f ms, writeSamples: %.3f ms\\n", (t_cache - t_start)*1000.0, (t_end - t_write_start)*1000.0);
    return res;
}
"""

content = content.replace("""    }
    if (colors.size() != selected.size()) return false;
    return ft.writeSamples(selected, colors);
}""", write_code)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)

