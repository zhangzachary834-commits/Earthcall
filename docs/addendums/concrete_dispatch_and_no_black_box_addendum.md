# Integrating Concrete Dispatch and the No Black Box Principle

In Earthcall's creation pipeline, resolving how a new being comes into existence must reconcile two opposing forces: the absolute necessity of maintaining invariant C++ structural bounds (e.g., that a `Person` truly represents a human and a `Body` is their invariant vessel) and the strict No Black Box Principle (Refusal 6), which demands that state and meaning remain accessible to authored Law via property paths rather than hidden in ungovernable native code.

This tension is resolved in `SingularSetToSetCreation::derive` (`src/ConstructedBeing/Singular/Creation/SingularSetToSetCreation.cpp`). Before any generic property-based evaluation occurs, the derivation process relies on concrete C++ `dynamic_cast` dispatch.

## The Concrete Dispatch Gateway

When a prototype is evaluated for derivation, the system checks its concrete C++ kind:

```cpp
if (dynamic_cast<const Person*>(&being) || dynamic_cast<const Body*>(&being) ||
    dynamic_cast<const BodyPart*>(&being) || dynamic_cast<const Soul*>(&being) ||
    dynamic_cast<const PersonPerspective*>(&being)) {
    // ... handles invariants directly ...
}

if (dynamic_cast<const Identity::FirstMover*>(&being) ||
    dynamic_cast<const Identity::FirstMoverRegister*>(&being)) {
    // ... handles invariants directly ...
}
```

This dispatch operates outside and *before* the generic property checks administered by `TransferPolicy::instance().canTransfer`. By resolving these invariants first, Earthcall ensures that the fundamental structure of reality—the human vessel (`Person`, `Body`, `Soul`) and the authoring conduit (`FirstMover`)—cannot be subverted, duplicated, or synthesized by a malformed prototype, regardless of what properties that prototype claims to carry.

## Relation to the No Black Box Principle

At first glance, hardcoding rules for `Person` and `Body` might seem to violate the No Black Box Principle, which forbids subsystems from defining what a thing's state means by keeping it hidden. However, the concrete dispatch does not hide state; it guarantees structural *form*.

Once the concrete dispatch confirms the legitimate structural derivation of the entity (or refuses it if invalid), the generic pathway takes over. `TransferPolicy::instance().canTransfer(PropertyPath::parse(...))` applies the No Black Box Principle directly:

1. **Concrete Invariants are Settled:** The "what" (this is a valid Body) is resolved by C++ structure.
2. **State is Governed by Law:** The "how" and "why" (the properties attached to that Body) are passed through the Transfer Policy, which evaluates paths entirely in the open.

There are no hidden permission levels. The C++ code doesn't secretly copy fields behind the back of the Law system; it uses concrete dispatch solely to assert the invariant structural boundaries, then immediately hands authority for all readable and writable state back to the registered property paths and the Transfer Policy.

This integration proves that Earthcall can maintain absolute C++ invariants for its core human and temporal structures (Refusal 1 exceptions) without ever resorting to black-box state manipulation, perfectly honoring the transparency demanded by Refusal 6.
