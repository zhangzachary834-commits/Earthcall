import re

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "r") as f:
    content = f.read()

def replace_fn(match):
    return """        } else if (slot.dynamicSlot) {
            // Check coercion for dynamic slots too to prevent type changing if it's currently a number
            double n = 0.0;
            PropertyValue coerced;
            if (propertyValueToNumber(v, n) && coerceLike(*slot.dynamicSlot, n, coerced)) {
                if (propertyValuesEquivalent(*slot.dynamicSlot, coerced)) return PathResult::Unchanged;
                if (slot.owner->setDynamicProperty(slot.dynamicKey, coerced)) {
                    return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
                }
            } else {
                if (propertyValuesEquivalent(*slot.dynamicSlot, v)) return PathResult::Unchanged;
                if (slot.owner->setDynamicProperty(slot.dynamicKey, v)) {
                    return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
                }
            }
            return PathResult::ReadOnly; // If setDynamicProperty fails
        }"""

pattern = r'\} else if \(slot\.dynamicSlot\) \{.*?\*slot\.dynamicSlot = v;\n\s*\}\n\s*// Announce.*?\n\s*return announce\(PathResult::Ok, nullptr, slot\.owner, slot\.dynamicKey\);\n\s*\}'
if not re.search(pattern, content, flags=re.DOTALL):
    print("NO MATCH 1")
else:
    content = re.sub(pattern, replace_fn, content, flags=re.DOTALL)
    print("MATCH 1")

def replace_fn2(match):
    return """    } else if (slot.dynamicSlot) {
        if (slot.owner->setDynamicProperty(slot.dynamicKey, PropertyValue(*vec))) {
            return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
        }
        return PathResult::ReadOnly;
    }"""

pattern2 = r'\} else if \(slot\.dynamicSlot\) \{\n\s*\*slot\.dynamicSlot = PropertyValue\(\*vec\);\n\s*return announce\(PathResult::Ok, nullptr, slot\.owner, slot\.dynamicKey\);\n\s*\}'
if not re.search(pattern2, content, flags=re.DOTALL):
    print("NO MATCH 2")
else:
    content = re.sub(pattern2, replace_fn2, content, flags=re.DOTALL)
    print("MATCH 2")

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
    f.write(content)
