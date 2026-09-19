# Law-Direct Traversal: Complexity Analysis and CI Results

**Date:** 2026-09-19  
**Status:** Post-implementation analysis; implementation merged in PR #234  
**CI evidence:** Earthcall focused CI run `35431978980`  
**Implementation merge:** `1de9c2801456aa9bdadeb7dd12c5cd72c1cbf21b`  
**Primary code:** `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, `ConditionModel.cpp`  
**Primary probes:** `tests/law/law_direct_stress_test.cpp`, `slow_adapter_parity_test.cpp`, `tests/singularity/slow_adapter_zone_perf_test.cpp`  
**Historical diagnosis:** `docs/Analysis/SLOW_ADAPTER_CLOCK_COUPLING_AND_ADAPTIVE_COMPUTE_ANALYSIS_2026-09-17.md`  
**Derived-state contract:** `docs/architecture/law/DERIVED_STATE_LEDGER.md`

---

## Executive summary

Zach's architectural question was precise: once the lower relevance tiers have already discovered a
stable Law-to-category route, why should the Law keep re-traversing that category proof on every
execution? The Slow Adapter had already learned retained roads, and Earthcall could already reify a
Law `routes-through` relation to a Formation, but the executable candidate ladder terminated at
`AdapterRoad`. There was no terminal hot-path tier that consumed the discovered relevance as a
direct derived binding.

PR #234 added that missing rung:

```text
Sweep -> Vocabulary -> AdapterRoad -> LawDirect
```

For the first implemented form, a current single positive conjunctive
`Related(relationKind, category)` road may crystallize into:

1. a concrete vector of relevant bearer pointers; and
2. a residual predicate in which **only that exact proved positive Related conjunct** is discharged.

Dynamic conditions remain live. `Any`, `Not`, quantifier-contained occurrences, multiple-road
ambiguity, opaque predicates, and stale currency fall downward rather than being guessed.

The result is not merely a label change. CI demonstrates actual work elimination.

### Dramatic Chess-shaped synthetic witness

The stress probe used:

- 32 category members;
- 128 category-scoped Laws;
- 128 irrelevant relations per member plus the useful category relation;
- 16 event pulses per measured batch;
- 3 alternating A/B rounds;
- Slow Adapter **enabled in both arms**;
- only Law-Direct toggled OFF vs ON.

Measured result:

| Metric | Immediately pre-Direct | Law-Direct | Change |
|---|---:|---:|---:|
| Median batch time | 13,827.417 ms | 3,439.637 ms | **4.02x speedup** |
| Relation graph queries | 131,072 | **0** | **100% eliminated** |
| Relation fan-out returned | 16,908,288 | **0** | **100% eliminated** |
| Law application records / batch | 65,536 | 65,536 | identical |
| One-time promotion median | — | 4.573 ms | paid once |
| Promotion amortization | — | 0.01 stress pulses | effectively immediate |

### Real authored Chess

The authored `saves/worlds/chess_app.json` probe measured three arms on the same CI runner:

| Mode | Frame median | Frame p95 | Sim-equivalent FPS | Law median |
|---|---:|---:|---:|---:|
| Adapter OFF, Direct OFF | 9.316 ms | 10.141 ms | 107.34 | 1.678 ms |
| Adapter ON, Direct OFF | 10.942 ms | 20.071 ms | 91.39 | 2.080 ms |
| Adapter ON, Direct ON | **9.611 ms** | **11.164 ms** | **104.04** | **1.866 ms** |

Against the immediately-pre-Direct architecture, Direct reduced Chess median frame time by about
**12.2%** and p95 frame time by about **44.4%**. Forty-two Chess Laws graduated to the
`law-direct` tier:

```text
tier_direct=42
tier_adapter=0
tier_vocabulary=16
tier_sweep=12
```

Relative to Adapter OFF, the completed Adapter + Direct system was only 0.295 ms slower at the frame
median in this run (9.611 vs 9.316 ms, about 3.2%), well inside the CI guard. The previous pathological
state — adapter enabled but no terminal direct rung — was the expensive one.

The most important conclusion is therefore structural:

> The Slow Adapter was not fundamentally wrong to discover category roads. The missing optimization
> was that successful lower-tier discovery had nowhere higher to graduate. Law-Direct turns the
> category road from a route traversed forever into construction scaffolding for a higher retained
> relevance binding.

---

## 1. What changed in the executable path

Before Law-Direct, a steady category-scoped event Law commonly followed this shape:

```text
event
  -> cached candidate-tier decision
  -> Vocabulary or AdapterRoad candidate set
  -> per-candidate required-property filter
  -> full condition
       -> Related(...) queries relation graph
       -> dynamic residual tests
  -> applyTo(...)
       -> full condition AGAIN on successful pre-check
            -> Related(...) queries relation graph AGAIN
            -> dynamic residual tests AGAIN
       -> action / audit / provenance
```

The exact duplicate second condition evaluation matters. For candidates whose first condition check
passed, `applyTo()` historically checked the full condition again before acting. A graph-shaped
`Related` condition could therefore be paid twice per successful candidate.

Law-Direct changes the steady path to:

```text
event
  -> cached LawDirect route
  -> concrete bearer vector
  -> residual condition
       -> dynamic residual tests only
       -> proved Related(...) conjunct is already discharged
  -> applyToAfterConditions(...)
       -> authority / jurisdiction / action / audit / provenance
       -> no duplicate condition evaluation
```

This is deliberately not "cache that the Law condition was true." Property values remain dynamic.
What is cached is narrower:

> **This current bearer belongs to this current proven relation road.**

That proof is invalidated by the existing Law/world/graph currency signals.

---

## 2. Symbols used in the complexity model

Let:

- `B` = total beings eligible in the world;
- `L` = number of Laws in the considered execution set;
- `M_l` = candidate/relevant bearers for Law `l`;
- `M` = representative/average `M_l`;
- `D_s` = endpoint relation degree of subject `s`, i.e. number of relations returned by
  `relationsInvolving(s)`;
- `D` = representative/average endpoint degree for candidates;
- `R` = total relation count in the world;
- `K_l` = number of required property names used by `Law::couldApplyTo`;
- `Q_l` = cost of the live residual condition after the proved relation conjunct is removed;
- `C_l` = size of Law `l`'s condition tree;
- `P` = number of event pulses / continuous evaluation opportunities;
- `S_l` = fraction of candidates whose first condition check succeeds and therefore reaches
  `applyTo`;
- `A_l` = cost of the Law's actual action/audit/provenance work after its condition holds;
- `E_c` = number of relation edges inspected while constructing a Slow Adapter road for category
  `c`.

The stress probe has:

```text
M = 32
L = 128
D = 129        // 128 irrelevant + 1 useful edge
P = 16
S = 1          // every intended member satisfies the live residual
```

---

## 3. Steady candidate-tier selection is already O(1) per Law

`refreshCandidateRoute()` is not supposed to redo route discovery per candidate. In the steady
case it checks cached currency:

- `Law::textRevision()`;
- `law.conditionRevision()`;
- `Universe::structuralRevision()`;
- `Universe::relationGeneration()`;
- the adapter road currency generation.

When these match, the route decision returns after a constant number of map lookups/comparisons.

Therefore the steady **tier-choice** cost is:

```text
Theta(1) per Law
Theta(L) per pulse across L Laws
```

This must be separated from the cost of **consuming the chosen candidate set**, which is necessarily
at least proportional to the number of bearers a Law must actually govern.

---

## 4. Immediately-pre-Direct hot complexity

### 4.1 Vocabulary candidate retrieval

When Vocabulary is the selected tier, Earthcall reads the retained vector for the chosen property
seed and walks its candidates.

Ignoring rare invalidation rebuilds, retrieval is:

```text
Theta(M_l)
```

because each candidate is visited and may be checked by `Law::couldApplyTo`.

If `K_l` required names are tested and property-presence lookup is abstracted as `H`, the
required-property filtering term is approximately:

```text
O(M_l * K_l * H)
```

This is not the dominant term in the hostile relation-heavy case, but it is real.

### 4.2 Related condition evaluation

Current `ConditionNode::Related` asks:

```cpp
Universe::instance().relationsInvolving(subject, edges)
```

then examines the returned endpoint neighborhood until it proves or rejects the requested relation.

For candidate `s`, worst-case relation work is:

```text
Theta(D_s)
```

With average degree `D` over `M_l` candidates:

```text
Theta(M_l * D)
```

per full condition pass.

### 4.3 Duplicate condition evaluation on successful applications

Before Direct, the event path did a full condition check before application, then successful
candidates entered `Law::applyTo()`, which performed the full condition again.

For a Law with success fraction `S_l`, the expected number of full condition evaluations per
candidate is:

```text
1 + S_l
```

Thus the graph-shaped portion is:

```text
Theta(M_l * D * (1 + S_l))
```

per Law per pulse.

Since `0 <= S_l <= 1`:

```text
Theta(M_l * D) <= cost <= Theta(2 * M_l * D)
```

The constant two is not asymptotically interesting, but it is operationally important; the stress
probe deliberately chooses `S = 1` so the duplicated work is visible.

### 4.4 Across many Laws and pulses

For homogeneous `L` Laws, `M` candidates, average degree `D`, and `P` pulses:

```text
PRE-DIRECT RELATION WORK
  = Theta(P * L * M * D * (1 + S))
```

With all candidates succeeding:

```text
  = Theta(2 * P * L * M * D)
```

The number of **relation graph queries** is not multiplied by `D`; each query returns a neighborhood
of size `D`.

So:

```text
graph-query calls = P * L * M * (1 + S)
edge examinations  = Theta(P * L * M * D * (1 + S))
```

---

## 5. Exact derivation of the stress-probe counts

The hostile probe uses:

```text
P = 16
L = 128
M = 32
S = 1
D = 129
```

Therefore pre-Direct relation-query calls are:

```text
P * L * M * (1 + S)
= 16 * 128 * 32 * 2
= 131,072
```

That is exactly what CI observed:

```text
relation_queries=131072
```

Each query returns the member's 129-edge endpoint neighborhood, so total returned relation fan-out is:

```text
131,072 * 129
= 16,908,288
```

Again, this is exactly what CI observed:

```text
relation_fanout_returned=16908288
```

This exact arithmetic agreement is stronger evidence than timing alone. The probe is measuring the
intended hot relation proof, not an unrelated source of work.

---

## 6. Law-Direct promotion complexity

Law-Direct does not appear from nowhere. It is constructed from a current Slow Adapter road.

For one eligible Law:

1. fetch the already-built road of size `M_l`;
2. inspect the Law's condition tree enough to verify one safe positive route;
3. filter road members through `Law::couldApplyTo`;
4. compile a residual predicate with that exact relation conjunct discharged;
5. store the concrete bearer vector and currency witnesses.

Abstractly:

```text
promotion_l
  = O(M_l * K_l * H + C_l)
```

No `Related` graph query is required by the direct promotion itself; the relation proof came from
the current retained road.

For `L_d` Laws promoted from a shared road:

```text
O(sum_l(M_l * K_l * H + C_l))
```

with the current implementation, because each Law stores its own direct bearer vector.

In the stress run, median one-time promotion for all 128 Laws was only:

```text
4.573 ms
```

while one pre-Direct measured pulse cost hundreds of milliseconds. CI computed amortization at about:

```text
0.01 stress pulses
```

So for this shape, promotion cost is negligible after the first use.

---

## 7. Law-Direct steady hot complexity

Once current, Direct returns the concrete bearer vector.

### 7.1 Candidate traversal

A category-wide Law governing `M_l` beings must still visit those `M_l` beings:

```text
Theta(M_l)
```

That is not accidental overhead. It is the irreducible cost of governing `M_l` distinct subjects.

### 7.2 Residual condition

The proved `Related` leaf is replaced with true **only in the safe positive conjunctive position**.
The remaining dynamic predicate is evaluated normally.

Thus:

```text
Theta(M_l * Q_l)
```

where `Q_l` excludes the discharged graph proof.

### 7.3 Application

After the residual passes, `applyToAfterConditions()` retains authority, jurisdiction, action,
trace, audit, provenance, drive-session and publication behavior, but does not repeat the condition.

If all `M_l` members apply and action cost is `A_l`:

```text
Theta(M_l * (Q_l + A_l))
```

per Law per pulse.

Most importantly, for the discharged category relation:

```text
steady hot relation-query calls = 0
steady hot relation-edge scans  = 0
```

which CI confirmed exactly.

Across homogeneous Laws/pulses:

```text
LAW-DIRECT HOT WORK
  = Theta(P * L * M * (Q + A))
```

If `Q` and `A` are O(1) with respect to relation degree:

```text
  = Theta(P * L * M)
```

---

## 8. What asymptotic class actually changed

It is tempting to say "Law-Direct makes Law execution O(1)." That is false for a category-wide Law.

If a Law must correctly affect `M` distinct beings, any correct executor has a lower bound:

```text
Omega(M)
```

simply to visit/act on those `M` beings.

The meaningful change is:

```text
pre-Direct relevance proof: Theta(M * D)
Law-Direct relevance proof: Theta(M)
```

for one Law/pulse when the residual is O(1).

Across `P` pulses and `L` similar Laws:

```text
pre-Direct: Theta(P * L * M * D)
Direct:     Theta(P * L * M)
```

So Direct removes `D` — relation neighborhood degree — from the steady hot path.

This is asymptotically significant when `D` grows with world density, relation richness, or
category membership complexity.

If `D` is truly bounded by a small constant, both expressions collapse to `Theta(P*L*M)` in
traditional asymptotic notation, and Direct is a constant-factor optimization. Earthcall's authored
worlds do not guarantee a tiny fixed `D`; richer beings can accumulate many Relations, which is why
removing `D` is architecturally meaningful.

### Single-subject case

If a direct Law narrows to one bearer, `M = 1`:

```text
pre-Direct relation proof: Theta(D)
Direct relation proof:     Theta(1) candidate access + residual
```

This is the "O(1)-ish direct traversal" intuition in its purest form.

### Category-wide case

For category-wide Chess Laws with `M ~= 32`:

```text
pre-Direct: Theta(32 * D)
Direct:     Theta(32)
```

The 32 is not a failure to become direct. It is the number of real governed beings.

---

## 9. Why the synthetic speedup is 4.02x, not 129x

The stress world removed a theoretical relation-degree factor of:

```text
D = 129
```

Yet wall-clock speed improved by 4.02x rather than 129x.

That is expected. Relation traversal is only part of total execution.

Even after graph work disappears, Earthcall still pays for:

- iterating 65,536 Law/subject applications per batch;
- event dispatch;
- residual property comparison;
- Law authority/jurisdiction checks;
- application record creation;
- bounded application-log maintenance;
- applied-event publication;
- candidate-vector iteration/copying;
- Rete and LawManager bookkeeping;
- ordinary C++ allocation/cache effects in the probe.

Therefore the meaningful decomposition is:

```text
T_pre
  = T_irreducible
  + T_relation-proof

T_direct
  = T_irreducible
  + T_direct-overhead
```

The probe shows that removing relation proof cuts total time from 13.827 s to 3.440 s, so the
relation-proof tax represented roughly three quarters of this intentionally hostile workload.

The remaining 3.44 s is not evidence Direct failed; it is mostly the work that a correct execution
still has to do.

---

## 10. Real Chess: granular interpretation

### 10.1 Tier migration

Immediately pre-Direct:

```text
tier_direct=0
tier_adapter=1
tier_vocabulary=57
tier_sweep=12
```

With Direct:

```text
tier_direct=42
tier_adapter=0
tier_vocabulary=16
tier_sweep=12
```

Interpretation:

- 42 Laws found a safe single-road positive conjunctive proof and crystallized;
- the prior adapter-road consumer disappeared;
- 16 Laws still legitimately use vocabulary;
- 12 still require sweep;
- Direct is selective rather than globally forced.

This selectivity is an important soundness property.

### 10.2 Median frame cost

```text
pre-Direct = 10.942291 ms
Direct     =  9.611333 ms
saved      =  1.330958 ms
```

Fractional improvement:

```text
1.330958 / 10.942291 ~= 0.1216
```

or about **12.2%**.

### 10.3 Tail cost

```text
pre-Direct p95 = 20.071250 ms
Direct p95     = 11.163584 ms
saved p95      =  8.907666 ms
```

Fractional reduction:

```text
8.907666 / 20.071250 ~= 0.4438
```

or about **44.4%**.

This is more operationally important than the median alone because the pre-Direct p95 exceeded a
16.67 ms 60-Hz frame budget. Direct pulled the measured p95 comfortably below that boundary in this
run.

### 10.4 Law-only timing

```text
pre-Direct law median = 2.079667 ms
Direct law median     = 1.866000 ms
saved                  = 0.213667 ms
```

or about **10.3%**.

The whole-frame improvement is larger than the Law-only median improvement, which means the benchmark
still contains cross-system/cache/scheduling effects. This is why the structural work counters and
tier counts are stronger evidence than attributing every tenth of a millisecond causally.

### 10.5 Against adapter-off floor

```text
adapter off frame median = 9.316000 ms
adapter + Direct         = 9.611333 ms
difference               = 0.295333 ms
ratio                    = 1.03
```

The completed system is therefore close to the adapter-off floor while retaining the learned-road
architecture.

The historical failure was specifically the intermediate state:

```text
adapter ON + no terminal Direct consumer
```

not the existence of retained relevance roads itself.

---

## 11. Controls: worlds where Direct did not activate

The CI run also measured Basic Pixel Changer and Noise Floor.

### Basic Pixel Changer

```text
tier_direct=0
tier_adapter=0
tier_vocabulary=2
tier_sweep=6
```

Timings varied somewhat:

```text
pre-Direct 40.665 ms
Direct     38.431 ms
```

but **no Law was in the Direct tier**. Therefore that delta cannot be credited to Law-Direct; it is
runner/order/cache noise or unrelated measurement variation.

### Noise Floor

```text
tier_direct=0
tier_adapter=0
tier_vocabulary=0
tier_sweep=1
```

Again, small timing movement occurred with no Direct activation.

These controls are useful precisely because they prevent a false narrative in which every favorable
timing delta is attributed to Direct. The strongest causal evidence is where:

1. Direct actually activates;
2. semantic output remains identical;
3. graph-query counts collapse;
4. wall time improves in the same direction.

The synthetic witness satisfies all four. Real Chess satisfies 1, 2 through the surrounding tests,
and a large timing improvement; its tier migration is explicit.

---

## 12. Slow Adapter road-construction complexity remains separate

Law-Direct eliminates repeated **consumption-time relation proof**. It does not make discovery free.

Current `SlowAdapter::build()`:

1. resolves the category;
2. gets the category's endpoint edges, or falls back to all world Relations if the category is
   unresolved;
3. walks edges of the requested relation kind;
4. resolves the member through each edge;
5. deduplicates members with linear `std::find`.

If `E_c` candidate edges are considered and the final road has `M` unique members, current
deduplication gives worst-case:

```text
O(E_c * M)
```

because each candidate member may linearly scan the already-collected member vector.

In the common one-useful-edge-per-member shape where `E_c = Theta(M)`:

```text
O(M^2)
```

road construction is possible.

If the category cannot be resolved and the adapter must scan all `R` world Relations:

```text
O(R * M)
```

worst-case with the same linear deduplication.

This cost is now on the independent Slow Adapter clock rather than the ordinary frame cadence, and a
shared RouteKey can serve many Laws. It is nevertheless a remaining optimization target.

A straightforward future improvement is expected-O(1) member deduplication with a temporary pointer
or SingularId hash set:

```text
current build:  O(E_c * M)
hashed dedup:   O(E_c) expected
```

while preserving the ordered member vector as the published road.

That optimization is orthogonal to Law-Direct.

---

## 13. Vocabulary-index complexity and why Direct is not just "better Vocabulary"

The vocabulary index is rebuilt only when world structure or relevant Law text moves.

The rebuild walks beings and their registered property names once, so its cost is roughly:

```text
O(sum over beings of registered property names)
```

plus dotted-root checks.

Steady frames pay only revision comparisons.

Vocabulary is therefore already a good answer to:

> Which beings even carry the vocabulary this Law mentions?

But Vocabulary does **not** answer:

> Which of those beings are already proved to stand in the Law's required relation?

In Chess the key pathological shape was often:

```text
Vocabulary candidates = 32
Category road members  = 32
```

Pre-Direct route selection correctly rejected an equal-width AdapterRoad because changing from one
32-element vector to another buys no narrowing.

Law-Direct changes the economics. An equal-width Direct route may still be superior because it
eliminates a condition proof, not merely candidates.

Thus:

```text
equal-width AdapterRoad:
    32 -> 32, same condition work
    no benefit

equal-width LawDirect:
    32 -> 32, but Related(...) is already proved
    real benefit
```

This is why Direct is a distinct tier rather than merely a wider definition of AdapterRoad.

---

## 14. Memory complexity

The current Direct rung stores a concrete bearer vector per promoted Law.

For `L_d` direct Laws:

```text
memory = O(sum_l M_l)
```

pointer slots, plus one residual predicate and small currency/route metadata per Law.

For homogeneous `M`:

```text
O(L_d * M)
```

In Chess this is small. Even a rough upper estimate of 42 Laws * 32 pointers is 1,344 pointers; at
8 bytes per pointer that is about 10.5 KiB before vector/object overhead.

At much larger scales, however, many Laws can share the same underlying RouteKey. The Slow Adapter
already shares that road, while Law-Direct currently duplicates the filtered bearer vector per Law.

A future interning layer could key a shared direct bearer set by something like:

```text
(RouteKey, required-property signature, currency)
```

allowing:

```text
current: O(L_d * M)
possible shared form: O(M_shared + L_d)
```

for families of Laws with identical bearer eligibility.

This should be pursued only if profiling shows memory/copy cost matters; the current form is simple
and explicit.

---

## 15. Candidate-vector copy cost

Current `sweepSubjects()` copies the `directSubjects` pointers into a local `chosen` vector while
filtering out unmade beings.

Therefore even after relevance proof is direct, candidate materialization itself is:

```text
Theta(M)
```

and performs `M` pointer writes.

This is semantically fine but not the theoretical minimum allocation behavior.

A later non-owning view/span over the current direct vector could reduce allocation/copy overhead:

```text
current candidate materialization: Theta(M) writes
view-based materialization:        Theta(1) setup + Theta(M) consumption
```

The total execution still remains Omega(M) because the Law must visit `M` beings, but memory traffic
would improve.

---

## 16. Invalidation complexity and safety

A Direct route is derived state. Its currency depends on:

- Law text revision;
- Law condition revision;
- world structural revision;
- relation generation;
- adapter-road currency.

Property **value** changes do not invalidate the direct bearer set, intentionally. They are evaluated
by the residual condition live.

Property **presence** changes do matter because they may change `couldApplyTo` membership; current
dynamic-property insertion/removal bumps `Universe::structuralRevision()`, which invalidates the
route.

Relation changes move `Universe::relationGeneration()`, invalidating the category proof.

Thus the direct optimization follows Earthcall's core derived-state rule:

> when proof currency is uncertain, fall downward; never silently keep a narrow answer.

### Refresh cost

Steady currency check:

```text
Theta(1) per Law
```

Invalidated Direct rebuild:

```text
O(M_l * K_l * H + C_l)
```

plus whatever Slow Adapter discovery cost was required if the underlying road itself was stale.

If invalidation happens every pulse, Direct cannot amortize and should not be expected to dominate.
The system is specifically valuable where structural relevance is stable while values change — a
very common authored-world shape.

---

## 17. Soundness constraints on residual compilation

The residual compiler discharges the proved route only through positive conjunction.

Safe first-rung example:

```text
Related(instance-of, category.chess.piece)
AND gridX == 4
AND turn == white
```

After the category route is proved:

```text
true
AND gridX == 4
AND turn == white
```

The dynamic pieces remain live.

The proof does **not** automatically propagate through:

- `Any`;
- `Not`;
- quantifiers;
- unrelated nested logical contexts.

The parity suite contains the adversarial shape:

```text
Related(X) AND NOT Related(X)
```

which must remain impossible even if the positive `Related(X)` is sufficient to establish a direct
road.

This is why Law-Direct cannot be implemented safely as a blind textual replacement of every matching
relation leaf.

---

## 18. Complexity summary table

| Operation | Immediately pre-Direct | Law-Direct |
|---|---:|---:|
| Steady tier decision / Law | Theta(1) | Theta(1) |
| Candidate bearer traversal / Law | Theta(M) | Theta(M) |
| Required-property filtering hot path | O(M*K*H) | removed from repeated hot traversal; paid at promotion/currency rebuild |
| Proved category relation checks | Theta(M*D*(1+S)) | **0 graph queries steady-state** |
| Live residual condition | paid 1+S times on success path | paid once |
| Successful application condition re-check | yes | no |
| Hot relevance work across P,L | Theta(P*L*M*D) when D dominates | **Theta(P*L*M)** |
| Direct promotion | n/a | O(M*K*H + C) / Law |
| Slow Adapter road build | O(E_c*M), or O(R*M) fallback worst-case today | same; discovery layer |
| Direct memory | n/a | O(sum M_l) |
| Minimum possible category-wide execution | Omega(M) | **Theta(M) relevance traversal, asymptotically optimal in M** |

---

## 19. What the result does NOT prove

The green CI does not prove that every Law should become Direct.

It does not prove:

- multi-road conditions can be safely collapsed;
- quantified graph conditions can be made direct with the same mechanism;
- arbitrary closure predicates can be introspected;
- persistent authored `routes-through` Relations should be written automatically;
- PropertyPath-qualified terminal relevance is complete;
- the Slow Adapter's road construction is optimally implemented;
- Direct helps worlds where no eligible route exists.

The Basic Pixel Changer and Noise Floor controls explicitly show worlds where `tier_direct=0`.

The implementation should therefore remain a **sound optional graduation**, not a universal mandate.

---

## 20. Next optimization rungs suggested by these measurements

### 20.1 Shared direct bearer-set interning

Many Laws share one route. Intern filtered bearer sets by route + eligibility signature to reduce
memory/copy duplication from O(L*M) toward O(M+L) for shared families.

### 20.2 Span/view candidate consumption

Stop copying the direct bearer vector on every sweep. Consume a non-owning current view when safe.

### 20.3 Hash-based Slow Adapter deduplication

Replace linear `std::find` member deduplication during road build. Expected road-build complexity can
fall from O(E_c*M) toward O(E_c).

### 20.4 PropertyPath-qualified direct relevance

The current Direct rung answers mostly **whom**. The terminal architecture should also retain
**which aspect/path** of that bearer a branch cares about, connecting Prophetic branch relevance with
direct traversal.

That is the natural continuation:

```text
Law / branch
   -> concrete bearer
   -> relevant PropertyPath
```

so irrelevant property changes need not wake even the residual branch.

### 20.5 Multi-road proof composition

Only after sound proof algebra is explicit should multiple necessary roads be intersected into a
direct bearer set. This must widen/fall back whenever any constituent proof becomes incomplete.

---

## 21. Final interpretation

The old Slow Adapter regression was not evidence that retained relational roads were the wrong
architecture.

It exposed an incomplete ladder.

The system could:

1. search broadly;
2. narrow by vocabulary;
3. discover category roads;
4. retain them;

but then it **kept driving the category road forever** instead of compiling the discovery into a
higher execution form.

Law-Direct closes that first loop.

The synthetic probe demonstrates the pure algorithmic result:

```text
131,072 graph queries -> 0
16,908,288 returned relation edges -> 0
13.827 s -> 3.440 s
4.02x faster
same 65,536 lawful applications
```

Real Chess demonstrates that the result survives contact with an authored world:

```text
42 Laws -> LawDirect
10.942 ms median -> 9.611 ms
20.071 ms p95    -> 11.164 ms
```

The deepest result is not the measured 4.02x or 12.2%.

It is the change in the shape of the computation:

```text
search / prove relevance repeatedly
        |
        v
Theta(P * L * M * D)

becomes

discover -> prove -> crystallize -> execute residual
        |
        v
Theta(P * L * M)
```

for the proven relation component.

The lower tiers are therefore not merely alternative indexes. They can now act as **developmental
machinery**: slower, more general structures discover stable relevance, and the engine compiles that
knowledge upward into a cheaper execution form until changing reality invalidates the proof and
forces a safe descent.

That is the first implemented version of the architecture Zach was pointing at when he asked why the
Law kept rerunning the category tier after the lower tiers had already established the route.

---

## Provenance and signature

**Human architectural prompt / direction:** Zach explicitly identified that the existing lower tiers
should establish a higher "Law direct traversal" tier rather than forcing Laws to rerun category
traversal forever, and then requested deliberately dramatic probes comparing immediately-pre-Direct
execution against the implemented direct tier.

**Implementation and analysis:** GPT-5.6 Sol.

**Operational session ID:** `sol/law-direct-analysis-20260919`  
**Timestamp:** 2026-09-19 02:29 PDT  
**Evidence run:** GitHub Actions `35431978980`  
**Implementation PR:** #234  
**Implementation merge:** `1de9c2801456aa9bdadeb7dd12c5cd72c1cbf21b`

*Signed: GPT-5.6 Sol*
