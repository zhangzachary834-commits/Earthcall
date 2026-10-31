# Identity Continuity and Mathematical Zone Bounds

**Status:** Architectural synthesis, not an implemented cross-system guarantee or a new authorization policy.
**Provenance:** Jules-authored candidate in PR #816, source-corrected by Sun Squid Intake III on 2026-10-29. The specific model behind the Jules session is not established; no simulated model/session attribution is asserted.

## Source documents

- [Succession is not in the world](../Agenda/Tasks/Specific%20Tasks/First%20Movers%20and%20Persons/Succession_Is_Not_In_The_World/Succession_Is_Not_In_The_World.md)
- [Zones as Mathematical Bounds](../Agenda/Tasks/Specific%20Tasks/Architecture%20and%20Ontology/Zones_As_Mathematical_Bounds/Zones_As_Mathematical_Bounds.md)

## What the documents actually establish

The identity audit identifies a **gap**, not an inviolable identity anchor: legacy name-to-key continuity is held in a local, unsigned `migration-ledger.json`, outside saved worlds and ordinary Laws. Its placement protects it from some in-world edits, but does **not** make it a signed proof of historical identity, a global identity authority, or a grant of present permission. The audit records historical Relation and authorship edges that can become unbound when a Person's identifier changes. Its later rounds propose distinct offices for **denotation** (whom a reference means), **contextual acceptance** (what evidence warrants continuity here), and **standing** (what the actor may do now). Signed claims and Events are proposed work, not already completed guarantees.

The Zone plan distinguishes mathematically bounded spatial domains from the derived reading of a Person's location. **Rung 1** of dimensional bounds is implemented and tested; later rungs, including law-writable running/present Zones and owner-authorized save-bound changes, remain planned. A child Zone's object view can include objects from ancestors; this is not a claim that all hierarchical admission or persistence semantics are finished.

## The connection, with authority kept explicit

A move across Zone bounds may change which spatial relationships and Laws are relevant, but must not silently rewrite historical identity or authorship. Conversely, a historical name match or local migration-ledger entry must not automatically authorize a Zone mutation. An owner-authorized bounds change (the Zone plan's Rung 6 obligation) needs its own explicit, verifiable authority path; the unsigned ledger alone cannot supply cryptographic authorization.

Keep these questions separate when implementing the bridge: *which Person is referred to*, *what continuity evidence is accepted in this context*, *who currently has standing to act*, and *which Zone owner has authorized the specific write*. Do not collapse them into one global name database or treat historical authorship as present permission.

## Verification frontier

- Witness name-to-key migration and reload without silently dropping historical Relation endpoints or re-authoring Laws.
- Exercise two Persons asserting the same former name; surface ambiguity rather than silently choosing one.
- Verify a Zone bounds write is refused without the relevant owner's authorization, including across Zone transitions and reloads.
- Keep the planned Rung 6 behavior labeled **unimplemented** until code and executable witnesses establish it.

This synthesis makes no new constitutional ruling about Person identity, Home, or First Mover standing.
