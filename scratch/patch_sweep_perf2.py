import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""        std::vector<Singular*> subjects = sweepSubjects(*law);""", """        auto t_sweep_start = glfwGetTime();
        std::vector<Singular*> subjects = sweepSubjects(*law);""")

content = content.replace("""        applyAndMaybeDrive(*law, *subject, records);
        }""", """        applyAndMaybeDrive(*law, *subject, records);
        }
        double sweep_dur = glfwGetTime() - t_sweep_start;
        static std::unordered_map<std::string, double> acc;
        acc[law->getIdentifier()] += sweep_dur;
        static int pp2 = 0;
        if (++pp2 == 500) {
            fprintf(stderr, "--- SWEEP DURATIONS ---\\n");
            for (const auto& kv : acc) {
                fprintf(stderr, "%s: %.3f ms\\n", kv.first.c_str(), kv.second * 1000.0);
            }
            acc.clear();
            pp2 = 0;
        }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
