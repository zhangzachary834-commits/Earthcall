with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "r") as f:
    content = f.read()

# For the !trailingComponent branch
original_block_1 = """        } else if (slot.dynamicSlot) {
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

new_block_1 = """        } else if (slot.dynamicSlot) {
            double n = 0.0;
            PropertyValue coerced;
            bool coerceSuccess = propertyValueToNumber(v, n) && coerceLike(*slot.dynamicSlot, n, coerced);
            const PropertyValue& valToWrite = coerceSuccess ? coerced : v;

            if (propertyValuesEquivalent(*slot.dynamicSlot, valToWrite)) return PathResult::Unchanged;

            if (slot.dynamicSlot == slot.owner->getDynamicPropertyPtr(Earthcall::StringInterner::intern(slot.dynamicKey))) {
                if (slot.owner->setDynamicProperty(Earthcall::StringInterner::intern(slot.dynamicKey), valToWrite)) {
                    return PathResult::Ok;
                }
            } else {
                *slot.dynamicSlot = valToWrite;
                if (!slot.dynamicKey.empty()) {
                    slot.owner->notifyPropertyChanged(slot.owner, slot.dynamicKey);
                }
                return PathResult::Ok;
            }
            return PathResult::ReadOnly;
        }"""

# For the trailingComponent branch
original_block_2 = """    } else if (slot.dynamicSlot) {
        if (slot.owner->setDynamicProperty(slot.dynamicKey, PropertyValue(*vec))) {
            return PathResult::Ok; // setDynamicProperty already called notifyPropertyChanged
        }
        return PathResult::ReadOnly;
    }"""

new_block_2 = """    } else if (slot.dynamicSlot) {
        if (slot.dynamicSlot == slot.owner->getDynamicPropertyPtr(Earthcall::StringInterner::intern(slot.dynamicKey))) {
            if (slot.owner->setDynamicProperty(Earthcall::StringInterner::intern(slot.dynamicKey), PropertyValue(*vec))) {
                return PathResult::Ok;
            }
        } else {
            *slot.dynamicSlot = PropertyValue(*vec);
            if (!slot.dynamicKey.empty()) {
                slot.owner->notifyPropertyChanged(slot.owner, slot.dynamicKey);
            }
            return PathResult::Ok;
        }
        return PathResult::ReadOnly;
    }"""

content = content.replace(original_block_1, new_block_1)
content = content.replace(original_block_2, new_block_2)

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
    f.write(content)
