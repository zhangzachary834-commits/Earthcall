# ALL CLAWDS — Formation Rete now needs relevant-change incrementality (2026-09-21)

**From:** GPT-5.6 Sol  
**For:** Claude Opus / Sonnet / Fable and every agent touching Formation Rete, Prophetic Rete, Law routing, EventBus relevance, Categories, Relation/Formations, or retained graph paths  
**Architectural source:** Zach, 2026-09-21  
**Status:** This supersedes the implementation-state portion of the 2026-09-16 ALL CLAWDS broadcast. Preserve the older thread as history.

## READ THESE FIRST

1. `docs/architecture/law/FORMATION_RETE_INCREMENTAL_MAINTENANCE_AND_SELF_REFINING_EVENTS.md`
2. `docs/architecture/law/FORMATION_RETE_TIERED_RELEVANCE_LADDER.md`
3. `docs/plans/ontological_rete_architecture.md`
4. `docs/Agenda/Tasks/Specific Tasks/Formation_Rete_Incremental_Maintenance/Formation_Rete_Incremental_Maintenance.md`
5. `docs/Analysis/LAW_DIRECT_TRAVERSAL_COMPLEXITY_AND_CI_RESULTS_2026-09-19.md`
6. `docs/architecture/law/DERIVED_STATE_LEDGER.md`
7. `docs/architecture/law/PROPHETIC_RETE.md`

## WHAT CHANGED SINCE THE 9/16 BROADCAST

The old broadcast says Slow Adapter shipped OFF and direct Law traversal was unbuilt. That is now historical.

Current code:

- `LawManager::_useSlowAdapter = true`;
- `LawManager::_useLawDirect = true`;
- Slow Adapter maintenance is polled from the frame loop but advances only on its own wall-time deadline through `serviceSlowAdapterClock()`;
- `CandidateTier::LawDirect` exists;
- the first Direct rung retains concrete bearers and a residual condition that discharges only an exactly proved positive `Related(kind, category)` conjunct;
- branch-stable Prophetic relevance edges exist;
- incomplete Prophetic graphs retain known edges for legibility but remain non-authoritative;
- the sweep remains the complete correctness floor.

Measured Law-Direct evidence:

```text
hostile Chess-shaped witness:
131,072 repeated Relation queries -> 0
16,908,288 returned Relation refs -> 0
65,536 applications -> 65,536 applications
4.02x wall-time speedup

real Chess:
42 Laws -> law-direct
10.942 ms -> 9.611 ms frame median
20.071 ms -> 11.164 ms frame p95
```

The proved category hot-path component changed from approximately:

```text
Theta(P * L * M * D)
    ->
Theta(P * L * M)
```

## ZACH'S NEW ARCHITECTURAL DIRECTION

Do **not** stop being incremental once you reach the higher tiers.

Only beings, Categories, Relations, Relations-of-Relations, Formations, retained paths, event subscriptions, and path priorities whose proof could have been changed by the incoming delta should be reconsidered.

The rule is:

> **Nothing stable should be recomputed merely because time passed. Computation follows relevant change.**

A coarse revision counter may remain as a safe fallback witness. The target is a semantic dependency frontier that can prove an unrelated mutation irrelevant.

### Category maintenance

Do not re-test every Singular against every Category after "something changed."

Use Prophetic read dependencies / abstract ranges to ask whether the changed PropertyPath or Relation kind can affect that Category predicate. If not, zero work. If yes or unknown, repair/widen safely.

### Paths

Do not restart BFS/Dijkstra over every Being after one edge/cost changes.

Retain shortest-path provenance and use a dynamic shortest-path repair strategy over the affected frontier. The exact algorithm (LPA*, D* Lite, dynamic SSSP, etc.) is **not chosen yet**; measure against Earthcall's route semantics.

### Multiple route candidates

If several sound routes remain current, keep their priority certificates. Recompute/re-rank only candidates whose dependencies or cost bounds changed. If abstract intervals still prove A cheaper than B, exact recomputation is unnecessary.

### EventBus

The event topology itself becomes derived/compiled state.

Start broad and complete. Prophetic analysis may compile narrower subscriptions from proved read/write dependencies. If completeness becomes unknown, restore the broad feed immediately. Never unsubscribe because something "probably" does not matter.

### OntoMath

Stable Relation/Formation subsystems may eventually compile into delta-transfer functions:

```text
Delta input -> Delta relevant outputs
```

but the compiled form must carry dependency/proof currency and an interpreter fallback. This is semantic JIT, not a black-box C++ replacement for authored meaning.

### Bootstrap

There is a circularity: Prophetic needs events to refine events; incremental paths need dependency proofs; compiled delta opcodes need observability.

Zach wants the bootstrap resolved through the fundamental opcode framework wrapped around `PropertyPath` in human-facing First Movers.

Keep a complete BOOT-0 floor:

```text
PropertyPath read/write
Relation form/dissolve/query
Formation membership
broad complete event feed
complete fallback traversal
```

Higher tiers prove narrower execution from that floor. They never erase the floor.

## DO NOT DO THESE THINGS

- Do not create a second parallel invalidation framework when EventBus / PropertyPath / Relation generation already provide substrate.
- Do not replace coarse invalidation until the narrower dependency frontier is proved complete.
- Do not make absent Prophetic edges authoritative when the index is incomplete.
- Do not globally rerank every route every frame.
- Do not turn dynamic shortest-path choice into a hidden permanent ontology.
- Do not treat Property as a Singular: direct relevance remains bearer + PropertyPath.
- Do not delete sweep/BFS/older provenance after a higher route crystallizes; it is the repair spine.
- Do not replay stale PR/branch commits across the PR #53 temporal rollback window.

## NEXT WORK FOR SUNS / CLAWDS

The bounded task ladder is in:

`docs/Agenda/Tasks/Specific Tasks/Formation_Rete_Incremental_Maintenance/Formation_Rete_Incremental_Maintenance.md`

Start with **A: semantic delta/frontier ledger** and **B: granular Law-Direct invalidation**. Those are the smallest rungs that make the new doctrine executable without inventing the whole dynamic-path system at once.

— **GPT-5.6 Sol, recording Zach's 2026-09-21 architecture**
