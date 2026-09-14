import re

with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'r') as f:
    content = f.read()

# We want to replace from "Property* PropertyPath::resolve(" down to the end of the file.
start_idx = content.find("PropertyPath::ResolvedSlot PropertyPath::resolve(")
if start_idx == -1:
    start_idx = content.find("Property* PropertyPath::resolve(")

new_methods = """PropertyPath::ResolvedSlot PropertyPath::resolve(Singular& root, std::size_t startIndex) const {
    ResolvedSlot slot;
    if (segments.empty() || startIndex >= segments.size()) return slot;

    Singular* currentOwner = &root;
    Property* currentRegistered = nullptr;
    PropertyValue* currentDynamic = nullptr;
    std::size_t i = startIndex;

    while (i < segments.size()) {
        if (currentOwner) {
            Property* foundReg = nullptr;
            std::size_t consumed = 0;
            const auto& idsFromHere = _joinedIds[i];
            
            for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
                Earthcall::StringId id = idsFromHere[runLength - 1];
                if (Property* candidate = currentOwner->findProperty(id)) {
                    foundReg = candidate;
                    consumed = runLength;
                }
            }

            PropertyValue* foundDyn = nullptr;
            if (!foundReg) {
                for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
                    Earthcall::StringId id = idsFromHere[runLength - 1];
                    if (PropertyValue* candidate = currentOwner->getDynamicPropertyPtr(id)) {
                        foundDyn = candidate;
                        consumed = runLength;
                    }
                }
            }

            if (!foundReg && !foundDyn) return slot;

            i += consumed;
            slot.owner = currentOwner;
            currentRegistered = foundReg;
            currentDynamic = foundDyn;
            currentOwner = nullptr;

        } else if (currentDynamic) {
            std::shared_ptr<PropertyDict>* pDict = std::get_if<std::shared_ptr<PropertyDict>>(currentDynamic);
            if (pDict && *pDict) {
                auto it = (*pDict)->elements.find(segments[i]);
                if (it != (*pDict)->elements.end()) {
                    currentDynamic = &it->second;
                    currentRegistered = nullptr;
                    i++;
                } else {
                    return slot;
                }
            } else {
                std::shared_ptr<PropertyList>* pList = std::get_if<std::shared_ptr<PropertyList>>(currentDynamic);
                if (pList && *pList) {
                    try {
                        std::size_t idx = std::stoull(segments[i]);
                        if (idx < (*pList)->elements.size()) {
                            currentDynamic = &(*pList)->elements[idx];
                            currentRegistered = nullptr;
                            i++;
                        } else {
                            return slot;
                        }
                    } catch (...) {
                        return slot;
                    }
                } else {
                    break;
                }
            }
        } else {
            return slot;
        }

        if (i == segments.size()) {
            slot.prop = currentRegistered;
            slot.dynamicSlot = currentDynamic;
            return slot;
        }

        PropertyValue val;
        if (currentRegistered) {
            if (Singular* next = currentRegistered->asSingular()) {
                currentOwner = next;
                currentRegistered = nullptr;
                currentDynamic = nullptr;
                continue;
            }
            val = currentRegistered->value();
        } else if (currentDynamic) {
            val = *currentDynamic;
        }

        if (Singular** nextSingular = std::get_if<Singular*>(&val)) {
            if (*nextSingular) { currentOwner = *nextSingular; currentRegistered = nullptr; currentDynamic = nullptr; }
        } else if (Object** nextObj = std::get_if<Object*>(&val)) {
            if (*nextObj) { currentOwner = reinterpret_cast<Singular*>(*nextObj); currentRegistered = nullptr; currentDynamic = nullptr; }
        } else if (Relation** nextRel = std::get_if<Relation*>(&val)) {
            if (*nextRel) { currentOwner = reinterpret_cast<Singular*>(*nextRel); currentRegistered = nullptr; currentDynamic = nullptr; }
        } else if (Formation** nextForm = std::get_if<Formation*>(&val)) {
            if (*nextForm) { currentOwner = reinterpret_cast<Singular*>(*nextForm); currentRegistered = nullptr; currentDynamic = nullptr; }
        }

        if (i == segments.size() - 1 && std::holds_alternative<glm::vec3>(val)) {
            const std::string& c = segments[i];
            if (isVec3Component(c)) {
                slot.prop = currentRegistered;
                slot.dynamicSlot = currentDynamic;
                slot.trailingComponent = c;
                return slot;
            }
        }
    }
    return slot;
}

PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out, std::size_t startIndex) const {
    ResolvedSlot slot = resolve(root, startIndex);
    
    if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            if (root.getDynamicProperty(segments[startIndex], out)) return PathResult::Ok;
        }
        return PathResult::NoSuchProperty;
    }

    PropertyValue v;
    if (slot.prop) v = slot.prop->value();
    else if (slot.dynamicSlot) v = *slot.dynamicSlot;

    if (slot.trailingComponent.empty()) {
        out = std::move(v);
        return !std::holds_alternative<std::monostate>(out) ? PathResult::Ok : PathResult::NoSuchProperty;
    }

    glm::vec3* vec = std::get_if<glm::vec3>(&v);
    if (!vec) return PathResult::BadComponent;
    out = PropertyValue(*componentOf(*vec, slot.trailingComponent));
    return PathResult::Ok;
}

PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v, std::size_t startIndex) const {
    ResolvedSlot slot = resolve(root, startIndex);

    const auto announce = [&](PathResult result, Property* prop, Singular* on, const std::string& fallbackName = "") {
        if (result == PathResult::Ok && on) {
            Singular::notifyPropertyChanged(on, prop ? prop->name() : fallbackName);
        }
        return result;
    };

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
            if (propertyValueToNumber(v, n) && coerceLike(*slot.dynamicSlot, n, coerced)) {
                *slot.dynamicSlot = coerced;
            } else {
                *slot.dynamicSlot = v;
            }
            // Announce on the owner with the appropriate segment name
            // (If it was a nested dict write, slot.owner might be the top-level singular)
            std::string announceName = segments.back();
            return announce(PathResult::Ok, nullptr, slot.owner, announceName);
        }
    }

    // Component write
    double n = 0.0;
    if (!propertyValueToNumber(v, n)) return PathResult::TypeMismatch;
    
    PropertyValue whole = slot.prop ? slot.prop->value() : *slot.dynamicSlot;
    glm::vec3* vec = std::get_if<glm::vec3>(&whole);
    if (!vec) return PathResult::BadComponent;
    
    float& lane = *componentOf(*vec, slot.trailingComponent);
    if (std::fabs(static_cast<double>(lane) - n) <= 1e-6 * std::max(1.0, std::fabs(n))) {
        return PathResult::Unchanged;
    }
    lane = static_cast<float>(n);
    
    if (slot.prop) {
        if (slot.prop->setValue(PropertyValue(*vec))) return announce(PathResult::Ok, slot.prop, slot.owner);
        return PathResult::ReadOnly;
    } else if (slot.dynamicSlot) {
        *slot.dynamicSlot = PropertyValue(*vec);
        return announce(PathResult::Ok, nullptr, slot.owner, segments.back());
    }
    return PathResult::NoSuchProperty;
}
"""

with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'w') as f:
    f.write(content[:start_idx] + new_methods)
