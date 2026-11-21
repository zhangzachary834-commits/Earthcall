# Northern Veil: selective folded-support traversal retained

Zach asked for the next highest-value item based on the mapped aurora equations, then commissioned implementation before measurement. This continues [the Sixth-one Sun's rendering thread](../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-selective-folded-support-retained) and follows [the equation/overlap model](NORTHERN_VEIL_EQUATION_AND_OVERLAP_MODEL_2026-10-11.md). Zach originated the minimum-necessary-mathematics/fidelity direction; Codex proposed and implemented the generic column index and its measured cost selection.

**Decision:** retain the final selective direct lookup with once-per-ray eligibility. Spawn cost falls **9.77%** in repeated native offscreen measurements; this stage claims no inside improvement, live FPS result or 30 FPS acceptance. Earlier direct/cached variants are rejected.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T13:27:06-07:00

## What changed, and what the proof means

The existing interval grid already proves most empty space. One global envelope loses the folds' local shape, and cell traversal still pays for many empty leaves. `VolumeZeroProof` now indexes every **unproven** cell into conservative rectangular support bands for each possible slice axis, choosing the axis with the least enclosed volume. This selection is generic: no Zone names, curve recognizers, authored constants, denser grid or additional interval evaluations. Northern Veil's four proofs independently choose local X; their 80/77/80/42 bands add 4,464 bytes to the outside combined proof payload at admission.

Each stored band contains all unknown leaf extents in its slab, with the original proof margin plus explicit outward rounding and a roundoff allowance. Exhaustive CPU checks validate enclosure, including unknown clocks/variables and positive density. An early nextafter-only envelope failed two enclosure checks; the final allowance repairs that arithmetic discrepancy.

The outside shader first computes geometric cost eligibility once per medium/ray: use the column when its axis crossing rate is at most half the greatest other-axis cell crossing rate. This heuristic ranks work; it is **not** a density theorem. Only a reached leaf already proven empty may consult its column. Padded ray/band intersection can then skip a longer empty interval, bounded before that slab's exit. Near-parallel and absent proofs retain the established leaf walk. A sample needed by any overlapping medium still prevents a collective jump.

Original authored box events, chord spacing, midpoint indices, channel arithmetic and contributing samples stay unchanged. This accelerates traversal of previously certified zeros; it does not discover new positive-density equivalences or contract unknown cells using new mathematical proofs. Certified contribution tails, analytic integration and adaptive resampling remain unfinished.

The measured expensive inside path retains its original helper signature and traversal; the extra column code is omitted by the existing inside/outside specialization. No new authored switch. Existing exact-zero proof enablement controls this stage; `volumeOutputErrorBudget`, its default zero, the positive full-lattice fallback, `volumeControl.z` and sample-count controls are unchanged.

## Derived-state ownership and invalidation

- CPU band arrays/axis are explicitly Kernel state owned by the existing `_volumeZeroProofs` memo. Density revision or half-extents rebuild them; unknown/unrevisioned input keeps the established fallback. Time alone does no proof/index rebuilding.
- Both lanes still carry the six global support bounds. Only the outside lane appends four f32 bounds per band. Bit 0 of `zeroProofPresent` means present; bits 1..2 encode axis + 1 (zero means the original words/six-bounds payload). Shader and upload accounting consume the same contract; authored parameter order stays fixed.
- Existing resident payload identity includes camera lane, revisions, extents and membership/order. Offset tables rebuild when required; stable bytes stay on the GPU. Reload/device teardown releases existing owning buffers/memos. The index creates no separate resource or authority mechanism.
- Per-ray eligibility is a local GPU value; there is no persistent ray/slice interval cache. Unchanged frames and warmed camera crossings retain existing lowering/pipeline caches.

## Exact verification

App and six named targets build. All six focused targets pass: `volume_zero_proof_test` **32/32**, native `webgpu_volume_zero_proof_test` **65/65**, sharing **55/55** CPU and **65/65** native, unified quadrature **15/15**, plus parameter-refresh checks. Expanded proof tests cover fold crossings in both directions, resized extents before proof-off eviction, replacement native device, warm uploads, unknown/mixed/reordered media, positive-budget full lattice and a full 2560×1440 spawn capture. Existing sharing tests cover separate clocks/domains, revisions, membership/removal and device recreation.

Eight separate 2560×1440 RGBA captures (spawn/inside/reverse/side, t=0 and 7.25) match the frozen renderer byte for byte: **14,745,600 bytes per image**, max error zero. These finite captures supplement the inherited cell proofs and exhaustive index-enclosure invariant; they do not certify every conceivable camera/frame or Person movement acceptance.

## Retention measurements

Comparison baseline: freshly rebuilt HEAD `790277e889b5246dde8fd2e351c00e369c8f89c3`, with prior compiler/residency/time/support stages retained and all six Perlin experiments removed. Concurrent non-rendering commits are preserved. The existing save and original probe source were read-only; save SHA256 remains `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`.

Native Metal Debug offscreen fixture, all four saved media, 2560×1440, t=7.25, same four `EC_CAM` views, 24 timed frames/run. Completion fences bracket each timed batch; no pixel readback occurs during it. Admission prewarms existing camera shader variants; the GPU warm draw/fence precedes measurement. Two sequential ABBA series per view. Initial inside inconsistency (+7.76% then +1.02%) prompted **two predeclared additional inside ABBA series** before retention; every earlier run remains included. The follow-ups are +0.57% and −0.13%, so the >3% regression did not repeat. Initial instability remains visible below, without asserting its cause or an inside gain.

| View | Baseline mean ms | Candidate mean ms | Cost change | Separate series |
|---|---:|---:|---:|---|
| Spawn | 61.35 | 55.36 | -9.77% | -11.71% / -7.86% |
| Inside | 124.81 | 127.67 | +2.29% | +7.76% / +1.02% / +0.57% / -0.13% |
| Reverse | 137.06 | 133.68 | -2.47% | -4.77% / -0.27% |
| Side | 89.09 | 88.85 | -0.27% | -0.03% / -0.50% |

Means include all run means: eight runs/view, sixteen inside, **960 timed frames total**. No floor/minimum comparison. Every timed run reports zero lowering, shader compiles, parameter uploads and proof uploads. CPU preparation is approximately 0.02–0.04 ms/frame; these counters do not isolate GPU sampling or command recording. Ordinary native timestamps remain disabled; volume GPU timing is unavailable. Thermal/load variation is apparent in run means, but its precise cause was not measured.

Spawn exceeds the 5% targeted-cost gate in both series (11.71% and 7.86% reductions). No other view shows repeatable >3% regression after inside follow-up. Reverse/side gains are smaller or inconclusive; inside's pooled **2.29% higher cost** is disclosed, with no speedup claim. These are fixed-view offscreen means, not live surface acquisition times, movement p95 or FPS.

## Rejected implementations

1. **Direct lookup everywhere:** eight captures exact and six focused targets green, but inside +14.20%, reverse +11.86%, side +26.51%, recurring in both series; spawn −12.59% does not redeem those regressions.
2. **Ray-local column interval cache without leaf guard:** six focused targets green, but first full-size spawn capture changes one red byte at (717,711) by two levels. Exact rejection; no performance gate pursued. The precise numerical/code-generation cause is unresolved.
3. **The same cache with the established leaf guard:** all eight images exact; inside +56.06% (+54.48% / +57.58%). Stop after that repeated failed gate; remaining views are unnecessary for rejection. Cache state/control/driver placement are hypotheses, not measured hardware occupancy.
4. **Outside direct lookup with eligibility per leaf:** all eight images exact; spawn only −3.35% (−1.81% / −4.88%). Other results: inside +2.32%, reverse +3.90%, side +0.03%, with no repeatable >3% regression. Below the targeted gain gate, so experiment only.
5. **Retained final version:** outside direct lookup, eligibility hoisted once per ray, original inside signature, no interval cache. Evidence above.

## Artifacts and future-agent handoff

Scratch evidence root: `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/folded-support/`. `baseline-provenance.json` freezes source/save hashes. `lookup-prototype/`, `cached/`, `guarded/`, `selective/`, `hoisted/` retain distinct binaries/captures/logs; the unguarded cache failed its first image and has no timing result. Final evidence is `hoisted/exact-results.json`, `focused-details.log`, `cost-results.json`, `inside-followup-results.json`, `all-final-cost-results.json`, `final-cost-summary.json`, `final-provenance.json`. Scripts use direct subprocess argument lists; no bash wrapper, save write or original-probe edit.

Do not resurrect the rejected interval cache as production, infer certified output bounds from small captures, silently expand this helper into inside shaders, or assume denser grids are a win. Further improvements must target actually demanded mathematics or cheaper traversal and pass fresh exact/retention gates. Follow [the Northern Veil task](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md) and [derived-state ledger](../architecture/law/DERIVED_STATE_LEDGER.md).

Ordinary app rebuilt. Saves, codecs, domain enums, authored equations, native resolution and authority are unchanged. No commits/pushes. Person still needs to assess continuous movement, thin details, flicker/curtain disappearance and usual-size frame-time p95 ≤33.3 ms. The next milestone is not established by these fixed captures/timings.
