# Second-Nature Law Forge / Universal Singular Creation Handoff — 2026-09-16

From: GPT-5.6 Sol  
Branch: `sol/second-nature-law-authoring-zone-20260916`

## Zach's governing correction

Do **not** implement Law creation as a Law-specific factory, and do not replace `ObjectConcept` with a new `SingularConcept` C++ noun.

Zach's repository directives are explicit:

- Singular set-to-set creation is intended to be the universal creation operation for every coherent child kind of `Singular`, rather than adding a new ActionKind/opcode for each type.
- creating a Law through Laws should therefore be a case of Singular creation, not `CreateLaw`.
- `ObjectConcept` should retire into the deeper idea that the "concept" is literally an ordinary Singular from which other Singulars branch.
- Person (and analogous kinds whose identity is grounded in an actual real-world being) must not be synthesized merely because the type inherits `Singular`.
- Relations also retain their constitutive rule: there is no empty Relation; creating one means creating an actual interaction between participants.

## Landed substrate

`SingularSetToSetCreation::derive` is now the sole public derivation seam used by the new Law authoring adapter. It currently has persistence-boundary support for authored Law and plain Object prototypes, with loud refusal for un-wired concrete runtime kinds instead of slicing them into Object.

This is intentionally a transitional rung. C++ still needs concrete constructor/persistence knowledge somewhere; that knowledge must converge into the persistence codec layer behind this one operation, not blossom into independent author-facing creation systems.

`SecondNatureLawAuthoring` is now deliberately thin. It resolves an authored interface request and delegates to the universal operation.

## Authored Zone

`SecondNatureLawForge` is a native Zone whose interface is authored state:

- Shape2D instrument Objects,
- authored invocation Laws,
- an ordinary state Object carrying selected prototype/target/status,
- disabled ordinary Law prototypes carrying `$TARGET` in their explicit Law text.

Clicking an instrument selects a prototype and publishes `law-authoring-instrument-invoked`; the substrate derives a normal enabled Law. The newborn is not a UI token and survives as ordinary Law text/provenance/triggers.

## Non-negotiable next step

The deepest remaining task is to route serialized `ActionNode::Create` / set-to-set `Synthesize` through this universal Singular creation operation and then extend the shared persistence codec coverage to the other birthable Singular kinds. Do not add `CreateLaw`, `CreateLexeme`, etc. `AuthorZone` / `AddRelation` should be reviewed as historical specialized opcodes: if their semantics are merely creation, fold them into the universal operation; if they encode constitutive invariants (Relation endpoints, Zone ownership), preserve those as constraints/data on universal creation rather than separate ontologies.

## Tests

`second_nature_law_authoring_test` now intentionally proves two kinds through the same operation: Law and Object. Keep that cross-kind assertion.
