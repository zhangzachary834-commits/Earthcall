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

pattern = r'\} else if \(slot\.dynamicSlot\) \{.*?\*slot\.dynamicSlot, coerced\)\) \{.*?\n\s*return announce\(PathResult::Ok, nullptr, slot\.owner, slot\.dynamicKey\);\n\s*\}\n\s*\} else \{.*?\n\s*return announce\(PathResult::Ok, nullptr, slot\.owner, slot\.dynamicKey\);\n\s*\}\n\s*return PathResult::ReadOnly; // If setDynamicProperty fails\n\s*\}'
if not re.search(pattern, content, flags=re.DOTALL):
    print("NO MATCH 1")
else:
    content = re.sub(pattern, replace_fn, content, flags=re.DOTALL)
    print("MATCH 1")

def replace_fn2(match):
    return """    } else if (slot.dynamicSlot) {
        if (slot.owner->setDynamicProperty(slot.dynamicKey, PropertyValue(*vec))) {
            return PathResult::Ok; // setDynamicProperty already called notifyPropertyChanged
        }
        return PathResult::ReadOnly;
    }"""

pattern2 = r'\} else if \(slot\.dynamicSlot\) \{\n\s*if \(slot\.owner->setDynamicProperty\(slot\.dynamicKey, PropertyValue\(\*vec\)\)\) \{\n\s*return announce\(PathResult::Ok, nullptr, slot\.owner, slot\.dynamicKey\);\n\s*\}\n\s*return PathResult::ReadOnly;\n\s*\}'
if not re.search(pattern2, content, flags=re.DOTALL):
    print("NO MATCH 2")
else:
    content = re.sub(pattern2, replace_fn2, content, flags=re.DOTALL)
    print("MATCH 2")

def replace_fn3(match):
    return """    if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            PropertyValue cur;
            if (root.getDynamicProperty(segments[startIndex], cur) && propertyValuesEquivalent(cur, v)) {
                return PathResult::Unchanged;
            }
            if (root.setDynamicProperty(segments[startIndex], v)) {
                return PathResult::Ok; // setDynamicProperty already called notifyPropertyChanged
            }
            return PathResult::TypeMismatch;
        }
        return PathResult::NoSuchProperty;
    }"""

pattern3 = r'if \(!slot\.prop && !slot\.dynamicSlot\) \{\n\s*if \(segments\.size\(\) - startIndex == 1\) \{\n\s*PropertyValue cur;\n\s*if \(root\.getDynamicProperty\(segments\[startIndex\], cur\) && propertyValuesEquivalent\(cur, v\)\) \{\n\s*return PathResult::Unchanged;\n\s*\}\n\s*if \(root\.setDynamicProperty\(segments\[startIndex\], v\)\) \{\n\s*return announce\(PathResult::Ok, nullptr, &root, segments\[startIndex\]\);\n\s*\}\n\s*return PathResult::TypeMismatch;\n\s*\}\n\s*return PathResult::NoSuchProperty;\n\s*\}'
if not re.search(pattern3, content, flags=re.DOTALL):
    print("NO MATCH 3")
else:
    content = re.sub(pattern3, replace_fn3, content, flags=re.DOTALL)
    print("MATCH 3")

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
    f.write(content)
