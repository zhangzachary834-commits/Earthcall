# SUN HANDOFF — Formation Rete Relevant-Change Incrementality

**Date:** 2026-09-21  
**From:** GPT-5.6 Sol  
**For:** the next GPT-5.6 Sol session(s) continuing Formation Rete  
**Branch carrying this handoff until merge:** `sol/formation-rete-incremental-handoff-20260921`  
**Do not start over. Do not replay stale pre-rollback commits.**

## Read in this order

1. `docs/architecture/law/FORMATION_RETE_INCREMENTAL_MAINTENANCE_AND_SELF_REFINING_EVENTS.md`
2. `docs/Agenda/Tasks/Specific Tasks/Formation_Rete_Incremental_Maintenance/Formation_Rete_Incremental_Maintenance.md`
3. `docs/plans/ontological_rete_architecture.md` §9
4. `docs/architecture/law/FORMATION_RETE_TIERED_RELEVANCE_LADDER.md` §12A
5. `docs/Analysis/LAW_DIRECT_TRAVERSAL_COMPLEXITY_AND_CI_RESULTS_2026-09-19.md`
6. `agent intercom/communication-threads/ALL CLAWDS - Formation Rete relevant-change incrementality 9-21-26.md`
7. `docs/architecture/law/DERIVED_STATE_LEDGER.md`
8. `docs/architecture/law/PROPHETIC_RETE.md`

## What is already built

Do not regress or duplicate these:

- complete sweep correctness floor;
- Vocabulary candidate tier;
- Relation endpoint index + `RelationManager::generation()`;
- Slow Adapter retained roads;
- independent Slow Adapter wall-time clock through `LawManager::serviceSlowAdapterClock()`;
- branch-stable Prophetic relevance edges;
- O(1) cached candidate-tier query;
- first executable `CandidateTier::LawDirect`;
- residual-condition discharge for one exactly proved positive conjunctive `Related(kind, category)` road;
- downward safe fallback on stale currency;
- Slow Adapter and Law-Direct default ON.

Law-Direct evidence already recorded:

```text
131,072 relation queries -> 0
16,908,288 returned relation refs -> 0
65,536 applications unchanged
4.02x hostile stress speedup

real Chess:
42 Laws Direct
10.942 -> 9.611 ms median
20.071 -> 11.164 ms p95
```

The proved relation component moved from approximately `Theta(P*L*M*D)` to `Theta(P*L*M)`.

## Zach's new direction

Incrementality must recurse upward.

A delta should wake only the proofs it can affect. Do not globally reconsider Categories, Relation/Formations, retained roads, shortest paths, event subscriptions, or route priority merely because a coarse revision moved.

Target pipeline:

```text
semantic delta
  -> Prophetic conservative dependency frontier
  -> affected proofs only
  -> affected route/path repair only
  -> affected route-priority comparisons only
  -> actual consequences
```

Unknown/incomplete analysis widens immediately.

The EventBus itself may become compiled derived state: broad complete feed first, narrow proved subscriptions later, broad fallback always retained.

Stable Relation/Formation change pipelines may eventually be OntoMath-compiled into delta-transfer functions with explicit proof currency and interpreter fallback.

Bootstrap must remain grounded in the fundamental human-facing opcode framework around `PropertyPath`.

## Start here — bounded next work

### Rung A — semantic delta/frontier ledger

Before code, inventory the exact current invalidation inputs for:

- `_candidateRoutes` / Law-Direct;
- Slow Adapter roads;
- Prophetic relevance edges;
- Vocabulary;
- referent map;
- Relation endpoint index;
- any current route/currency structure consumed by Formation Rete.

For each, write:

```text
derived from
-> semantic dependency frontier
-> coarse fallback currency
-> relevant delta families
-> highest surviving repair source
-> adversarial test
```

Do not invent a new event framework if the existing EventBus / PropertyPath callback / Relation generation already carries the needed fact.

### Rung B — granular Law-Direct invalidation

This is the first implementation target.

Current Direct currency is deliberately safe but coarse. Prove that an unrelated Relation mutation does not affect a particular Direct road, then preserve that road instead of rebuilding it.

Required A/B witness:

1. warm a Direct road;
2. mutate many unrelated Relations;
3. assert zero Direct rebuilds / zero renewed category queries;
4. mutate the carrying Relation;
5. assert immediate stale/fallback/repair;
6. introduce an opaque/unknown dependency and assert coarse fail-open invalidation returns.

Do not remove `relationGeneration()` from the safety story until the narrower frontier has a completeness witness.

## After A/B

Proceed in this order unless measurement gives a reason to change it:

C. incremental Category membership;  
D. incremental Slow Adapter road repair;  
E. dynamic shortest-path experiment;  
F. incremental multi-route priority;  
G. self-refining EventBus subscriptions;  
H. OntoMath delta compilation;  
I. First-Mover opcode/PropertyPath bootstrap integration;  
J. PropertyPath-qualified Direct.

For dynamic shortest paths, do **not** prematurely hard-code "Dijkstra." Compare an appropriate dynamic SSSP family such as LPA* / D* Lite against full recomputation under Earthcall's actual cost semantics.

## Safety doctrine

- Prove impossibility/irrelevance before narrowing.
- Missing/opaque knowledge means widen/fallback.
- Lower tiers remain provenance and repair spine after crystallization.
- Property is predication, not a Singular; terminal relevance is bearer + PropertyPath.
- Priority/cost mechanisms are execution scaffolding until exposed through authored Earthcall structure.
- Count eliminated work; do not accept timing alone.
- A whole-graph change is allowed to cost whole-graph work. The architecture removes unnecessary global recomputation, not reality.

## Branch safety

The PR #53 temporal rollback incident remains binding.

Before any write:

1. resolve the **current** `sync-from-earthcall-main`;
2. branch from that exact current default;
3. never replay stale Jules/agent commits across the rollback window;
4. after each push, compare against current default and inspect changed-file count/additions/deletions;
5. a surprising mass deletion is a stop signal, not "probably refactor."

## Handoff sentence

**Do not optimize Formation Rete by making the next search faster; optimize it by making unchanged truth stop needing another search at all.**
