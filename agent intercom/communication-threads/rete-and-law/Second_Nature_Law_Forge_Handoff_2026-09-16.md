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

## 2026-09-18 — Astra's experience specification, from Zach's live report

Zach values this PR's code but experienced the Forge as a few buttons stamping gold/blue Laws. He requested a full specification grounded in the hand forming Law and whole, human authorship. Read [The Hand Forms the Law](../../../docs/plans/SECOND_NATURE_LAW_FORGE_EXPERIENCE_SPECIFICATION.md), especially the implementation map, ordered increments, and acceptance matrix; track work in the [Second-Nature task](../../../docs/Agenda/Tasks/Specific%20Tasks/Second_Nature_Law_and_Zone_Features/Second_Nature_Law_and_Zone_Features.md).

For Sol/Jules: preserve the universal seam and native closure; deliver selection → example → timing/reach → isolated rehearsal → keep once → edit same identity → save/re-enter before expanding presets. Inert drafts, structured definition editing with proper invalidation, and rehearsal isolation are missing capabilities to implement, not APIs this spec asserts exist. Existing creation immediately enables the newborn. Existing Zone tests inject click events; add actual input-path witnesses. Preserve Zach's saved Zone; use a development identity or isolated root.

Both existing local test binaries passed (Zone: 28/28); no rebuild or live UI claim. This pass writes documentation only. It corrects earlier documents' unsupported universal undo / exact Prophetic prediction assumptions.

*Codex (GPT-6 Astra), session `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44`, 2026-09-18T12:47:50-07:00.*
