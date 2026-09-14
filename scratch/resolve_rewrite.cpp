#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"

// We need to implement PropertyPath::resolve
PropertyPath::ResolvedSlot PropertyPath::resolve(Singular& root, std::size_t startIndex) const {
    ResolvedSlot slot;
    if (segments.empty() || startIndex >= segments.size()) return slot;

    Singular* currentOwner = &root;
    Property* currentRegistered = nullptr;
    PropertyValue* currentDynamic = nullptr;
    std::size_t i = startIndex;

    while (i < segments.size()) {
        if (currentOwner) {
            // Traversal from a Singular
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

            if (!foundReg && !foundDyn) {
                return slot; // Path broken
            }

            i += consumed;
            slot.owner = currentOwner;
            currentRegistered = foundReg;
            currentDynamic = foundDyn;
            currentOwner = nullptr;
        } else if (currentDynamic) {
            // Traversal from a PropertyValue (Dict or List)
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
                    // Try to parse segments[i] as int
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
                    // It's neither a Dict nor a List, can't traverse deeper
                    // Wait, what if it's a vec3? We handle trailing component later
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

        // Need to descend further.
        // What is the value we are looking at?
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

        // Check if value is a Singular pointer
        if (Singular** nextSingular = std::get_if<Singular*>(&val)) {
            if (*nextSingular) {
                currentOwner = *nextSingular;
                currentRegistered = nullptr;
                currentDynamic = nullptr;
            }
        } else if (Object** nextObj = std::get_if<Object*>(&val)) {
            if (*nextObj) {
                currentOwner = reinterpret_cast<Singular*>(*nextObj);
                currentRegistered = nullptr;
                currentDynamic = nullptr;
            }
        } else if (Relation** nextRel = std::get_if<Relation*>(&val)) {
            if (*nextRel) {
                currentOwner = reinterpret_cast<Singular*>(*nextRel);
                currentRegistered = nullptr;
                currentDynamic = nullptr;
            }
        } else if (Formation** nextForm = std::get_if<Formation*>(&val)) {
            if (*nextForm) {
                currentOwner = reinterpret_cast<Singular*>(*nextForm);
                currentRegistered = nullptr;
                currentDynamic = nullptr;
            }
        } else if (std::get_if<std::shared_ptr<PropertyDict>>(&val) || std::get_if<std::shared_ptr<PropertyList>>(&val)) {
            // Keep currentDynamic as is, it will be handled in the next loop iteration's `else if (currentDynamic)`
            // Wait, if it was registered, currentDynamic is null! 
            // We need a way to point to the dict/list. But wait, `currentRegistered->value()` returns a COPY of the PropertyValue!
            // We cannot get a pointer to the internal PropertyValue of a Registered Property, because `value()` returns by value!
            // BUT std::shared_ptr is a reference-counted pointer. The copy of the PropertyValue contains a copy of the shared_ptr, which points to the same dict!
            // So we can't easily set `currentDynamic` to point to it... wait, yes we can! If we need to write to the dict, we modify the dict's elements. We don't need to overwrite the shared_ptr itself!
            // However, `currentDynamic` is a `PropertyValue*`. We would need it to point to a persistent `PropertyValue`.
            // If the dict/list is inside a registered property, where does the `PropertyValue*` point to?
            // This suggests registered properties should NOT return Dicts/Lists by value if we want to mutate them deeply. But Dicts/Lists are shared_ptrs, so mutating their elements is fine!
            // For now, assume Dicts/Lists are only in `_dynamicProperties`.
        }

        // Trailing vec3 component?
        if (i == segments.size() - 1) {
            if (std::holds_alternative<glm::vec3>(val)) {
                const std::string& c = segments[i];
                if (c == "x" || c == "y" || c == "z" || c == "r" || c == "g" || c == "b") {
                    slot.prop = currentRegistered;
                    slot.dynamicSlot = currentDynamic;
                    slot.trailingComponent = c;
                    return slot;
                }
            }
        }
    }
    return slot;
}
