# Rendering Relevance Economics After PR #329

**Status:** bounded analysis complete  
**Date:** 2026-09-23  
**Evidence lineage:** PR #350, successor to PR #329  
**Canonical branch at write-up:** `sync-from-earthcall-main` @ `67bb9d0cd446c557bedd01c7d9544d7b6f89ec2a`

## Executive conclusion

PR #329 established a Scene-Spatial synthesis/proof substrate without granting it renderer authority. The successor investigation asked the next question: **even when a proof is true, is asking whether it applies cheaper than simply evaluating the authored mathematics?**

For the maintained single-authored-Perlin WebGPU workload, the answer is sharply split:

1. Merely carrying proof-capable shader topology while proof traversal is OFF has no stable, material cost in the measured witness. A separate production NO-PROOF shader is therefore not justified by this evidence.
2. Turning generic per-sample relevance traversal ON is substantially slower: about 34–38% in the two maintained camera views.
3. The slowdown is not explained by recurring proof-buffer uploads. Those were zero. The dominant failure is the economics of relevance discovery itself.
4. The current generic traversal asks vastly more relevance questions than the number of exact evaluations it avoids. Horizon required 83,649 candidate consultations to avoid 144 exact samples: roughly 581 consultations per saved sample.
5. Attempts to reduce apparent query count with direct-run artifacts still fail once hidden record tests are counted. One representative horizon case performed 144,135 record tests to avoid 139 exact samples.
6. For an arbitrary Perlin ray-march sample, there is no already-known semantic theorem key. Deriving such a key from position is itself spatial classification. A supposed O(1) theorem table does not solve the problem if a spatial search is required to discover the table index.

The architectural lesson is therefore:

> **Prophetic rendering is promising where execution identity is already known. It is not automatically economical where the renderer must first search for which prophecy applies.**

Or more compactly: **the proof can be cheap; the pilgrimage to the proof can be expensive.**

This analysis grants no new renderer authority.

---

## 1. Problem statement

The post-PR329 investigation began from an important distinction.

A theorem may be mathematically correct and still be a bad optimization.

The relevant cost equation is not merely:

```
exact evaluations avoided > 0
```

It is closer to:

```
benefit =
    cost(exact work avoided)
  - cost(relevance discovery)
  - cost(proof tests)
  - cost(dispatch)
  - cost(artifact construction/repair)
  - cost(upload/residency effects)
  - cost(pipeline/topology complexity)
```

If the renderer spends more effort proving that it may skip an authored computation than the computation itself would have cost, the theorem remains true but the optimization loses.

The successor therefore treated relevance as an economic problem rather than a theorem-breadth problem.

---

## 2. Invariants held throughout the investigation

The experiment was deliberately constrained so performance work could not silently alter Earthcall semantics.

### 2.1 Exact authored mathematics remains sovereign

Proof machinery is conservative acceleration metadata. Unknown, stale, unsupported, or invalid proof state must fail open to the exact authored path.

### 2.2 PR #329 remains a zero-authority boundary

The Scene-Spatial / semantic observer work did not gain pixel-changing authority merely because a benchmark found useful facts. Promotion requires a separately measured consumer.

### 2.3 Semantic channels remain distinct

Canonical mathematical equality does not imply semantic interchangeability.

In particular:

- source radiance `rho` is not medium density;
- density is not extinction;
- extinction is not scattering;
- scattering is not chroma;
- phase/directionality is independent;
- V4 self-emission is independent.

A zero-density theorem therefore cannot erase independently authored self-emission.

### 2.4 V1–V4 volumetric semantics remain independent

No relevance optimization in this investigation was allowed to collapse the authored transport semantics introduced across the volumetric rungs.

---

## 3. The three-arm experiment

The first task was to separate three hypotheses that are easy to conflate:

1. perhaps merely carrying proof-capable WGSL makes the default renderer slower;
2. perhaps proof traversal is expensive only because of upload/residency churn;
3. perhaps the traversal/query itself is the cost.

The benchmark therefore compared three distinct arms.

### Arm A — NO-PROOF-SHADER

Range-proof WGSL was structurally absent. The proof storage binding, proof helper/traversal code, proof-named fields, and per-iteration proof branch were removed while neutral instance slots were retained so the instance stride remained comparable.

### Arm B — PROOF-CAPABLE-OFF

The production proof-capable topology remained present, but traversal was disabled.

### Arm C — PROOF-ON

The same proof-capable topology executed relevance traversal.

The benchmark also balanced all six arm orderings to reduce ordering bias and required semantic parity between NO-PROOF and PROOF-CAPABLE-OFF.

Native evidence came from focused workflow run `35923074892` / #2923 on code head `86b98fcbc49a93b114922829633060174d4fde8d`. All four required jobs succeeded:

- SDF range-proxy verification;
- Focused CPU;
- authored-Perlin Release A/B;
- Slow Adapter independent clock.

The native witness was 2880x1800.

---

## 4. Shader-topology result

Proof-capable WGSL measured **45,590 bytes**. The structurally proof-free form measured **39,635 bytes**, a reduction of **5,955 bytes**.

The proof-free benchmark source generator was actually slower in isolation because it used string surgery:

- proof-free source generation median: 0.196875 ms;
- proof-capable generation median: 0.008958 ms;
- 25 samples.

This is a cold/source-generation artifact, not steady-frame evidence. It is specifically a reason **not** to mistake the benchmark seam for a production architecture.

The important question was whether the larger dormant shader imposed a reproducible steady-frame cost.

---

## 5. Dormant proof topology: no stable material tax

### Horizon view

| Metric | NO-PROOF | PROOF-CAPABLE-OFF |
|---|---:|---:|
| Median wall time | 54.094167 ms | 55.374938 ms |
| Balanced paired ratio | — | 1.0179x |
| Balanced paired delta | — | +0.940751 ms |
| CPU gather median | 0.006542 ms | 0.006416 ms |
| CPU submission median | 0.160209 ms | 0.143813 ms |
| Resident range bytes | 256 | 256 |

The paired wall difference was approximately **+1.79%** for proof-capable OFF.

### 45-degree view

| Metric | NO-PROOF | PROOF-CAPABLE-OFF |
|---|---:|---:|
| Median wall time | 45.776646 ms | 45.584958 ms |
| Balanced paired ratio | — | 0.9896x |
| Balanced paired delta | — | -0.497604 ms |
| CPU gather median | 0.008333 ms | 0.005937 ms |
| CPU submission median | 0.234604 ms | 0.136208 ms |
| Resident range bytes | 256 | 256 |

Here the sign reversed: proof-capable OFF was approximately **1.04% faster** by the paired measure.

GPU timestamps were unavailable on the runner, so this analysis makes no GPU-only attribution.

### Interpretation

A real structural penalty should reproduce with directionally stable evidence. Instead, the small difference reversed sign between views, CPU gather/submission did not reveal a dormant-proof penalty, and residency was identical.

**Verdict:** there is no stable evidence here for a material default-OFF proof-topology tax.

Therefore Earthcall should **not** introduce a separate production no-proof shader topology on the basis of this experiment. Doing so would increase shader/pipeline architecture complexity without a reproducible measured benefit.

The NO-PROOF arm remains valuable as an experimental control.

---

## 6. Active traversal: the cost appears immediately

Turning traversal ON changed the picture dramatically.

### Horizon

- PROOF-CAPABLE-OFF median: 55.374938 ms
- PROOF-ON median: 74.635250 ms
- paired ratio: **1.3428x**
- paired delta: **+19.044000 ms**

### 45-degree

- PROOF-CAPABLE-OFF median: 45.584958 ms
- PROOF-ON median: 62.748500 ms
- paired ratio: **1.3775x**
- paired delta: **+17.246916 ms**

That is roughly a **34–38% slowdown**.

At the same time, recurring range-proof uploads were **0 bytes**.

This sharply narrows the causal search. The measured problem is not recurring upload churn, and the dormant shader itself did not show a stable material penalty. The cost appears when the renderer actually performs relevance discovery and proof execution.

---

## 7. Why “queries” alone are a misleading metric

The investigation then examined whether relevance work at least eliminated enough exact authored evaluations to justify its cost.

For the horizon witness, generic traversal performed:

- **83,649 candidate consultations**;
- **144 exact samples avoided**;
- **0.001721 avoided samples per consultation**;
- approximately **581 consultations per exact sample avoided**.

The inherited 45-degree baseline was approximately **476 consultations per saved sample**.

This immediately explains why a mathematically valid proof can lose economically: the exact computation is being replaced by hundreds of relevance decisions.

But top-level query count is still not enough.

A supposedly cheaper direct-run representation could report fewer artifact queries while each query scans multiple records. That hidden work must be counted.

A representative horizon Z/min-run-1 candidate reported:

- **16,015 top-level artifact queries**;
- **144,135 underlying record tests**;
- **139 exact samples avoided**.

The query count looked dramatically better than the generic 83,649 number. The real record-test count was worse.

The direct-run candidates therefore failed the explicit **10x graduation threshold**:

- best horizon combined gain: **2.3578x — REJECT**;
- best 45-degree combined gain: **2.7271x — REJECT**.

### Measurement principle

Any future relevance benchmark must account for the entire decision path:

```
semantic query
    -> candidate lookup
    -> hierarchy/range traversal
    -> record tests
    -> metadata tests
    -> dispatch
    -> exact evaluations avoided
```

Counting only the outermost function call can turn hidden linear work into a fake O(1) victory.

---

## 8. The key architectural question: can relevance discovery disappear?

Once range-grid and direct-run approaches failed, the investigation asked a stronger question:

> Can the Scene DAG compile relevance into execution so that the renderer no longer searches for applicable proofs at all?

The desired shape is:

```
already-known execution identity
    -> fixed artifact slot
    -> bounded metadata test
    -> exact or proven action
```

The rejected shape is:

```
sample position
    -> search/classify
    -> find theorem slot
    -> theorem test
    -> action
```

The second structure may have a fast theorem lookup after classification, but the classification is the relevance query. Renaming the final lookup does not eliminate the search.

---

## 9. Why the single-Perlin marcher does not have a direct key

At an arbitrary ray-march step the renderer already knows the sample position.

What it does **not** already know is a stable semantic identifier meaning:

> this sample belongs to theorem region N.

To obtain that identifier, the renderer must classify the point spatially. Candidate mechanisms include:

- range-grid lookup/traversal;
- direct-run scanning;
- hierarchy traversal;
- another spatial index;
- some equivalent classification function.

Every one of these is relevance discovery.

Therefore a benchmark of the form:

```
sample -> synthetic integer -> theorem table[integer]
```

would be misleading unless production already possesses that integer for independent reasons.

If production must actually do:

```
sample position
    -> derive synthetic integer through spatial classification
    -> theorem table[integer]
```

then the expensive operation has merely been moved outside the measured lookup.

The investigation deliberately did **not** implement such a synthetic test-only dispatch table, because it would prove only that array indexing is fast. It would omit the operation under investigation.

This is an important negative result rather than a failure to implement.

---

## 10. Where direct prophetic dispatch may still win

The negative result is specific to generic **per-sample spatial relevance discovery** for the maintained Perlin workload. It is not a rejection of Scene-DAG synthesis or prophetic rendering.

There are renderer boundaries where semantic identity may already be known before any proof decision.

Examples include:

- a known radiance source binding;
- a known medium binding;
- a known Object/program;
- a known semantic channel;
- a compiled authored subexpression already selected by execution;
- other stable execution slots created for reasons independent of theorem lookup.

At such a boundary, a future artifact could plausibly have the shape:

```
known_source_slot -> conservative action metadata
```

rather than:

```
world_position -> search_for_source_or_region -> action metadata
```

That difference is fundamental.

The current `RenderedFieldSemanticObserver` hints at this distinction because its caller already knows whether it is observing a source-rho or medium-density binding. But the observer is intentionally diagnostic and is **not** yet an authority substrate.

---

## 11. Identity and lifetime requirements before authority

The current diagnostic observer uses identity roughly shaped as:

```
(Channel, Piecewise*, revision)
```

That is acceptable for observation but insufficient as a future authoritative execution key without stronger lifetime guarantees.

An authoritative consumer must survive hostile cases such as:

- premise mutation;
- source removal and re-addition;
- address/slot reuse;
- revision changes;
- stale artifacts;
- producer-contract mistakes.

A stale or ambiguous artifact must fail open to exact evaluation.

Raw address equality plus revision bookkeeping cannot simply be assumed to provide semantic lifetime identity. A future promoted consumer needs stable identity/provenance appropriate to Earthcall's object and authored-property model.

---

## 12. Requirements for any future consumer

A future relevance consumer should not inherit authority merely because the underlying theorem is sound. It should pass a separate economic and semantic gate.

### Hot-path accounting

Measure:

- dispatch lookups;
- metadata tests and branches;
- record tests;
- hierarchy walks;
- hash probes;
- spatial searches;
- exact evaluations avoided.

A claim of direct dispatch should report **zero hidden relevance search**.

### Construction and repair accounting

Measure:

- artifact compile/build time;
- incremental repair time;
- bytes produced;
- resident bytes;
- upload bytes and frequency;
- pipeline/shader effects.

### Incrementality

An authored mutation should repair only its affected dependency frontier. Unaffected artifact slots should preserve identity and cached state.

Camera movement or unrelated runtime motion should not rebuild semantic artifacts unless the relevant theorem explicitly depends on those values.

### Hostile fail-open tests

Test:

- mutation;
- removal/re-addition;
- slot reuse;
- stale revision;
- deliberately stale artifact presentation;
- unsupported theorem forms.

Every uncertain case returns to exact authored evaluation.

### Semantic sovereignty

Canonical math sharing must not merge semantic authority across channels.

In particular, zero density does not imply zero emission, and byte-identical expressions in `SourceRho` and `MediumDensity` remain semantically distinct.

### Native A/B before pixel authority

A test-only artifact can demonstrate correctness and accounting, but it cannot justify an FPS claim or pixel authority.

Promotion requires a native pixel-authoritative A/B against the exact path.

---

## 13. Rejected hypotheses

### Hypothesis 1: dormant proof WGSL is the main default renderer tax

**Rejected.** The small difference reversed sign between the two maintained views. CPU-side measurements and residency did not reveal a stable dormant penalty.

### Hypothesis 2: zero recurring uploads should make proof traversal cheap

**Rejected.** PROOF-ON remained roughly 34–38% slower while recurring proof uploads were zero.

### Hypothesis 3: fewer top-level artifact queries imply good economics

**Rejected.** Hidden record tests can dominate. The representative direct-run candidate turned 16,015 apparent queries into 144,135 record tests.

### Hypothesis 4: a synthetic direct-dispatch integer proves the Perlin problem solved

**Rejected.** If the integer must be derived from sample position through spatial classification, the expensive relevance query still exists.

### Hypothesis 5: the next step is simply proving more theorems

**Rejected for this task.** The current failure is not a shortage of true facts. It is the cost of delivering relevance to the consumer.

### Hypothesis 6: canonical mathematical identity permits cross-channel authority

**Rejected.** Semantic channel is part of the meaning of the authored field. Equal math does not collapse rho, density, extinction, scattering, chroma, phase, or emission into one authority lane.

---

## 14. A useful model: relevance economics

The experiments suggest treating relevance machinery as an execution market.

Let:

- `E` = cost of one exact authored evaluation;
- `S` = number of exact evaluations safely skipped;
- `Q` = relevance consultations;
- `R` = hidden record/branch tests;
- `D` = direct dispatch cost;
- `B` = amortized artifact build/repair cost;
- `M` = memory/residency/upload cost.

A rough profitability condition is:

```
S * E > cost(Q, R, D) + B + M
```

The current Perlin witness fails because `Q` and especially hidden `R` are large relative to `S`.

The strongest future optimization is therefore not necessarily a faster search. It is a transformation that makes `Q` and `R` disappear from the hot path because execution identity already selects the relevant artifact.

This is why **already-known execution keys** are the promising boundary.

---

## 15. Implications for Scene-DAG / Prophetic Rendering architecture

PR #329's conceptual direction survives this investigation, but with a stricter economic interpretation.

A Scene DAG can be valuable in at least three distinct ways:

1. **Compile shared authored structure once.**  
   Reuse canonical authored subexpressions and preserve incremental repair.

2. **Propagate conservative semantic facts ahead of execution.**  
   Prove things such as zero contribution/support where sound.

3. **Attach those facts to execution identities that already exist.**  
   This is the critical economic step. If the renderer already knows the source/program/medium/channel slot, theorem selection can become part of dispatch rather than a separate search.

The third point is what distinguishes potentially profitable prophetic execution from a conventional acceleration structure decorated with proofs.

A spatial acceleration structure may still be worthwhile on its own merits. But if its purpose is solely to discover which theorem applies, its lookup cost must be compared honestly with the exact authored work it replaces.

---

## 16. What should *not* be done next

This investigation does not support:

- splitting production into proof and no-proof shader families merely because proof WGSL is larger;
- broadening theorem families before identifying an economical consumer;
- reviving range-grid/direct-run designs without materially different full-cost accounting;
- claiming direct dispatch by excluding key derivation from the benchmark;
- allowing diagnostic `Piecewise*` identity to become authoritative without lifetime guarantees;
- using one volumetric channel's theorem to erase another channel's authored semantics;
- granting PR329 observer state pixel authority.

These would either contradict the measurements or skip the authority gate.

---

## 17. What a legitimate successor should investigate

A future successor should begin with a consumer for which the execution key is already present independently of proof lookup.

A strong first question is not:

> “How can we search the proof database faster?”

It is:

> “Where does the renderer already know exactly which authored semantic thing it is executing, and can the Scene DAG attach conservative action metadata directly to that identity?”

Candidate boundaries include source admission, medium admission, known Object/program dispatch, channel dispatch, and compiled shared authored subexpressions.

Such a successor should remain bounded. It should pick one consumer, prove hostile fail-open identity behavior, measure full construction/repair/residency costs, then run a native exact-vs-authoritative A/B before promotion.

If the consumer requires a new search merely to locate its proof, it should be rejected early.

---

## 18. Final verdict

The relevance-economics successor after PR #329 reached a genuine stopping point.

**Settled findings:**

- Dormant proof-capable shader topology showed no stable material default-OFF tax in the maintained witness.
- A separate production NO-PROOF shader is not justified by these measurements.
- Active generic relevance traversal was approximately 34–38% slower.
- Recurring proof uploads were zero, so upload churn does not explain the slowdown.
- Generic horizon traversal required approximately 581 relevance consultations per exact sample avoided.
- Direct-run variants remained uneconomic after hidden record tests were included.
- The maintained single-Perlin per-sample problem has no already-known theorem key; deriving one from position recreates spatial relevance discovery.
- Synthetic direct-dispatch benchmarks that omit key derivation would be invalid evidence.
- Already-keyed semantic execution boundaries remain promising future consumers.
- No proof state earned pixel authority in this investigation.

The deepest result is not that “prophetic rendering failed.” It is more precise:

> **Prediction is valuable when it can be fused into an execution identity the machine already possesses. Prediction becomes expensive when the machine must repeatedly search the world to discover which prediction is relevant.**

For Earthcall, the next frontier is therefore not broader prophecy. It is **prophecy attached to identity**—with exact authored truth beneath it, local invalidation around it, and measured economics before authority above it.
