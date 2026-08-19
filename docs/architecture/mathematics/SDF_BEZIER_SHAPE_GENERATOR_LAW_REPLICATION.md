# SDF, Complex, and Bézier Patch Shape Generator Law Replication

**Status as of 2026-08-19.** Zach's note in Phase 2 — OntoMath must host the SDF
and the Bézier, and expose the variables as properties — is the part that
landed (six-rung unification, 2026-08-18). What did **not** land as written
here:

- Tool / placement state lives on `Singularity::Core::CreationChannel`, not
  `@person`. A Person is not a Person's tool (refusals audit). `cameraPos` /
  `cameraForward` stay on the Person.
- The live click event is `onMouseClicked` with subject the channel. This
  document's `person-clicked-mouse` and the Law Graph picker's `mouse-clicked`
  are two other names for the same click. Unification of that vocabulary is
  still open (Shape Generator 3D law audit 1.4).
- The Person-facing spawn law is `shape-generator-3d-law`, not
  `law-shape-generator`. Hardcoded `Tool::ShapeGenerator3D` is still the
  developer bypass (rung R6 not deleted).

Math architecture: [`ONTOMATH_FRAMEWORK.md`](ONTOMATH_FRAMEWORK.md),
[`Geometry_OntoMath_Remaining_Rungs.md`](../../Agenda/Tasks/Specific%20Tasks/Geometry_OntoMath_Remaining_Rungs.md).
Person-facing path: [`SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md`](../../audits/SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md).

## 1. Executive Summary & Ontological Mandate

This document specifies the architecture and implementation for replicating the hardcoded SDF / complex shape / Bézier patch generator into Earthcall's native **Law, MetaLaw, and Singular Set-to-Set Creation** systems.

In accordance with `AGENTS.md`, `NEW_KIND_FRAMEWORK.md`, `NO_BLACK_BOX.md`, `LAW_AND_CREATION_SYSTEM.md`, and `LAW_MIGRATION_FRAMEWORK.md`:
1. **No new C++ classes for domain nouns:** No `BezierGenerator` or `SdfSpawner` classes in C++. All shape generator logic and configurations are authored in-world as data (`ObjectConcept`, `Law`, `MetaLaw`).
2. **No Black Box:** All interaction and placement states (cursor hit point, camera vectors, tool selection, grid snap size, placement mode) must be registered as legible `PropertyPath`s. Camera facts stay on the Person; tool / placement state is on `CreationChannel`.
3. **`ObjectConcept` is the ONE Set-to-Set Machine:** Geometry templates, Bézier control nets, SDF field trees, and member transforms are authored as `ObjectConcept` instances, not procedural C++ factory functions.
4. **Law Authorship & Authority:** Every generation law must have authored provenance (`authoredBy = person.getIdentifier()`).
5. **Edges, Not Levels:** Triggers fire on discrete, past-tense event transitions (`person-clicked-mouse`), not per-frame polling.

---

## 2. Migration Ladder (R0 to R5)

```
[R0: Opaque C++ Generator]
       │
       ▼
[R1: State Legibility] ───► Expose cursor, camera, mode, & transform math as PropertyPaths on Person
       │
       ▼
[R2: Event Audibility] ───► Publish 'person-clicked-mouse' & tool transitions on EventBus
       │
       ▼
[R3: Template Capture] ───► Author 'concept-bezier-patch' & 'concept-complex-sdf' ObjectConcepts
       │
       ▼
[R4: Law Displacement] ──► Construct 'law-shape-generator' with ActionNode::Kind::Spawn
       │
       ▼
[R5: MetaLaw Governance] ─► Author MetaLaws for rate-limits, zones, & TransferPolicy gates
       │
       ▼
[R6: Native / C++ Delete] ─► Remove hardcoded placement branches from Tool.cpp
```

---

## 3. Detailed Architecture

### Phase 1: State Legibility & Audibility (Rungs R1 & R2)
- **Person** (facts about where the Person is looking from):
  - `@person.cameraPos` (`vec3`), `@person.cameraForward` (`vec3`)
- **CreationChannel** (the tool; identifier `creation-channel`):
  - `@creation-channel.active3DMode` (`std::string`: `"Create"` arms the spawn law)
  - `@creation-channel.activeShapeKind` (`int`)
  - `@creation-channel.placementMode` (`std::string`: `"InFront"`, `"CursorSnap"`, `"ManualDistance"`)
  - `@creation-channel.gridSnap` / `gridSnapSize`
  - `@creation-channel.cursorHitPos`, `cursorSpawnPos`, `cursorSpawnTransform` (computed)
  - `@creation-channel.activeColor`
- **Events:**
  - `person-clicked-mouse` (subject: `@person`, button: 0)
  - `tool-selected` (subject: `@person`, tool: string)
  - `placement-mode-changed` (subject: `@person`, mode: string)
- **Objects**
  - (add properties here)

### Phase 2: Set-to-Set Template Authoring via `ObjectConcept` (Rung R3)
- **`concept-bezier-patch`:**
  - `MemberTemplate`: `ShapeKind::Patch` (bicubic Bézier control point grid $[P_{ij}]_{i,j=0}^3$, UV domain bounds $[0, 1]^2$).
  - Material: `material.clay` (diverges on first modification via `ownMaterial`).
  - `TransferPolicy`: Kernel/Governable tier.
- **`concept-complex-sdf`:**
  - `MemberTemplate`: `ShapeKind::SdfField` (composite `geom::SdfNode` tree, smooth-min/union blends).
  - `PropertyMapping`: Derivation transforms mapping scale/curvature.

NOTE from Zach: This is not sufficient by itself. If a shape uses an implicit SDF to calculate its surface, we need OntoMath to actually host that SDF
and expose the OntoMath variables as properties so the Person can physically modify the properties.
Beizer patch likewise should use OntoMath. No black boxes.

**(2026-08-19) That hosting is what the six-rung unification did.** SDF primitives
and CSG are `MathNode` factories; Bézier is three `ScalarForm`s; `field.*` /
`patch.*` / `field.ast` are registered. The remaining black box on this
document is the Person-facing *gesture* (audit §2), not the math.

### Phase 3: Shape Generation Law Authoring (Rung R4)
- **`law-bezier-generator`:**
  - Author: `@person`
  - Trigger: `person-clicked-mouse`
  - Condition: `@person.activeTool == "bezier-patch-generator" && @person.canAuthor == true`
  - Action: `ActionNode::Kind::Spawn` referencing `@concept-bezier-patch` with placement `@person.cursorSpawnTransform`.
- **`law-complex-sdf-generator`:**
  - Trigger: `person-clicked-mouse`
  - Condition: `@person.activeTool == "complex-sdf-generator"`
  - Action: `ActionNode::Kind::Spawn` referencing `@concept-complex-sdf`.

### Phase 4: MetaLaw Governance & Higher-Order Rules
- **`metalaw-shape-cooldown`:** Dynamic rate limiting by toggling `@law-bezier-generator.enabled`.
- **`metalaw-zone-grid-enforcement`:** Zone policy enforcing `@person.placementMode := "CursorSnap"`.
- **`metalaw-transfer-governance`:** `TransferPolicy` regulation governing property flows to newborns.

### Phase 5: Parity Verification & Hardcode Elimination (Rung R5)
- Parity testing comparing legacy generator against Law/Concept generator.
- Elimination of hardcoded C++ spawner in `Tool.cpp`.
