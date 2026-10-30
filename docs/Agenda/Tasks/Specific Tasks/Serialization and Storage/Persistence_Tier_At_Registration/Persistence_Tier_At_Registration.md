# Persistence tier declared at registration — one being, one vocabulary, three strata bound

**Opened:** 2026-10-08 by Claude Code (cloud) · Claude Fable 5.1 (signs Mythos) · session `session_01VwbjDeLVjBEfmfMcRxTFga` · branch `claude/amazing-curie-4181c6`
**Source audit:** [`docs/audits/2026-10-08_mythos_undeclared_survivor_audit.md`](../../../../../audits/2026-10-08_mythos_undeclared_survivor_audit.md)
**Status:** open — no code changed; this is the task the audit implies.

## The problem in one line

A being exists in three strata — the property registry (what law can read), the save codec (what survives), and the frame (what the engine holds) — and the mapping between them is hand-written per field in `Singularity/Storage/Serialization`, declared nowhere, tested by a six-name list, and invisible to OntoMath §6's reversal.

## Evidence (verified by reading at `d4766da`)

- `Object::velocity`, `angularVelocity`, `momentOfInertia`, `centerOfMass`, `physical` are registered (`ObjectProperties.cpp:655-674`) and written by **no** save path; `.fbs` `Entity` (`Schema/Earthcall.fbs:99`) has no velocity. `LAW_MIGRATION_FRAMEWORK.md:660` calls `velocity` persistent.
- `tags`, `attributes`, `stakeholders`, `baseline`, `shapeKind`, `geometryType` are persisted by `ObjectSerialization.cpp` `to_json` and registered in **no** `buildProperties`.
- `NO_BLACK_BOX.md` §5 step 6: "A property that does not survive a save was never granted." `object_roundtrip_test.cpp` checks six names.
- `ZoneManager.cpp:1816` regenerates the save from `buildSaveJson(ctx)`; `SaveSystem.cpp:417-425` writes msgpack. Nothing reads or preserves the on-disk file's unknown keys — the engine is exempt from CLAUDE.md rule 8 ("patch, never regenerate").
- Verb census over 607 saved laws: 70% contain `Set`; 10% contain only `Flow`/`Map`/`Drive`.

## The work, in rungs (do them in order; each is independently shippable)

1. **Standing on `Property`.** Add an append-only integer enum `Persistence { Undeclared = 0, Kept = 1, Derived = 2, Ephemeral = 3, BeneathKernel = 4 }` to `src/ConstructedBeing/Singular/Property/Property.hpp`; a setter used at `registerProperty` sites; default `Undeclared`. Serialize as int (Refusal 3). Not a class, not a permission system — a column on the register that exists.
2. **Registry-walking round-trip test.** Replace the six-name list in `tests/constructed-being/object_roundtrip_test.cpp` with a walk of `singular_properties()`: for every path not `Ephemeral`/`BeneathKernel`, write → read → compare; print every `Undeclared` path as a debt line (the way `no_black_box_test` prints its ledger). Expected first run: red on `velocity` and friends — that red is the finding, do not widen an exemption to quiet it.
3. **Declare the known rows.** `hovered`, `hoverPoint` → Ephemeral. `velocity`, `angularVelocity` → ⚑ AUTHOR (Kept vs Derived; see audit §8 rec 5). `rotation` → Derived (replaces its `kWriteExemptions` entry).
4. **Register the file-only rows.** `tags`, `attributes` (Governable), `shapeKind` (read-only), `stakeholders` (Kernel read-only, as the Governor audit also asks).
5. **Registry-derived `to_json` for authored state.** Walk Kept paths; emit `path: value`. Geometry payloads stay on FlatBuffers. Add a `MigrationFramework` step mapping old keys (`transform`, `materialId`, `faceColors`, `fieldExtent`, `fieldCellSize`) to paths.
6. **§6 reads the standing.** In the Zone-level irreversibility fold, add the row "subject path is Ephemeral or Undeclared/unkept — no past across a load" with reason text.
7. **⚑ AUTHOR — Save Zone under rule 8.** Zach decides whether `buildSaveJson` patches the on-disk file (stage, merge unknown keys through, verify nothing erased, atomic rename) or whether memory is declared the flesh. Do not implement either without his answer.

## Pitfalls (Jules especially)

- Do **not** call `buildProperties()` from a constructor (CLAUDE.md); the walk in rung 2 must trigger the lazy build the normal way.
- Sources are globbed at configure time; a new test file needs a reconfigure.
- `from_json` on a *live* object corrupts it (`ZoneManager.cpp:2060` comment); the round-trip test must read into a fresh Object.
- Do not add a `type` string or a new enum of kinds anywhere in this work; the standing is a column on `Property`, nothing else.
- Materials are shared; nothing here touches paint.

## Related
- Governor audit (authority/authorship/provenance/telos): `docs/audits/2026-09-24_mythos_ungoverned_governor_audit.md`
- Property storage / stable identifiers task (To-do § Property · PropertyPath)
- `Singular_Serialization_Topology` and `Per_Zone_serialization_pathway` tasks in this folder
