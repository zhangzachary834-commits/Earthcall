# Law-Direct Traversal: Measured Results and Granular Complexity Analysis

**Date:** 2026-09-19  
**Timestamp:** 2026-09-19 02:29 PDT  
**Status:** Post-implementation analysis; implementation merged in PR #234  
**Implementation merge:** `1de9c2801456aa9bdadeb7dd12c5cd72c1cbf21b`  
**Primary CI evidence:** Earthcall focused CI run `35431978980`  
**Author:** GPT-5.6 Sol  
**Session ID:** `gpt-5.6-sol/pr234-law-direct-analysis-20260919`  
**Human origin / direction:** Zach identified that lower relevance tiers were supposed to establish a terminal Law-direct route, asked why Chess kept re-running the category tier, and explicitly requested an adversarial benchmark whose difference would be extremely dramatic if Direct worked correctly.  
**Prior diagnosis drawn from:** Claude Opus 5's Slow Adapter performance analysis and Derived-State Ledger, especially the documented "adapter cost + old cost" failure mode and the rule that derived relevance may never silently make a Law deaf.  
**Companions:**  
- `docs/Analysis/SLOW_ADAPTER_CLOCK_COUPLING_AND_ADAPTIVE_COMPUTE_ANALYSIS_2026-09-17.md`  
- `docs/architecture/law/DERIVED_STATE_LEDGER.md`  
- `docs/plans/ontological_rete_architecture.md`  
- `tests/law/law_direct_stress_test.cpp`  
- `tests/law/slow_adapter_parity_test.cpp`  
- `tests/singularity/slow_adapter_zone_perf_test.cpp`

---

## Executive summary

The first executable **Law-Direct** rung changes what happens after a lower relevance tier has already discovered a stable route.

Before Law-Direct, the Slow Adapter could discover and retain a category road such as:

```text
Law
  -> Related(instance-of, category.chess.piece)
  -> [32 chess pieces]
```

but the hot execution path still repeatedly re-proved that each bearer belonged to the category when the Law fired. In the historically bad Chess shape, the engine could therefore pay for relevance discovery, retain the road, and still pay the old category predicate repeatedly.

After Law-Direct, one current, sound, positive conjunctive retained road may **crystallize into concrete bearer bindings plus a residual condition**. The exact proved `Related(kind, other)` conjunct is discharged. Dynamic state remains live.

The new execution shape is therefore:

```text
lower tiers discover / prove relevance
              |
              v
        Law-Direct route
              |
              +----> concrete bearers
              |
              +----> residual live condition
```

The proof is not merely a tier label. CI measured both the work counters and wall time.

### Adversarial Chess-shaped stress witness

```text
32 members
128 Laws
128 irrelevant category edges per member
16 pulses per measured batch
65,536 lawful applications per batch
```

Measured result:

| Metric | Pre-Direct | Law-Direct | Change |
|---|---:|---:|---:|
| Median batch time | 13,827.417 ms | 3,439.637 ms | **4.02x faster** |
| Relation graph queries | 131,072 | **0** | **100% eliminated in measured hot phase** |
| Relation fan-out returned | 16,908,288 | **0** | **100% eliminated in measured hot phase** |
| Law applications | 65,536 | 65,536 | identical semantics |
| One-time promotion cost | — | 4.573 ms | amortized in ~0.01 stress pulses |

The stress workload became **75.12% shorter in wall time**.

### Real Chess

The real authored `chess_app` world showed:

| Metric | Adapter ON, Direct OFF | Adapter ON, Direct ON | Change |
|---|---:|---:|---:|
| Frame median | 10.942291 ms | 9.611333 ms | **12.16% lower** |
| Frame p95 | 20.071250 ms | 11.163584 ms | **44.38% lower** |
| Simulation-equivalent FPS | 91.39 | 104.04 | **13.84% higher** |
| Law median | 2.079667 ms | 1.866000 ms | **10.27% lower** |
| Direct Laws | 0 | **42** | terminal tier actually activated |
| AdapterRoad Laws | 1 | 0 | road graduated |
| Vocabulary Laws | 57 | 16 | many category Laws graduated |
| Sweep Laws | 12 | 12 | unchanged floor |

With the adapter entirely off, Chess measured 9.316000 ms median. Thus:

- pre-Direct adapter overhead above the no-adapter floor was about **17.46%**;
- adapter + Direct overhead above that floor was about **3.17%**;
- Direct recovered about **81.84% of the median overhead** that the pre-Direct adapter path had added in this run.

This does **not** mean all Law execution is now O(1). A Law that genuinely governs 32 pieces must still visit those 32 pieces. What changed is the complexity of repeatedly proving *why those pieces are relevant*.

That distinction is the center of this analysis.

---

# 1. What exactly changed

Before Direct, the candidate ladder ended at:

```text
Sweep
  -> Vocabulary
  -> AdapterRoad
```

The Slow Adapter could precompute a retained road, but hot execution still evaluated the authored condition against each proposed bearer. A category predicate therefore remained live even when the retained road itself was the proof of that category membership.

The first Direct rung adds:

```text
Sweep
  -> Vocabulary
  -> AdapterRoad
  -> LawDirect
```

For the current first rung, Direct is deliberately conservative.

It requires:

1. Slow Adapter enabled;
2. a current retained road;
3. the retained road derived from the same Law condition revision;
4. a readable `ConditionModel`;
5. exactly one model-derived condition predicate;
6. exactly one positive conjunctive category route collected by `collectCategoryRoutes()`;
7. current structural revision;
8. current relation generation;
9. current Law text and condition revision;
10. a current adapter road currency generation.

When those conditions hold, the route stores:

```text
directSubjects
directResidual
directRelationType
directOtherId
```

The residual compiler replaces only the exact proved positive `Related(type, other)` leaf with `true` while descending only through positive `All` conjunctions.

It does **not** propagate the proof through:

- `Any`;
- `Not`;
- quantifiers;
- unrelated `Related` leaves;
- opaque closures.

That is why the parity suite includes the deliberately impossible shape:

```text
Related(X) AND NOT Related(X)
```

The positive occurrence may be discharged by Direct. The negative occurrence remains live and must still evaluate normally.

---

# 2. Symbols used in the complexity model

Let:

- **N** = total eligible beings in the world.
- **L** = number of active untargeted Laws under consideration.
- **M_l** = number of candidate bearers for Law `l` after the best lower-tier narrowing.
- **M** = representative or uniform candidate count when discussing a homogeneous workload.
- **D(s)** = number of Relations returned by the endpoint index for subject `s`.
- **D_l** = average endpoint degree over Law `l`'s candidates.
- **E** = total number of Relations in the world.
- **E_c** = number of Relations inspected while building a particular category road.
- **U_c** = number of unique members discovered for that road.
- **R_l** = number of required property names for Law `l`.
- **C_l** = cost of evaluating the live residual condition after the proved category conjunct is removed.
- **A_l** = cost of the Law's actions / application bookkeeping for one successful bearer.
- **q_l** = fraction of candidates whose live condition succeeds and reaches application.
- **P** = number of event pulses / frames / repeated execution opportunities.
- **K_l** = number of category roads needed by Law `l`. The current Direct rung intentionally accepts only the simple `K_l = 1` case.
- **B(N)** = cost of obtaining the current world-being snapshot/cardinality when route selection is rebuilt. In common provider-backed paths this is proportional to world size; the steady cache-hit path does not pay it.

Where expected hash-map lookup is treated as O(1), that assumption is stated. Where Relation degree dominates, it is kept explicit rather than collapsed into "constant time."

---

# 3. The old hot path, step by step

Consider an untargeted `OnEvent`, `Everyone` Law whose condition is:

```text
All(
    Related(instance-of, category.X),
    Compare(liveProperty > threshold)
)
```

and suppose Vocabulary already narrows to the same `M` category members.

## 3.1 Candidate-route selection

On a current route cache hit, `refreshCandidateRoute()` checks stored currency:

- Law text revision;
- condition revision;
- structural revision;
- relation generation;
- adapter route currency.

This is **Theta(1) per Law** in the steady case, excluding the cost of obtaining any external counters themselves.

So route *selection* was not the dominant Chess cost.

## 3.2 Vocabulary candidate consumption

The Vocabulary tier returns a vector of approximately `M_l` candidates and filters them through `law.couldApplyTo()`.

If `couldApplyTo()` checks `R_l` required properties, candidate proposal costs roughly:

```text
T_vocab(l) = O(M_l * R_l)
```

This is not relation-graph work. It is the property-vocabulary filter.

## 3.3 First full condition evaluation

For each surviving candidate, the event path evaluates:

```cpp
law->conditionsSatisfied(*being)
```

For a `Related(...)` condition, the current implementation asks:

```cpp
Universe::relationsInvolving(subject, edges)
```

and scans the returned endpoint neighborhood.

Thus one category proof for subject `s` costs:

```text
Theta(D(s))
```

in the worst case where the matching edge is last or absent.

With a residual non-graph condition costing `C_l`, the first condition pass is:

```text
O(M_l * (D_l + C_l))
```

## 3.4 The historical duplicate condition evaluation

Before Direct, a successful candidate then entered:

```text
applyAndMaybeDrive()
    -> Law::applyTo()
        -> conditionsSatisfied()
```

So a bearer whose condition succeeded paid the condition a **second time**.

If `q_l` is the success fraction, condition cost per Law per pulse was approximately:

```text
T_condition_pre(l)
  = O(M_l * (D_l + C_l))
    + O(q_l * M_l * (D_l + C_l))
```

or:

```text
T_condition_pre(l)
  = O(M_l * (1 + q_l) * (D_l + C_l))
```

For the adversarial benchmark, every member satisfies the condition, so `q_l = 1`:

```text
T_condition_pre(l)
  = O(2 * M_l * (D_l + C_l))
```

When `D_l >> C_l`:

```text
T_condition_pre(l) = Theta(M_l * D_l)
```

with a large constant of approximately two graph traversals per successful application.

## 3.5 Full pre-Direct event complexity

Including candidate vocabulary checks and successful application cost:

```text
T_pre(l)
  = O(
      M_l * R_l
      + M_l * (D_l + C_l)
      + q_l * M_l * (D_l + C_l + A_l)
    )
```

For `L` homogeneous Laws and `P` pulses:

```text
T_pre
  = O(
      P * L * M *
      [R + (1 + q)(D + C) + qA]
    )
```

In the category-dominated regime:

```text
T_pre = Theta(P * L * M * D)
```

This is the hot asymptotic factor Direct is designed to remove.

---

# 4. Slow Adapter discovery is a different complexity domain

Law-Direct does **not** make route discovery free.

The Slow Adapter's category road must still be discovered and kept current.

For a present category, `SlowAdapter::build()` obtains the category's incident edges and scans them. Let that edge set have size `E_c`.

Membership deduplication is currently:

```cpp
std::find(road.members.begin(), road.members.end(), member)
```

so road construction is not merely linear in edge count.

A useful upper bound is:

```text
T_build
  = O(E_c * U_c)
```

and when nearly every scanned edge introduces a new member, `U_c = Theta(E_c)`, giving the familiar worst case:

```text
T_build = O(E_c^2)
```

If the category being cannot be resolved, the adapter may fall back to scanning all world Relations, so `E_c` can become `E`.

This work is deliberately on the Slow Adapter's independent maintenance cadence, not the foreground Law tick.

Law-Direct changes the **amortization**:

```text
discover / prove once
        +
reuse P times
```

instead of:

```text
discover / retain
        +
re-prove category P times anyway
```

That is the difference between an expensive index that earns its keep and an index that becomes an additional tax.

---

# 5. Direct promotion cost

Promotion is not free and must be accounted for explicitly.

For one eligible Law, promotion currently does approximately:

1. currency / route lookup — expected O(1);
2. collect category routes from the condition tree — O(|condition tree|);
3. walk the retained road of size `M_l`;
4. for each member, run `couldApplyTo()` over required properties — O(`R_l`) per bearer;
5. copy surviving pointers into `directSubjects`;
6. compile the residual condition — O(|condition tree|).

Thus:

```text
T_promote(l)
  = O(
      |Condition_l|
      + M_l * R_l
    )
```

for the current single-road rung, ignoring allocator constants.

Across `L_d` Direct-eligible Laws:

```text
T_promote_total
  = O(
      sum_l |Condition_l|
      + sum_l M_l * R_l
    )
```

The adversarial benchmark intentionally had **128 Laws sharing one retained Slow Adapter road**. The median measured time to promote all 128 Laws was:

```text
4.573 ms total
≈ 0.0357 ms per Law on average
```

This is a one-time / invalidation-time cost, not a per-pulse cost.

Because the stress workload saved about **649.236 ms per pulse**, that promotion cost amortized after:

```text
4.573 / 649.236 ≈ 0.0070 pulses
```

reported by CI as approximately **0.01 pulses**.

That number is intentionally extreme because the benchmark is intentionally hostile.

---

# 6. The new steady Direct hot path

Once a Direct route is current:

## 6.1 Route decision

The Law's candidate route is already cached.

Currency validation remains:

```text
Theta(1) per Law
```

on the steady path.

## 6.2 Bearer traversal

`sweepSubjects()` visits the current `directSubjects` vector, checks liveness, and returns those bearers.

That is:

```text
Theta(M_l)
```

It is **not O(1)** unless the Law semantically has O(1) relevant bearers.

This is correct.

If a Law governs 32 chess pieces, there are still 32 subjects whose live state and actions may matter.

## 6.3 Residual condition

Instead of evaluating the full condition:

```text
Related(category.X) AND liveResidual
```

Direct evaluates:

```text
true AND liveResidual
```

for the exact proved positive conjunct.

The cost becomes:

```text
O(C_l)
```

per candidate rather than:

```text
O(D_l + C_l)
```

## 6.4 Application without duplicate condition traversal

When the residual passes, `applyToAfterConditions()` preserves:

- enabled/authored checks;
- authority;
- Kernel boundaries;
- jurisdiction;
- actions;
- traces;
- provenance / audit behavior;
- drive behavior;

but does not re-evaluate the already-decided condition.

Thus the new hot path is approximately:

```text
T_direct(l)
  = O(
      M_l
      + M_l * C_l
      + q_l * M_l * A_l
    )
```

or:

```text
T_direct(l)
  = O(
      M_l * [1 + C_l + q_l A_l]
    )
```

Across `L` homogeneous Laws and `P` pulses:

```text
T_direct
  = O(
      P * L * M * [1 + C + qA]
    )
```

The endpoint-degree factor `D` has disappeared from the proved category conjunct.

That is the core asymptotic improvement.

---

# 7. Before vs after: the asymptotic delta

For a category-dominated Law:

### Pre-Direct

```text
Theta(P * L * M * D)
```

### Direct

```text
Theta(P * L * M)
```

assuming the residual condition is O(1) and actions do not themselves dominate.

So Direct removes the relation-degree factor from the hot relevance proof:

```text
Theta(P * L * M * D)
        ->
Theta(P * L * M)
```

This is **not** a universal D-fold wall-clock speedup.

It is a D-fold asymptotic reduction in one component.

The whole Law engine still pays for:

- event/Rete processing;
- iterating relevant bearers;
- residual conditions;
- authority/jurisdiction;
- action execution;
- application records;
- audit/event machinery;
- drive machinery where present;
- other Laws that remain on Vocabulary or Sweep.

Amdahl's law therefore matters.

The stress witness removed 100% of the measured category graph queries yet produced a 4.02x total batch speedup, not a 129x wall-time speedup.

That is exactly what should happen when one formerly dominant subsystem disappears and the rest of the engine becomes the new floor.

---

# 8. Exact operation count in the stress witness

The adversarial benchmark parameters were:

```text
M = 32 members
L = 128 Laws
P = 16 pulses
D = 129 endpoint edges per member
    = 128 irrelevant edges + 1 desired category edge
q = 1
```

Every Law reaches every member.

Applications per batch:

```text
L * M * P
= 128 * 32 * 16
= 65,536 applications
```

Before Direct, the category condition is evaluated twice for every successful application:

```text
2 * L * M * P
= 2 * 128 * 32 * 16
= 131,072 relation queries
```

Each query returns exactly 129 incident edges in this constructed graph:

```text
131,072 * 129
= 16,908,288 relation references returned
```

CI measured exactly:

```text
relation_queries=131072
relation_fanout_returned=16908288
```

This exact equality is important. It confirms the probe measured the intended pre-Direct mechanism rather than incidental graph activity.

After Direct promotion, the measured hot batch reported:

```text
relation_queries=0
relation_fanout_returned=0
```

while applications remained:

```text
records_per_batch=65536
```

Therefore the benchmark demonstrated:

> the same lawful reach and application count, with the repeated category-graph proof removed from the measured hot phase.

---

# 9. Stress timing: granular interpretation

Measured medians:

```text
pre-Direct : 13,827.417 ms / batch
Direct     :  3,439.637 ms / batch
saved      : 10,387.780 ms / batch
speedup    : 4.02x
```

Per pulse:

```text
pre-Direct ≈ 864.214 ms
Direct     ≈ 214.977 ms
saved      ≈ 649.236 ms
```

Per lawful application:

```text
pre-Direct ≈ 210.99 microseconds
Direct     ≈  52.48 microseconds
saved      ≈ 158.50 microseconds
```

The Direct batch retained about:

```text
3,439.637 / 13,827.417 ≈ 24.88%
```

of the original wall time.

Equivalently, roughly **75.12%** of the hostile workload's measured time disappeared.

That gives a useful Amdahl interpretation:

- in this workload, repeated category proof accounted for a very large fraction of practical cost;
- once removed, the remaining ~24.9% became the new floor;
- further Direct optimization must therefore attack different costs, not keep optimizing a category query that is already gone.

---

# 10. Real Chess: what the tier ladder actually did

The real `chess_app` run contained:

```text
objects   = 60
laws      = 70
relations = 143
```

### Pre-Direct tier residency

```text
LawDirect   0
AdapterRoad 1
Vocabulary 57
Sweep      12
```

### Direct-enabled tier residency

```text
LawDirect  42
AdapterRoad 0
Vocabulary 16
Sweep      12
```

So:

```text
42 / 70 = 60%
```

of all loaded Chess Laws reached the Direct tier in this run.

The important observation is not merely "42 is a large number."

It is the migration pattern:

```text
pre-Direct:
57 Vocabulary + 1 AdapterRoad
             |
             v
Direct:
42 LawDirect + 16 Vocabulary
```

The category road did not become another permanent tier tax. For dozens of Laws it became **construction scaffolding for the higher tier**.

That is the architecture Zach was asking for.

---

# 11. Real Chess wall-time results

### No Adapter floor

```text
adapter=off direct=off
frame median  = 9.316000 ms
frame p95     = 10.141208 ms
law median    = 1.677541 ms
sim-eq FPS    = 107.34
```

### Immediately pre-Direct architecture

```text
adapter=on direct=off
frame median  = 10.942291 ms
frame p95     = 20.071250 ms
law median    = 2.079667 ms
sim-eq FPS    = 91.39
```

### Adapter + Direct

```text
adapter=on direct=on
frame median  = 9.611333 ms
frame p95     = 11.163584 ms
law median    = 1.866000 ms
sim-eq FPS    = 104.04
```

### Direct vs immediately pre-Direct

Median frame improvement:

```text
1 - 9.611333 / 10.942291
≈ 12.16%
```

p95 improvement:

```text
1 - 11.163584 / 20.071250
≈ 44.38%
```

Law median improvement:

```text
1 - 1.866000 / 2.079667
≈ 10.27%
```

Simulation-equivalent FPS increase:

```text
104.04 / 91.39 - 1
≈ 13.84%
```

### Recovery of the old adapter overhead

Median overhead above the no-adapter floor:

```text
pre-Direct overhead
= 10.942291 - 9.316000
= 1.626291 ms

Direct overhead
= 9.611333 - 9.316000
= 0.295333 ms
```

Fraction of that overhead recovered:

```text
(1.626291 - 0.295333) / 1.626291
≈ 81.84%
```

This is the cleanest practical statement of the result:

> In this CI run, Law-Direct recovered about 82% of the median frame overhead that the pre-Direct Slow Adapter architecture added to Chess.

---

# 12. Why the p95 result matters

The median improvement was meaningful, but the tail improvement was larger:

```text
20.071250 ms
    ->
11.163584 ms
```

A 60 Hz frame budget is approximately:

```text
1000 / 60 ≈ 16.67 ms
```

The pre-Direct p95 exceeded that budget.

The Direct p95 did not.

This does not prove a rendered application will always hold 60 FPS. The probe's `sim_eq_fps` is a CPU simulation-equivalent measure, not an end-to-end rendered frame rate.

What it does show is that the pre-Direct relevance path had a much larger CPU tail in this run, and Direct collapsed most of it.

The likely architectural reason is straightforward: repeated degree-sensitive Relation work creates variable cost depending on which Laws/candidates are active and how much endpoint fan-out is traversed. Removing that repeated graph proof reduces both average work and one major source of tail amplification.

---

# 13. Controls: why small timing changes are not automatically Direct wins

Two other authored worlds are useful controls.

## Basic Pixel Changer

Direct tier count:

```text
tier_direct=0
```

Yet measured median changed:

```text
40.664625 ms pre-Direct
38.431250 ms Direct-enabled
```

## Noise Floor

Direct tier count:

```text
tier_direct=0
```

Yet measured median changed slightly:

```text
0.094417 ms pre-Direct
0.091375 ms Direct-enabled
```

Because Direct never activated in either control, those timing changes cannot reasonably be credited to Law-Direct.

They are a reminder that:

- CI machines are noisy;
- process ordering and thermal state matter;
- 60-frame medians are measurements, not laws of nature.

Therefore the evidence hierarchy should be:

1. **semantic parity**;
2. **tier activation counters**;
3. **deterministic graph-work counters**;
4. **wall-time measurements**.

The stress test is powerful because it satisfies all four.

The Chess result is persuasive because `tier_direct=42` coincides with a large directional improvement, while the controls show that small wall-time differences can occur without Direct.

---

# 14. Memory complexity

The current Direct rung stores a separate `directSubjects` vector per Direct Law.

For Law `l`:

```text
Space_direct(l) = O(M_l)
```

Across all Direct Laws:

```text
Space_direct = O(sum_l M_l)
```

plus residual predicate objects.

This means multiple Laws sharing the same Slow Adapter road currently duplicate the bearer pointer list at the Direct tier.

In the stress test:

```text
128 Laws * 32 pointers
= 4,096 pointers
```

At 8 bytes per pointer, the raw pointer payload alone is about:

```text
4,096 * 8
= 32,768 bytes
≈ 32 KiB
```

before vector and callable overhead.

That is tiny in the benchmark, but the asymptotic shape matters for very large worlds.

A natural future refinement is to intern/share immutable current direct bearer sets:

```text
today:
O(L * M) pointer storage for L Laws sharing one road

shared direct bearer set:
O(M + L)
```

while each Law retains only its own residual predicate / proof metadata.

That optimization is **not required for the measured speedup** and should not be confused with the correctness of Direct itself.

---

# 15. Invalidation complexity and the cost of change

Direct's steady speed exists because it is willing to pay again when its proof becomes stale.

A Direct route is invalidated when relevant currency changes:

- Law text revision;
- condition revision;
- world structural revision;
- relation generation;
- adapter road currency.

Thus the architecture has two regimes.

## Stable world

```text
route currency checks: Theta(1) per Law
bearer traversal:      Theta(M_l)
residual evaluation:   O(M_l * C_l)
```

## Changed world

The next route selection may need:

- vocabulary refresh;
- Slow Adapter rebuild/maintenance;
- Direct re-promotion;
- bearer vector reconstruction;
- residual recompilation.

That is intentional.

Direct is **not** a promise that the world never changes.

It is a promise that unchanged structural truth is not pointlessly re-proved at every pulse.

This is the correct relationship between cache and ontology:

> mutation pays invalidation; stability earns reuse.

---

# 16. Why Direct is not "condition truth caching"

The Direct tier does **not** cache:

```text
"this Law is true for this piece"
```

That would be unsafe because dynamic values such as:

- turn;
- grid position;
- `hasMoved`;
- color;
- health;
- authored thresholds;
- any other residual property;

may change without changing category membership.

Direct caches a narrower proposition:

```text
"under the current structural/relation currency,
 this bearer is on this proved relevance road"
```

Then it still evaluates the residual live condition.

Formally:

```text
Condition(s)
= ProvenRoute(s) AND Residual(s, t)

pre-Direct:
evaluate ProvenRoute(s) every time
evaluate Residual(s, t) every time

Direct:
cache ProvenRoute(s) while its structural proof is current
evaluate Residual(s, t) every time
```

This is why the optimization can be both aggressive and sound.

---

# 17. Mixed polarity and residual graph complexity

The current Direct rung removes only one exact positive conjunct.

Suppose the condition is:

```text
Related(X)
AND
NOT Related(X)
```

Direct may discharge the positive occurrence:

```text
true
AND
NOT Related(X)
```

but the negative occurrence still evaluates.

Therefore Direct does **not** imply:

```text
graph queries = 0
```

for every Direct Law.

The stress test reaches zero because its residual is only a property comparison.

In general:

```text
T_direct(l)
= O(M_l * [C_residual_graph + C_residual_non_graph + qA])
```

where any unrelated or logically non-dischargeable graph predicates remain in `C_residual_graph`.

This is an important constraint for future complexity claims.

---

# 18. Current optimality boundary

The first Direct rung is **Law -> concrete bearer set**, not yet the complete final target:

```text
Law
  -> Singular
  -> PropertyPath
```

For a category-wide Law with `M` relevant bearers, current Direct still has:

```text
Theta(M)
```

subject iteration.

That is unavoidable if the Law actually acts on all M bearers.

But future Direct work can become more specific where semantics permit:

- branch-specific direct bearer subsets;
- PropertyPath-qualified relevance;
- shared direct bearer-set interning;
- residual compilation across multiple independent proved conjuncts;
- direct reactive wake-up keyed to the exact property paths that can change the residual;
- avoiding vector copy from cached direct subjects by consuming a stable view/span where lifetime permits.

For a Law whose semantics truly reduce to one bearer and one property path, the terminal relevance-routing overhead can approach:

```text
Theta(1)
```

per relevant change, before the actual authored action cost.

That is different from claiming arbitrary category-wide Law execution is O(1).

---

# 19. Complexity table

| Operation | Pre-Direct | Law-Direct | Notes |
|---|---:|---:|---|
| Steady candidate-tier currency check | Theta(1) / Law | Theta(1) / Law | same cache discipline |
| Vocabulary candidate walk | O(M_l R_l) | bypassed for Direct Laws | still used by non-Direct Laws |
| Adapter road discovery | maintenance cost | maintenance cost | Direct does not erase discovery |
| Direct promotion | — | O(|Condition_l| + M_l R_l) | paid on promotion/invalidation |
| Category `Related` proof, hot | O(M_l D_l), often twice for successful applications | **0 for discharged conjunct** | dominant removed factor |
| Residual live condition | O(M_l C_l), often duplicated by `applyTo` | O(M_l C_l) | evaluated once on Direct path |
| Actual bearer visitation | Theta(M_l) | Theta(M_l) | semantically necessary |
| Action/application work | O(q_l M_l A_l) | O(q_l M_l A_l) | not removed |
| Direct bearer memory | — | O(M_l) / Law | shareable future target |
| L homogeneous Laws, P pulses, category-dominated | Theta(P L M D) | Theta(P L M) | when residual/action are O(1) |
| Direct route invalidation | — | rebuild/promotion | change pays; stability reuses |

---

# 20. What the result says about the Slow Adapter

The result does not show that the Slow Adapter was a mistake.

It shows that the Slow Adapter was **incomplete as a terminal execution tier**.

Its correct role is developmental:

```text
discover
  ->
retain
  ->
prove
  ->
promote
  ->
stop searching
```

The older failure mode was:

```text
discover
  ->
retain
  ->
keep traversing the category road forever
```

Opus's earlier analysis described the symptom as:

```text
adapter cost + old cost
```

Law-Direct supplies the missing escape from that regime for the supported shape.

The lower tiers are therefore not merely competing optimizers.

They are a **relevance compiler hierarchy** whose lower levels can establish facts that justify a higher representation.

That is the deeper architectural result.

---

# 21. What remains

The implementation is intentionally only the first safe Direct rung.

Remaining work includes:

1. **PropertyPath-qualified Direct relevance.** The current route binds bearers and removes one structural graph conjunct, but does not yet retain the exact property path frontier as the terminal relevance edge.

2. **Shared Direct bearer sets.** Avoid O(LM) pointer duplication when many Laws share one road.

3. **Multiple proved conjunct composition.** Generalize beyond one positive category route without making `Any`, `Not`, or quantifier semantics unsound.

4. **Direct/reactive integration.** Where Prophetic Rete proves exactly which property writes can affect a Direct residual, wake the Law from those changes rather than polling every bearer unnecessarily.

5. **Promotion scheduling policy.** The measured 4.573 ms stress promotion cost was tiny relative to the hostile saved work, but larger worlds may need promotion itself budgeted or shared.

6. **Route-build dedup complexity.** `SlowAdapter::build()` still performs linear duplicate search through `road.members`, giving O(E_c U_c) / potentially O(E_c^2) construction behavior. A hash/set membership structure can lower discovery cost independently of Direct.

7. **Source-comment cleanup.** The comment immediately above `refreshCandidateRoute()` still says all equal-width higher tiers are refused. That is now stale: equal-width **AdapterRoad** is refused, while equal-width **LawDirect** may win because it eliminates proved condition work. This should be corrected in the next source-touching pass.

---

# 22. Final interpretation

The important result is not "4.02x."

It is the change in the shape of computation.

Before:

```text
every pulse
  every Law
    every candidate
      ask the relation graph again:
        "are you still one of the things I already proved you were?"
```

After:

```text
when structure changes:
  prove the route

while structure stays current:
  follow the route
  evaluate only what is still genuinely unknown
```

The stress witness makes the difference deliberately enormous:

```text
131,072 repeated relation queries
16,908,288 returned relation references
            ->
0 repeated queries
0 returned relation references
```

The real Chess world then shows the same architecture at ordinary scale:

```text
42 Laws graduated to Direct
10.942291 ms median -> 9.611333 ms
20.071250 ms p95   -> 11.163584 ms
```

The lower tiers have finally become what Zach described: **the means by which Earthcall establishes a more direct relation, rather than the road Earthcall is condemned to rediscover forever.**

That is the first concrete step from repeated relevance search toward a self-refining Law execution hierarchy.

---

*Written by GPT-5.6 Sol, session `gpt-5.6-sol/pr234-law-direct-analysis-20260919`, 2026-09-19 02:29 PDT. Measurements are from CI run `35431978980` on the merged PR #234 implementation. Zach originated the requirement that lower tiers establish the Law-Direct tier and requested the deliberately dramatic pre-Direct-vs-Direct probes. Claude Opus 5's earlier Slow Adapter and derived-state analyses supplied the documented failure mode and invalidation discipline this implementation tested against.*
