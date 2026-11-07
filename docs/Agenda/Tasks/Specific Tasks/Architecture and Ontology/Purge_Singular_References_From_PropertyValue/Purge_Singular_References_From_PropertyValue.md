# Purge Singular references from PropertyValue

**Status:** Landed and verified (2026-11-04) — Rung 0 of [`PROPERTY_MEMORY_MICROMASTERY_IMPLEMENTATION_PLAN_2026-11-04.md`](../../../../../plans/PROPERTY_MEMORY_MICROMASTERY_IMPLEMENTATION_PLAN_2026-11-04.md).  
**Origin:** o3's property-pointer seam audit in `docs/Analysis/CLOCK_POINTER_BODY_SEAMS_ANALYSIS_2026-10-13.md`, followed by Zach's verdict.  
**Implemented by:** Antigravity · Gemini 3.8 Flash · session `102da8a2-db47-4627-8ce6-cf1235aeabd4` · 2026-11-04.

## Author decision

The old idea that a Singular might have a "property of having a Singular" is retired.

A Property is a **predicate / attribute**, not a hidden carrier of another being. If semantic meaning depends on another Singular, that between-being structure belongs in a first-class `Relation` / `Formation`, not in `PropertyValue`.

This is stronger than replacing raw pointers with `SingularId`: an ID stored as a property could preserve the same ontological mistake while merely making it memory-safe.

## Landed Implementation & Verification

- Removed `Singular*`, `Object*`, `Relation*`, and `Formation*` from `PropertyValue` variant (`src/ConstructedBeing/Singular/Property/PropertyValue.hpp`).
- Verified constitutive sub-Singular traversal via `Property::asSingular()` (`PropertyRef::asSingular()` and `ComputedProperty::asSingular()`) remains completely intact without storing pointers in `PropertyValue`.
- Cleaned up touchpoints in `PropertyValueJson.cpp`, `PropertyPath.cpp`, `ActionModel.cpp`, `ObjectConcept.cpp`, `LawGraphWindow.cpp`, and `Zone.cpp`.
- Added compile-time regression guard in `tests/singularity/property_memory_access_test.cpp`:
  ```cpp
  static_assert(!is_property_value_alternative<Singular*>);
  static_assert(!is_property_value_alternative<Object*>);
  static_assert(!is_property_value_alternative<Relation*>);
  static_assert(!is_property_value_alternative<Formation*>);
  ```
- All tests passed: `property_memory_access_test`, `dynamic_property_reachability_test`, `law_creation_test`, `object_concept_test`, `no_black_box_test`, `property_path_precalc_test`. Save files remained 100% clean.
