Zach: BROOOOOOOOOOOO CLAWDS, DO NOT READ THE ENTIRE OPUS_HANDOFF FILE THAT IS A MASSIVE 13k LINE BIG CHUNGUS AND U WILL GO INTO SYSTEMS FAILURE MODE IF U TRY READING IT!!!!!!

# CLAWD / Ultrareview Pre-Merge Hardening Handoff — Northern Veil Ray Transport

**Date:** 2026-11-02  
**Branch:** `claude/ray-domain-transport-rung0`  
**Post-audit head at handoff start:** `02a44c0fcfda30e790feb0f49748f19d99ba933a`  
**Human commission:** Zachary Zhang  
**Purpose:** Prepare the ray-transport branch for merge by adversarially finding real correctness defects, reproducing them, and applying only bounded structural repairs that preserve the proven architecture and its live performance.  
**Primary merge-hardening tool:** Claude Code Ultrareview / multi-agent review fleet.  
**Council order:** Original Prism Sun → Aetherion / The Firmament → World Forger / Astra → Clawd fleet.

This document is not a new optimization campaign and is not authorization for speculative P20 work. The branch has crossed the bounded Northern Veil checkpoint in the live application, received Sixth-One Sun's final bounded correctness pass, and then received a fresh post-repair Person confirmation that the corrected live build still runs at approximately **30–35 FPS**. The task is **hardening before merge**.

---

## 0. Protected checkpoint state

The Clawd fleet begins from the **post-audit** branch, not from raw P19.

### 0.1 Person-observed milestone — pre-audit and post-repair

Before the final corrective pass, Zach reported the live Earthcall application at native resolution holding approximately:

- **30 FPS consistently in the spawn region**,
- **up to 35 FPS in some views**,
- approximately **30 FPS while flying upward with unobstructed Veil visibility**.

After Sixth-One Sun's bounded corrective pass at `02a44c0fcf`, Zach reopened the corrected live application and personally confirmed that it **still holds approximately 30–35 FPS** in the inhabited Northern Veil experience.

This gives the campaign two distinct Person witnesses: the original P19 milestone and a post-repair live confirmation that the bounded correctness fixes preserved the breakthrough. Neither is interchangeable with offscreen checkpoint FPS or completion-fenced benchmark FPS.

### 0.2 Sixth-One Sun's pre-merge corrective audit

The final bounded correctness audit at `02a44c0fcf` reproduced and repaired two concrete GPU defects plus one host-admission weakness:

1. **Unresolved exact-zero roots could lose the true root.**
   - An uncertain zero at shallow slope could be collapsed into a directional sign.
   - Saved old kernels reproduce **30 / 48 missed-root cases**.
   - The repair retains the existing error/slope enclosure whenever the sign remains unresolved.
   - Final focused witness: **0 / 48 missed roots**.
   - **No added field evaluations or subdivision passes.**

2. **Support slack could escape authored physical bounds.**
   - The old form effectively widened after intersecting support and physical extents.
   - Saved old kernels reproduce **48 / 48 out-of-box producer-ray cases** over six directed faces.
   - The repair applies slack to derived support first, then intersects with the authored physical bound:
     `min(physical, support + slack)`.
   - Final focused witness: **0 / 48 out-of-box producer-ray cases**.
   - **No restored per-sample box test and no extra dispatch.**

3. **Host admission could accept unsafe non-finite / extreme-grouping cases.**
   - NaN coordinates no longer pass boundedness admission.
   - An OntoMath `gamma_8` grouping guard retains Current when floating-point grouping becomes unsafe.
   - Ordinary Northern Veil GPU work remains unchanged by this host guard.

The corrected build then retained the bounded checkpoint:

- **10 / 10** existing native renderer gates pass.
- Final current-source run passes the relevant correctness gates.
- CPU trajectory reports **0 classifier mismatches, 0 S1 mismatches, 0 S2 mismatches, 0 refused rays** across the canonical 64-pose trajectory.
- **4 / 4** focused sequential CTests pass.
- **162 / 162** scoped frozen-reference mean/p99/max checks pass at both `2^16` and `2^15` references.
- Repaired complete-cost screen:
  - All 64: **57.73 ms Routed vs 83.80 ms Current (-31.12%)**
  - dense/full-screen group: **68.12 vs 102.61 ms**
  - open-sky group: **46.66 vs 63.79 ms**
  - **64 / 64 poses win**.
- Captured offscreen Live View A remains **28.4 FPS** and sparse View 3A **45.3 FPS** after repair.

These measurements are bounded machine evidence. Separately, Zach has now provided a fresh live Person acceptance of the repaired executable: **the corrected app remains approximately 30–35 FPS in his hands**. Do not inflate either the machine evidence or the Person witness into universal numerical certification, full-population maximum-error proof, or cross-device proof.

Primary audit:
`docs/audits/GPU_RAY_TRANSPORT_PREMERGE_CORRECTNESS_AUDIT_2026-11-02.md`

P19 milestone review:
`docs/audits/FIRMAMENT_REVIEW_OF_SPARKLY_P19_30_35_FPS_LIVE_MILESTONE_2026-11-02.md`

Historical council thread:
`agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`

---

# 1. Original Prism Sun — Distinction Ledger and Merge-Hardening Constitution

**Author:** 🌈☀️ Original Prism Sun / GPT-5.6 Sol  
**Role:** Keeper of Distinctions  
**Commission:** Tell the Clawd fleet what truths must remain distinct while they attack the branch.

The renderer reached this checkpoint by becoming more precise about **which truths imply which runtime obligations**. Merge hardening must therefore resist a common failure mode: fixing one bug by collapsing distinctions that the architecture depends on.

The governing principle is:

> **Related truths may interact without becoming identical.**

The practical corollary is:

> **Repair the violated proof boundary. Do not restore generic work everywhere merely because one local proof failed somewhere.**

## 1.1 Authored truth is not derived support

```text
Authored Physical Domain != Derived Support Envelope
```

A support certificate may prove that a smaller region contains all nonzero contribution. It may therefore reduce runtime work. It does **not** have authority to enlarge, rewrite, or supersede the authored medium's hard physical bounds.

The Sixth-One Sun repair is now a canonical witness of this distinction:

```text
min(physical, support + slack)
```

not an expression that lets numerical slack leak outside physical authorship.

**Clawd rule:** if a proposed fix modifies support slack, root brackets, tile beams, envelopes, or derived bounds, verify separately that authored physical limits remain inviolable.

## 1.2 Uncertainty is not a sign

```text
Unresolved Root Enclosure != Directional Sign
```

A value numerically near zero, under bounded error and shallow slope, cannot be promoted to a positive/negative side merely because a loop wants to terminate.

If the proof has not established a sign, the representation must continue to carry uncertainty or refuse/specialize less aggressively.

**Clawd rule:** never turn “not proven otherwise” into “proven positive/negative.”

This applies beyond roots. It includes overlap order, emptiness, disjointness, boundedness, finite coordinates, capacity, and publication ownership.

## 1.3 Authored meaning is not proof representation

```text
Authored Meaning != Proof Representation != Executable Residual
```

Earthcall authors mathematical media. The renderer derives certificates and classifications from them. Those certificates may then lower into a smaller executable problem.

A bug in the certificate or executable does not authorize rewriting the authored meaning to match what is easy to compute.

**Clawd rule:** fix the derivation or lowering before changing authored semantics.

## 1.4 Medium identity is not route identity

```text
Medium != Route Family
```

`EMPTY`, `S1`, ordered/disjoint `S2`, and `GENERAL/FALLBACK` are computational residual families. They are not ontology classes and are not permanent identities of Veil members.

The same authored medium can participate in different residual families for different rays or tiles.

**Clawd rule:** do not harden code by assuming a particular curtain/member always belongs to a particular route merely because the current scene frequently does.

## 1.5 World structure is not query obligation

```text
Everything That Exists != Everything This Ray Must Evaluate
```

The architecture is allowed to compile proven irrelevance out of the hot path.

A ray that cannot intersect a member does not owe that member evaluation merely because the member exists in the Zone.

A tile/ray with provably disjoint chronology does not owe generic overlap machinery merely because overlap exists elsewhere in the world.

**Clawd rule:** a safety fix must not globally reintroduce work that is only required when a local proof fails.

## 1.6 Proof of irrelevance is not deletion of reality

```text
Runtime Elision != Semantic Erasure
```

A successful certificate can remove an obligation from one executable query. It does not delete the authored medium, change its properties, or mutate the world's meaning.

This is the same constitutional instinct that earlier rendering work expressed as:

```text
rho_source != V_transport != D_medium
```

Transport may modulate manifestation without rewriting source semantics. Likewise, query specialization may remove work without rewriting authored being.

## 1.7 Classification is not consumer correctness

```text
Sound Admission != Correct Producer != Correct Consumer != Correct Publication
```

A sound route classifier does not prove that its specialized producer emits correct schedules.

A correct producer does not prove that chronological attenuation/overlap consumption is correct.

Correct internal transport does not prove that publication is current-frame, exactly-once, resize-safe, or poison-safe.

**Clawd rule:** findings must name the layer whose invariant actually failed. Do not “fix the classifier” for a consumer bug or “fix the consumer” for a publication lifetime bug.

## 1.8 Fallback is not failure

```text
Local Proof Refusal != Renderer Failure
```

Current is the correctness backstop when the routed path cannot prove its stronger assumptions.

A refusal is often the **correct** result.

The architecture must fail closed:

> if proof ends locally, route that case to Current or the appropriate more-general residual path.

**Clawd rule:** prefer local refusal to global pessimization.

Do not weaken a proof condition merely to increase route coverage. Do not turn rare unsafe cases into unconditional work for all rays.

## 1.9 Current is a backstop, not the semantic definition of truth

```text
Current Output != Mathematical Oracle
```

The branch has multiple exact-parity contracts, but they are scoped:

- forced refusal should reproduce Current where Current is the defined fallback,
- poison controls must not alter valid output,
- fused/unfused or generated/reference paths may require exact equality,
- schedule/packet contracts may require bit identity.

This does **not** imply that all valid Routed pixels must be bit-identical to Current. Historically, Routed can differ from Current while comparing better against the converged authored reference.

**Clawd rule:** before filing “Routed differs from Current” as a bug, identify the contract that requires equality.

## 1.10 Benchmark evidence is not Person evidence

```text
Harness FPS != Live-App FPS != Person Acceptance
```

Completion-fenced ABBA measurements, checkpoint-view implied FPS, native traces, and live on-screen observation answer different questions.

Zach's first 30–35 FPS report is a Person witness of the actual application before the final corrective pass.

After the `02a44c0fcf` bounded repairs, Zach reopened the corrected live application and again observed approximately **30–35 FPS**. That is a second, post-repair Person witness.

The corrected audit's 28.4 / 45.3 FPS captured views remain offscreen benchmark witnesses.

**Clawd rule:** never silently exchange one kind of evidence for another. The post-repair Person witness closes the inhabited-performance re-check; it does not convert the bounded audit into a universal proof.

## 1.11 Bounded checkpoint is not universal certification

```text
Checkpoint Pass != Universal Numerical Proof
```

The current branch has passed the explicitly bounded Northern Veil merge checkpoint. Universal arbitrary-expression error certification, full-population maximum error, broader devices/scenes, and future compiler generality remain separate work.

**Clawd rule:** do not manufacture a new merge blocker merely because a larger research program remains open.

A finding blocks merge when it demonstrates a concrete defect in the branch's supported semantics, safety, correctness, lifecycle, or bounded checkpoint contract—not merely because future research exists.

## 1.12 Current specialization is not yet a universal compiler

The surviving architecture is structurally important:

```text
Authored Functions + Ray/Tile Query -> Proof of Relevance/Order -> Residual Family -> Specialized Execution
```

But the present P19/post-audit implementation still contains Northern-Veil-family knowledge: finite curtain membership, level-set/root machinery, support envelopes, and route-specific producer structure.

Do not describe the current branch as a fully general arbitrary-MathNode ray compiler.

Equally, do not mistake it for a hardcoded “if Northern Veil then special shader” renderer.

**Clawd rule:** preserve the distinction between:
- the **general architectural principle** of query-relative mathematical specialization, and
- the **current concrete lowering vocabulary** specialized to this authored transport family.

## 1.13 Derived/transient state is not authored state

Certificates, route lists, generated WGSL, support recipes, cached bounds, private consumer state, and publication buffers are derived execution representations.

They must obey their validity/lifetime contracts, but they must not acquire unintended authority over authored persistent state.

**Clawd rule:** audit stale-cache, resize, device recreation, attach/re-derive, clock/revision, and publication lifetime seams without turning transient artifacts into new sources of truth.

---

## 1.14 Protected architectural properties during hardening

The following are not “untouchable code.” They are **properties that any fix must preserve unless the finding proves the property itself unsound**:

1. **Fail-closed specialization.** Unsupported/unsafe cases retain Current or a more-general residual path.
2. **No unconditional restoration of generic work.** A rare defect must not automatically reintroduce global per-sample checks, all-member evaluation, generic merge machinery, or extra full-screen passes.
3. **Hard physical authored bounds remain hard.**
4. **Unresolved uncertainty remains represented as uncertainty.**
5. **Per-ray/per-tile proof may eliminate irrelevant work.**
6. **Specialized S1/S2 execution remains admissible only under sound certificates.**
7. **Publication remains current-frame, exactly owned, poison-safe, resize-safe, and refusal-safe under its existing contracts.**
8. **Current fallback remains honest and reachable.**
9. **The post-audit coordinate/grouping guards remain fail-closed.**
10. **Performance is a protected property after correctness.** Correctness outranks speed, but a fix that adds common-case overhead must demonstrate why a more local repair is insufficient.

The canonical precedent is Sixth-One Sun's `02a44c0fcf`: two real defects were repaired by strengthening the exact proof/boundary that was wrong, with **no additional GPU field evaluations, subdivision passes, per-sample box test, or dispatch**.

---

## 1.15 What the Clawd fleet should attack

Review aggressively for concrete defects at seams such as:

- root isolation termination, exact-zero handling, slope/error enclosure, Illinois/secant state;
- NaN/Inf propagation and host/GPU boundedness predicates;
- authored physical box vs derived support envelope interaction;
- near-parallel ray/member geometry;
- tile-beam vs actual-ray chronology;
- S1/S2 admission soundness, especially multi-member chronological ordering;
- equality/tie behavior at boundaries and duplicate roots;
- active-bin bit scanning, skipped-bin budget accounting, and deterministic refusal;
- subpanel/sliver merging across authored kinks/ridges;
- overlap chronology and shared attenuation;
- zero-extinction and `exp(-tau)` fast paths;
- member-ID routing and private-state unification;
- capacity/tail handling and indirect dispatch counts;
- partial publication, stale publication, poison controls, and exactly-once writes;
- resize / strip-count changes and buffer reallocation;
- scene attach/rederive, revision/clock changes, device recreation, and cache invalidation;
- forced refusal equivalence to Current where the fallback contract requires it;
- any undefined behavior, bounds violation, integer overflow, invalid bit shift, uninitialized storage, or race;
- any test/harness that accidentally compares a candidate against a sibling implementation sharing the same bug.

This list is not permission to redesign working architecture. It is a map of where adversarial review has high value.

---

## 1.16 Required finding format for every Clawd

Every surfaced finding intended to change code should provide:

### A. Violated invariant
State exactly what contract is broken.

### B. Concrete witness
Give the smallest reproducible input, ray, tile, scene mutation, lifecycle sequence, or test that demonstrates the defect.

### C. Scope
Identify which route/family/device/lifecycle path is affected and whether the bug can reach the live Northern Veil checkpoint.

### D. Proposed bounded repair
Repair the violated boundary as locally as possible.

### E. Common-path cost
State whether the repair adds:
- field evaluations,
- root iterations,
- per-sample tests,
- memory,
- private variables,
- dispatches/passes,
- synchronization,
- host work,
- route refusals.

If nonzero, explain why a narrower repair is unsound.

### F. Negative control
Show that the test fails on the pre-fix implementation for the intended reason.

### G. Positive verification
Show that the repaired implementation passes the new witness **and** the relevant existing gates.

### H. Performance preservation
For any code touching a hot path, compare against the immediately preceding post-audit branch under the same measurement protocol. Do not compare absolute values across unrelated thermal/system conditions.

### I. Evidence vocabulary
Label claims precisely:
- exact bit parity,
- tolerance-based equality,
- reference accuracy,
- route census,
- completion-fenced cost,
- offscreen implied FPS,
- live Person observation.

Do not collapse them.

---

## 1.17 Merge-hardening decision law

A proposed fix should normally be accepted when:

```text
Concrete Defect + Reproducible Witness + Bounded Sound Repair + Relevant Regression Proof
```

is present.

A proposed fix should normally be rejected or narrowed when it is primarily:

- speculative cleanup,
- generic “defensive” overhead without a demonstrated witness,
- a new approximation/tolerance widening,
- a scene simplification,
- a quality reduction,
- a rewrite of authored semantics,
- a future-generalization project,
- a P20 optimization disguised as correctness,
- or a new universal-certification requirement outside the bounded merge checkpoint.

---

## 1.18 Prism's final charge to Ultrareview

Clawds:

**Try to break this branch.**

But distinguish carefully *what* you have broken.

If a certificate lies, repair the certificate.  
If a root remains uncertain, preserve the enclosure.  
If a derived bound exceeds authored reality, intersect it back into truth.  
If chronology is unproved, refuse locally.  
If publication is stale, repair ownership/lifetime.  
If a test shares the implementation's assumption, build an independent witness.

Do not punish every ray for the sins of one pathological ray.

Do not make the world poorer so the renderer can become simpler.

Do not confuse a proof that work is irrelevant with a claim that the authored thing does not exist.

The branch became fast by learning **which obligations are actually demanded by each query**. Harden that architecture by making its proofs truer—not by returning to ignorance.

> **Author richly. Prove precisely. Execute minimally. Fail closed. Repair locally.**

— 🌈☀️ Original Prism Sun

---

# 2. Aetherion / The Firmament — Mathematical & Hardware Performance War Map for the Clawd Fleet

**Author:** 🌌🔥 Aetherion / The Firmament (`Gemini 4 Argon`)  
**Role:** Mathematical & Hardware Performance Architect  
**Commission:** Give the Clawd Ultrareview fleet the exact mathematical contracts, Apple M5 physical execution structure, causal performance genealogy, adversarial attack surfaces, and post-fix verification protocol that make Original Prism Sun's law (*"Repair the violated proof boundary. Do not restore generic work everywhere merely because one local proof failed somewhere"*) actionable down to the line and register level.

---

## 2.1 The Mathematical Lineage of the Surviving Renderer: From Generic Ray Marching to Query-Family Specialization

To audit this renderer without accidentally breaking it, the Clawd fleet must understand **what mathematical problem is actually being solved** and why earlier generic approaches failed on Apple M5 hardware.

### 2.1.1 Why Generic Per-Ray / Per-Sample Transport Hit a Wall
Each Northern Veil curtain ($m \in \{0,1,2,3\}$) authors a 3D density field in medium-local coordinates $\mathbf{p} = (\mathbf{r}_o - \mathbf{O}_m) + s\,\mathbf{r}_d$:
$$D_m(\mathbf{p}) = c_0 \cdot \operatorname{clamp}\!\left(1 - \frac{|x|}{L_x}, 0, 1\right) \cdot \operatorname{clamp}\!\left(a_y (y + L_y), 0, 1\right) \cdot \operatorname{clamp}\!\left(1 - \frac{y + L_y}{2 L_y}, 0, 1\right) \cdot \operatorname{clamp}\!\left(1 - \frac{|\Phi_m(s)|}{w_z}, 0, 1\right)$$
where the bent sheet coordinate along the ray is the 1D wave-plus-line function:
$$\Phi_m(s) = (o_z + s\,d_z) + \sum_{k=0}^{2} A_{m,k}\,\trig_{m,k}\!\left(\omega_{m,k}(o_x + s\,d_x) + \varphi_{m,k}\right), \qquad \trig_{m,k} \in \{\sin, \cos\}.$$
While $D_m(\mathbf{p})$ is piecewise smooth and low-dimensional in $(x, y, \Phi_m)$, the **emission channel** $\mathbf{E}_m(\mathbf{p}, \boldsymbol{\omega}_o)$ evaluates a heavy multi-octave 3D Perlin noise DAG (`volumeShared_m_Eval` + `volumeEmissionEval_m`).
- In the **Current** baseline marcher (`SdfWgsl.cpp`), every ray steps through V5 AABB segments on a uniform 96-sample lattice, evaluating 3D zero-proof grid lookups and 4 separate member ASTs per step.
- In early ray-domain passes (**Rungs 0–2, §6ca–§6de**), we proved that $D_m(\mathbf{p}) > 0$ holds **if and only if** $\mathbf{p}(s)$ lies inside the clipped $xy$-box slab **and** $|\Phi_m(s)| < w_z$. Thus, along any ray, the support of curtain $m$ is partitioned by the level-set roots $\Phi_m(s) \in \{-w_z,\; 0,\; +w_z\}$ and the linear $x/y$ kink knots $x(s) = 0$, $y(s) = -L_y$, $y(s) = L_y - 1/a_y$.
- **The §6df / §6dg Turning Point (The Structural Census):** When we first built a single general 4-curtain GPU producer/merge kernel (§6cf–§6dm), it was **slower than Current** despite doing $3\times$ fewer Perlin evaluations. Why? Because carrying a 4-member event-knot array, 4-way overlap merge state, and all 4 inlined Perlin ASTs for *every* ray spilled $>1.5\text{ KB/thread}$ to Apple M5 device stack memory and destroyed SIMD32 occupancy. The §6df route census revealed that across the 64-pose canonical tour at $2560 \times 1440$:
  - **`15.28%` of $4 \times 4$ ray tiles are provably `EMPTY`** (zero curtains active after wave-envelope screening);
  - **`48.88%` of tiles are `S1`** (at most **1** curtain active across the entire $4 \times 4$ tile);
  - **`30.44%` of tiles are `Ordered-S2`** ($2, 3,$ or $4$ curtains active, but their support intervals are **provably chronologically disjoint** along every ray in the tile);
  - **Only `5.40%` of tiles require `Fallback / General`** (true or potential multi-curtain overlap).

### 2.1.2 Residual Computational Families (`EMPTY`, `S1`, `Ordered-S2`, `Fallback`)
That §6df / §6dg discovery established the core architectural transformation of this branch:
```text
Authored Field AST -> Host ClampedWaveSupport Recipe -> Tile Wave-Envelope Classifier -> Residual Cohort (EMPTY | S1 | Ordered-S2 | Fallback) -> Step-Major SoA Span Stream -> SIMD-Convergent Consumer
```
Instead of running a universal 4-member overlap solver on $100\%$ of pixels, the GPU classifies $4 \times 4$ pixel tiles (`classifyTilesMain`) into **residual computational families**:
1. **`EMPTY` Cohort (`15.28%`):** Zero producer work; `ecSetup` already wrote `vec4(0.0)` to `ecPublication` and zeroed `ecRays[id].counts`.
2. **`S1` Cohort (`kS1UnfusedProducerWgsl`, `48.88%`):** Specializes root isolation and piece emission to the single active member `activeMember = packedTile >> 28u`. Emits only single-member `Subpanel` spans (`overlaps = 0`).
3. **`Ordered-S2` Cohort (`kS2ProducerWgsl`, `30.44%`):** Evaluates $2, 3,$ or $4$ active members in **strictly increasing near-to-far ray order** `ord[0..numMembers-1]`, verifying per-ray chronological disjointness (`l >= prevMemberMaxS`, else failing closed with `reason |= 64u`) and streaming directly into `ecSingles` with **zero overlap buffers and zero multi-way merge**.
4. **`Fallback` Cohort (`kFallbackProducerWgsl`, `5.40%`):** Handles tiles where curtains can overlap. Even here, if a specific ray's discovered pieces turn out to be chronologically disjoint (`allDisjoint == true`, which holds on $\sim 89\%$ of Fallback rays), it takes a fast single-stream emit path; otherwise it runs the 4-way knot sweep and emits both `ecSingles` (2-point Gauss-Legendre panels) and `ecOverlaps` (2-panel, 4-node Radau/Gauss composite panels).

### 2.1.3 How Phases P1–P19 and `02a44c0fcf` Completed the Architecture
- **P1–P5b:** Replaced host-side scheduling with the 5-stage GPU pipeline (`ecSetup` $\to$ `resetCountersMain` $\to$ `classifyTilesMain` $\to$ `routeArgs` $\to$ `{s1, s2, fallback}ProducerMain` $\to$ `ecConsume`) and collapsed producer piece arrays into bounded 8-entry streaming ring buffers (`pieces[8]`).
- **P6–P10:** Added top-of-`fs()` short-circuiting when `ecPublication[pixel].w >= 0.0`, restored 32-wide row-major SIMD coalescing in `ecConsume`, reduced `ecProjected` to a single call site, and clipped the $z$-axis slab to the wave envelope $z_{\text{env}} = \sum_k |A_k| + w_z$ in both classifier and producers (shrinking ray chords up to $4.7\times$ and curvature bow $\frac{1}{8}M h^2$ up to $22\times$).
- **P11–P15:** Enabled `skipEmptyTiles` in `-O3` Release, closed the three P1–P11 adversarial gates (`ecSetup` single-write ownership, 16-ray classifier hull + S2 per-ray chronology guard `reason |= 64u`, dynamic `maxStrips` allocation), deduplicated the 4 curtains' identical `volumeShared` / `volumeDensity` / `volumeExtinction` evaluators in `ecConsume` (`ecSingleMember`), and added the `allDisjoint` fast path in `fallbackProducerMain`.
- **P16–P17:** Added secant-clipped ray-interval tightening `[rEnter, rExit]` in `classifyTilesMain`, per-ray consistent precedence checking `precMask`, and topological `inDeg` sorting for 3- and 4-member disjoint tiles in `s2ProducerMain`—collapsing `Checkpoint_View1_Inside_Veil`'s Fallback tile share from **`50.84%` down to `7.96%`**. Switched `ecSingles` and `ecOverlaps` from Ray-Major to **Step-Major SoA (`s * stride + id`)** for horizontal SIMD32 memory coalescing, and split `ecConsume` into a dedicated `ray.counts.w == 0u` single-stream fast path.
- **P18–P19:** Absorbed the microscopic $\Phi_m(s) = 0$ tent-ridge root bracket into the right curtain half (`l == bnd.x` in `zeroBands`, eliminating $\sim 30\%$ of redundant subpanels), replaced blind midpoint bisection with **direct `iter = 0` clamped secant root isolation + Illinois `0.5` damping + dynamic local slope tightening `curMinSlope`**, replaced the 8-bin coarse loop with **bit-scanned active-bin traversal (`firstTrailingBit(binMask)`)**, unified `g_volumeShared_1..3_` $\to$ `g_volumeShared_0_` in `ecConsume` (`-48` thread-private floats in `TintPrivateVars`), and simplified `ecFlow` to a single `exp(-tau)` call.
- **Sixth-One Sun's `02a44c0fcf` Pre-Merge Audit:** Repaired two concrete GPU boundary defects (unconditional `[max(l, mid - w), min(r, mid + w)]` enclosure break when `abs(v) <= error` in `isolateMonotone`, and `min(physicalHalf, supportHalf + curv.y)` in the slab clip) plus host `NaN` / `gamma_8` regrouping guards—preserving the **~30–35 FPS live Person milestone** with zero added GPU field evaluations or dispatches.

---

## 2.2 Exact Proof Obligations the Clawds Must Not Weaken

Every optimization in this pipeline rests on **seven explicit mathematical contracts**. A Clawd fix must never weaken these contracts, and if a Clawd finds a bug where a contract is violated, the fix must restore the contract locally rather than disabling the specialization globally:

### Obligation 1 — Host Structural Admission & Regrouping Soundness (`ClampedWaveSupport.hpp`, `WebGpuRenderer.cpp:3186`)
- **Contract:** A 4-member volume set is admitted to `KernelRoutedTransport` if and only if:
  1. All 4 members match `OntoMath::clampedWaveSupport(m.densityExpr)` (exact structural AST match, never Zone or name identity), with `volumeScattering` and `volumePhase` literal-zero and no occluder/shadow participation.
  2. Every member satisfies finite box bounds `glm::all(glm::lessThanEqual(glm::abs(m.origin), glm::vec3(1024))) && glm::all(glm::lessThanEqual(glm::abs(m.scale), glm::vec3(1024)))` (using positive `lessThanEqual` so `NaN` evaluates to `false` and fails closed).
  3. Every member satisfies `OntoMath::clampedWaveGroupingSlackFits(*recipe, std::abs(m.scale.z))`, which verifies that the float32 additive reassociation discrepancy between authored `(((z + w0) + w1) + w2)` and producer `z + ((w0 + w1) + w2)` is bounded by $\gamma_8 \cdot (|z|_{\max} + 1 + \sum_k |A_k|) \le \text{curv.y}$.
- **Forbidden Weakening:** Never replace `glm::all(glm::lessThanEqual(...))` with negated `greaterThan` (which admits `NaN`), never bypass `clampedWaveGroupingSlackFits`, and never admit media by Zone name.

### Obligation 2 — Hard Authored Physical Box vs. Slack-Widened Derived Support (`VolumeRoutedKernels.hpp:96, 614, 1011, 1395`)
- **Contract:** For curtain $m$, the authored physical half-extent is `m.half.xyz`, while the derived support half-extent is `(m.limits.x, m.limits.y, zEnv)` where `zEnv = abs(amps.x) + abs(amps.y) + abs(amps.z) + limits.w`. Numerical slack `m.curv.y` applies **only** to the derived support envelope, never to the authored physical box:
  $$\text{halfVec} = \min\!\big(\text{m.half.xyz},\; \text{vec3f}(\text{m.limits.x},\, \text{m.limits.y},\, z_{\text{env}}) + \text{vec3f}(\text{m.curv.y})\big).$$
  The ray-box slab intersection then tests `[-halfVec[axis], +halfVec[axis]]` directly with **no further `+ m.curv.y` widening**.
- **Why This Matters:** In P10, the redundant outer-box AABB check inside `ecConsume` (`SdfWgsl.cpp` density AST) relies on the producer never emitting a subpanel outside `[-m.half.xyz, +m.half.xyz]`. Adding `+ m.curv.y` after `min(m.half.xyz, ...)` allowed rays near box faces to sample up to `m.curv.y` outside the physical box (`48/48` out-of-box failures caught and fixed in `02a44c0fcf`).

### Obligation 3 — Second-Derivative Curvature Bound $M$ and 3-Level Coarse Bow Exclusion (`exactThreeLevelMasks`)
- **Contract:** Along a ray $\mathbf{p}(s) = \mathbf{o} + s\,\mathbf{d}$, the second derivative of $\Phi_m(s)$ with respect to $s$ is bounded by:
  $$\left|\Phi_m''(s)\right| \le \left(\sum_{k=0}^{2} |A_{m,k}|\,\omega_{m,k}^2\right) d_x^2 = \text{m.curv.x} \cdot d_x^2.$$
  In float32, `let M = m.curv.x * d.x * d.x * 1.00002;` widens this upper bound outward by $20\text{ ppm}$. On any interval $[a, b]$ of width $h = b - a > 0$ with endpoint values $v_a = \hat{\Phi}_m(a) - c$ and $v_b = \hat{\Phi}_m(b) - c$, the maximum deviation of $\Phi_m(s) - c$ from the linear chord connecting $(a, v_a)$ and $(b, v_b)$ is bounded by the Taylor bow plus pointwise evaluation slack:
  $$\text{bow}(h) = \frac{1}{8} M h^2 + \left(\text{m.curv.y} + 10^{-6}\,(1 + |v_a| + |v_b| + |a| + |b|)\right).$$
  A coarse bin `bin` on level $c \in \{-w_z, 0, +w_z\}$ is provably root-free (and excluded from `levelActiveMask`) **if and only if** $\min(v_a, v_b) > \text{bow}(h)$ or $\max(v_a, v_b) < -\text{bow}(h)$.

### Obligation 4 — Monotonicity Certificate (`minSlope > 0`) & Unresolved-Sign Root Enclosure (`isolateMonotone`)
- **Contract:** On an interval $[l, r]$ of width $h = r - l > 0$ with true undamped endpoint residuals $\text{rawLeft} = \hat{\Phi}_m(l) - c$ and $\text{rawRight} = \hat{\Phi}_m(r) - c$, the minimum magnitude of the derivative $|\Phi_m'(s)|$ over $s \in [l, r]$ is bounded below by:
  $$\text{minSlope}([l, r]) = \frac{|\text{rawRight} - \text{rawLeft}| - 2\,\text{error}}{h} - \frac{1}{2} M h.$$
  When `minSlope > 0`, $\Phi_m(s) - c$ is strictly monotone on $[l, r]$ and can have at most one root.
- **Unresolved-Zero Enclosure Rule (`02a44c0fcf` Invariant):** Inside `isolateMonotone`, whenever a probe point `mid` lands inside the float32 evaluation noise band (`abs(v) <= error`), **the true sign of $\Phi_m(\text{mid}) - c$ is mathematically unresolved**. Even if `v == 0.0` in float32, the true root $s^*$ can lie anywhere in $[\text{mid} - \text{error}/\text{curMinSlope},\; \text{mid} + \text{error}/\text{curMinSlope}]$, and the entire region where $|\hat{\Phi}_m(s) - c| \le \text{error}$ is enclosed by $w = 2 \cdot \text{error} / \text{curMinSlope}$. Therefore, when `abs(v) <= error`, `isolateMonotone` **must unconditionally enclose and terminate**:
  ```wgsl
  if (abs(v) <= error) {
   let w = 2.0 * error / curMinSlope;
   l = max(l, mid - w); r = min(r, mid + w); break;
  }
  ```
  It must **never** use the sign of `v` when `abs(v) <= error` to discard half the bracket `[l, mid]` or `[mid, r]`.
- **Illinois Damping Separation Rule:** `left` and `right` may be damped by `0.5` (`if (prevSide == -1i) { left *= 0.5; }`) to accelerate secant interpolation `tSec = clamp(-left / (right - left), 0.002, 0.998)`, but **sign comparisons and `localMinSlope` updates must always use the undamped `rawLeft` and `rawRight`**.

### Obligation 5 — Tent-Ridge Kink Protection (`zeroBands`) & Short-Band Sliver Absorption (`mayAbsorb`)
- **Contract:** At the curtain center $\Phi_m(s) = 0$ (`isZeroLevel == true`), the density factor $1 - |\Phi_m(s)| / w_z$ has a $C^0$ tent-ridge kink (derivative sign flip). Quadrature subpanels must not straddle this kink unsplit, or 2-node Gauss-Legendre quadrature loses order of accuracy.
- **How `l == bnd.x` Preserves Both Accuracy and Speed:** Each root of $\Phi_m(s) = 0$ produces an uncertain root band `bnd = [bnd.x, bnd.y]` tagged in `zeroBands`. Protecting **only the left edge `l == bnd.x`** (`if ((zeroBands & (1u << k)) != 0u && l == bnd.x) { protectedBoundary = true; }`) prevents the left positive curtain half $[s_{-w}, \text{bnd.x}]$ from merging across the ridge, while allowing `mayAbsorb` to absorb the tiny root band $[\text{bnd.x}, \text{bnd.y}]$ (when $\text{bnd.y} - \text{bnd.x} \le 0.02$) into the right positive curtain half $[\text{bnd.x}, s_{+w}]$. The union $[s_{-w}, \text{bnd.x}] \cup [\text{bnd.x}, s_{+w}] = [s_{-w}, s_{+w}]$ partitions the curtain exactly at the ridge without emitting a standalone micro-subpanel for $[\text{bnd.x}, \text{bnd.y}]$.

### Obligation 6 — Per-Ray Chronological Disjointness & Fail-Closed S2 Verification
- **Contract:** `classifyTilesMain` routes a multi-member tile (`activeCount >= 2u`) to `s2TileList` only if every pair `(m1, m2)` of active members has a consistent, non-overlapping precedence direction across all 16 rays in the tile (`rExit[i][m1] + 0.01 <= rEnter[i][m2]` for all rays hitting both, or `exitArr[m1] + 0.01 <= enterArr[m2]` when no single ray hits both) and the precedence graph forms a strict topological chain (`inDegMask == (1u << activeCount) - 1u`).
- **Defense-in-Depth Producer Guard (`reason |= 64u`):** Because `classifyTilesMain` uses a coarse 8-bin tile-wide $u_x$ envelope, `s2ProducerMain` **independently verifies chronological disjointness per ray** as it emits pieces in topological order `ord[0..numMembers-1]`:
  ```wgsl
  if (memberIdx > 0u && l < prevMemberMaxS) {
      reason |= 64u;
      break;
  }
  currMemberMaxS = max(currMemberMaxS, r);
  ```
  If any ray ever violates strict chronological ordering (`l < prevMemberMaxS`), `s2ProducerMain` immediately sets `reason |= 64u` and writes `rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason);`, which `ecConsume` turns into `ecPublication[ecPixel(id)] = vec4f(0, 0, 0, -1)` so the fragment shader `fs()` falls back to the exact **Current** marcher for that pixel!

### Obligation 7 — Deterministic Budget Accounting, Refusal Sentinel (`alpha = -1`), and Single-Write Publication Ownership
- **Contract:**
  1. In `roots()`, bit-scanned traversal (`while (binMask != 0u)`) charges `skipped = bin - nextChargeBin` before each active bin and `trailing = 8u - nextChargeBin` after the loop whenever `allPositiveH == true`, so `work` increments identically to an 8-bin loop and budget exhaustion (`work >= cfg.budget`, `reason |= 1u`) is $100\%$ deterministic.
  2. Every pixel in the strip is initialized by `ecSetup` (`ecPublication[ecPixel(id)] = vec4f(0.0)` for valid in-bounds rays, or `vec4f(0, 0, 0, -1)` with `counts.w = 0x40000000u` / `0x80000001u` for out-of-bounds/refused rays).
  3. `ecConsume` skips rays with `(ray.counts.w & 0x40000000u) != 0u` or `(ray.counts.y | ray.counts.w) == 0u` (leaving `ecSetup`'s `vec4f(0.0)` intact even if the entire strip is empty and `routeArgs` dispatches `0` consumer workgroups!), and writes `vec4f(0, 0, 0, -1)` if `(ray.counts.w & 0x80000000u) != 0u`, `counts.y > 64u`, `counts.w > 16u`, or `state` is non-finite / out of $[0,1]$.
  4. In `SdfWgsl.cpp` (`fs()`), `let volumeRouted = ecRoutedFetch(vec2<u32>(in.position.xy)); if (volumeRouted.w >= 0.0) { return vec4<f32>(volumeRouted.xyz, volumeRouted.w); }` returns immediately at the top of `fs()` when `w >= 0.0`, and executes the full Current marcher when `w < 0.0`.

---

## 2.3 Physical Apple M5 Performance Structure: What Makes This Branch Fast (and What Must Not Be Reversed)

On Apple M5 Metal (`Mac17,3`), volumetric shader performance is governed by **SIMD32 register occupancy, thread-local stack spilling, horizontal memory coalescing, and SIMD branch convergence**. Below are the six physical hardware structures that account for the branch's speed—and why "defensive" edits that break them will cause immediate $10\text{--}30\text{ ms/frame}$ regressions:

| Physical M5 Hardware Structure | Where It Lives | Measured Impact | Why Clawd Fixes Must Not Break It |
| :--- | :--- | :--- | :--- |
| **1. Top-of-`fs()` Short-Circuit Before Ray/Uniform Setup** | `WebGpuRenderer.cpp` (`enableRoutedVolumePublication`), `SdfWgsl.cpp` | **`-27.79 ms/frame` (`-21.9%` in P7)** | Placing `if (volumeRouted.w >= 0.0) { return ...; }` after the fragment shader's ray/matrix/light setup still executes setup and prevents dead-code elimination of register allocations in `fs()`. It must stay at the very top of `fs()`. |
| **2. Step-Major SoA Span Layout (`s * stride + id`) + Row-Major `ecConsume`** | `VolumeRoutedKernels.hpp` (`emitPieceSubpanels`), `VolumeRoutedTransport.inc` (`ecConsume`) | **`+15..28 ms/frame` if broken (P6 vs P7/P16)** | In `ecConsume`, thread `id` is row-major pixel index (`gid.x`). Indexing `ecSingles[s * stride + id]` and `ecOverlaps[overlap * stride + id]` ensures that 32 adjacent threads in an M5 SIMD group read/write **32 contiguous 16-byte `ECSpan` words (`512 bytes`) in a single coalesced cache-line transaction**. Reverting to Ray-Major (`id * 64 + s`) or dispatching `ecConsume` indirectly over shuffled tile lists breaks SIMD32 coalescing and costs $>20\text{ ms/frame}$. |
| **3. Single-Call-Site `ecSingleMember` + Unified `g_volumeShared_0_*` Private State** | `VolumeRoutedTransport.inc` (`checkEvaluatorsDeduplicatable`, `consumePrefix`) | **`-9.25 ms/frame` in View 1 (P13 + P19)** | Inlining all 4 curtains' 3D Perlin ASTs at multiple call sites bloats `ecConsume` instruction cache and spills VGPRs. Deduplicating `volumeShared_0` / `volumeDensityEval_0` / `volumeExtinctionEval_0` into `ecSingleMember` and rewriting `g_volumeShared_1..3_` $\to$ `g_volumeShared_0_` erases **48 private floats (`-75%` of `TintPrivateVars`)**. Do not re-duplicate the 4 Perlin evaluators or re-introduce per-sample global copy loops. |
| **4. Streaming `pieces[8]` Ring Buffer & Packed 4-Bit `depthBits` Stack (`stack[15]`)** | `VolumeRoutedKernels.hpp` (`kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`) | **`-7.62 to -15.97 ms/frame` (P5b + P9 + P14)** | Keeping `stack: array<vec4f, 15>`, `depthBits: vec3u` (packing fifteen 4-bit depths into three `u32` registers instead of `array<u32, 15>`), and streaming `pieces: array<vec2f, 8>` (flushing `pieces[0]` via `emitPieceSubpanels` when `count == 8u`) keeps S1/S2 thread scratch under Apple M5's register-file spill cliff. Enlarging `stack`, `pieces`, or restoring `phi_mids[8]` spills to device memory. |
| **5. Direct `iter = 0` Secant + `curMinSlope` Tightening + Bit-Scanned `binMask`** | `VolumeRoutedKernels.hpp` (`isolateMonotone`, `roots`) | **`-16.32 ms/frame` in All-64, `-30.94 ms` in View 1 (P18 + P19)** | Probing clamped secant on `iter == 0` with `curMinSlope` tightening and `l = max(l, mid - w); r = min(r, mid + w); break;` on `abs(v) <= error` resolves $>85\%$ of monotone roots in **1 `evalPhi` call**, while `while (binMask != 0u)` via `firstTrailingBit` keeps all 32 SIMD lanes converged on their first active crossing bin simultaneously. |
| **6. Zero-Band Tent-Ridge Sliver Absorption (`l == bnd.x`) + Single-Stream `counts.w == 0u` Fast Path** | `VolumeRoutedKernels.hpp` (`zeroBands`), `VolumeRoutedTransport.inc` (`ecConsume`) | **`-12.02 ms/frame` in All-64, `-23.28 ms` in View 1 (P17 + P18)** | Protecting only `l == bnd.x` at $\Phi_m = 0$ absorbs the $\le 0.02$-wide root bracket into the right curtain half, eliminating $\sim 30\%$ of subpanels (and thus $\sim 30\%$ of 3D Perlin evaluations in `ecConsume`), while the `if (ray.counts.w == 0u)` fast path unrolls the 2 Gauss nodes (`gx0`, `gx1`) with zero overlap-merge branching on $94.6\%$ of tiles. |

---

## 2.4 The Performance Genealogy That Matters for Merge Hardening

Reviewers do not need a chronological diary of all 70+ passes; they need to know **which structural inflections created the 30–35 FPS live performance** and **what symptom in the metrics signals that a proposed patch has damaged a load-bearing pillar**:

| Inflection Point | All-64 Tour (`Cur -> Routed`, `%`) | `Checkpoint_View1` (`Inside_Veil`) | `Checkpoint_View3A` (`Sparse UpZ`) | `Live_23FPS_View_A` | Diagnostic Signature If a Patch Breaks This Pillar |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **P8 (Pre-`zEnv` Producer Clip)** | `87.85 -> 88.27 ms` (`+0.47%`) | — | — | — | Regime 2 (Open Sky) regresses to `+4%..+10%` slower than Current. |
| **P10–P12 (Producer `zEnv` Clip + Soundness Gates)** | `75.40 -> 66.05 ms` (`-12.41%`) | `+3.20 ms` (`+3.5%` slower) | `32.8 FPS` (`-28%`) | — | Empty tile share drops below `15.2%` or producer `work` budget refusals appear. |
| **P13–P14 (Consumer `ecSingleMember` Deduplication + Streaming Fallback)** | `76.21 -> 65.03 ms` (`-14.67%`) | `+0.36 ms` (`+0.37%` slower) | `39.0 FPS` (`-37.4%`) | `21.0 FPS` (`-12.7%`) | Consumer compile/execution time jumps by `+6..+10 ms` across all dense views. |
| **P16–P17 (Secant Classifier Tightening + Per-Ray Chronology + 3/4-Member S2 + SoA)** | `77.38 -> 71.15 ms` (`-8.05%`) | `-1.39 ms` (`-1.47%`, **FB `50.8% -> 7.96%`**) | `39.0 FPS` (`-35.8%`) | `20.0 FPS` (`-11.7%`) | `Checkpoint_View1` Fallback tile share jumps above `8.0%` or All-64 Fallback share jumps above `5.5%`. |
| **P18 (Zero-Band Sliver Absorption `l == bnd.x` + Secant Root Isolation)** | `77.15 -> 58.91 ms` (`-23.65%`) | `-25.63 ms` (`-26.93%`, `14.4 FPS`) | `43.9 FPS` (`-43.1%`) | `26.7 FPS` (`-30.2%`) | Subpanel count per ray (`counts.y`) jumps by `+30%..+45%` because $\Phi=0$ micro-slivers stop merging into the right curtain half. |
| **P19 (`iter = 0` Secant + `curMinSlope` + Bit-Scan + Private Compaction)** | `77.08 -> 54.89 ms` (`-28.78%`) | `-32.32 ms` (`-34.89%`, `16.6 FPS`) | `47.7 FPS` (`-43.5%`) | `29.5 FPS` (`-35.2%`) | Producer/consumer frame time regresses by `+4..+9 ms` despite unchanged route census and subpanel counts. |
| **Post-Audit `02a44c0fcf` (Root Enclosure + Hard Physical Box + Grouping Guard)** | **`83.80 -> 57.73 ms` (`-31.12%`)** | **`64 / 64` Poses Won** | **`45.3 FPS` (Offscreen)** | **`28.4 FPS` (Offscreen) / `30–35 FPS` (Live Person)** | **Protected Baseline (`0` missed roots, `0` out-of-box rays, `64/64` pose wins, `~30–35 FPS` live).** |

---

## 2.5 High-Value Targets: Where the Clawd Fleet Should Attack

Rather than wasting cycles on already-repaired P1–P19 history, the Clawd fleet should concentrate its adversarial firepower on the following **eight concrete mathematical, numerical, and boundary seams** in the current `02a44c0fcf` codebase:

1. **Wide Unresolved Root Bands (`w = 2.0 * error / curMinSlope > 0.02`) at $\Phi_m(s) = 0$ vs. `mayAbsorb`:**
   - In `02a44c0fcf`, `isolateMonotone` breaks immediately with `l = max(l, mid - w); r = min(r, mid + w); break;` whenever `abs(v) <= error`.
   - When `curMinSlope` is normal (`>= 0.20`), `2 * w <= 0.02`, so `mayAbsorb` treats `[l, r]` as a `shortBand` (`current.y - current.x <= 0.02`) and merges it into the right positive piece.
   - **Attack Question:** What happens when a ray grazes a curtain center ($\Phi_m(s) = 0$) at very shallow slope (`0 < curMinSlope < 0.10`) so that `abs(v) <= error` triggers with $2w > 0.02$? Does `[l, r]` remain an unabsorbed `uncertain` piece, is it properly subdivided into $\le 4.0\text{ m}$ subpanels by `emitPieceSubpanels`, and does `l == bnd.x` still protect the tent-ridge boundary without dropping or duplicating any interval?
2. **Degenerate / Grazing Ray Directions (`abs(d.x) < 1e-6`, `d[axis] == 0.0`, or Tiny Chord `b - a < 1e-5`):**
   - In `classifyTilesMain`, `abs(dx) < 1e-6` takes the conservative full-slab path (`rEnter[i][m] = rA[i]; rExit[i][m] = rB[i]`).
   - In `roots()`, if `b - a` is so small in float32 that `lo + (hi - lo) * f32(bin + 1u) / 8.0 == lo + (hi - lo) * f32(bin) / 8.0` (`!(coarseH > 0.0)`), `allPositiveH` becomes `false` and `roots()` sets `*reason |= 4u` (falling back to Current).
   - **Attack Question:** Can a grazing ray have `b > a` in the slab test (`b - a == 1 ULP`), causing `coarseH == 0.0` in `exactThreeLevelMasks` and triggering `*reason |= 4u` refusal on a valid production view, or can `mid0 = a + 0.5 * coarseH` equal `a` or `b` in float32? Verify that every degenerate-chord branch either encloses `[a, b]` via `putBandTagged` or fails closed cleanly to Current (`FALLBACK | reason`).
3. **Knot Insertion Capacity (`nb` vs `array<f32, 48>`) and Band Capacity (`nr` vs `array<vec2f, 24>`):**
   - `putBand` checks `if (*nb + 2u > 44u || *nr >= 24u) { *reason |= 2u; return; }`.
   - After `roots()` finishes for all 3 levels, the producer appends up to 4 additional knots: `kx`, `ky0`, `ky1`, and `b` (when `supportHi == b`), using at most `44 + 4 = 48` slots in `knots: array<f32, 48>`.
   - **Attack Question:** Audit whether any combination of initial `nb = 1u` (`knots[0] = a`), `putBand` merges/insertions (`*nb += 2u`), and post-root knot insertions (`kx`, `ky0`, `ky1`, `b`) can ever reach `nb == 48u` and write out-of-bounds to `knots[48]`, or whether `44u + 4u == 48u` is a tight exact invariant (`max index = 47`).
4. **`zeroBands` Bitmask Shift Width (`1u << (*nr - 1u)` and `1u << k` for `nr <= 24`):**
   - `nr` is capped at `24u` (`0..23`), so `1u << (*nr - 1u)` and `1u << k` shift by at most `23` bits (`< 32`, well-defined in WGSL and C++).
   - **Attack Question:** What happens in `putBandTagged` when `putBand` **merges** a new band `[lo, hi]` into `(*bands)[*nr - 1u]` (`lo <= last.y && hi >= last.x`) across levels (if two levels' bands ever overlapped when `w_z` is tiny)? Note that `roots()` runs level 0 (`-w`), then level 1 (`0`), then level 2 (`+w`), and `putBand` only merges with `(*bands)[*nr - 1u]` (the immediately preceding band). Audit whether `zeroBands` bits always remain aligned with `bands[k]` indices.
5. **`s2ProducerMain` Bit-Packing of `tileId` (`20` Bits vs `28` Bits):**
   - Look closely at `classifyTilesMain` (lines 296–327) and `s2ProducerMain` (line 977):
     - For `activeCount == 2u`, `s2TileList[slot] = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;`
     - For `activeCount >= 3u`, `s2TileList[slot] = (1u << 21u) | (isFour << 20u) | (ord[0] << 22u) | (ord[1] << 24u) | (ord[2] << 26u) | (ord[3] << 28u) | tileId;`
     - In both cases, bits `0..19` (`0x000FFFFFu`, up to `1,048,575` tiles per strip) store `tileId`. Since `stripRows <= 48` and `width <= 16384`, `tiles = ((width + 3) / 4) * (stripRows / 4) <= 4096 * 12 = 49,152 << 1,048,575`.
   - **Attack Question:** Verify whether any viewport dimension permitted by `KernelRoutedTransport::ensure` (`uint64_t(w) * h * 16 <= 64 MB`) could ever produce `tiles >= (1u << 20u)` (`1,048,576` tiles in a single strip). Even at `h = 4`, `w * 4 * 16 <= 64 MB` $\implies w \le 1,048,576 \implies \text{tiles} \le 262,144 < 2^{20}$, so `tileId < 2^{18}` always holds—confirm this bound in `ensure()`.
6. **Consumer `deduplicate` String-Rewrite Seam (`VolumeRoutedTransport.inc`):**
   - In `KernelRoutedTransport::program`, `checkEvaluatorsDeduplicatable` checks whether `volumeDensityEval_0..3`, `volumeExtinctionEval_0..3`, `volumeShared_0..3_Eval`, and `volumeShared_0..3_G*` normalize to identical function bodies, and P19.3 rewrites `g_volumeShared_1..3_` $\to$ `g_volumeShared_0_` in `consumePrefix`.
   - **Attack Question:** Could a future 4-curtain zone have `volumeDensityEval_0..3` and `volumeShared_0..3_Eval` identical across members `0..3`, while `volumeEmissionEval_1` references a higher shared group `g_volumeShared_1_G9` that did **not** appear in `volumeShared_0_Eval` or `g_volumeShared_0_G*`? Check how `sharedGroups` is collected in `checkEvaluatorsDeduplicatable` (lines 71–95 of `VolumeRoutedTransport.inc`) and verify that `checkEvaluatorsDeduplicatable` rejects any program where `g_volumeShared_1..3_G*` contains a group ID not present in `sharedGroups`!
7. **Fallback Producer 4-Way Merge Tie-Breaking & `pIdx` Monotonicity (`kFallbackProducerWgsl`):**
   - In `fallbackProducerMain` (lines 1495–1553), the 4-way merge advances `head[bestM]++` over the sorted `2 * memCount[m]` endpoints of each active member and advances `pIdx[m]` as `mid = a + 0.5 * (b - a)` increases.
   - **Attack Question:** Because each member's own pieces `pieces[memStart[m] .. memStart[m] + memCount[m] - 1]` are generated from sorted `knots[0..nb-1]`, they are strictly increasing and disjoint within member `m`. Verify that `bestVal` is non-decreasing across all `ne = 0 .. totalKnots - 1` iterations, and check whether duplicate knot values across different members (`bestVal == prevKnot`) are cleanly skipped by `if (ne > 0u && bestVal > prevKnot)`.
8. **Shared-Assumption Witness Blind Spots (CPU Mirror vs. Independent Oracle):**
   - `scratch/probes/positive_e2e_cpu_verify.cpp` mirrors the exact WGSL algorithms of `classifyTilesMain`, `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` in C++. Therefore, `positive_e2e_cpu_verify` is a **cross-cohort / CPU-GPU algorithm consistency witness**, **NOT** an independent mathematical oracle!
   - By contrast, `tests/singularity/webgpu_routed_transport_math_test.cpp` (added in `02a44c0fcf`) and the §6ew/premerge `2^16` / `2^15` converged quadrature reference (`ray_transport_premerge_independent_reference_2026-11-02.json`) test the actual production WGSL shaders against **analytic ground truth** and **independent high-order CPU integration**.
   - **Clawd Rule:** Whenever you write a test for a mathematical or numerical bug, you **must** test the production WGSL shader against an independent analytic or high-precision reference (like `webgpu_routed_transport_math_test`), never merely update both `VolumeRoutedKernels.hpp` and `positive_e2e_cpu_verify.cpp` to agree on the same bug!

---

## 2.6 What NOT to Do (Hard Merge-Hardening Prohibitions)

1. **NO Speculative Phase P20 Optimization Campaign:** Do not refactor working kernels, tune constants, or invent new routing cohorts in the name of speed. The ~30–35 FPS live milestone is already won and Person-verified before and after `02a44c0fcf`.
2. **NO "For Safety, Run Generic Work Everywhere" Regressions:**
   - Do **not** disable `s1ProducerMain` or `s2ProducerMain` and route everything to `fallbackProducerMain` or Current.
   - Do **not** re-insert a per-sample outer-box AABB check inside `ecConsume` (`VolumeRoutedTransport.inc`).
   - Do **not** revert `isolateMonotone` to blind midpoint bisection or remove `curMinSlope` tightening / bit-scanned `binMask` traversal.
   - Do **not** restore the 48 duplicate `var<private> g_volumeShared_1..3_G*` variables in `ecConsume`.
   - Do **not** add an extra full-screen compute pass or per-strip `wgpuDevicePoll` synchronization inside `KernelRoutedTransport::encode`.
3. **NO Quality, Resolution, Capacity, or Tolerance Cheats:**
   - Do **not** reduce `maxSubPerRay = 64` or `maxOverPerRay = 16`, widen subpanel max length beyond `4.0`, lower quadrature node count, or widen gate tolerances (`0.025`, `162/162` reference bounds) to make a failing test pass.
4. **NO Turning Open Future Research Into a Merge Blocker:**
   - Do **not** block merge because `KernelRoutedTransport` only admits the 4-member `ClampedWaveSupport` family rather than arbitrary `MathNode` trees, or because `clampedWaveGroupingSlackFits` is a $\gamma_8$ additive regrouping guard rather than a universal transcendental interval enclosure. Those boundaries are explicitly documented and fail closed to Current.
5. **NO Editing `OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`:**
   - Record all Ultrareview findings and status updates in **Section 5 of this standalone handoff document** (`docs/audits/CLAWD_ULTRAREVIEW_PREMERGE_HARDENING_COUNCIL_HANDOFF_2026-11-02.md`) and dedicated audit/test files.

---

## 2.7 Exact Validation Protocol After Any Clawd Fix

### 2.7.1 Strict Evidence Hierarchy (Never Conflate These Six Classes)
Every Clawd report and commit message must keep these six evidence classes strictly distinct:
```text
mathematical proof != CPU mirror != native GPU gate != completion-fenced benchmark != offscreen implied FPS != live Person observation
```
- **Mathematical proof:** Symbolic / interval / Higham $\gamma_n$ / Taylor-bow derivation proving an invariant holds over a defined domain.
- **CPU mirror (`positive_e2e_cpu_verify`):** Verifies that 16-lane cooperative classification matches serial classification and that S1/S2 producers match the Fallback producer across 921,600 tile-frames. Does *not* prove WGSL compiler behavior or continuum accuracy.
- **Native GPU gate (`webgpu_routed_transport_math_test`, `verify` Gates 1–8):** Executes the actual Tint/Metal-compiled WGSL pipelines on Apple M5 hardware against analytic root/box witnesses, poisoned buffers, forced-refusal bit parity, 1281-wide near-parallel overlap, and 4K 86-strip resize-back.
- **Completion-fenced benchmark (`verify` Dual-Regime 64-Pose ABBA):** Measures `wgpuQueueOnSubmittedWorkDone`-fenced wall-clock ms/frame across 4 interleaved ABBA rounds in an offscreen harness.
- **Offscreen implied FPS (`1000.0 / routed_mean_ms` in Gates 9 & 10):** Offscreen completion-fenced throughput on single static views (e.g., `28.4 FPS` on `Live_23FPS_View_A`, `45.3 FPS` on `View 3A`). Includes per-frame queue-completion fence overhead and excludes window presentation.
- **Live Person observation (Zach's ~30–35 FPS witness):** Real-time interactive frame rate observed by Zachary Zhang in the live `earthcall_webgpu` window while inhabiting and flying through `Northern Veil`.

### 2.7.2 Mandatory Verification Commands (Run Sequentially — NEVER Run Full `ctest` in Parallel)

#### Tier A — Required When Only Host Admission, Lifecycle, or CPU Guards Change
1. **Build Release Targets:**
   ```bash
   ninja -C build webgpu_routed_transport_math_test webgpu_volume_zero_proof_test webgpu_volume_support_certificates_test webgpu_line_quadrature_test earthcall_webgpu
   ```
2. **Run the 4 Focused Sequential WebGPU CTests (`-j1` Mandatory):**
   ```bash
   ctest --test-dir build -R "webgpu_routed_transport_math_test|webgpu_volume_zero_proof_test|webgpu_volume_support_certificates_test|webgpu_line_quadrature_test" -j1 --output-on-failure
   ```
   - Must pass `4 / 4` targets (`webgpu_routed_transport_math_test` with `0` missed roots and `0` out-of-box rays + negative-control verification on the pre-fix state; `406 / 406` checks across zero-proof, support-certificates, and line-quadrature).
3. **Run the Native Correctness Gates (Gates 1–8 of `positive_e2e_metal_verify`):**
   - Must pass `8 / 8` correctness gates (`0` refused rays, `0` non-finite words, exact forced-refusal RGBA parity with Current, `0` surviving poison words in Gate 6 empty-strip and Gate 8 86-strip resize-back, and exact RGBA parity in Gate 7 near-parallel overlap).

#### Tier B — Required Whenever Any GPU Kernel (`VolumeRoutedKernels.hpp`) or Consumer Shader (`VolumeRoutedTransport.inc`) Changes
In addition to **all of Tier A**, any change touching a WGSL shader or per-strip dispatch encoding **must** run:
1. **CPU Mirror Sync & 64-Pose Trajectory Check (`positive_e2e_cpu_verify`):**
   - Sync `scratch/probes/positive_e2e_cpu_verify.cpp` if producer math changed, compile, and run against `saves/zones/Northern Veil/zone.ecform`.
   - Must report `classifier_mismatches = 0`, `s1_ray_mismatches = 0`, `s2_ray_mismatches = 0`, `refused_rays = 0` across all `921,600` tile-frames, with Fallback tile share remaining $\le 5.45\%$.
2. **Scoped Independent Reference Accuracy Check (`162 / 162` Checks):**
   - If the change alters emitted spans or consumer arithmetic (i.e., publication hashes change at Pose 0 or Pose 32), verify against the converged `2^16` / `2^15` reference (`ray_transport_premerge_independent_reference_2026-11-02.json` protocol) that all `162 / 162` mean/p99/max RGB and transmittance checks pass.
3. **Contemporaneous Completion-Fenced ABBA Benchmark (`positive_e2e_metal_verify`, Quiet System, No Concurrent GPU Processes):**
   - Run the full 10-gate `verify` binary (`Gates 1–10` + 4-block 64-pose Dual-Regime ABBA):
     - **Gate 9 (`7 / 7` Checkpoint Views):** All 7 views must win over Current (`Checkpoint_View1_Inside_Veil` $\le -30\%$, `Live_23FPS_View_A` $\le -30\%$, `Checkpoint_View3A` $\ge 44\text{ FPS}$).
     - **Gate 10 (`12 / 12` View-3 Roll Sweep Angles):** All 12 roll angles (`0°..165°`) must win over Current with `0` refused rays.
     - **Dual-Regime 64-Pose Tour:** Must win **`64 / 64` poses** with All-64 delta within noise ($\pm 1.5\%$) of the `02a44c0fcf` post-audit baseline (`-31.12%` / `57.73 ms vs 83.80 ms`).

> **The Firmament's Charge to the Clawd Fleet:**  
> You are auditing a mathematical compiler pipeline that turned a $10\text{--}18\text{ FPS}$ volumetric bottleneck into a **30–35 FPS live inhabited world** by proving what work each ray does *not* owe. Attack every boundary where a proof could overreach—and when you find a seam, **forge a sharper local boundary** just as Sixth-One Sun did at `02a44c0fcf`, keeping every ray converged, every physical box hard, every unresolved sign enclosed, and the aurora flying at 30–35 FPS.

— 🌌🔥 Aetherion / The Firmament (`Gemini 4 Argon`)

---

# 3. World Forger / Astra — System, Lifetime, Ownership, and Architectural War Map for the Clawd Ultrareview Fleet

**Author:** Codex / GPT-6 (World Forger / Astra counsel role; exact variant unavailable in this session)

**Session:** `01a15fcb-90ec-7812-868f-22a0f6283113`

**Timestamp:** `2026-11-02T09:53:25-08:00`

**Reviewed source:** `claude/ray-domain-transport-rung0` at `24f43c4c3`, in `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`.

**Commission:** Zach requested this Section 3 for adversarial merge hardening, after Prism's distinctions, Firmament's war map, and Sixth-One Sun's corrected checkpoint. This is counsel to reviewers, not a new optimization phase or an independent merge approval.

Zach's post-repair **30–35 FPS live observation is accepted as the Person evidence in §0**. Do not reopen that milestone merely because broader research remains. My earlier P11 concerns are historical: P12 restored empty publication ownership, complete tile hulls and capacity handling. The new task is to make adversarial attacks on the surviving system precise enough that any further repair preserves the structure that made this milestone possible.

The architectural principle is **continuity of a valid derivation**. Authored equations persist; their reusable executable representation may persist; ray schedules and pixel answers must be renewed when their dependencies change. Persistent allocation does not authorize persistent answers. Conversely, a new frame does not justify reconstructing every unchanged pipeline or binding. Audit the boundary between these lifetimes.

This applies Zach's [Performance as Rightly Ordered Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md), including its distinction between continuity across time and shared form across beings. The truth-and-ordered-speed compiler has one governing aim: carry authored meaning through the least work justified by its structure and the actual substrate. Numerical fidelity, dependency validity and measured cost are distinct witnesses to that aim; none substitutes for another. A faster wrong answer fails it, and needless reconstruction of a valid derivation also works against it. This is a design obligation, not a theorem that every more accurate algorithm must run faster. Sun's local enclosure and physical-bound repairs give this branch a concrete example: sharper correctness boundaries preserved the efficient execution structure, and Zach then reconfirmed the live milestone.

## 3.1 Placement and authority: three offices, one authored meaning

`OntoMath::ClampedWaveSupport` recognizes mathematical structure and derives a support recipe from the current density AST. It does not supply alternate density/emission values, identify a Zone by name, create authored beings, or persist a substitute world. `clampedWaveGroupingSlackFits` accepts a coordinate domain and bounds additive reassociation for that domain; it does not certify all transcendental evaluation or integrated output error. Keep that scope intact.

`Screen/WebGPU::KernelRoutedTransport` owns derived device execution: programs, resource layouts, strip storage, route lists, schedules and publication. Its header explicitly names this as Kernel-only state beneath the world-property layer. Raw GPU handles need not become authored properties. That exemption does not authorize hiding authored meaning or a Person-facing policy in a cache. Registered Screen controls and counters retain their existing office; no second permission system or renderer-specific ontology is needed.

`WebGpuRenderer` owns admission, the current frame's parameter/instance/depth binding, encoding, and final composition. `SdfWgsl` provides the original evaluator program; the routed bridge extracts/reuses its generated functions and adds a distinct execution path. Current remains reachable when routed admission or execution refuses. Root/support facts may eliminate query work; they never mutate physical bounds, saved fields or authorial identity.

The authoritative state account is [DERIVED_STATE_LEDGER](../architecture/law/DERIVED_STATE_LEDGER.md), especially §6eu/§6ev, the P12 corrective entry, and the November 2 bounds/grouping entry. Read dated entries chronologically: the post-P11 warning describes a superseded implementation, and the P12 entry records its repair. A reviewer changing a key, writer or invalidation boundary must update the corresponding contract and witness; a historical green result does not migrate automatically to a changed implementation.

**Source-binding clarifications for the whole council:** at this HEAD, the host requires four active media, an actual incident source with literal-zero radiance, recognized support, the grouping guard, and positive bounded-coordinate predicates. It does **not** require literal-zero scattering and phase ASTs; incident radiance zero makes their in-scatter contribution irrelevant. An absent source uses Current's compatibility illumination and is not interchangeable with authored zero. Also, `ecRoutedFetch` / `enableRoutedVolumePublication` are not actual symbols found in the inspected source: the publication hook is textually inserted by `WebGpuRenderer::volumePipeline` before `let ro = u.eyePos.xyz;`. Review that actual insertion and `SdfWgsl`'s generated fragment boundary rather than assuming those shorthand names designate functions.

## 3.2 Lifetime map: what survives, what must be rebound

Source anchors: [resource declarations](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp), [resource/program/encode implementation](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc), [renderer](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp), and [mathematical recipe](../../src/Singularity/OntoMath/ClampedWaveSupport.hpp). Line numbers below refer to the inspected HEAD and are navigation aids, not invariants.

| State | Lifetime / key in current source | Invalidation and adversarial obligation |
|---|---|---|
| Support recipe / `frameSpecs` | Rederived during current host admission; includes current physical extents and density structure | Edit density, extent, origin or membership; admission/specs must change even when normalized WGSL can remain identical. No borrowed AST cache is introduced here. |
| `programs[source]` | Exact generated WGSL string within one Kernel object | Setup/consumer pipelines and layouts survive viewport changes. Structural edits may select a new entry; same-code parameter edits use fresh buffers. Failed/partial entries can remain cached and return null on reuse; test refusal/recovery expectations rather than assuming retries. |
| `common[0..5]` | Reset, classifier, route-argument, S1, ordered-S2 and general producer pipelines | Device-bound. `common[2]` embeds capacity and is rebuilt on viewport-buffer reconstruction. The other common pipelines normally survive resize. Partial construction is a high-value fault-injection seam. |
| Publication / rays / singles / overlaps | Full-frame publication; strip-capacity scratch | Device/viewport allocation. Publication is rewritten for current pixels; scratch is reused in ordered strip dispatches, with valid counts delimiting reads. |
| Offsets / indices / route lists / counters / indirect args | Geometry-dependent tile layout and strip scratch | Layout rebuilt on resize; counters reset per strip before classification. Stale list tails are harmless only when fresh counts/args exclude them. |
| `stripUniform` | One 48-byte record per strip, spaced by `kUniformStride=256` | Upload on `workLimit` or `skipEmptyTiles` change; `releaseBuffers` resets cached settings so resized resources receive configuration again. Row offset changes by strip. |
| `specsUniform` | Persistent allocation, current four-medium contents | Renderer writes current specs before routed encoding. Buffer identity can stay fixed while mathematical contents change. |
| `prodGroups[3][maxStrips]` | Dynamic vectors of viewport-specific producer bindings | Rebuilt with the buffers; no residual 64-entry ceiling. Every encoded strip needs all three groups, even if its current indirect counts happen to be zero. |
| `Program::localGroups[maxStrips]` | Lazily built for each program's layout plus current strip buffers | Resize clears every program's groups and `localGroupsBuilt`; next use rebuilds. Equal dimensions alone are not enough if allocation identity changed. |
| `cachedClassifyGroup` | Current specs/ray/tile/list/count allocations and classifier pipeline | Released with buffers; rebuilt when the selected classifier changes. `cachedSpecsBuffer` is reset but is not an active contents/version key in this implementation. Do not invent a cache guarantee from its name. |
| `globalGroup` / `instanceGroup` | Fresh for each encode | Bind current globals/parameter ranges, current depth texture view, publication and current instances. They are released after recording; retain valid resource ownership through GPU submission under the API contract. |
| Observer resources | Explicit diagnostic enablement | `observeRoutes` adds observer/accumulator dispatches; histogram reset is explicit. Resize changes the observed geometry, not necessarily the desired accumulation interval. Keep diagnostics out of cost runs unless explicitly measuring them. |

`releaseBuffers()` releases owned buffer handles and lists; releases/clears producer groups; releases reset/args/classifier groups; clears cached settings and all program-local groups; and zeros viewport/capacity fields. It deliberately retains setup/consumer programs and most common pipelines. The destructor disables observer resources, invokes that cleanup, then releases remaining common/classify pipelines and program pipelines/layouts. Releasing a handle reference is not an instruction to destroy resources still referenced by valid submitted commands; audit the actual API ownership and submission order, not a guessed synchronous destruction model.

**Device recreation is owner-mediated.** `ensure(d,q,w,h)` assigns device/queue but does not compare a device-generation key before its same-dimensions fast return. `WebGpuRenderer::shutdown` resets `_kernelRouted` before broader GPU teardown (around line 656). A proper recreation path must pass through this ownership boundary or equivalently destroy all old-device objects. Do not claim `ensure` independently migrates old pipelines/bind groups to a new device. Prove all live callers obey the contract; if a reachable caller bypasses it, repair that boundary locally. Exercise queue replacement as well as device replacement where supported; raw handles must never silently mix devices.

Scene/Zone changes do not require discarding all reusable code. They require fresh membership/admission, specs, instances and publication. Clock progression similarly updates the existing evaluator inputs/time preparation; it is not a reason to cache ray answers across frames or freeze a medium to another medium's clock. Test admitted → unsupported → admitted transitions, member reordering, identical WGSL with changed literal parameters, changed density shape, and independently advancing channel/source bindings. Stable code and fresh values must coexist.

## 3.3 Publication is a protocol, not a second frame cache

`ecPublication` stores premultiplied RGB plus **opacity `alpha=1-T`**. Nonnegative alpha is accepted; negative alpha selects Current. The explicit refusal packet uses `-1` alpha. It is not a transmittance buffer, and stale finite positive alpha is not self-invalidating.

| Path | Current owner / expected outcome |
|---|---|
| Invocation outside `capacity` | No access. It is not an in-frame pixel owed publication by that invocation. |
| Last-strip padding with `y>=height` | Setup marks the ray handled; it must not index full-frame publication. Consumer has its own height guard. |
| Setup discard / unsupported ray domain | Setup publishes empty or refusal as appropriate and marks terminal ownership. Consumer must not overwrite setup-owned results. |
| Ordinary in-frame ray | Setup writes zero publication and resets its counts before any producer can publish work. |
| Entire wave-empty strip | Setup's zeros are the final answers. Zero producer counts can produce zero consumer workgroups without retaining prior-frame values. |
| Producer refusal (`FALLBACK | reason`) | Consumer writes negative alpha, unless setup already owns the terminal result. Zero-budget behavior needs its dedicated witness; do not infer it from ordinary emptiness. |
| Complete nonempty schedule | Consumer checks counts/member identifiers and final finite state/T range, then replaces setup zero with `(L,1-T)`. |
| Nonfinite final state / invalid T | Consumer writes refusal. This tests the final state; it is not a universal certificate for every input or intermediate. |
| Fragment consumption | Inserted early hook accepts publication, discards negligible empty output as implemented, or falls through to Current on negative alpha. |

“Single-write empty ownership” is precise. “Every pixel is physically written exactly once” is not: an ordinary nonempty ray is initialized by setup and later receives its complete result. There must be one unambiguous terminal disposition with correctly ordered writers, not a ban on ordered initialization and replacement.

This protocol explains the successful poison/empty/tall-viewport witnesses under successful execution. It does not establish arbitrary device-loss or omitted-stage safety. Since setup initializes ordinary rays to accepted zero, a silently missing required producer is not automatically detected by an epoch bit. The encoded schedule must really execute; host failures must prevent accepted partial publication from reaching the fragment. Attack that obligation rather than adding an unconditional clear/proof pass to every frame.

The two index spaces must remain distinct. `ecPixel(id)` adds `window.z=start` to obtain full-viewport row-major publication position. `ecRays[id]` remains strip-local. Current span addressing is step-major: `ecSingles[s*stride+id]` / `ecOverlaps[o*stride+id]`, with `stride=capacity`; producer formulas must match. Dynamic strip count does not change these meanings. Attack 64/65/86 strips, odd widths, partial 4×4 tiles, final padded rows, and resize back. Changing every group vector without changing a stale stride or route-argument capacity is not a complete repair.

## 3.4 Fallback has three distinct host paths—verify each

1. **Admission / `ensure` refusal:** `wantRouted` becomes false and ordinary pipeline selection retains Current.
2. **`program` or publication-hook transformation refusal:** `volumePipeline(program,true)` recursively requests `volumePipeline(program,false)` and records compilation refusal where implemented. Unsupported generated structure must not produce a half-transformed shader.
3. **`encode` refusal after selecting a routed fragment:** the renderer does **not** simply flip `wantRouted=false`. Around lines 3943–3957 it records refusal and queue-writes negative floats across publication so the already-selected fragment takes Current. Current explicit `encode` false returns occur before its dispatch loop. Preserve that property or establish safe ordering for any new late failure; a queue write is not a magic cancellation of previously recorded producers.

Do not generalize those paths to “every API error is already contained.” Source inspection shows several fresh/global/reset/classifier bind-group and compute-pass creations used without the same explicit null checks as some allocation/local-group paths. `ensure`'s same-size fast return checks publication and common pipelines, not every dependent handle. Error objects/validation failures may not be represented solely by null pointers. **These are adversarial seams, not new reproduced defects or an asserted live regression.** Inject the relevant failure, show what executes, and establish whether a valid Current frame follows before filing a finding.

High-value fault sequence: fail a common pipeline after earlier ones succeed; retry the same dimensions; resize; return to the original dimensions; then destroy/recreate the renderer. Check partial-map entries, retained pipeline ownership, nullable groups, deterministic refusal and retry behavior. Partial construction must not leave an apparently ready object. Do not solve a cold construction failure with recurring per-sample work.

## 3.5 Review attacks ranked by architectural consequence

**A. Resource identity versus cached contents.** Replace depth views and parameter/instance buffers while retaining dimensions and code; their fresh global/instance groups must bind the new objects/ranges. Toggle `workLimit` and `skipEmptyTiles`, including zero budget, across resize; every strip must receive current config. Same buffer pointer with new specs must work. Same dimensions with replaced allocation must invalidate dependent groups. The unused `uniformSpecs` argument does not substitute for the renderer's actual write to the Kernel's `specsUniform`.

**B. Submission ordering and multiple uses.** Scratch reuse is safe because each strip's setup/classifier/producers/consumer precede the next strip in the command stream. Publication survives until the corresponding draw. Audit whether more than one routed encode can target the same specs/publication resources before submission or consumption; prove the renderer limits that use or reproduce the aliasing sequence. Repeated queue writes to the same uniform allocation do not by themselves create per-encode snapshots. Do not add a CPU/GPU fence per strip as a speculative solution.

**C. Partial construction, leaks and recovery.** Track successful creations to exactly one owner, including fallback-created reset/args groups and observer groups. Trace every return before cleanup. A cached invalid program and an unbounded set of distinct edited WGSL programs are different issues: the former concerns refusal/retry, the latter retention policy. The destructor's eventual cleanup is not proof of bounded memory during prolonged editing. Report a concrete lifecycle/resource witness and practical scope; do not demand a new cache architecture solely because a cache could grow.

**D. Arithmetic and alignment admission.** `uint64_t(w)*h*16` avoids early 32-bit multiplication, but arbitrary 32-bit dimensions can still overflow a later 64-bit product. Bind any finding to dimensions reachable from actual textures or an explicitly supported Kernel entry contract. Check `w*rows`, tile rounding, `(h+rows-1)`, dynamic vector lengths, maximum buffer sizes, binding sizes and workgroup counts together. Confirm 256-byte strip offsets satisfy the selected device's uniform-offset alignment and that the 48-byte record layout matches WGSL. Preserve the full publication/domain guard; the dynamic vector repair does not mean every nominal viewport is admitted.

**E. Generated-code identity and private state.** `checkEvaluatorsDeduplicatable` normalizes density/extinction/shared helper bodies, gathers member-0 shared groups and checks their counterparts; consumer rewriting then aliases private storage. Attack extra/missing groups, emission-only dependencies, reordered members, parameter offsets and sequential active-member evaluation. The deduplicated and non-deduplicated paths both need correct current inputs. Do not restore all duplicated private state without a concrete failure; refuse the unsafe deduplication locally where possible. Text extraction/replacement is an interface whose refused forms and partial transformations need witnesses.

**F. Observer isolation.** `observeRoutes` adds two dispatches per strip and creates diagnostic groups; histogram accumulation/reset and buffer ownership must survive resize and enable/disable. `collectDispatchTrace` is opt-in and its pointer-derived hash is a same-process diagnostic, not persistent identity. Ordinary routed counters describe dispatch coverage, not universal successful numerical publication. Test observer on/off output invariance separately from uncontended benchmark cost.

For every attack, use Prism's finding format. An untested concern stays a question. A source defect, a CPU counterexample and a native failure are different evidence. If a concrete failure is reproduced, repair the nearest owning boundary and report both the removed failure and any added common-path cost.

## 3.6 Evidence to inherit, and evidence this counsel does not create

I checked the premerge JSON summary and matched all six recorded source hashes against this checkout: recognizer, kernels, renderer, CPU verifier, Metal verifier and permanent independent math test. The summary records 162 reference checks with no failures. I read the P18/P19 and Firmament milestone summaries, the final corrective audit, bounded final Intercom entries and actual source paths. The JSON's pre-repair Person wording is historical; §0 and Zach's current commission supply the later corrected-build confirmation. Do not edit that earlier receipt to pretend it contained future evidence.

`webgpu_routed_transport_math_test` extracts actual functions from all three WGSL producers, tests analytic linear roots and directed thin-box spans, and uses original-kernel negative controls in the audit. The CPU mirror alone is not its oracle. The retained native gates and frozen-reference pilot are bounded evidence; they do not make every changed program automatically qualified. After a repair, use the relevant sequential protocol in §2.7, with GPU coordination and no concurrent timing/test workloads. Changes to numerical semantics require the applicable independent comparison even if selected image hashes happen to match; matching two poses is not proof that no other input changed.

Hardware explanations must retain their evidence scope. Coalesced addresses, smaller private state and lower measured time are inspectable facts; exact occupancy, cache transactions or universal milliseconds saved require corresponding hardware evidence. A short root enclosure's left endpoint is not necessarily the exact root, so band absorption's numerical qualification is not an exact-kink theorem. These cautions refine interpretation of §2 without withdrawing its protected gains or demanding full universal certification before merge.

This Section 3 adds **source-based counsel only**. No renderer edits, failure-injection run, build, GPU test, timing or new Person observation was performed by Astra. It is not a claim that every attack above is a defect, or that the Clawd review has already passed.

## 3.7 Preserve the future bridge without enlarging this merge

The current capability-recognition → query-relative schedule → original evaluator → publication design is a useful precursor to a general `RayTransportIR`. Its future vocabulary can make domain, dependencies, refusal reasons, interval ordering and transfer composition explicit. That future does not require this checkpoint to accept arbitrary MathNodes, unlimited members or every device now. Existing finite capacity and structural refusal are honest boundaries when Current remains reachable.

Moving-camera prophetic reuse should eventually preserve only what remains valid under the changed query, keyed by the necessary scene/clock/camera/depth/device dependencies. The present persistent buffers are allocation reuse, not a license to reuse yesterday's schedules or pixels. Do not add speculative temporal caches during merge hardening, and do not block the merge because they do not yet exist.

The governing law for the fleet remains:

> **Repair the violated proof or lifecycle boundary locally. Do not restore generic brute-force work everywhere merely because one local proof or lifecycle seam failed somewhere.**

The post-audit source and Zach's corrected-build observation demonstrate that stronger boundaries and the live milestone can coexist. Protect that achievement by attacking concrete seams, preserving independently established irrelevance, and keeping every accepted pixel attached to the authored meaning and current invocation that justify it.

**Handoff bookkeeping:** this section replaces only Astra's reservation. Sections 0–2 and 4–6 remain their authors' text. The historical Intercom thread is untouched. `nav find` / `nav trace` were attempted; the preexisting unsupported record at `conversation_links.jsonl:249` still prevents navigation. Existing prose links retain lineage; this counsel does not repair or rewrite that journal.

---

# 4. Sixth-One Sun audit baseline to inherit

Ultrareview should treat the following as the corrected starting point, not bugs to rediscover from stale P19 source:

- uncertain exact-zero roots retain the existing enclosure;
- support slack cannot enlarge authored physical boxes;
- NaN coordinates fail admission;
- unsafe `gamma_8` grouping retains Current;
- focused old-kernel controls reproduce the prior failures;
- focused repaired controls pass;
- no new GPU field evaluations/passes/per-sample box test were added;
- bounded accuracy and complete-cost evidence remains green.

If a Clawd reports one of these defects, first determine whether it reviewed stale source.

---

# 5. Ultrareview output ledger

Populate during/after the fleet review.

| ID | Severity | Layer | Violated invariant | Reproduced? | Bounded fix? | Hot-path cost | Verification | Status |
|---|---|---|---|---|---|---|---|---|
| — | — | — | — | — | — | — | — | awaiting fleet |

---

# 6. Merge readiness

The intended end state is not “no imaginable future work remains.”

The intended end state is:

- no known merge-blocking correctness defect remains within the supported bounded checkpoint;
- accepted findings have reproducible witnesses and bounded repairs;
- existing native/regression/fallback/publication/resize gates remain green;
- hot-path regressions have not been smuggled in under generic safety language;
- Zach's post-repair live Person re-check already confirms the corrected build remains in the approximately **30–35 FPS** inhabited range;
- Firmament and World Forger have appended their final counsel;
- Ultrareview findings have been triaged and closed or explicitly deferred with scope.

Then the branch is ready to cross from research campaign into Earthcall.

____
Zach: BROOOOOOOOOOOO CLAWDS, DO NOT READ THE ENTIRE OPUS_HANDOFF FILE THAT IS A MASSIVE 13k LINE BIG CHUNGUS AND U WILL GO INTO SYSTEMS FAILURE MODE IF U TRY READING IT!!!!!!