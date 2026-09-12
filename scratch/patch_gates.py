import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    if (law.actionModel()) {
        std::vector<PropertyPath> writes;
        law.actionModel()->collectPaths(writes);
        for (const PropertyPath& w : writes) {
            if (!w.segments.empty() && !w.segments.front().empty() &&
                w.segments.front()[0] == '@') {
                return true;
            }
        }
    }

    // A gate ignores its subject, so ANY Singular answers it identically — and
    // the law itself is one, always alive, and never a member of the world it
    // is asked about. Deliberately not a member of the population: a world may
    // legitimately be empty at this moment.
    ECA::Event probe;
    probe.type = "law-gate";
    Singular& standIn = const_cast<Law&>(law);
    for (const ConditionNode* gate : gates) {
        if (!gate->compile()(probe, standIn)) return false;
    }

    return true;"""

replace_str = """    // A gate ignores its subject, so ANY Singular answers it identically — and
    // the law itself is one, always alive, and never a member of the world it
    // is asked about. Deliberately not a member of the population: a world may
    // legitimately be empty at this moment.
    ECA::Event probe;
    probe.type = "law-gate";
    Singular& standIn = const_cast<Law&>(law);
    bool gatesInitiallyTrue = true;
    for (const ConditionNode* gate : gates) {
        if (!gate->compile()(probe, standIn)) {
            gatesInitiallyTrue = false;
            break;
        }
    }

    // If the gate is already false, the law won't run AT ALL, so it can't possibly
    // execute its actions to flip the gate. We can safely hoist the FALSE!
    if (!gatesInitiallyTrue) return false;

    // THE GUARD THAT MAKES HOISTING TRUE SOUND.
    // If the gates are TRUE, but the law writes to a qualified root, the law might 
    // flip the gate to FALSE partway through its per-subject execution.
    // In that case, we CANNOT hoist the true! We must return true to force the 
    // per-subject fallback loop to evaluate the gate properly for each subject.
    if (law.actionModel()) {
        std::vector<PropertyPath> writes;
        law.actionModel()->collectPaths(writes);
        for (const PropertyPath& w : writes) {
            if (!w.segments.empty() && !w.segments.front().empty() &&
                w.segments.front()[0] == '@') {
                return true;
            }
        }
    }

    return true;"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp")
else:
    print("Could not find string in Law.cpp")
