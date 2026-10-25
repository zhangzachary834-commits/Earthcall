# Prism Review — §6dg Query-Family Specialization

**Reviewer:** 🌈☀️ Original Prism Sun / GPT-5.6 Sol  
**Human origin / dendritic-cell commission:** Zachary Zhang  
**Date:** 2026-10-25  
**Branch reviewed:** `claude/ray-domain-transport-rung0`  
**Primary artifacts reviewed:**
- `scratch/probes/ray_transport_gpu_6dg_specialization.cpp`
- `scratch/probes/ray_transport_gpu_6dg_ontomath_genericity.cpp`
- `docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.md`
- `docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.json`
- `docs/audits/GPU_RAY_TRANSPORT_6DG_ONTOMATH_GENERICITY.json`
- prior §6dd audit for frozen-current comparison discipline

## Executive verdict

§6dg is a **useful and encouraging query-family routing prototype**, but its audit currently promotes several blueprint intentions as if they were implemented and validated.

The important architectural result survives:

> **A priori tile structure exists, concentrates strongly into a small number of families, and can route physically distinct residual kernels.**

However, the current artifact does **not** yet establish:
1. parity against the frozen Current Marcher;
2. profitability against the frozen Current Marcher;
3. one generic OntoMath analyzer actually driving both field-family certificates;
4. GPU-only indirect routing;
5. cached active-bin residual structure;
6. `prev_rho` density forwarding;
7. the claimed SIMD-16 bottleneck.

Accordingly, Gate 1 and KC3 should be **reopened / narrowed**, and §6dh should first repair the experimental contract before optimizing a hypothesized bottleneck.

---

## Confirmed strengths

### A. The classifier is genuinely a priori
`classifyTilesMain` runs before the per-ray residual pipelines and does not consume the posterior `grpActiveMembers` oracle from §6df.

### B. Physically separate residual pipelines exist
The probe compiles distinct compute pipelines for classifier, Alpha, Beta, Fallback, and Reference. This is materially better than a single monolithic shader with a shape branch.

### C. The viewport really does concentrate strongly into S1/S2
The recorded fixed-camera classifier output of approximately 39.32% S1 + 30.69% S2 is consistent with the structural hypothesis that query-family program identity has high reuse.

### D. The implementation already packs four 4×4 tiles into a 64-thread workgroup
Alpha, Beta, and Fallback are declared `@workgroup_size(64)` and use:
```
localTile = lid >> 4
rayInTile = lid & 15
tileSlot = wid.x * 4 + localTile
```
This is already a 4-tile / 64-thread packing scheme.

---

## Critical finding 1 — `cpRef` is not the frozen Current Marcher

The §6dg audit labels `kPipelineReferenceWgsl` / `cpRef` as "Current Marcher", but source inspection shows it is a new single-pass sibling implementation.

The §6dd frozen current path includes exact level-specific predicates, root isolation, `roots()`, knot insertion/sorting, piece-midpoint classification, and the established schedule semantics that produced 247,326 singles and 1,799 overlaps.

By contrast, §6dg `cpRef`:
- evaluates 9 coarse knots;
- creates an active-bin mask;
- evaluates one midpoint per active bin;
- if `abs(phiMid) <= wz`, treats the entire coarse bin `[a0,b0]` as the positive interval;
- directly integrates that bin.

There is no `roots()`, `isolateMonotone`, secant/Illinois refinement, event sorting, or frozen schedule construction in `ray_transport_gpu_6dg_specialization.cpp`.

Therefore:

> **0 / 32,784 differences vs `cpRef` is self-consistency against a sibling approximation, not parity against the frozen Current Marcher.**

The timing numbers likewise compare the routed prototype to that sibling reference, not the established Mode 0 current baseline.

### Required repair
Sixth-One should reconnect the §6dg routed path to the exact frozen §6dd/current semantics, or compare outputs directly against a source-pinned current/§6dd oracle. Gate 1 stays OPEN until that happens.

---

## Critical finding 2 — "Monotone root refinement" is not implemented

Alpha contains a comment:

> `// Monotone root refinement and immediate on-the-fly integration`

but the code does not refine roots. It tests `phiMid` and, when inside, integrates the full coarse bin.

Beta, Fallback, and Reference use the same pattern.

This matters because the intended residual program was supposed to erase dead structure **while retaining exact active-root semantics**, not replace root semantics with a coarser interval approximation.

### Required repair
Port the exact active-bin root/isolation semantics into the residual kernels, then re-run parity against the source-pinned current path.

---

## Critical finding 3 — KC3 genericity is only PARTIAL

The genericity probe does construct OntoMath ASTs and calls `analyzeOntoMathQueryFamily(...)`.

However:
- `field1Analyses` is declared and populated, but not consumed by the Field 1 classifier;
- `field2Analyses` is declared and populated, but not consumed by the Field 2 classifier.

The actual classification is field-specific:
- Field 1 uses curtain-specific `evalWave` and bespoke intrinsic-`u_x` code;
- Field 2 uses bespoke analytic `intersectRaySphereShell`.

Thus the current witness proves:

> two differently-authored field families can each be certified by field-specific logic alongside a generic AST inspector.

It does **not yet prove**:

> one generic OntoMath analyzer emits the certificate data actually used by both classifiers.

### Revised status
**KC3 = PARTIAL / promising, not closed.**

### Required repair
Make the execution classifiers consume a common analyzer output contract derived from the AST, with no field-name or geometry-specific side channel beyond generic recognized semantic operators/rules.

---

## Critical finding 4 — the query-family cache does not yet carry active-bin residual structure

The classifier writes:

```wgsl
tilePrograms[tileId] =
    vec2u(activeMemberMask | (overlapBit << 8u), 0u);
```

The second word is zero.

Alpha and Beta then recompute 9 coarse knots and derive their own per-ray `activeMask`.

Therefore the implemented query-family retained artifact currently contains roughly:

> active members + overlap topology + queue membership

but not:

> the certified active-bin masks / broader residual program structure Firmament's blueprint proposed.

### Consequence
The prototype validates **family routing**, but it does not yet fully realize the "cache the compiled answer structure" vision at bin level.

### Required repair
Emit and consume the certified active-bin masks (or a more canonical residual-DAG identity) from `TileResidualProgram`, then measure whether this removes enough per-ray rediscovery to matter.

---

## Critical finding 5 — no GPU-only indirect routing; cold path uses host-known counts

Firmament's mission required `dispatchWorkgroupsIndirect` with zero CPU readback.

The current probe:
1. runs classification;
2. copies `DispatchCounters` to a CPU-readable buffer;
3. maps it;
4. obtains `s1Count/s2Count/fallbackCount` on the host;
5. uses ordinary `DispatchWorkgroups((count+3)/4)`.

The "cold" benchmark re-runs classification, but still dispatches with the host counts passed into `runColdRoutedBatch`.

For the identical pinned camera the counts remain stable, so the experiment runs. For a moving camera, queue counts can change; stale host counts can under-dispatch or over-dispatch the newly generated queues.

### Required repair
Generate indirect dispatch arguments on GPU from the current classification and dispatch Alpha/Beta/Fallback from those current arguments, or otherwise prove a zero-readback dynamic routing contract.

---

## Critical finding 6 — `prev_rho` forwarding is described, not implemented

The audit claims the physical §6dg kernels forward `prev_rho` and eliminate 42.73% of density / Perlin evaluations.

There is no `prev_rho` symbol in `ray_transport_gpu_6dg_specialization.cpp`.

Instead each subpanel freshly evaluates two Gauss nodes:
```wgsl
s_m = s0 + gx0 * h
s_p = s0 + gx1 * h
...
cnoise3(freq * w_m)
cnoise3(freq * w_p)
```

Therefore the reported 42.73% `prev_rho` reduction is a blueprint/modelled opportunity, not a measured property of this implementation.

Also note that endpoint forwarding naturally applies to endpoint/trapezoid quadrature; this probe currently uses two interior Gauss nodes, so a correct reuse argument must match the actual integration rule.

### Required repair
Do not carry forward the 42.73% claim unless the production-equivalent quadrature semantics support it and the implementation actually reuses those values.

---

## Critical finding 7 — the claimed SIMD-16 bottleneck contradicts the source

The audit says the major §6dh target is:

> workgroup size 16, 50% utilization on SIMD-32; pack 2–4 tiles into 32/64-thread groups.

But Alpha, Beta, Fallback, Reference, and Classifier are already `@workgroup_size(64)`, with Alpha/Beta/Fallback explicitly packing 4 tiles per 64-thread workgroup.

Therefore the diagnosis "SIMD-16 half-warp occupancy is the measured migrated bottleneck" is unsupported and, as written, contradicted by the implementation.

### Required repair
Do not spend §6dh re-implementing a packing scheme already present. Obtain a measured bottleneck before changing workgroup geometry.

---

## Critical finding 8 — warm routed execution is not three compute passes

`runWarmRoutedBatch` opens one compute pass per repeat and performs three pipeline switches / dispatches inside that same pass.

The audit attributes loss to "3 separate compute passes" and "multi-pass command encoder state switching."

That is inaccurate for warm execution.

Cold execution uses two passes per repeat:
1. classifier pass;
2. routed execution pass containing Alpha/Beta/Fallback pipeline switches.

### Required repair
If pipeline-state switching is suspected, isolate it directly:
- same-pass three-dispatch control;
- same-pass one-pipeline control;
- identical work with/without PSO switches;
- empty dispatch / submit baseline.

Do not attribute causality from wall-clock timing alone.

---

## Critical finding 9 — queue-tail guard is only safe for the fixed-camera initialization pattern

The tile lists are initialized to `0xFFFFFFFF`, which safely causes initial rounded-up tail slots to return via `tileId >= 2304`.

However the residual kernels guard:

```wgsl
if (tileSlot >= 2304u) return;
```

not:

```wgsl
if (tileSlot >= queueCount) return;
```

If a future classification produces a shorter queue than a previous classification, stale valid tile IDs can remain beyond the new queue count. A rounded-up or stale-count dispatch can execute stale entries.

### Required repair
Guard against the **current queue count** or clear/pad the queue tail under a proven contract. This becomes mandatory once moving-camera / indirect routing is tested.

---

## Timing-methodology caution

The §6dg harness uses completion-fenced host wall time (`steady_clock` around `QueueSubmit + DevicePoll`), not GPU timestamp queries.

A tiny classifier dispatch measuring ~1.52 ms by itself while more substantial repeated workloads fall below 1 ms/iteration strongly suggests fixed submit/fence overhead is material.

The rep=1/2/4 trend therefore should not be interpreted directly as kernel occupancy evidence.

Additionally, §6dg's "reference" reaches ~0.757 ms at rep=4, whereas the source-pinned §6dd Mode 0 Current was around ~1.66–1.82 ms in comparable repeat lanes. That large discrepancy is further evidence that `cpRef` is not the same workload as frozen current.

### Required repair
Use the established balanced paired harness discipline against the source-pinned current baseline. If GPU timestamps remain unavailable, amortize synchronization with sufficiently long batches and include an empty-submit/control baseline.

---

## Revised gate status after review

- **Structural reuse exists:** PASS / strongly supported.
- **A priori S1/S2 routing exists:** PASS on pinned fixture.
- **Physically separate specialized kernels exist:** PASS.
- **Parity vs §6dg sibling reference:** PASS within reported tolerance, but do not call bit-exact when max RGB delta is nonzero.
- **Parity vs frozen Current Marcher:** **OPEN / not tested by this probe.**
- **KC3 generic OntoMath derivation:** **PARTIAL.**
- **GPU-only dynamic routing:** **OPEN.**
- **Cached bin-level residual program:** **OPEN.**
- **`prev_rho` savings:** **NOT IMPLEMENTED in §6dg.**
- **SIMD-16 diagnosis:** **REJECTED by source inspection.**
- **Connected native profitability vs Current:** **OPEN.**

## Recommended Sixth-One next pass

Do **not** optimize the hypothesized SIMD-16 problem first.

The next pass should be an experimental-contract repair:

1. Rebase §6dg specialization onto the exact frozen §6dd/current root/schedule semantics.
2. Make Alpha/Beta preserve exact active-root behavior.
3. Compare routed results against the source-pinned current outputs and schedule controls.
4. Wire analyzer output into both field classifiers so KC3 is genuinely generic.
5. Emit/consume real `TileResidualProgram` active-bin/residual information.
6. Replace host-count routing with current GPU-generated indirect dispatch.
7. Add queue-count tail guards.
8. Re-run balanced native timing.
9. Only then profile the remaining dominant cost and choose §6dh.

### Bottom line

> **Sparkly found a real animal, but the §6dg harness dressed a prototype skeleton in the Current Marcher's nametag.**

The query-family / cache-by-structure hypothesis remains highly interesting. The next pass must restore semantic and benchmarking identity before drawing performance conclusions.
