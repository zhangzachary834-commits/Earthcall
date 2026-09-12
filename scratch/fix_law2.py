import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

target = """            if ((tD - tC) * 1000.0 > 0.5) {
                printf("[EVAL] Law '%s' conditionsSatisfied on '%s' took %.2f ms\\n", law->getIdentifier().c_str(), subject->getIdentifier().c_str(), (tD - tC) * 1000.0);
            }
            if (!subject || Universe::instance().isUnmade(subject)) continue;
            const bool holds = law->conditionsSatisfied(*subject);"""

replacement = """            if ((tD - tC) * 1000.0 > 0.5) {
                printf("[EVAL] Law '%s' conditionsSatisfied on '%s' took %.2f ms\\n", law->getIdentifier().c_str(), subject->getIdentifier().c_str(), (tD - tC) * 1000.0);
            }"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, "w") as f:
        f.write(content)
    print("Fixed double holds!")
else:
    print("Could not find double holds")
