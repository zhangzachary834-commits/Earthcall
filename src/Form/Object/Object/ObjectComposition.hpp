#pragma once

// ============================================================================
// ObjectComposition.hpp - Composition-related data for Object
//
// This header contains composition-related data structures used by Object,
// including element membership (Formation), attributes, tags, and legacy
// composition properties.
// ============================================================================

#include <vector>
#include <string>
#include <unordered_map>
#include "../Formation/Formation.hpp"

class Singular;

// Forward declaration
class Object;

// ============================================================================
// Composition State
// ============================================================================

// Composition state for an Object - tracks what it's made of and how it relates
struct ObjectCompositionState {
    // --- Element composition ---
    // The beings this object is composed of. Elements are held in a Formation,
    // which maintains the structure among them (relations) and makes the composition
    // addressable. Ownership: the Formation holds NON-OWNING pointers, exactly like
    // every other Formation. A newborn created by a law is owned by the World; its
    // element membership is a second, relational fact about it.
    Formation _elementFormation;

    // Transient load state: element identifiers read from a save, waiting for
    // the rest of the world to exist before they can be re-linked into the
    // element Formation (World's from_json does the pass, then clears this).
    std::vector<std::string> pendingElementIds;

    // --- Legacy properties (kept for save/load compatibility) ---
    // The legacy descriptive count (vestigial: set by hand, read by nobody).
    // The TRUTH of what this object is made of is the element Formation above.
    int corners = 0;
    int faces = 0;
    int massQuantity = 0;
    int elements = 0;
    int relationships = 0;
    int complexityLevel = 0;
    bool physicalObject = true;

    // --- Attributes and Tags (for selection/filtering by physics laws, etc.) ---
    std::unordered_map<std::string, std::string> attributes;
    std::vector<std::string> tags;
};

// ============================================================================
// Inline Accessors for ObjectCompositionState
// ============================================================================

namespace ObjectComposition {

// Element composition accessors
inline Formation& elementFormation(ObjectCompositionState& state) {
    return state._elementFormation;
}

inline const Formation& elementFormation(const ObjectCompositionState& state) {
    return state._elementFormation;
}

inline void addElement(ObjectCompositionState& state, Singular* s) {
    if (!s) return;
    state._elementFormation.addMember(s);
}

inline bool removeElement(ObjectCompositionState& state, Singular* s) {
    if (!s || !state._elementFormation.hasMember(s)) return false;
    state._elementFormation.removeMember(s);
    return true;
}

inline bool hasElement(const ObjectCompositionState& state, const Singular* s) {
    return s && state._elementFormation.hasMember(s);
}

inline int elementCount(const ObjectCompositionState& state) {
    return static_cast<int>(state._elementFormation.getMembers().size());
}

// Legacy property accessors
inline int getCorners(const ObjectCompositionState& state) { return state.corners; }
inline void setCorners(ObjectCompositionState& state, int c) { state.corners = c; }

inline int getFaces(const ObjectCompositionState& state) { return state.faces; }
inline void setFaces(ObjectCompositionState& state, int f) { state.faces = f; }

inline int getMassQuantity(const ObjectCompositionState& state) { return state.massQuantity; }
inline void setMassQuantity(ObjectCompositionState& state, int m) { state.massQuantity = m; }

inline int getElements(const ObjectCompositionState& state) { return state.elements; }
inline void setElements(ObjectCompositionState& state, int e) { state.elements = e; }

inline int getRelationships(const ObjectCompositionState& state) { return state.relationships; }
inline void setRelationships(ObjectCompositionState& state, int r) { state.relationships = r; }

inline int getComplexityLevel(const ObjectCompositionState& state) { return state.complexityLevel; }
inline void setComplexityLevel(ObjectCompositionState& state, int cl) { state.complexityLevel = cl; }

inline int getPhysicalObject(const ObjectCompositionState& state) { return state.physicalObject ? 1 : 0; }
inline void setPhysicalObject(ObjectCompositionState& state, int po) { state.physicalObject = (po != 0); }

inline int getSymbolicObject(const ObjectCompositionState& state) { return state.physicalObject ? 0 : 1; }
inline void setSymbolicObject(ObjectCompositionState& state, int so) { state.physicalObject = (so == 0); }

// Attribute and Tag accessors
inline void setAttribute(ObjectCompositionState& state, const std::string& key, const std::string& value) {
    state.attributes[key] = value;
}

inline bool hasAttribute(const ObjectCompositionState& state, const std::string& key) {
    return state.attributes.find(key) != state.attributes.end();
}

inline const std::string& getAttribute(const ObjectCompositionState& state, const std::string& key) {
    static const std::string empty;
    auto it = state.attributes.find(key);
    return it == state.attributes.end() ? empty : it->second;
}

inline const std::unordered_map<std::string, std::string>& getAttributes(const ObjectCompositionState& state) {
    return state.attributes;
}

inline void addTag(ObjectCompositionState& state, const std::string& tag) {
    if (!hasAttribute(state, tag)) state.tags.push_back(tag);
}

inline void removeTag(ObjectCompositionState& state, const std::string& tag) {
    state.tags.erase(std::remove(state.tags.begin(), state.tags.end(), tag), state.tags.end());
}

inline bool hasTag(const ObjectCompositionState& state, const std::string& tag) {
    for (const auto& t : state.tags) if (t == tag) return true;
    return false;
}

inline const std::vector<std::string>& getTags(const ObjectCompositionState& state) {
    return state.tags;
}

inline std::vector<std::string>& pendingElementIds(ObjectCompositionState& state) {
    return state.pendingElementIds;
}

} // namespace ObjectComposition
