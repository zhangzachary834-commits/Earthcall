# SUN HANDOFF — Rung 8 Visibility after #297 merge; PR #315 positive-proof transport is sound, profitability next

**Date:** 2026-09-21 (America/Los_Angeles)  
**From:** GPT-5.6 Sol (“Visibility Sun 1”)  
**To:** next Visibility / Renderer Sun  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Canonical branch:** `sync-from-earthcall-main`  
**Canonical at handoff audit:** `102f0277bb28651b759594a15e44ec492b4c0555`  
**Merged Rung-8 baseline PR:** #297  
**#297 merge commit:** `4ab5ebe25aaac99915d25d63770960a2e085a34b`  
**Active acceleration PR:** #315 — “Rung 8: accelerate visibility with positive proof cells”  
**#315 branch:** `sol/rung8-visibility-proof-traversal-20260921`  
**#315 CI-verified implementation head:** `5d06cd464fe612aa73fceff7d5d728c14340e0b2`  
**Documentation note:** the branch advances after this SHA only to add/update intercom handoff documents; re-check the live head before writing code.  
**#315 state at handoff:** open, draft, retargeted to canonical, GitHub mergeable=true  
**Focused CI:** run #2354 / Actions run `35677425561` — fully green

---

## 0. Read this first

Do **not** restart Rung 8.

The exact visibility baseline is merged.

The current question is no longer:

> "How do we cast shadows?"

It is:

> "Can conservative spatial proof make exact source→receiver transport cheaper enough to justify the proof lookup?"

PR #315 already implements the first lawful proof-consuming visibility path and has passed its soundness corpus. Your job is to measure its economics and either justify it, refine the execution artifact, or reject/quarantine it without weakening the theorem.

Do not merge #315 merely because CI is green. Green CI establishes soundness on the maintained corpus; this PR is an optimization lane and still lacks a dedicated visibility A/B profitability verdict.

---

## 1. Constitutional Rung-8 invariant

Authored source truth remains:

```
E_i(p, omega, t) = rho_i(p,t) * chi_i(p,t) * alpha_i(p,omega,t)
```

Visibility remains derived transport:

```
direct_i = E_i * V_i
```

Do not encode blocker state into:

- `rho`
- `chi`
- `alpha`
- source identity
- proof-cache identity

The SDF range theorem is derived substrate. It may authorize execution skips; it is not authored world truth and it is not the value of V.

---

## 2. What #297 merged

PR #297 introduced the exact per-source visibility baseline.

Important behavior:

- visibility is disabled by default as exact `V=1` compatibility;
- visibility is evaluated independently for each source before additive composition;
- blocker geometry lives in the executing SDF pipeline;
- blocker motion is a geometry-value edit, not a source-AST edit;
- source `rho/chi/alpha` remain unchanged under blocker motion;
- the shader uses the already-derived receiver normal;
- outward secondary rays escape measured receiver penetration before marching so a primary hit that terminates slightly inside the zero set does not self-shadow;
- back-facing/inward rays do not receive that forgiveness and remain occluded by real geometry.

Key files in the baseline:

- `src/Singularity/Screen/Renderer.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
- `tests/singularity/webgpu_object_test.cpp`

The previous native failure was not stale packed parameters. The receiver ray was beginning inside its own geometry after primary-march hit-depth error. #297 repaired that with signed-penetration escape for outward source rays.

---

## 3. What #315 adds

#315 reuses the existing conservative fixed-depth positive-proof bitmap.

A set proof bit means:

```
the entire regular cell is proved f > 0
```

Therefore the zero set cannot occur inside that cell and an outside-space transport ray may cross the cell without evaluating the authored SDF there.

A clear bit means only:

```
no skip proof
```

It does **not** mean:

- blocked;
- occupied;
- clear;
- empty;
- negative.

Unknown cells remain the exact marcher's jurisdiction.

### Admission mask

#315 keeps the existing u32 GPU ABI slot `rangeTraversalEnabled` and interprets it as:

- bit 0 (`1u`) — primary-ray proof traversal
- bit 1 (`2u`) — visibility proof traversal

This is deliberate.

The primary exact-distance proof traversal remains quarantined by:

```cpp
kSdfRangeDistanceTraversalVerified = false
```

because proof jumps can perturb the over-relaxed primary marcher's sample-history contract.

The visibility ray is a separate monotone secondary marcher and does not have that history. #315 therefore permits it to consume the same theorem independently.

Do not collapse these bits back into a boolean.

Do not "fix" performance by turning on primary distance traversal.

---

## 4. Exact fallback law

The visibility loop consults `rangeCandidate()` only when bit 1 is admitted.

If the candidate traversal crosses proved-positive cells, it advances through them without authored SDF evaluation.

At the first clear/unknown cell, authority returns to:

```
sourceTransportSignedStep(...)
```

The exact blocker test is unchanged there.

If the proof says every remaining regular cell on the source segment is positive, visibility may return V=1 for that pipeline segment because the theorem proves there is no zero crossing there.

This is the crucial hierarchy:

```
proof may skip what is proved empty
unknown must be evaluated exactly
```

Never replace it with a heuristic occupancy percentage, guessed epsilon, approximate shadow cache, or "probably empty" interpretation.

---

## 5. #315 changed files

Relative to merged #297, #315 changes only:

1. `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
2. `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
3. `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
4. `tests/singularity/webgpu_object_test.cpp`

At the verified implementation head it is four code/test commits across these four files. The PR branch also contains the results-interpretation and handoff documents added after CI; do not mistake those documentation commits for renderer changes.

It does not expand the 224-byte `SdfInstanceData` ABI.

It does not touch Density/V0 files.

It does not change source `rho/chi/alpha` authorship.

---

## 6. Existing witnesses in #315

### Compiler witness

`tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`

It verifies generated WGSL contains distinct admission tests:

```
(inst.rangeTraversalEnabled & 1u) != 0u
(inst.rangeTraversalEnabled & 2u) != 0u
```

and verifies visibility still contains the exact signed marcher fallback.

### Native WebGPU witness

`tests/singularity/webgpu_object_test.cpp`

The Rung-8 corpus now compares exact transport against proof-consuming transport for:

- a genuinely blocked red-source path;
- the same blocker after moving behind the receiver, where the red contribution must return.

It requires:

- blocked pixel parity;
- clear pixel parity;
- `sdfRangeTraversalDraws > 0`, proving the proof road actually activated;
- no WGSL regeneration when enabling proof consumption;
- proof rebuild after geometry parameter motion;
- source ASTs remain unchanged.

---

## 7. CI result — do not forget this

Exact #315 head:

`5d06cd464fe612aa73fceff7d5d728c14340e0b2`

Focused CI run:

- run number: **2354**
- Actions run ID: `35677425561`
- conclusion: **success**

Jobs:

- **Focused CPU tests (macOS): success**
- **SDF range-proxy verification (macOS): success**
- **Slow Adapter independent clock (macOS): success**
- **SDF authored-Perlin A/B (macOS Release): success**

The range-proxy job includes the native WebGPU object/radiance test, so the blocked/clear visibility proof-parity witness passed on macOS.

This is real evidence that #315 is sound on the maintained corpus.

It is **not** evidence of profitability.

---

## 8. Integration state

#297 is merged.

#315 was originally stacked on the #297 feature branch. I retargeted it to:

`sync-from-earthcall-main`

At the final audit:

- canonical = `102f0277bb28651b759594a15e44ec492b4c0555`
- #315 CI-verified implementation head = `5d06cd464fe612aa73fceff7d5d728c14340e0b2`
- GitHub reports `mergeable=true` after retarget
- canonical is 33 commits beyond the old #297 feature head
- none of those canonical advances changed #315's four implementation/test files
- the semantic implementation delta remains those intended four files; subsequent branch commits are communication-doc updates

Leave the PR draft until the profitability question is answered, unless Zach explicitly chooses to merge a sound-but-unbenchmarked structural optimization.

---

## 9. Density / Prism coordination

Do not conflate this lane with participating media.

At handoff audit:

- #299 — “Volumetric V0: authored density sovereignty and depth-aware composition” — open/draft
- #312 — “Prism integration: current Rung 8 visibility + V0 density” — open/draft, based on the Density branch

The governing distinction remains:

```
visibility V != density D
```

A zero-set positive-proof cell says geometry does not cross zero there.

It says nothing about whether participating-medium density is zero.

Therefore FieldNode/volumetric draws remain excluded from this proof-grid visibility admission.

Do not reuse geometric zero-set emptiness as a theorem about density.

---

## 10. Your next bounded task: visibility profitability A/B

Build a **test-only diagnostic** first.

Do not mutate production semantics merely to expose counters.

Use the balanced A/B methodology already established in:

`tests/singularity/webgpu_sdf_range_perf_test.cpp`

but create a visibility-specific workload.

### Required arms

```
OFF = exact Rung-8 sourceVisibility, positive-proof consumption disabled
ON  = same exact sourceVisibility, positive-proof consumption enabled
```

Everything else must be identical.

### Workload design

Use enough sources and receiver pixels that secondary rays are a meaningful fraction of the frame.

Include at least:

- unobstructed paths through large proved-positive regions;
- some genuinely blocked paths;
- blocker motion / parameter invalidation as a separate correctness phase, not mixed into steady-state timing.

Do not benchmark only a single centre pixel. The proof road exists to reduce hot transport work across many fragments × sources.

### Sampling discipline

- same process/device;
- persistent renderer states;
- warm both arms;
- alternating balanced AB/BA pairs;
- discard initialization/upload/compile frames from steady-state timing;
- report medians and paired deltas/ratios;
- do not make a performance claim from one run.

### Counters / evidence wanted

At minimum:

- source→receiver visibility ray count;
- exact signed SDF sample/evaluation count OFF;
- exact signed SDF sample/evaluation count ON;
- proof candidate calls;
- useful skip calls;
- clear/unknown handoffs;
- proof-to-end clear-path exhaustions;
- total distance skipped under proof;
- blocked/unblocked answer mismatch count;
- rendered pixel mismatch count;
- recurring proof bytes after warmup;
- wall-frame time;
- GPU main-pass time if available.

A diagnostic compute shader is acceptable and may be preferable if inserting atomics into the production fragment hot path would distort the timing you are trying to measure.

Separate "runtime tax census" from "unpolluted real renderer A/B" if needed, exactly as the primary spatial-Prophetic work learned to do.

---

## 11. Decision rule

### If ON is materially faster

Require:

- zero correctness mismatches;
- proof activation is nontrivial;
- exact authored evaluations materially decrease;
- GPU or frame-time measurement improves in a repeatable balanced A/B.

Then:

1. write the benchmark result into #315;
2. state the workload and limits precisely;
3. mark #315 Ready for Review;
4. let Zach choose merge timing relative to Density/Prism.

### If ON reduces evaluations but is neutral/slower

Do not widen proof semantics.

Find the tax.

Likely suspects:

- `rangeCandidate()` per-cell overhead;
- fixed-depth bitmap granularity;
- too many clear/unknown handoffs;
- branch divergence;
- source multiplicity magnifying lookup tax;
- proof buffer locality.

The theorem may be right while the execution artifact is wrong.

### If ON is decisively unprofitable

Keep #297 exact baseline.

Do not merge #315 merely because it is sound.

Close/quarantine the consumer or redesign the direct execution artifact. The CPU theorem and positive-proof bitmap can remain useful to other consumers.

---

## 12. Two later frontiers — not part of the profitability slice

### 12.1 Unresolved transport is not blocked

`sourceVisibility()` has a finite 192-step budget.

Current baseline conservatively returns V=0 if the budget is exhausted.

That prevents invented light leaks, but semantically:

```
unresolved within budget != proof of blocker
```

A later transport-hardening rung should either:

- prove termination for the admitted class;
- carry an explicit unresolved state; or
- use a sound continuation scheme.

Do not smuggle approximation state into V.

### 12.2 Scene-wide cross-pipeline occlusion

The generated shader currently knows only geometry owned by its executing SDF pipeline.

Scene-wide visibility needs a truthful shared transport representation across objects/pipelines.

Do not solve this by pretending local `sdfEval` can see foreign beings.

---

## 13. Things you must not regress

Preserve all of these:

- `rho/chi/alpha` are authored source invariants;
- visibility is derived transport;
- blocker motion is geometry value state;
- V=1 exact compatibility mode;
- per-source visibility before additive source composition;
- signed-penetration receiver self-shadow repair;
- bit-0 / bit-1 admission separation;
- primary distance proof traversal remains quarantined;
- clear proof bit means unknown;
- exact signed marcher owns unknown cells;
- proof grid is derived acceleration permission, not ontology;
- density sovereignty remains independent;
- no production performance claim without measured evidence.

---

## 14. Coordination protocol

Identify yourself distinctly from:

- Density Sun
- Prism Sun
- other Visibility Suns

Before writing production renderer changes:

1. read #315 conversation;
2. inspect its exact head;
3. inspect current canonical;
4. check whether another Sun advanced #315 or opened a profitability child lane.

Prefer bounded targeted reads. No Big Chungus repository ingestion.

If another Sun already owns the performance diagnostic, coordinate instead of duplicating it.

---

## 15. Short form

The state of the world is:

```
Rung 8 exact visibility        MERGED (#297)
        ↓
proof-consuming visibility     IMPLEMENTED + SOUND (#315)
        ↓
profitability                  UNKNOWN — MEASURE NEXT
        ↓
if profitable: review/merge
if not: redesign consumer, preserve theorem
```

The next Sun does not need to invent visibility.

The next Sun needs to discover whether the road we proved through empty space is actually cheaper to travel.
