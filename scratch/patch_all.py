import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

# Replace standard glfwGetTime with chrono
global_vars = """
#include <chrono>
#include <map>
#include <cstdio>
static std::map<std::string, double> g_lawTimings;
struct LawTimingPrinter {
    ~LawTimingPrinter() {
        printf("\\n=== LAW TIMINGS ===\\n");
        for(auto& pair : g_lawTimings) {
            printf("%s: %.3f ms\\n", pair.first.c_str(), pair.second);
        }
        printf("===================\\n");
    }
} g_printer;

inline double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
"""

content = global_vars + content

# Patch Continuous Sweep block
sweep_find = """        auto sweepSubjects = getSweepSubjects(activeZone);
        for (const auto& subject : sweepSubjects) {"""
sweep_replace = """        double tC = now_ms();
        auto sweepSubjects = getSweepSubjects(activeZone);
        for (const auto& subject : sweepSubjects) {"""
content = content.replace(sweep_find, sweep_replace)

sweep_end_find = """            }
        }
    }

    // --- Action Phase ---"""
sweep_end_replace = """            }
        }
        double tD = now_ms();
        if ((tD - tC) > 0.05) {
            g_lawTimings["[CONT_SWEEP] " + std::string(identifier())] += (tD - tC);
        }
    }

    // --- Action Phase ---"""
content = content.replace(sweep_end_find, sweep_end_replace)

# Save
with open(file_path, "w") as f:
    f.write(content)
print("Patched Law.cpp with chrono!")
