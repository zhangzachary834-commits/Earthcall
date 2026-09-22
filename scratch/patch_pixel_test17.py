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
                    return PathResult::Ok; // setDynamicProperty already called notifyPropertyChanged
                }
            } else {
                if (propertyValuesEquivalent(*slot.dynamicSlot, v)) return PathResult::Unchanged;
                if (slot.owner->setDynamicProperty(slot.dynamicKey, v)) {
                    return PathResult::Ok; // setDynamicProperty already called notifyPropertyChanged
                }
            }
            return PathResult::ReadOnly; // If setDynamicProperty fails
        }"""

pattern = r'\} else if \(slot\.dynamicSlot\) \{\n\s*// Check coercion.*?return PathResult::ReadOnly; // If setDynamicProperty fails\n\s*\}'
if not re.search(pattern, content, flags=re.DOTALL):
    print("NO MATCH 1")
else:
    content = re.sub(pattern, replace_fn, content, flags=re.DOTALL)
    print("MATCH 1")
    with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
        f.write(content)
