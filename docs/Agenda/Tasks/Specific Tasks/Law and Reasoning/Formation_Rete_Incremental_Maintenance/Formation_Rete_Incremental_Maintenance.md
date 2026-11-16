# Formation Rete — Relevant-Change Incremental Maintenance

**Status:** ◐ architecture recorded; the multi-Zone referent/residence bridge for Rung A landed locally on 2026-11-15, while granular Law-Direct/Slow-Adapter Relation invalidation remains open.
**Parent:** [Formation Rete](../Formation_Rete/Formation_Rete.md)  
**Architecture:** [Relevant-Change Incrementality addendum](../../../../../architecture/law/FORMATION_RETE_INCREMENTAL_MAINTENANCE_AND_SELF_REFINING_EVENTS.md)  
**Plan:** [Ontological Rete plan](../../../../../plans/ontological_rete_architecture.md)  
**Origin:** Zach, 2026-09-21. Formalized into implementation rungs by GPT-5.6 Sol.

## Goal

Make Formation Rete's **maintenance** obey the same incremental principle Law-Direct proved for execution: only proofs, Categories, Relations/Formations, routes, event boundaries, and route priorities whose premises could have changed should be reconsidered.

## Do not start over

The current substrate is load-bearing:

- Prophetic branch-stable relevance edges;
- complete sweep fallback;
- Vocabulary tier;
- Relation endpoint index and generation;
- Slow Adapter on its independent wall-time clock;
- O(1) cached tier query;
- first executable Law-Direct rung;
- Derived-State Ledger safety discipline.

Extend these. Do not replace them with a second relevance system.

## Task ladder

- [~] **A — Semantic delta/frontier ledger:** multi-Zone residence, referent membership, retained working-set, and TransferPolicy rule currencies are implemented and entered in the Derived-State Ledger; the complete CandidateRoute/Slow-Adapter/Prophetic delta inventory remains open.
- [ ] **B — Law-Direct granular invalidation:** replace coarse Relation-generation invalidation only where a complete per-route dependency frontier can prove an unrelated Relation mutation irrelevant; retain coarse generation as fail-open fallback.
- [ ] **C — Incremental Category membership:** map Category predicates to their PropertyPath/Relation read frontier and re-evaluate only changed bearers whose deltas can affect membership.
- [ ] **D — Incremental retained-road repair:** preserve unaffected segments/provenance of Slow Adapter roads rather than rebuilding a whole road for an unrelated graph mutation.
- [ ] **E — Dynamic shortest-path experiment:** compare full BFS/Dijkstra recomputation with an affected-frontier dynamic SSSP family on sparse edge/cost mutations; choose the algorithm from measurements and Earthcall cost semantics.
- [ ] **F — Incremental route priority:** retain multiple current candidate routes with dependency/cost certificates and update only candidates/comparisons whose ordering could change.
- [ ] **G — Prophetic EventBus recompilation:** compile broad complete event feeds into narrower dependency channels, with immediate widening when analysis becomes opaque/incomplete.
- [ ] **H — OntoMath delta compilation:** compile one stable Relation/Formation subsystem into a bounded delta-transfer function with interpreter parity and explicit proof currency.
- [ ] **I — First-Mover bootstrap:** ground the above in the fundamental human-facing opcode framework around PropertyPaths; no parallel hidden command ontology.
- [ ] **J — PropertyPath-qualified Direct:** continue Tier 6 from bearer-level direct routes toward branch/bearer/PropertyPath relevance so irrelevant value writes do not wake unrelated residual work.

## Acceptance rules

Every rung must prove:

1. irrelevant mutation causes no repair;
2. relevant mutation repairs the right frontier;
3. unknown/opaque mutation widens safely;
4. optimizer-off/broad-feed execution has identical lawful observables;
5. stale routes retain provenance for downward repair;
6. work counters demonstrate what was eliminated, not timing alone.

## Complexity target

Move broad invalidation from work proportional to global maintained structure toward:

```text
O(Delta + affected dependency frontier + affected route repair + changed route comparisons + actual consequences)
```

without claiming the worst case disappears. A mutation that genuinely changes the whole graph may still require whole-graph work.

## Handoff rule

Before implementing a rung, read the 2026-09-21 CLAWD broadcast and the architecture addendum. Keep the PR #53 temporal-rollback discipline: branch from the current corrected default, inspect ancestry/diff before push, and never replay stale branch commits across the rollback window.

## 2026-11-15 — ActiveZones relevant-change bridge

Zach's live report that a 52-Zone boot lagged, and the Twin Firmament's four-source forensic account, supplied the concrete pressure for this pass. Codex / GPT-5.6 Sol extended their proposed Rungs 1–4 without pretending Rung B was already proved:

- `ActiveZones::beings()` now retains its deduplicated working set under explicit structural, per-Zone referent, relation, provider, and unmaking currency instead of allocating and harvesting on every caller.
- Zone residence no longer manufactures Formation membership. Ordinary Objects remain resident and Law-reachable without being silently asserted as authored Formation members; `syncFormationMembers()` preserves explicit meaning and no longer copies every global Lexeme into every Zone.
- `resolveLawRoot` and direct `PropertyPath` use one interned referent route. Active operands aggregate locally; inactive loaded Zones own lazy per-Zone caches, and unqualified ambiguity refuses.
- `ZoneManager`'s one residence index serves `TransferPolicy`; it is structural-currency keyed instead of time/frame keyed and does not rebuild on a legitimate miss.
- `TransferPolicy` compiles conservative rule candidates by operation, path, and cell-bearing family, then applies the unchanged canonical authority semantics. Every rule mutation explicitly poisons the derived table.

The global `ActiveZones::structuralRevision()` and relation generation remain the coarse safety floor for CandidateRoute and Slow Adapter. The next pass must not replace them with jurisdiction-only stamps until it can prove the complete frontier of unqualified roots, shared provider beings, canonical `CellId` aliases, Relation roads, and opaque dependencies. The acceptance witness remains the original one: unrelated Relation changes cause zero Direct rebuilds, carrying Relation changes repair immediately, and unknown dependencies widen.

*Codex / GPT-5.6 Sol / session `01a1e2e9-2234-7510-8e51-f98034f60923` / 2026-11-15T23:01:56-08:00 (PST). Commissioned by Zach; architectural diagnosis originated in the Twin Firmament's Intercom scroll and was extended here into executable derived-state currency.*
