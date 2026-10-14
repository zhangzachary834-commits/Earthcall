# Purge Singular references from PropertyValue

**Status:** open — Zach author decision recorded 2026-10-13.  
**Origin:** o3's property-pointer seam audit in `docs/Analysis/CLOCK_POINTER_BODY_SEAMS_ANALYSIS_2026-10-13.md`, followed by Zach's verdict.

## Author decision

The old idea that a Singular might have a "property of having a Singular" is retired.

A Property is a **predicate / attribute**, not a hidden carrier of another being. If semantic meaning depends on another Singular, that between-being structure belongs in a first-class `Relation` / `Formation`, not in `PropertyValue`.

This is stronger than replacing raw pointers with `SingularId`: an ID stored as a property could preserve the same ontological mistake while merely making it memory-safe.

## Current fossil

`src/ConstructedBeing/Singular/Property/PropertyValue.hpp` still admits:

```cpp
Singular*
Object*
Relation*
Formation*
```

Those alternatives allow identity-bearing references to bypass the explicit Relation graph, and their deliberately non-comparable semantics can also wake change propagation on repeated writes.

## Acceptance

- Remove being-pointer alternatives from `PropertyValue`.
- Audit PropertyList / PropertyDict nesting so beings cannot be smuggled through recursive property containers.
- Find every producer and consumer of those alternatives and classify each use as an attribute, a Relation/Formation, a machine-only transient handle, or dead fossil.
- Replace semantic cross-being references with explicit Relations/Formations.
- Do not replace the pointers with a generic `SingularId` property escape hatch.
- Add a regression guard proving Properties cannot encode a hidden being edge.
- Preserve legitimate non-being machine values and OntoMath values without widening the Property ontology again.
