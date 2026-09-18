import re

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "r") as f:
    content = f.read()

def replace_fn(match):
    return """        } else if (slot.dynamicSlot) {
            // Check coercion for dynamic slots too to prevent type changing if it's currently a number
            double n = 0.0;
            PropertyValue coerced;
            if (propertyValueToNumber(v, n) && coerceLike(currentVal, n, coerced)) {
                if (propertyValuesEquivalent(currentVal, coerced)) return PathResult::Unchanged;
                if (slot.owner->setDynamicProperty(slot.dynamicKey, coerced)) {
                    return announce(PathResult::Ok, nullptr, slot.owner, Earthcall::StringInterner::resolve(slot.dynamicKey));
                }
            } else {
                if (propertyValuesEquivalent(currentVal, v)) return PathResult::Unchanged;
                if (slot.owner->setDynamicProperty(slot.dynamicKey, v)) {
                    return announce(PathResult::Ok, nullptr, slot.owner, Earthcall::StringInterner::resolve(slot.dynamicKey));
                }
            }
            return PathResult::ReadOnly;
        }"""

pattern = r'\} else if \(slot\.dynamicSlot\) \{.*?\*slot\.dynamicSlot = v;\n\s*\}\n\s*return announce\(PathResult::Ok, nullptr, slot\.owner, Earthcall::StringInterner::resolve\(slot\.dynamicKey\)\);\n\s*\}'
if not re.search(pattern, content, flags=re.DOTALL):
    print("NO MATCH")
else:
    content = re.sub(pattern, replace_fn, content, flags=re.DOTALL)
    with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
        f.write(content)
