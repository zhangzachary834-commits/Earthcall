with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'r') as f:
    content = f.read()

import re

old_code_1 = """
        } else if (slot.dynamicSlot) {
            *slot.dynamicSlot = v;
            // The Property was authored dynamically.
            // (If it was a nested dict write, slot.owner might be the top-level singular)
            std::string announceName = segments.back();
            return announce(PathResult::Ok, nullptr, slot.owner, announceName);
        }
"""
new_code_1 = """
        } else if (slot.dynamicSlot) {
            *slot.dynamicSlot = v;
            return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
        }
"""
if old_code_1 in content:
    content = content.replace(old_code_1, new_code_1)
    print("Replaced 1")

old_code_2 = """
    if (slot.prop) {
        if (slot.prop->setValue(PropertyValue(*vec))) return announce(PathResult::Ok, slot.prop, slot.owner);
        return PathResult::ReadOnly;
    } else if (slot.dynamicSlot) {
        *slot.dynamicSlot = PropertyValue(*vec);
        std::string announceName = slot.trailingComponent.empty() ? segments.back() : segments[segments.size() - 2];
        return announce(PathResult::Ok, nullptr, slot.owner, announceName);
    }
"""
new_code_2 = """
    if (slot.prop) {
        if (slot.prop->setValue(PropertyValue(*vec))) return announce(PathResult::Ok, slot.prop, slot.owner);
        return PathResult::ReadOnly;
    } else if (slot.dynamicSlot) {
        *slot.dynamicSlot = PropertyValue(*vec);
        return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
    }
"""
if old_code_2 in content:
    content = content.replace(old_code_2, new_code_2)
    print("Replaced 2")

with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'w') as f:
    f.write(content)

