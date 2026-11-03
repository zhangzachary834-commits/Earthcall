# Semantic Query-Space Rendering: Performance Model and Falsification Analysis

**Status:** analytical companion to the active Zach + Original Prism architecture proposal; **not measured implementation performance and not an implementation directive**  
**Date:** 2026-10-21  
**Human origin of the architectural hypothesis:** Zachary Zhang  
**Analysis and formalization:** Original Prism Sun / GPT-5.6 Sol  
**Broadcast status:** intentionally quiet while Zach and Original Prism continue developing the design.

**Companion proposal:** ../architecture/interrelations/SEMANTIC_STATE_SPACE_PROOF_REGIONS_AND_MATERIALIZED_VIEWS_PROPOSAL_2026-10-21.md  
**Related measured evidence:** ../agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md  
**Performance-analysis infrastructure:** ../Agenda/Tasks/Specific Tasks/Performance and Runtime/Standalone_Performance_Analysis/Standalone_Performance_Analysis.md

---

## 0. Question

Zach's stronger rendering hypothesis is not merely "use a better proof grid."

It is:

> Analyze semantic boundaries over unresolved movement and ray/query state, pull their relevance backward through the ray generator, and reuse one compiled semantic answer across an entire family of future rays while that validity region remains true.

This document asks one narrow question:

\[
\boxed{\text{Could that architecture plausibly be faster than the current Northern Veil path?}}
\]

The answer from this analysis is:

> **Yes, plausibly — but only if it eliminates enough repeated field and branch work, and only if the query-region lookup plus analysis amortization stay below explicit break-even thresholds.**

The central runtime-only sensitivity study places a broad plausible median near **82.7 ms/frame** against the controlled **138.5 ms/frame midpoint baseline**, or about **1.68x**, with a broad 5th–95th percentile speedup band of about **1.38x–2.21x** under the assumptions stated below.

Those are **model outputs, not benchmark results**.

---

## 1. What is actually measured today

The current ray-transport evidence gives several useful anchors.

The controlled native Northern Veil comparison used approximately:

\[
136\text{–}141\ \mathrm{ms/frame}
\]

for the relevant 2560x1440 offscreen view. This analysis uses the midpoint

\[
T_0 = 138.5\ \mathrm{ms/frame}
\]

only as a convenient reference value.

The current built camera-independent certified grid march measured approximately:

\[
2463\ \mathrm{ms}
\]

for the four-curtain depth-only frame-equivalent compute path, around 18x slower than the controlled native frame.

That current grid path also reported roughly:

\[
1000\ \sigma\text{-evaluations/pixel}
\]

plus roughly:

\[
1178\ \text{switch evaluations/pixel}
\]

on hit pixels.

The corresponding native path was estimated near:

\[
S_0 \approx 29
\]

positive-medium sigma evaluations per ray/pixel in that view.

The independent uniform-Simpson oracle reported roughly:

\[
8.5
\]

sigma samples/pixel for the target transmittance error.

Firmament's review then identified that the existing second-order Simpson mathematical coefficient was much looser than the direct unweighted C2 Peano-kernel coefficient and estimated that the existing second-order path might fall into approximately:

\[
10\text{–}14
\]

sigma evaluations/pixel after correcting the coefficient and avoiding unnecessary C4-only Perlin lattice splitting.

That last 10–14 figure is still a prediction until the corrected implementation is rerun.

---

## 2. What is not measured and therefore must remain an assumption

The repository does **not** currently contain a trustworthy decomposition of the 138.5 ms controlled frame into:

\[
T_{\sigma}
+
T_{\text{semantic/switch}}
+
T_{\text{other}}.
\]

In particular, we do not yet know directly:

- the fraction of native frame cost attributable to expensive field/sigma evaluation;
- the fraction attributable to branch/switch/support rediscovery that backward compilation could eliminate;
- the cost of a future query-validity lookup;
- the cost of instantiating a compiled ray-family template;
- the actual analysis build cost for a useful movement/query validity region;
- the useful lifetime of such a region under real Person/camera movement.

Therefore the simulation below is a **sensitivity analysis over explicit unknowns**, not a fitted performance prediction.

The existing standalone performance analyzer is well suited to recording these terms once fresh traces and workload counters exist. It should eventually replace assumed fractions with measured ones.

---

## 3. Runtime model

Split the current controlled frame into three conceptual fractions:

\[
f_{\sigma}
\]

for expensive field/sigma evaluation,

\[
f_{\mathrm{sem}}
\]

for repeated semantic/branch/switch/support reasoning that the new architecture may partly eliminate,

and

\[
1-f_{\sigma}-f_{\mathrm{sem}}
\]

for residual work assumed unchanged in the first-order model.

Let:

\[
S_0 = 29
\]

be the native field-evaluation count,

\[
S_n
\]

be the field-evaluation count after semantic/query-space compilation,

\[
e_{\mathrm{sem}}
\]

be the fraction of repeated semantic cost eliminated,

and

\[
Q
\]

be new per-frame query-regime lookup / ray-template instantiation overhead.

The first-order runtime model is:

\[
T_{\mathrm{new}}
=
T_0
\left[
(1-f_{\sigma}-f_{\mathrm{sem}})
+
f_{\sigma}\frac{S_n}{S_0}
+
f_{\mathrm{sem}}(1-e_{\mathrm{sem}})
\right]
+
Q.
\]

This model intentionally does not award speedup for work the architecture has not explicitly removed.

---

## 4. Uncertainty sweep

A deterministic Monte Carlo sensitivity sweep used:

\[
f_{\sigma}\sim U(0.50,0.75),
\]

\[
f_{\mathrm{sem}}\sim U(0.08,0.20),
\]

with samples rejected when:

\[
f_{\sigma}+f_{\mathrm{sem}}>0.92,
\]

so at least 8% of the baseline remains outside those two categories.

The candidate compiled path used:

\[
S_n\sim U(8,18),
\]

\[
e_{\mathrm{sem}}\sim U(0.65,0.95),
\]

and:

\[
Q\sim U(1.5,12.0)\ \mathrm{ms/frame}.
\]

The sweep used 200,000 draws and retained 196,964 valid combinations.

These ranges are **not measurements**. They are deliberately broad assumptions chosen to ask whether the architecture has a plausible economic basin at all.

### Result distribution

The resulting frame-time quantiles were:

| Quantile | Modeled frame time |
|---|---:|
| 5% | 62.6 ms |
| 25% | 74.1 ms |
| 50% | **82.7 ms** |
| 75% | 90.6 ms |
| 95% | 100.0 ms |

The corresponding speedup quantiles were:

| Quantile | Modeled speedup |
|---|---:|
| 5% | **1.38x** |
| 25% | 1.53x |
| 50% | **1.68x** |
| 75% | 1.87x |
| 95% | **2.21x** |

Interpretation:

> Under these broad assumptions, the architecture does not need an extreme best case to beat the controlled baseline. But the model is only as good as the unknown cost shares, which must be measured before any production claim.

---

## 5. Four concrete scenarios

To make the sensitivity easier to reason about, four deterministic scenarios use the same equation.

### Weak

Assume:

\[
f_{\sigma}=0.50,\quad
f_{\mathrm{sem}}=0.08,\quad
S_n=18,\quad
e_{\mathrm{sem}}=0.65,\quad
Q=12\mathrm{ms}.
\]

Then:

\[
T_{\mathrm{new}}\approx117.0\mathrm{ms}
\]

and:

\[
\text{speedup}\approx1.18x.
\]

Even a weak semantic win can still help, but only modestly.

### Plausible middle

Assume:

\[
f_{\sigma}=0.62,\quad
f_{\mathrm{sem}}=0.14,\quad
S_n=12,\quad
e_{\mathrm{sem}}=0.85,\quad
Q=6\mathrm{ms}.
\]

Then:

\[
T_{\mathrm{new}}\approx77.7\mathrm{ms}
\]

and:

\[
\text{speedup}\approx1.78x.
\]

### Strong

Assume:

\[
f_{\sigma}=0.70,\quad
f_{\mathrm{sem}}=0.16,\quad
S_n=8,\quad
e_{\mathrm{sem}}=0.92,\quad
Q=4\mathrm{ms}.
\]

Then:

\[
T_{\mathrm{new}}\approx51.9\mathrm{ms}
\]

and:

\[
\text{speedup}\approx2.67x.
\]

### Aggressive

Assume:

\[
f_{\sigma}=0.75,\quad
f_{\mathrm{sem}}=0.18,\quad
S_n=5,\quad
e_{\mathrm{sem}}=0.97,\quad
Q=2\mathrm{ms}.
\]

Then:

\[
T_{\mathrm{new}}\approx30.4\mathrm{ms}
\]

and:

\[
\text{speedup}\approx4.56x.
\]

This last scenario is an aspirational envelope, not a forecast. It requires both very strong specialization and tiny query-space overhead.

---

## 6. Why unknown movement changes the economics

Let:

\[
A(M)
\]

be the cost to build a semantic/query validity region over movement envelope \(M\),

\[
N(M)
\]

be the number of useful frames or query batches for which that result remains valid,

and

\[
R(M)
\]

be the runtime cost while using it.

The amortized cost is:

\[
T(M)
=
\frac{A(M)}{N(M)}
+
R(M).
\]

This is the core reason Zach's unknown-movement formulation matters.

A frame-specific compiler can be extremely tight but may throw its work away immediately.

A movement-valid compiler may spend more up front but reuse its answer for many future rays and frames.

Using the Monte Carlo median runtime-only result:

\[
R\approx82.67\mathrm{ms/frame},
\]

the saving before analysis amortization is:

\[
138.5-82.67
=
55.83\mathrm{ms/frame}.
\]

Therefore a compile cost \(A\) breaks even after approximately:

\[
N_{\mathrm{break}}
>
\frac{A}{55.83}.
\]

Examples:

| Analysis cost | Break-even lifetime |
|---|---:|
| 50 ms | 0.9 frames |
| 200 ms | 3.6 frames |
| 500 ms | 9.0 frames |
| 1000 ms | 17.9 frames |
| 2000 ms | 35.8 frames |

At 60 Hz, 1000 ms of analysis would need roughly 0.30 seconds of useful reuse to break even under the median runtime assumption; 2000 ms would need roughly 0.60 seconds.

Again, these are model outputs, not measured movement lifetimes.

---

## 7. Amortized examples for a 1000 ms analysis

Using the median runtime-only result:

\[
R=82.67\mathrm{ms},
\]

and a hypothetical:

\[
A=1000\mathrm{ms},
\]

the total amortized frame cost becomes:

| Validity lifetime | Amortized frame time | Effective speedup |
|---|---:|---:|
| 30 frames | 116.0 ms | 1.19x |
| 60 frames | 99.3 ms | 1.39x |
| 120 frames | 91.0 ms | 1.52x |
| 300 frames | 86.0 ms | 1.61x |
| 600 frames | 84.3 ms | 1.64x |

This is why the validity region should not merely be tight. It should survive long enough to repay its construction.

---

## 8. The real optimization variable is the validity envelope

A larger movement/query region usually buys:

\[
N(M)\uparrow
\]

because the compiled answer survives more movement.

But it may also cause:

\[
\text{proof tightness}\downarrow
\]

and therefore:

\[
R(M)\uparrow.
\]

A smaller region often gives the opposite trade.

So the architecture should not ask:

> How large should the grid cells be?

The stronger question is:

\[
\boxed{
M^*
=
\arg\min_M
\left[
\frac{A(M)}{N(M)}
+
R(M)
\right]
}
\]

subject to correctness and the declared execution-faithfulness model.

This may be one of the most important mathematical consequences of Zach's unknown-movement refinement.

---

## 9. Query-space compilation versus a spatial grid

For a semantic boundary:

\[
K=\{p:g(p)=0\}
\]

and ray generator:

\[
r(m,u,s)=o(m)+s\,d(m,u),
\]

the backward compiler studies:

\[
g(r(m,u,s))=0.
\]

This produces a relevance surface in movement/ray/query coordinates.

A query validity region may then prove:

\[
Q_0\Rightarrow\text{cannot hit }K,
\]

\[
Q_1\Rightarrow\text{one crossing in }[s_a,s_b],
\]

or:

\[
Q_2\Rightarrow K_1\prec K_2.
\]

If those facts hold for every ray in the region, runtime does not need to rediscover them per sample.

The performance hypothesis is therefore not merely:

\[
29\rightarrow12\text{ field samples}.
\]

It is potentially:

\[
\text{millions of repeated semantic queries}
\rightarrow
\text{one AOT/incremental proof}
+
\text{cheap regime lookup}.
\]

That is why a query-space representation can have a stronger amortization story than a camera-specific ray optimization.

---

## 10. What the current 2463 ms grid result does and does not say

The current grid march proves one implementation is very slow.

It does **not** establish that:

- certification itself must be slow;
- query-space specialization must be slow;
- a movement-valid compiled ray family must be slow;
- ray-domain synthesis must be slow.

The 2463 ms result is especially important as a warning about **where proof machinery executes**.

A proof representation can be mathematically excellent and still lose if each pixel must pay for:

\[
\text{DDA traversal}
+
\text{certificate reads}
+
\text{switch evaluation}
+
\text{error bookkeeping}
+
\text{field evaluation}.
\]

Zach's backward-compilation proposal is interesting precisely because it attempts to move more of that semantic work out of the repeated pixel path.

---

## 11. Break-even on query lookup

Ignoring analysis amortization for a moment, define the runtime before query overhead:

\[
B
=
T_0
\left[
(1-f_{\sigma}-f_{\mathrm{sem}})
+
f_{\sigma}\frac{S_n}{29}
+
f_{\mathrm{sem}}(1-e_{\mathrm{sem}})
\right].
\]

Then the maximum new query/runtime overhead that still ties the baseline is:

\[
Q_{\max}=T_0-B.
\]

For the four scenarios above:

| Scenario | Runtime before new lookup | Maximum lookup overhead before losing |
|---|---:|---:|
| Weak | 105.0 ms | 33.5 ms |
| Plausible | 71.7 ms | 66.8 ms |
| Strong | 47.9 ms | 90.6 ms |
| Aggressive | 28.4 ms | 110.1 ms |

This does **not** mean tens of milliseconds of lookup overhead are acceptable engineering targets.

It means that if the architecture truly removes the assumed amount of field and semantic work, there is substantial economic headroom.

The correct target should remain much lower: the query lookup should ideally be an O(1)-like or shallow-hierarchy operation with small, coherent GPU state.

---

## 12. The missing measurement that matters most

The dominant unknown is not the Peano constant anymore.

It is the real decomposition:

\[
138.5\mathrm{ms}
=
T_{\sigma}
+
T_{\mathrm{branch/support}}
+
T_{\mathrm{other}}.
\]

Without that decomposition, no simulation can tell us whether 12 field evaluations removes 10 ms or 80 ms.

The next profiling pass should therefore instrument:

\[
\text{sigma/field evaluation count and time},
\]

\[
\text{switch/branch count and time},
\]

\[
\text{spatial/query traversal count and time},
\]

\[
\text{proof-record/cache reads},
\]

and:

\[
\text{residual fixed GPU work}.
\]

The existing standalone performance analyzer can receive these as explicit workload-model dimensions and imported traces/counters. It should report assumptions as assumptions rather than turning structural counts into fake milliseconds.

---

## 13. Strongest falsification test

Do not build a general semantic-state-space compiler first.

Choose one expensive Northern Veil kink/switch family.

Construct only enough backward analysis to classify a bounded movement/ray family into regimes such as:

\[
\text{MISS},
\quad
\text{ONE CROSSING},
\quad
\text{KNOWN BRANCH ORDER},
\quad
\text{UNKNOWN/FALLBACK}.
\]

Then measure four quantities:

\[
A(M)
\]

analysis/build cost,

\[
N(M)
\]

useful validity lifetime under real movement,

\[
R(M)
\]

runtime using the compiled query regions,

and:

\[
R_0
\]

the same path with per-ray rediscovery.

The architecture earns another rung only if:

\[
\frac{A(M)}{N(M)}+R(M)<R_0.
\]

That is the entire theory condensed into one falsifiable inequality.

---

## 14. Failure modes

### Query-space explosion

The preimage of complicated nonlinear semantic boundaries under movement and ray generation may fragment into too many regimes.

If region count grows faster than the repeated work it removes, stop.

### Validity thrash

If tiny camera motion crosses proof facets constantly, then:

\[
N(M)\approx1
\]

and the expensive analysis cannot amortize.

### Over-broad validity

If \(M\) is so broad that every semantic feature becomes "maybe relevant," then the compiled answer collapses back toward the conservative grid problem.

### GPU lookup bureaucracy

If identifying the query region requires a large divergent traversal, the semantic compiler may simply move the Big Chungus from field evaluation into lookup.

### Memory bandwidth

A mathematically tiny program can still lose if each ray fetches large scattered proof records.

### Incorrect factorization of movement

Movement state and angular/ray state are correlated. Treating them as an independent Cartesian product can destroy tightness and recreate false ambiguity.

---

## 15. What would count as a major success

The first major success is **not** a complete replacement renderer.

It would be evidence that one semantic feature can be analyzed once over a useful movement envelope and then reused across many rays such that:

\[
\text{switch/support work per ray}
\downarrow
\]

without an offsetting increase in query traversal,

while:

\[
N(M)\gg1.
\]

A particularly strong witness would show all of:

\[
S_n\lesssim15,
\]

substantial switch elimination,

small regime lookup cost,

movement validity over many frames,

and:

\[
\frac{A(M)}{N(M)}+R(M)<138.5\mathrm{ms}
\]

on the controlled Northern Veil comparison.

That would not prove the entire architecture universally optimal. It would prove the new problem formulation buys something concrete.

---

## 16. Relation to the earlier chat-level estimate

An earlier quick chat estimate gave approximately 1.5x–2.1x as a plausible middle range and around 2.5x–4x as a stronger upside envelope.

This deeper model uses a more explicit decomposition and broader uncertainty ranges. Its 5th–95th percentile runtime-only speedup interval is approximately:

\[
1.38x\text{–}2.21x
\]

with median:

\[
1.68x.
\]

The deterministic strong case is:

\[
2.67x
\]

and the deliberately aggressive case is:

\[
4.56x.
\]

The difference is not a contradiction; the earlier numbers were a rough first-pass scenario sketch. This document supersedes them as the explicit current sensitivity model.

---

# Conclusion

The performance theory is plausible because the proposed architecture changes the unit of repeated work.

Instead of asking every concrete ray to rediscover:

\[
\text{support}
+
\text{kinks}
+
\text{branch order}
+
\text{integration structure},
\]

it attempts to prove those facts once for a **family of unresolved future queries** and reuse them while their premises remain valid.

The mathematical performance target is therefore:

\[
\boxed{
\min_M
\left[
\frac{A(M)}{N(M)}
+
R(M)
\right]
}
\]

not simply minimum samples per ray.

The present model says there is a meaningful economic basin in which this can win.

The present evidence does **not** say Earthcall occupies that basin yet.

The next truthful step is measurement.

---

## Attribution

**Zach originated the architectural performance hypothesis:** semantic kinks transformed into angular/query-space structure; extra-dimensional set-space bounds over unknown movement; one set-space answer applying to all represented rays; backward/Magic-Set-style compilation.

**Original Prism Sun contributed:** the explicit query-preimage formulation, the amortized cost objective, this sensitivity model, break-even analysis, failure modes, and the falsification contract.

No simulated number in this document is attributed to measured renderer performance unless explicitly identified as an existing measurement.
