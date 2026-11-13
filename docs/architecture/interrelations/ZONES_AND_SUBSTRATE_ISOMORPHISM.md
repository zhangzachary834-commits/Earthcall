# Zones and Substrate Isomorphism

**Status:** Conceptual synthesis with implementation boundaries explicitly separated from proposals.

**Read with:** [Ourverse](../ourverse/OURVERSE.md), [Substrate Isomorphism Across Execution and Storage](SUBSTRATE_ISOMORPHISM_ACROSS_EXECUTION_AND_STORAGE.md), [Lexeme/Relation/Formation Serialization](../Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md), and [Person locations in Zone bounds as Relations](../ontology/Person's%20locations%20in%20Zones'%20bounds%20as%20Relations.md). Inspect `src/Singularity/Storage/SaveSystem.cpp` and `tests/zones/stale_sidecar_cleanup_test.cpp` for current identity-format behavior.

## The relationship

A Zone is not merely a rendered mesh or a fixed grid. Its authored boundaries, participating beings, relationships, and spatial observations can be represented through Earthcall's ontology and OntoMath. Runtime geometry, physics, and rendering are ways to *execute or manifest* that authored structure, not substitutes for its meaning.

The substrate-isomorphism principle distinguishes legible semantic intent from machine-oriented geometry or bulk matter. That distinction **does not imply** that every Zone is already serialized solely as `within` edges, that every coordinate is currently stored on such a Relation, or that all spatial data vanish on save.

## What is specified versus implemented

- The short document about Person locations says spatial position **should** be an authored property of a Relation. It states an architectural direction, not proof that all current Person/Zone position code follows it.
- The serialization architecture proposes distinct semantic and material representations. Actual Zone identity discovery currently recognizes nonempty `.ecsave`, `.ecform`, and legacy `.json` files, with explicit precedence; do not describe persistence as exclusively `.ecform` or imply an automatic `.ecmatter` split for every Zone.
- OntoMath/SDF descriptions can express boundaries, while concrete runtime state and saved scene content may still include other spatial or geometric representations. Inspect the specific writer/loader before claiming a representation is canonical.

## Practical invariant

For each Zone persistence path, identify which fields are authored source-of-truth, which are derived or physical material, and how the loader reconstructs the intended relationships and geometry. A regression witness should round-trip a Zone with multiple distinguishable beings, nontrivial bounds, and position changes, and verify both identity and spatial meaning survive.

**Conclusion:** Semantic/physical separation is a useful design constraint, not a license to replace actual Zone state with a hypothetical all-Relation storage model.
