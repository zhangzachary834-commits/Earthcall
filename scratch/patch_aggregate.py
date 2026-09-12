import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

# I will add global counters at the top of Law.cpp
inject_top = """
#include <map>
#include <string>
#include <iostream>
struct LawTiming {
    double rete_time = 0;
    int rete_count = 0;
    double cont_time = 0;
    int cont_count = 0;
    double event_time = 0;
    int event_count = 0;
};
static std::map<std::string, LawTiming> g_lawTimings;
static int g_tickCount = 0;

struct TickCounter {
    ~TickCounter() {
        std::cout << "\\n=== LAW TIMING AGGREGATES ===\\n";
        for (const auto& kv : g_lawTimings) {
            double total = kv.second.rete_time + kv.second.cont_time + kv.second.event_time;
            if (total > 0.05) {
                printf("Law '%s': TOTAL=%.2fms (Rete: %d evals, %.2fms) (Cont: %d evals, %.2fms) (Event: %d sweeps, %.2fms)\\n",
                    kv.first.c_str(), total,
                    kv.second.rete_count, kv.second.rete_time,
                    kv.second.cont_count, kv.second.cont_time,
                    kv.second.event_count, kv.second.event_time);
            }
        }
    }
} g_tickCounter;
"""
content = content.replace('#include "Law.hpp"', '#include "Law.hpp"\n' + inject_top)

target_rete = """            applyAndMaybeDrive(*law, *subject, records);"""
replace_rete = """            auto tR0 = glfwGetTime();
            applyAndMaybeDrive(*law, *subject, records);
            auto tR1 = glfwGetTime();
            g_lawTimings[law->getIdentifier()].rete_time += (tR1 - tR0) * 1000.0;
            g_lawTimings[law->getIdentifier()].rete_count++;"""
content = content.replace(target_rete, replace_rete)

target_cont = """            const bool holds = law->conditionsSatisfied(*subject);"""
replace_cont = """            auto tC0 = glfwGetTime();
            const bool holds = law->conditionsSatisfied(*subject);
            auto tC1 = glfwGetTime();
            g_lawTimings[law->getIdentifier()].cont_time += (tC1 - tC0) * 1000.0;
            g_lawTimings[law->getIdentifier()].cont_count++;"""
content = content.replace(target_cont, replace_cont)

target_event = """                std::vector<Singular*> subjects = sweepSubjects(*law);"""
replace_event = """                auto tE0 = glfwGetTime();
                std::vector<Singular*> subjects = sweepSubjects(*law);
                auto tE1 = glfwGetTime();
                g_lawTimings[law->getIdentifier()].event_time += (tE1 - tE0) * 1000.0;
                g_lawTimings[law->getIdentifier()].event_count++;"""
content = content.replace(target_event, replace_event)

with open(file_path, "w") as f:
    f.write(content)
print("Patched for aggregates!")
