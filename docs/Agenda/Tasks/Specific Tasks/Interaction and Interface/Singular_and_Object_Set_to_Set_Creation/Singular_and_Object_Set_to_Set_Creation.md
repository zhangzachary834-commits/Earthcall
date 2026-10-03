# Singular and Object Set-to-Set Creation

**Status**: Composition rung verified (2026-08-16); universal prototype creation partially implemented (2026-10-02).
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

## Universal prototype birth — 2026-10-02

*Codex / GPT-6 / session `01a0e64f-5853-7d30-8196-995b4fd16b89` /
2026-10-02 17:02 PDT. Zach asked Create and set-to-set creation to admit
every existing Singular subclass except Persons, First Movers, and other
real-world correspondents; he explicitly required ambiguity to remain open.*

The existing `Create` Action kind now accepts a prototype `path`. Its resolved
Singular determines the concrete constructor adapter; no new domain class,
category enum, or per-kind opcode was introduced. The mechanical `codec` tags
in persisted records select existing C++ constructors and are not ontology
categories. `Synthesize` can compose these exact actions. `Spawn` remains the
legacy ObjectConcept route; an empty-path Create remains the existing shaped
Object route. The existing Law Graph Create editor exposes the prototype path,
fresh identity/name, and explicit Relation participants; conflicting legacy
Object settings remain visible and require the Person to clear them.

```cpp
auto action = ActionNode::createFrom("@lexeme.source-hope", "lexeme.authored-hope",
    {ActionNode::addProperty("", "meaning", std::string("gift"))});
auto persistedAction = action.toJson();
```

Use the exact ActionNode JSON emitted by the factory/compiler; serialized
enum values and operand envelopes are fixed by `ActionModel.hpp` and
`ActionNode::toJson`.

| Existing concrete kind | Implemented boundary |
|---|---|
| Object | Existing Zone Object storage; new identity and present authorship; a composed prototype refuses until member custody is explicit. |
| Law | Existing LawManager/active Zone closure; executing authors are retained; reviewed C++ authority is not inherited. |
| Lexeme | Same physical instance enters the language identity index and destination Zone; symbol collisions remain ambiguity. |
| Material | Existing `material.<name>` identity contract and MaterialManager adapter. |
| Formation / Community | New container identity; references to actual members and enduring Relations; Community members remain actual Persons. |
| Relation | Explicit participant tokens (`containerToken`, `elementToken`) or Request endpoints; duplicate enduring bonds refuse; constitutive assertions are checked. |
| Moment | New stable authored identity independent of time coordinates; exact temporal forms persist. Legacy unnamed Moments retain their coordinate identifiers. |
| ObjectConcept | Existing recipe and inherited Object codec; captured state without an exact codec refuses. |
| FieldNode | Existing Zone spatial-field storage and exact OntoMath codec. |

Generic retained beings are exposed at `storedSingulars` and persisted with
their destination Zone. This is machine lifetime custody, **not** the authored
Zone–bearer–PropertyPath custody Relation from the memory micromastery task.
The staged loader resolves graph participants before admission and refuses
missing or ambiguous participants. Typed authored references preserve their
pointer alternative and resolve in the restored Zone. Derived Relation views
retain actual bonds without inventing an interaction or changing their weight.

Whole-prototype birth consults the existing TransferPolicy. A closed gate
currently causes a conservative refusal: no gate is silently opened and no
protected field is omitted. Whether an authored replacement may permit that
birth remains Zach's open decision; exact per-codec read footprints remain
work. Ordinary unlisted properties retain TransferPolicy's open default.

Shared/cyclic list or dictionary memory refuses rather than flattening alias
topology. Do not confuse this adapter with general memory-cell birth or
cross-Zone shared-cell persistence. Legacy shaped Create has not been migrated
through the prototype transfer preflight.

### Open authorial decisions and next implementation seams

- Zone/Home prototype birth needs an explicit owner-inheritance decision.
- Event/Utterance birth needs the authored-occurrence versus witnessed-occurrence decision.
- Timeline birth needs a decision about reference versus birth of its contained Moments.
- Ourverse: local birth is permitted; global birth is forbidden; global means the entire continuous machine-wide space. The authored extent representation is unspecified, so both remain refused by this adapter. See [Ourverse](../../../../../architecture/ourverse/OURVERSE.md).
- Copying protected state needs the refusal-versus-explicit-replacement decision; do not infer an authorization from source authorship.
- Ordinary Objects need the authored Relation identifying real-world correspondence before the correspondent refusal can cover them; the old visual `physicalObject` flag is not proof of such correspondence. Known Person/constitutive-body/First-Mover/machine-channel classes already refuse.
- Cyclic references between generic stored beings need a staged shell/restoration codec; the current dependency loader refuses unresolved cycles.
- Universal unmaking remains open: the existing Destroy action explicitly admits only Objects and Laws; new constructor adapters do not authorize deletion of enduring Relations or human correspondents.
- Person verification of the prototype authoring control and broader universal admission of authored subclasses remain open; unknown subclasses refuse instead of being sliced into a base class.

### Verification for this pass

`earthcall_webgpu` and the focused test targets built successfully. The
universal test covers save-record order independence and confirms that a
Formation referencing a separately stored Relation restores to that same
physical Relation, with history and weight preserved. Executed regression
results are recorded below; no inhabited save
was edited and no live Person witness is claimed.

Ten headless tests passed: `universal_singular_creation_test`,
`second_nature_law_authoring_test`, `singular_set_to_set_test`, `law_model_test`,
`moment_test`, `timeline_test`, `lexeme_graph_roundtrip_test`,
`zone_relation_roundtrip_test`, `zone_spatial_field_roundtrip_test`, and
`unsaved_preserve_test`. The universal test was rerun after the final codec
tag/key distinction fix. This is focused verification, not a full-suite verdict.

The GLFW-dependent `formation_merge_test`, `formation_topology_test`,
`prophetic_rete_test`, and `no_black_box_test` passed with desktop access
(the sandboxed GLFW attempt was interrupted). Editor interaction remains a
Person check in [the verification list](../../../For%20Zach/Person%20Verification%20List.md).
