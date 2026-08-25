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
#include "Relation/Formation/Formation.hpp"

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
// There is no `namespace ObjectComposition` of free-function accessors here.
//
// There was one — ~95 lines mirroring every member of the struct above — and
// nothing ever called it. `Object` reads `_composition` directly (Object.hpp)
// and defines its real methods in ObjectComposition.cpp. It took ONE commit for
// the unused copy to drift: its `addTag` asked `hasAttribute(state, tag)` — the
// key→value map — instead of `hasTag`, so it appended duplicate tags and
// silently dropped a tag whose name matched an existing attribute. The live
// `Object::addTag` was correct the whole time.
//
// Two ways to say the same thing is how that happens. The state lives in the
// struct; the behavior lives on Object. If a caller needs composition behavior,
// call the Object method — do not reintroduce a parallel surface.
// ============================================================================
