with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'r') as f:
    content = f.read()

old_code_1 = """            // Announce on the owner with the appropriate segment name
            // (If it was a nested dict write, slot.owner might be the top-level singular)
            std::string announceName = segments.back();
            return announce(PathResult::Ok, nullptr, slot.owner, announceName);
        }
    }"""

new_code_1 = """            // Announce on the owner with the appropriate segment name
            // (If it was a nested dict write, slot.owner might be the top-level singular)
            return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
        }
    }"""

old_code_2 = """    } else if (slot.dynamicSlot) {
        *slot.dynamicSlot = PropertyValue(*vec);
        return announce(PathResult::Ok, nullptr, slot.owner, segments.back());
    }"""

new_code_2 = """    } else if (slot.dynamicSlot) {
        *slot.dynamicSlot = PropertyValue(*vec);
        return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
    }"""

if old_code_1 in content:
    content = content.replace(old_code_1, new_code_1)
    print("Replaced 1")
else:
    print("Could not find old code 1")

if old_code_2 in content:
    content = content.replace(old_code_2, new_code_2)
    print("Replaced 2")
else:
    print("Could not find old code 2")

with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'w') as f:
    f.write(content)
