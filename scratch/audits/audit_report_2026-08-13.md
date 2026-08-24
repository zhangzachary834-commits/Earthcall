# Earthcall Architecture Audit Report

**Date:** 2026-08-13  
**Auditor:** Mistral Vibe CLI Agent  
**Scope:** Full codebase audit against the Six Refusals and architectural principles  

---

## Executive Summary

**Status: CRITICAL VIOLATIONS FOUND**

The codebase has **one critical architectural violation** that must be addressed immediately. 
All other audited areas comply with the architectural principles. The test suite passes 
(44/45, with the expected deliberate failure), and the build is clean.

---

## Findings

### CRITICAL VIOLATIONS

#### 1. EventEntity - Refusal #1 Violation (KIND FLOOR)

**File:** `src/Singularity/Core/EventEntity.hpp` / `.cpp`

**Violation:** 
- `EventEntity` is a subclass of `Singular` for a **domain noun** (event)
- This violates **NEW_KIND_FRAMEWORK.md §6 (The Kind Floor), Rule 1:**
  > No new subclass of `Singular` or `Object` for a domain noun

**Supporting Evidence:**
- The class comment states: "Promoted Event to a Singular so it can exist natively in the 
  physics graph and have relations/properties applied to it by the Law system"
- This is exactly the wrong approach. Events should be authored in-world as data (Relations 
  with event-type edges), not as C++ classes
- The architecture explicitly rejects this pattern in the robotics example:
  > A robot is not a new kind of being... it is Objects + Relations + Formation + 
  > ObjectConcept + Laws

**Additional Violation:**
- EventEntity uses **generated UUIDs** for identifiers (line 10-14 of EventEntity.cpp)
- This violates the stable identifier requirement from NO_BLACK_BOX.md:
  > Generated ids (`law-7`) change between runs. Any being that law-text names must 
  > override `getIdentifier()` with a stable slug
- EventEntity is added to Universe (line 15 of EventEntity.cpp: 
  `Universe::instance().addActiveEvent(this)`), meaning it IS addressable by laws
- Law.cpp subscribes to Custom events and adds them to `_activeCustomEvents` 
  (line 1441) and Universe (line 1443)

**Correct Approach:**
Events should be represented as:
- **Relations** with a `type` field set to the event type (e.g., `"landed"`, `"jump-started"`)
- The existing ECA::Event struct already handles this pattern correctly
- Custom events from the EventBus should be translated to Relations, not wrapped in EventEntity

**Severity:** CRITICAL - This is an ontological schism

**Action Required:** Remove EventEntity class entirely. Replace with Relation-based event representation.

---

### MEDIUM PRIORITY ISSUES

#### 2. Lexeme - Unstable Identifier

**File:** `../../src/ConstructedBeing/Singular/Lexeme/Lexeme.cpp`

**Issue:** Lexeme uses generated UUIDs for identifiers (lines 10-14)

**Violation:** Potentially violates stable identifier principle if Lexemes are ever 
addressed by law text.

**Current Status:** No evidence of law text addressing Lexemes (`@lexeme.*` not found in saves)

**Context:** Lexeme IS one of the eight structural categories in BeingKind enum, so it's 
ontoologically legitimate. However, the use of generated IDs is inconsistent with the 
principle that beings addressable by law must have stable identifiers.

**Recommendation:** If Lexemes might be law-addressable in the future, change to stable 
identifiers (e.g., `"lexeme.<symbol>"`). If they will never be law-addressable, this is 
acceptable but should be documented.

**Severity:** MEDIUM - Preventative

---

### PASSING CHECKS

#### Directory Structure (Refusal #2)
✅ **PASS** - No top-level domain directories exist
- `src/` contains only ontological directories: ConstructedBeing, Identity, Legacy, 
  OurVerse, Person, Relation, Singularity, ZonesOfEarth
- Physical modality channel correctly placed at `Singularity/Physical/`

#### Enum Values (Refusal #3)
✅ **PASS** - BeingKind enum is append-only
- Values 0-8 are the eight structural categories + AnyBeing
- Values 12-13 are retired pair quantifiers (removed from Kind enum but documented)
- No burned values are reused

#### Body Usage (Refusal #4)
✅ **PASS** - Body reserved for Persons only
- `Person/Body/Body.hpp` defines Body as part of human form
- No evidence of Body being used for non-Person entities
- Proper exception documented in NEW_KIND_FRAMEWORK.md

#### Person Usage (Refusal #5)
✅ **PASS** - Person means Human only
- Person requires Soul and Body (human components)
- No AI agents or non-human entities modeled as Person
- All Person constructions involve human identity

#### No Black Box (Refusal #6)
✅ **PASS** - no_black_box_test passes
- All audited beings register properties correctly
- EventEntity registers: eventType, sourceId, targetId
- PhysicalChannel registers: enabled
- Sealed register beings (World, Ourverse, Formation, Soul) are documented debt
- Write exemptions (Object::rotation, Object::face.*.activeLayer) are documented

#### Build System
✅ **PASS** - Clean build
- cmake configure and build complete without errors
- All source files properly included

#### Test Suite
✅ **PASS** - 44/45 tests pass
- Expected deliberate failure: webgpu_particle_test (PENDING_FEATURE_TESTS)
- All architectural guard tests pass:
  - paint_test
  - object_roundtrip_test
  - channel_paths_test
  - no_black_box_test

#### Other Singular Subclasses
✅ **PASS** - All legitimate
- **Material**: Ontological primitive for appearance (AUTHORED_CATEGORIES.md §4)
- **FieldNode**: Bridge between OntoMath and spatial world (legitimate channel)
- **Formation**: Ontological category
- **Relation**: Ontological category
- **Object**: Ontological category
- **Zone**: Ontological category
- **World**: Ontological category
- **Ourverse**: Ontological category
- **Lexeme**: Ontological category (structural)
- **Person**: Ontological category
- **Soul**: Part of Person ontology (human form exception)
- **Perspective**: Uninstantiable stub

---

## Root Cause Analysis

The EventEntity violation appears to be a well-intentioned but architecturally incorrect 
attempt to make events "first-class" in the ontology. The developer recognized that events 
need to participate in the property/relation system but chose the wrong mechanism 
(new C++ class) instead of the correct one (authored Relations).

This is exactly the pattern that NEW_KIND_FRAMEWORK.md was written to prevent. The 
document's own example (the robot) shows that domain concepts should be composed from 
existing ontological primitives, not added as new C++ types.

---

## Recommendations

### Immediate (Blocker)
1. **Remove EventEntity** - Delete `src/Singularity/Core/EventEntity.hpp` and `.cpp`
2. **Update EventBus** - Remove Event::Custom wrapper or translate to Relation-based events
3. **Update Law.cpp** - Remove _activeCustomEvents and associated handling
4. **Update Universe** - Remove addActiveEvent/removeActiveEvent methods

### Short-term
1. **Lexeme identifiers** - Consider changing to stable identifiers if law-addressable
2. **Soul properties** - Add ComputedProperty for _identity with null setter (per no_black_box_test comment)

### Documentation
1. Add EventEntity removal to commit messages as architectural correction
2. Update any documentation that references EventEntity

---

## Verification

Run the following to verify fixes:
```bash
# Build and test
cmake -S . -B build ... [flags]
cmake --build build -j8
ctest --test-dir build --output-on-failure -j4

# Should show 44/45 pass (webgpu_particle_test Not Run)

# Check no EventEntity references remain
grep -r "EventEntity" src/ --include="*.cpp" --include="*.hpp"
# Should return no results
```

---

## References

- NEW_KIND_FRAMEWORK.md §1-3 (The seams, Admission Test, Composition Ladder)
- NEW_KIND_FRAMEWORK.md §6 (The Kind Floor)
- NO_BLACK_BOX.md §3 (Admission Test), §4 (Sealed Register)
- AUTHORED_CATEGORIES.md §2 (The shape - category as Relation graph)
- BUILD_AND_ENVIRONMENT.md (Test expectations)
- ENGINEERING_DISCIPLINE.md (End-to-End Coherence, Integrity Check)
