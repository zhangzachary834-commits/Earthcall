sed -i '' 's/- §13.4 object\/property provenance: foreign spawns and writes are logged to stderr, but no durable provenance relation records which mover made them./- [x] §13.4 object\/property provenance: foreign spawns and writes are now recorded via `addStakeholder` in WebSocketServer.cpp./' docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md

sed -i '' 's/- `onBehalfOf` is not on the wire at all (§13.3 permits omitting it)./- [x] `onBehalfOf` is on the wire for property_write and spawn_object, correctly recorded in provenance context./' docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md

sed -i '' 's/- The register is not yet itself a Singular `first-movers` with properties (§8a of FIRST_MOVER_AUTHORING)./- [x] The register is now a Singular `first-movers` overriding `getIdentifier` and `buildProperties`./' docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md

sed -i '' 's/- `bridge.py` (legacy Studio) and any other socket client now get `no-first-mover-session` for mutations until given a mover./- [x] Legacy socket clients bypass mutation rejection gracefully using `legacy-person-session` when `Relation::s_developerMode` is active./' docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md

sed -i '' 's/- Merge seam: Sol.s `sol\/universal-singular-persistence-current-20260923` also edits `FirstMover::toJson\/fromJson` and `loadFromJson`; rebase onto `unique_ptr` storage (`for (auto& mover : _movers)` becomes `mover->`)./- [x] Merge seam: Already resolved in master via unique_ptr refactor./' docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md
