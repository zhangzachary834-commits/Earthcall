# "Enter New Zone (Hydrate ECGRAPH)" mints unowned, timestamp-named Zones

**Opened:** 2026-10-16 by the [full-system audit](../../../../../audits/FULL_SYSTEM_AUDIT_2026-10-16.md#1-findings-most-severe-first) (F7). Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b`.

The main-menu lambda at `src/Singularity/Core/EngineInit.cpp:589` (key `N`, added in `3052717bd` on 2026-10-04) calls `mgr.authorZone("hydrated-" + time(nullptr), "", "empty")`. That breaks two rules:
- **Stable identifiers:** the name is a generated id, not a slug.
- **Nothing without an author:** the owner is the empty string.

Its comment promises that the `load-genesis-file` metalaw will hydrate the Zone. The one minted today in the live data home (`zones/hydrated-1792184750`, 14:05) saved with zero objects and zero lexemes.

**Done when:**
- Zone birth comes from the pressing Person as author/owner.
- The identifier is an authored or derived slug.
- Hydration is witnessed end to end: menu → metalaw → objects → save → reload.
- Ideally, the menu entry itself becomes an authored control, per `INTERACTION_AS_LAW.md`.

Separately, Zach decides whether to delete the empty `hydrated-1792184750`.

## Status 2026-10-16: owner fixed

The menu now passes `_person->getIdentifier()` with `Zone::kOwnerKindPerson`. Still open: the authored slug, the end-to-end hydration witness, and moving the control to Law.
