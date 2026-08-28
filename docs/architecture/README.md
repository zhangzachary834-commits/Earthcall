# Architecture corpus

The workshop half of the ontology. `AGENTS.md` at the repo root is the router;
this directory is the library it points into.

A **framework name is not a directory name.** `Integration/` stays
`Integration/` (it is a docs-side name for the Foreign modality — see
`docs/BUILD_AND_ENVIRONMENT.md` § The tree). The folders below are
*subjects*, not subsystems, and they do not have `src/` twins.

| Folder | Holds |
|---|---|
| [`ontology/`](ontology/) | What things ARE: kinds, categories, black box, the tree, substrate, Joys |
| [`law/`](law/) | What a Law is, migration, algorithms-as-law, First Mover authoring, interaction-as-law |
| [`events/`](events/) | Event bus vs handler, hover / person / relation / physics events |
| [`mathematics/`](mathematics/) | OntoMath, geometry unification, SDF/Bézier law replication, Far Lands |
| [`ourverse/`](ourverse/) | Ourverse the being; second-person frameworks |
| [`migration/`](migration/) | In-flight plans: game-elimination, keyboard, security, semantic network, leftover UI todo |
| [`Integration/`](Integration/) | Foreign-app integration (docs twin of `src/Singularity/Foreign/`) |

Bare filenames in older notes (`NEW_KIND_FRAMEWORK.md`) mean the file in the
folder above. Prefer the folder-qualified path from now on.

The 2026-08-17 geometry-vs-OntoMath audit is historical; current math record is
[`mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`](mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md)
(executed) and
[`../Agenda/Tasks/Specific Tasks/Geometry_OntoMath_Remaining_Rungs.md`](../Agenda/Tasks/Specific%20Tasks/Geometry_OntoMath_Remaining_Rungs.md).
