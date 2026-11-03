# Formation Rete — Relevant-Change Incremental Maintenance

**Status:** ☐ architecture recorded; implementation not yet begun as a single integrated rung (2026-09-21).  
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

- [ ] **A — Semantic delta/frontier ledger:** enumerate the existing PropertyPath, Relation, Formation/admission, Zone/provider, Law-text/condition, and unknown-source deltas that can invalidate each current Formation-Rete derived structure.
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
