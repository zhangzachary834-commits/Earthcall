import re

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "r") as f:
    content = f.read()

def replace_fn(match):
    return """PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v, std::size_t startIndex) const {
    ResolvedSlot slot = resolve(root, startIndex);

    const auto announce = [&](PathResult result, Property* prop, Singular* on, const std::string& fallbackName = "") {
        if (result == PathResult::Ok && on) {
            Singular::notifyPropertyChanged(on, prop ? prop->name() : fallbackName);
        }
        return result;
    };

    std::string pathStr = toString();
    if (pathStr.find("full-canvas") != std::string::npos) {
        std::cerr << "setValue DEBUG path=" << pathStr << " slot.prop=" << (slot.prop ? slot.prop->name() : "null") << " slot.dynamicSlot=" << (slot.dynamicSlot ? "yes" : "no") << " slot.owner=" << (slot.owner ? slot.owner->getIdentifier() : "null") << " dynamicKey=" << Earthcall::StringInterner::resolve(slot.dynamicKey) << std::endl;
    }

    if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            PropertyValue cur;
            if (root.getDynamicProperty(segments[startIndex], cur) && propertyValuesEquivalent(cur, v)) {
                return PathResult::Unchanged;
            }
            if (root.setDynamicProperty(segments[startIndex], v)) {
                return announce(PathResult::Ok, nullptr, &root, segments[startIndex]);
            }
            return PathResult::TypeMismatch;
        }
        return PathResult::NoSuchProperty;
    }

    if (slot.trailingComponent.empty()) {
        PropertyValue currentVal = slot.prop ? slot.prop->value() : *slot.dynamicSlot;
        if (propertyValuesEquivalent(currentVal, v)) return PathResult::Unchanged;
        
        if (slot.prop) {
            if (slot.prop->setValue(v)) return announce(PathResult::Ok, slot.prop, slot.owner);
            double n = 0.0;
            PropertyValue coerced;
            if (propertyValueToNumber(v, n) && coerceLike(currentVal, n, coerced)) {
                if (propertyValuesEquivalent(currentVal, coerced)) return PathResult::Unchanged;
                if (slot.prop->setValue(coerced)) return announce(PathResult::Ok, slot.prop, slot.owner);
            }
            if (currentVal.index() != v.index()) return PathResult::TypeMismatch;
            return PathResult::ReadOnly;
        } else if (slot.dynamicSlot) {
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
            return PathResult::ReadOnly; // If setDynamicProperty fails
        }
    }"""

pattern = r'PropertyPath::PathResult PropertyPath::setValue\(Singular& root, const PropertyValue& v, std::size_t startIndex\) const \{.*?if \(slot\.trailingComponent\.empty\(\)\) \{.*?return PathResult::ReadOnly;\s*\}\s*\}'
if not re.search(pattern, content, flags=re.DOTALL):
    print("NO MATCH")
else:
    content = re.sub(pattern, replace_fn, content, flags=re.DOTALL)
    with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
        f.write(content)
