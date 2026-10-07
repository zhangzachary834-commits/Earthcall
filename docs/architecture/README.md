# Architecture corpus

The workshop half of the ontology. `AGENTS.md` at the repo root is the router;
this directory is the library it points into.

A **framework name is not a directory name.** `Integration/` stays
`Integration/` (it is a docs-side name for the Foreign modality — see
`docs/BUILD_AND_ENVIRONMENT.md` § The tree). The folders below are
*subjects*, not subsystems, and they do not have `src/` twins.

| Folder | Holds |
|---|---|
| [`ontology/`](ontology/) | What things ARE: kinds, categories, black box, the tree, substrate, micro-mastery, Joys, primary and sub-Relations, the [`Property as Predication`](ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md) boundary between beings and their attributes, the proposed [`Property Storage and OntoMath Binding`](ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md) semantics, and [`Performance as Truth`](ontology/PERFORMANCE_AS_TRUTH.md) |
| [`law/`](law/) | What a Law is, migration, algorithms-as-law, First Mover authoring, interaction-as-law, Prophetic Rete, derived-state ledger, Formation Rete and its [`tiered relevance ladder`](law/FORMATION_RETE_TIERED_RELEVANCE_LADDER.md), [`direct-relevance`](law/FORMATION_RETE_DIRECT_RELEVANCE_ADDENDUM.md), and [`Property-addressing`](law/PROPERTY_ADDRESSING_IN_FORMATION_RETE.md) companions |
| [`events/`](events/) | Event bus vs handler, hover / person / relation / physics & collision events ([`events/PHYSICS_AND_COLLISION.md`](events/PHYSICS_AND_COLLISION.md)) |
| [`mathematics/`](mathematics/) | OntoMath, geometry unification, SDF/Bézier law replication, and the [`Geometry Execution Substrate Manifesto`](mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md) separating authored meaning from geometry IR/backend execution |
| [`ourverse/`](ourverse/) | Ourverse the being; second-person frameworks |
| [`migration/`](migration/) | In-flight plans: game-elimination, keyboard, security, semantic network, leftover UI todo |
| [`Integration/`](Integration/) | Foreign-app integration (docs twin of `src/Singularity/Foreign/`), including the bidirectional [`HTML as Lexeme Formation`](Integration/HTML_LEXEME_FORMATION_BRIDGE.md) DOM bridge |
| [`interrelations/`](interrelations/) | Cross-checks between architectural systems (e.g. Rete + Semantics, UI + Multiplayer, Substrate + IR); the human-origin chain and Git genealogy behind grounded Relation-kind identity are recorded in [`RELATION_IDENTITY_ORIGIN_AND_INTEGRATION_GENEALOGY.md`](interrelations/RELATION_IDENTITY_ORIGIN_AND_INTEGRATION_GENEALOGY.md). Other intersections include the [`Formation Rete and HTML Bridge`](interrelations/FORMATION_RETE_AND_HTML_BRIDGE.md), [`Prophetic Rete and TransferPolicy`](interrelations/PROPHETIC_RETE_AND_TRANSFER_POLICY.md), and [`No Black Box and Geometry Substrate`](interrelations/NO_BLACK_BOX_AND_GEOMETRY_SUBSTRATE.md) |
| [`Design/`](Design/) | 2D/3D visual grammar, OntoMath raster formations, granular pixel mastery ([`Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`](Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md)), Lexeme-Relation serialization ([`Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md`](Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md)), and foundational design specifications |

Cross-cutting historical observability is governed by [`PER_SINGULAR_DURABLE_LOGGING.md`](PER_SINGULAR_DURABLE_LOGGING.md): the console is an immediate observation surface, while meaningful history is routed toward durable subsystem chronicles and per-Singular histories that survive process shutdown.

Bare filenames in older notes (`NEW_KIND_FRAMEWORK.md`) mean the file in the
folder above. Prefer the folder-qualified path from now on.

The 2026-08-17 geometry-vs-OntoMath audit is historical; current math record is
[`mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`](mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md)
(executed), the governing representation/execution boundary is
[`mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md`](mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md),
and the remaining work is tracked in
[`../Agenda/Tasks/Specific Tasks/Rendering and OntoMath/Geometry_OntoMath_Remaining_Rungs/Geometry_OntoMath_Remaining_Rungs.md`](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Geometry_OntoMath_Remaining_Rungs/Geometry_OntoMath_Remaining_Rungs.md).
