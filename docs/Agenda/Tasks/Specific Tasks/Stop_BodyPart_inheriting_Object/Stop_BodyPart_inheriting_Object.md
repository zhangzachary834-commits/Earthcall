# Stop BodyPart inheriting Object (Refusal #4)

## Context & Motivation
Under Refusal #4 and the Kind Floor rules (`docs/architecture/ontology/NEW_KIND_FRAMEWORK.md` §2):
> "A `Body` is the representation of an embodied *someone*. Objects have visual components — geometry, fields, materials. A robot arm has no Body."
> "The human form. `BodyPart` and constitutive members for the `Person` vessel are invariant ontological structures, not domain nouns, and thus admitted in C++."

Previously, `class BodyPart` inherited both from `Object` and `Formation` (`class BodyPart : public Object, public Formation`). This was an ontological contradiction:
1. An `Object` represents an authored domain or visual entity, whereas `BodyPart` represents an invariant constitutive member of a Person's bodily vessel.
2. Inheriting from `Object` leaked domain object assumptions into the human form and vice versa (e.g. `Object.hpp` holding pointers and forward declarations to `BodyPart`).

## Implementation Summary
1. **Inheritance & Hierarchy**:
   - `BodyPart` inherits directly and purely from `Formation` (`class BodyPart : public Formation`), which in turn is a `Singular`.
   - Constitutive limbs (`Arm`, `Torso`, `Leg`, `Head`, `Foot`, `Hand`, `Neck`, etc.) inherit from `Limb` / `BodyPart`, remaining cleanly inside the `Person/Body/` ontology.
2. **Visual Components via Composition**:
   - `BodyPart` owns its primary geometry as a member `Object` (`std::unique_ptr<Object> _primaryObject`) and manages composite sub-objects (`std::vector<std::unique_ptr<Object>> _subObjects`).
   - Spatial transformation, shape changes, face painting, and automations delegate cleanly to the underlying `Object` instances while `BodyPart` functions as their parent `Formation`.
3. **Decoupling from Object**:
   - Removed forward declarations and residual references to `BodyPart` in `src/ConstructedBeing/Singular/Object/Object.hpp`.
   - Replaced dead `dynamic_cast<BodyPart*>(obj)` code in `Tool.cpp` with proper formation / CreatorConsoleState body part resolution.
   - Cleaned up serialization type conversions (`ObjectTypes::ShapeKind`) in `Serialization.cpp`.

## Verification
- Full test suite built (`earthcall_webgpu`, tests).
- `substrate_split_test` verified and passed (28/28 checks).
- Invariants checked: 0 regressions, 0 broken invariants.
