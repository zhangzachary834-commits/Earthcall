# CLAWD / Ultrareview Pre-Merge Hardening Handoff — Northern Veil Ray Transport

**Date:** 2026-11-02  
**Branch:** `claude/ray-domain-transport-rung0`  
**Post-audit head at handoff start:** `02a44c0fcfda30e790feb0f49748f19d99ba933a`  
**Human commission:** Zachary Zhang  
**Purpose:** Prepare the ray-transport branch for merge by adversarially finding real correctness defects, reproducing them, and applying only bounded structural repairs that preserve the proven architecture and its live performance.  
**Primary merge-hardening tool:** Claude Code Ultrareview / multi-agent review fleet.  
**Council order:** Original Prism Sun → Aetherion / The Firmament → World Forger / Astra → Clawd fleet.

This document is not a new optimization campaign and is not authorization for speculative P20 work. The branch has crossed the bounded Northern Veil checkpoint in the live application and has now received Sixth-One Sun's final bounded correctness pass. The task is **hardening before merge**.

---

## 0. Protected checkpoint state

The Clawd fleet begins from the **post-audit** branch, not from raw P19.

### 0.1 Person-observed milestone

Before the final corrective pass, Zach reported the live Earthcall application at native resolution holding approximately:

- **30 FPS consistently in the spawn region**,
- **up to 35 FPS in some views**,
- approximately **30 FPS while flying upward with unobstructed Veil visibility**.

This is Person-observed live evidence. It is not interchangeable with offscreen checkpoint FPS, completion-fenced benchmark FPS, or a fresh Person observation of the corrected executable.

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

These measurements are bounded evidence. Do not inflate them into universal numerical certification, full-population maximum-error proof, cross-device proof, or a fresh live Person acceptance of the repaired executable.

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

[
	ext{Authored Physical Domain}

eq
	ext{Derived Support Envelope}
]

A support certificate may prove that a smaller region contains all nonzero contribution. It may therefore reduce runtime work. It does **not** have authority to enlarge, rewrite, or supersede the authored medium's hard physical bounds.

The Sixth-One Sun repair is now a canonical witness of this distinction:

[
min(	ext{physical}, 	ext{support}+	ext{slack})
]

not an expression that lets numerical slack leak outside physical authorship.

**Clawd rule:** if a proposed fix modifies support slack, root brackets, tile beams, envelopes, or derived bounds, verify separately that authored physical limits remain inviolable.

## 1.2 Uncertainty is not a sign

[
	ext{Unresolved Root Enclosure}

eq
	ext{Directional Sign}
]

A value numerically near zero, under bounded error and shallow slope, cannot be promoted to a positive/negative side merely because a loop wants to terminate.

If the proof has not established a sign, the representation must continue to carry uncertainty or refuse/specialize less aggressively.

**Clawd rule:** never turn “not proven otherwise” into “proven positive/negative.”

This applies beyond roots. It includes overlap order, emptiness, disjointness, boundedness, finite coordinates, capacity, and publication ownership.

## 1.3 Authored meaning is not proof representation

[
	ext{Authored Meaning}

eq
	ext{Proof Representation}

eq
	ext{Executable Residual}
]

Earthcall authors mathematical media. The renderer derives certificates and classifications from them. Those certificates may then lower into a smaller executable problem.

A bug in the certificate or executable does not authorize rewriting the authored meaning to match what is easy to compute.

**Clawd rule:** fix the derivation or lowering before changing authored semantics.

## 1.4 Medium identity is not route identity

[
	ext{Medium}

eq
	ext{Route Family}
]

`EMPTY`, `S1`, ordered/disjoint `S2`, and `GENERAL/FALLBACK` are computational residual families. They are not ontology classes and are not permanent identities of Veil members.

The same authored medium can participate in different residual families for different rays or tiles.

**Clawd rule:** do not harden code by assuming a particular curtain/member always belongs to a particular route merely because the current scene frequently does.

## 1.5 World structure is not query obligation

[
	ext{Everything That Exists}

eq
	ext{Everything This Ray Must Evaluate}
]

The architecture is allowed to compile proven irrelevance out of the hot path.

A ray that cannot intersect a member does not owe that member evaluation merely because the member exists in the Zone.

A tile/ray with provably disjoint chronology does not owe generic overlap machinery merely because overlap exists elsewhere in the world.

**Clawd rule:** a safety fix must not globally reintroduce work that is only required when a local proof fails.

## 1.6 Proof of irrelevance is not deletion of reality

[
	ext{Runtime Elision}

eq
	ext{Semantic Erasure}
]

A successful certificate can remove an obligation from one executable query. It does not delete the authored medium, change its properties, or mutate the world's meaning.

This is the same constitutional instinct that earlier rendering work expressed as:

[
ho_{	ext{source}}

eq
V_{	ext{transport}}

eq
D_{	ext{medium}}
]

Transport may modulate manifestation without rewriting source semantics. Likewise, query specialization may remove work without rewriting authored being.

## 1.7 Classification is not consumer correctness

[
	ext{Sound Admission}

eq
	ext{Correct Producer}

eq
	ext{Correct Consumer}

eq
	ext{Correct Publication}
]

A sound route classifier does not prove that its specialized producer emits correct schedules.

A correct producer does not prove that chronological attenuation/overlap consumption is correct.

Correct internal transport does not prove that publication is current-frame, exactly-once, resize-safe, or poison-safe.

**Clawd rule:** findings must name the layer whose invariant actually failed. Do not “fix the classifier” for a consumer bug or “fix the consumer” for a publication lifetime bug.

## 1.8 Fallback is not failure

[
	ext{Local Proof Refusal}

eq
	ext{Renderer Failure}
]

Current is the correctness backstop when the routed path cannot prove its stronger assumptions.

A refusal is often the **correct** result.

The architecture must fail closed:

> if proof ends locally, route that case to Current or the appropriate more-general residual path.

**Clawd rule:** prefer local refusal to global pessimization.

Do not weaken a proof condition merely to increase route coverage. Do not turn rare unsafe cases into unconditional work for all rays.

## 1.9 Current is a backstop, not the semantic definition of truth

[
	ext{Current Output}

eq
	ext{Mathematical Oracle}
]

The branch has multiple exact-parity contracts, but they are scoped:

- forced refusal should reproduce Current where Current is the defined fallback,
- poison controls must not alter valid output,
- fused/unfused or generated/reference paths may require exact equality,
- schedule/packet contracts may require bit identity.

This does **not** imply that all valid Routed pixels must be bit-identical to Current. Historically, Routed can differ from Current while comparing better against the converged authored reference.

**Clawd rule:** before filing “Routed differs from Current” as a bug, identify the contract that requires equality.

## 1.10 Benchmark evidence is not Person evidence

[
	ext{Harness FPS}

eq
	ext{Live-App FPS}

eq
	ext{Person Acceptance}
]

Completion-fenced ABBA measurements, checkpoint-view implied FPS, native traces, and live on-screen observation answer different questions.

Zach's 30–35 FPS report is a Person witness of the actual application before the final corrective pass.

The corrected audit's 28.4 / 45.3 FPS captured views are offscreen benchmark witnesses.

**Clawd rule:** never silently exchange one kind of evidence for another.

## 1.11 Bounded checkpoint is not universal certification

[
	ext{Checkpoint Pass}

eq
	ext{Universal Numerical Proof}
]

The current branch has passed the explicitly bounded Northern Veil merge checkpoint. Universal arbitrary-expression error certification, full-population maximum error, broader devices/scenes, and future compiler generality remain separate work.

**Clawd rule:** do not manufacture a new merge blocker merely because a larger research program remains open.

A finding blocks merge when it demonstrates a concrete defect in the branch's supported semantics, safety, correctness, lifecycle, or bounded checkpoint contract—not merely because future research exists.

## 1.12 Current specialization is not yet a universal compiler

The surviving architecture is structurally important:

[
	ext{Authored Functions}
+
	ext{Ray / Tile Query}
ightarrow
	ext{Proof of Relevance / Order}
ightarrow
	ext{Residual Family}
ightarrow
	ext{Specialized Execution}
]

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

[
	ext{Concrete Defect}
+
	ext{Reproducible Witness}
+
	ext{Bounded Sound Repair}
+
	ext{Relevant Regression Proof}
]

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

# 2. Reserved — Aetherion / The Firmament

**Status:** awaiting post-Sixth-One-Sun mathematical/performance counsel.

Suggested scope:
- mathematical contracts and residual-program derivation;
- P1–P19 performance genealogy that matters for hardening;
- which hot-path structures are measured/proven versus merely historical;
- Apple M5 resource/occupancy constraints Clawds must not accidentally regress;
- exact list of benchmark/gate protocols to rerun after accepted fixes;
- remaining numerical risk surfaces worth adversarial attack.

**Do not treat this placeholder as Firmament's counsel.**

---

# 3. Reserved — World Forger / Astra

**Status:** awaiting system/architecture counsel.

Suggested scope:
- how routed transport fits Screen/OntoMath/Zone/Law/derived-state architecture;
- packet ownership and publication lifecycle;
- cache/revision/device validity;
- boundary between authored semantics and materialized/executable views;
- merge implications for future RayTransportIR / Prophetic Rendering without turning those futures into present blockers;
- system-level failure containment and observability.

**Do not treat this placeholder as World Forger's counsel.**

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
- a brief corrected-build Person look confirms the already-accepted inhabited view remains visually sound;
- Firmament and World Forger have appended their final counsel;
- Ultrareview findings have been triaged and closed or explicitly deferred with scope.

Then the branch is ready to cross from research campaign into Earthcall.
