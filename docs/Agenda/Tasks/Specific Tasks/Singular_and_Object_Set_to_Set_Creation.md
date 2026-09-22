# Singular and Object Set-to-Set Creation

**Status**: Completed and verified (2026-08-16)  
**Related**: `LawGraphWindow.cpp`, `tests/singular_set_to_set_test.cpp`

## Summary of Implementation
- Exposed all 19 `ActionNode::Kind` values (`Create`, `AddProperty`, `AddElement`, `RemoveProperty`, `RemoveElement`, `Destroy`, `Synthesize`, `PlayAudio`) in the Law Editor dropdown in `LawGraphWindow.cpp` (previously limited to kinds 0–10).
- Reworked serialized kind 17 (`Synthesize`) from a monolithic `ObjectConcept`/Object-only shortcut into a visible composition marker:
  - Child `Create` actions birth objects.
  - Child `Set`, `AddProperty`, `AddElement`, and `Map` actions shape the newborn.
  - `Map` reads live event input sets through `@event.subject` / `@event.object` `PropertyPath`s, ensuring derived values remain authored OntoMath rather than concept machinery.
- Law Editor authors `Create`/`Sequence` steps instead of selecting a concept.
- Historical kind-17 concept JSON is preserved on save but refuses loudly until re-authored to prevent silent behavior or data loss.

## Verification
- Verified with `singular_set_to_set_test`: a composed `Synthesize` creates a newborn, grants it a property, and maps the event subject's `position.x` into that property. Confirmed JSON serialization contains the action tree instead of a concept ID.
- Added 3 direct test cases for `ActionNode::Kind::Synthesize`:
  1. Empty-children `Synthesize` refuses and births nothing.
  2. Composed `Synthesize` with two `Create` children births two distinct newborns.
  3. `Map` bound to `@event.object` reads the other event participant.
- Test suite passing: 45/45 (with `webgpu_particle_test` as the standing deliberate failure).
