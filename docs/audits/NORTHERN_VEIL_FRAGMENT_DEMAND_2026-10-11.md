# Northern Veil: fragment-demand census and rejected empty-transport candidate

Zach commissioned the next testing/implementation pass unless Astra had advised holding it. [Astra’s latest parent review](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#two-bounded-research-moons--findings-and-parent-review) supports a bounded demand census and defers the projected support guard until executed-arithmetic enclosures are sound. Zach originated the minimum necessary mathematics/native fidelity direction; Astra proposed actual demand and the numerical admission boundary; Codex implemented this diagnostic and screened one narrower candidate.

**Decision:** retain a reproducible, read-only diagnostic tool; retain **no production optimization** from this pass. The empty-transport candidate matches eight full-resolution captures but fails economics: inside pooled cost **100.73→105.11 ms (+4.34%)**, with separate series **+8.02% / +1.04%**. The instrumented census changes some RGBA bytes by one level, so its counts describe the instrumented program and must not be promoted to exact uninstrumented-fragment evidence. Existing sample-local scheduling remains production.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T20:53:25-07:00 (PDT).

## Implemented diagnostic and boundaries

[Driver](../../scripts/northern_veil_fragment_demand.py) and [read-only fixture probe](../../scratch/northern_veil_fragment_demand_probe_2026_10_11.cpp) build separately linked baseline/diagnostic binaries against the configured app objects. The driver transforms a **scratch copy** of `WebGpuRenderer.cpp`, adds a fragment-only storage binding and counters to generated combined WGSL, and never edits renderer/compiler sources. This is fixture-specific diagnostic instrumentation, not a Northern Veil rendering rule or an authored property. It requires the four-current-curtain form and refuses missing expected instrumentation hooks.

The saved MsgPack `MigrationRoot` envelope is decoded **once at startup** into the existing fixture representation. JSON report files are diagnostic records, not a save-codec migration or per-frame expression serialization. The original `nv_ref.cpp`/`nv_probe` and save stay read-only. No new channel field, authority path, sample control, cutoff, compiler cache or production resource is introduced.

Select a deterministic 64×36 grid of physical pixel centres (2,304 rays at 2560×1440); private counters reset with each fragment invocation. Count sample visits/collective proof jumps/transport demand, positive-density overlaps, and each medium’s membership/support/proof/density/shared-shape/exact-factor-zero/noise demand. Only selected fragments write their 64-counter record, at completion or before discard. There are **no atomics or per-sample storage writes**. The temporary 589,824-byte buffer is zeroed before each fixture draw, copied/mapped only after rendering, and released after the dump; process termination owns cleanup on diagnostic abort. It is scratch Kernel state, not retained world meaning. Single-medium prewarm programs are not counted.

Native end-to-end execution validates `proof query = proven empty + density demand`, `shared demand = density demand`, `shared demand = factor-zero exits + noise demand`, positive density ≤ noise demand, and `sample visits = all-proven jumps + transport updates`. These equalities are fixture accounting checks, not universal mathematics or pixel-fidelity proofs. Independently repeated portable runs reproduce every raw counter and baseline-image SHA across all eight cases.

Reproduce after `cmake --build build --target earthcall_webgpu -j8` in a desktop GPU session:

```sh
python3 scripts/northern_veil_fragment_demand.py --output /private/tmp/nv-fragment-demand-new-run
```

The output directory must be new and outside the repository. `--width`, `--height`, `--times` and `--camera ex,ey,ez,lx,ly,lz` supply an explicit native view. Default views are the existing spawn/inside/reverse/side fixtures, times 0/7.25, 2560×1440. The tool prints no frame-performance result; instrumented timings are never retention evidence. A no-GPU refusal is a failed diagnostic run, never an assertion pass.

## Native diagnostic counts

All four media, original sample-count/default cutoff/proof controls, native Metal offscreen at 2560×1440. No opaque scene geometry or live surface is present. Counts below aggregate medium evaluations across 2,304 selected rays at t=7.25; all listed demands are identical at t=0. Units differ from sample counts: multiple media can be demanded by one sample.

| View | Proof queries | Unknown density demands | Fold demands | Fold exact-zero exits | Noise demands |
|---|---:|---:|---:|---:|---:|
| Inside | 228,879 | 147,796 | 132,815 | 65,631 | 67,184 |
| Spawn | 66,849 | 35,917 | 30,310 | 14,752 | 15,558 |
| Reverse | 172,447 | 100,405 | 93,428 | 47,149 | 46,279 |
| Side | 135,758 | 69,667 | 58,721 | 29,095 | 29,626 |

Inside has 173,487 loop visits, 40,618 collective proof jumps and 132,869 transport-demand samples. Noise/positive-medium evaluations total 67,184; only 181 samples enter more than one positive medium. Thus at least 65,685 transport-demand samples have no positive medium. The fold-zero fraction is **49.42%** of reached folds; those evaluations already avoid noise. Simultaneous positive-medium overlap is sparse in these views; this weakens another shared direction calculation as the immediate priority. It does not eliminate physical ray-depth overlap or establish hardware cost fractions. About half of reached folds discovering zero reinforces the certificate research direction; the broader CPU table’s 42.21% and principal-domain 21.86% rejection opportunities are different measures, neither admitted GPU skips.

### Instrumentation observer effect

The original evaluator expressions/grouping/sample positions are left textually intact, but additional state/control/storage changes GPU compilation. Every diagnostic image differs from its baseline by at most one level. The cause (fusion, register allocation, backend lowering, etc.) is **not measured**. Even one-level differences mean the diagnostic fails the exact-stage contract; they are not certified by the positive fidelity budget. The counter program is therefore excluded from production and exact original-fragment attribution.

| View | Time | Changed channel bytes | Maximum error |
|---|---:|---:|---:|
| inside | 0 | 19,989 | 1 |
| spawn | 0 | 313 | 1 |
| reverse | 0 | 105,582 | 1 |
| side | 0 | 24,840 | 1 |
| inside | 7.25 | 15,943 | 1 |
| spawn | 7.25 | 245 | 1 |
| reverse | 7.25 | 83,205 | 1 |
| side | 7.25 | 20,229 | 1 |

These are 14,745,600-byte comparisons per case. Repeatability and accounting strengthen the diagnostic’s internal consistency, not universal soundness or exact baseline equivalence. Selected counts cannot be scaled into a trustworthy whole-screen hardware time. No native volume GPU timestamp, surface-acquire diagnosis or movement p95 is claimed.

## Candidate: skip only the current empty transport update

In a scratch compiler copy, initialize one sample-local `sampleContributes` boolean; set it only in the existing positive-density branch. After the existing all-proven jump, if it is still false, continue to the **next original sample**, avoiding a transport update with zero extinction/source. All original density/proof evaluation, summation order and positive-density arithmetic remain unchanged. No future interval is certified or removed; no opacity cutoff is added. This candidate targets current sample overhead independently of the deferred numerical support certificate.

Eight full-resolution noninstrumented candidate captures at all four views and t=0/7.25 match the baseline every byte. The candidate changes neither evaluator fragments nor GPU clocks. This finite witness clears the image gate for measurement, not a universal floating-point theorem.

Two predeclared sequential A–B–B–A series, inside first, 24 readback-free timed frames per run after admission prewarming and a GPU warm fence. Performance uses separate, noninstrumented binaries and no image/census workload concurrently. All **8 runs / 192 frames** remain included:

| Run | Version | Mean ms |
|---|---|---:|
| 1 | A | 87.05 |
| 2 | B | 92.24 |
| 3 | B | 113.43 |
| 4 | A | 103.35 |
| 5 | A | 103.73 |
| 6 | B | 105.49 |
| 7 | B | 109.26 |
| 8 | A | 108.81 |

Series changes +8.01996%/+1.03980%; pooled +4.33811%. The predeclared screen requires ≥5% improvement in both targeted series before spending other-view measurements. It fails, so stop and reject without searching for favorable reruns. No other-view cost result is claimed. Every warm run still reports zero lowering/compiles/parameter/proof uploads; CPU preparation is 0.0176–0.0284 ms. Fewer source-level zero updates did **not** produce a retained speedup; backend causes remain unmeasured. Production is untouched, so no new broad production-suite claim is necessary or made.

## Provenance, artifacts and remaining work

Initial baseline HEAD `3248063a0751d6f3f7eb11337dcaf614cda59fd2`. The shared checkout advanced concurrently to `7df0702273bb50ee08bf976d0bf4cdb4ae3523fd` while this pass ran. Rendering/OntoMath/Northern Veil save paths are unchanged between those revisions; repeated fresh baseline images match every prior SHA. No commits/pushes were made by this session. Unrelated concurrent changes were preserved.

Save SHA256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496` remains unchanged; original probe SHA is unchanged. Scratch root `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/fragment-demand/` contains `provenance.json`, frozen `baseline-SdfWgsl.cpp`, `SdfWgsl-zero-transport.cpp`, `zero-exact-results.json`, `zero-cost-results.json`, `zero-retention-rejection.json`, `census-summary.json`, all per-case captures/counter records/logs and the measured binaries. `portable-run/` contains the end-to-end durable tool’s provenance, per-ray payloads, reports and paired images. `durable-verification/` is build-only; an initial manually invoked smoke image omitted camera/census environment settings and is excluded from reported comparisons.

Zach’s approximately 21 FPS report remains Person evidence at his supplied `(-1.45, 0.3, -1.3)` position. Actual native eye/look, framebuffer, running build and motion interval were requested but unavailable during this pass. Do not convert those raw coordinates into fixture coordinates or claim the census is his view. No live appearance change is expected from the new tool. Person movement/appearance acceptance and the 30 FPS milestone remain open.

Next substantial renderer work still needs a backend-aware support certificate with finite converted inputs, rounded coordinate/bin selection, admitted trig domains/errors, guarded arithmetic/fusion and unknown fallback. Add the uncertainty-margin negative witness before admitting that shortcut. A census at the actual live native view remains useful once its context is known; the instrumentation observer effect must stay explicit or be solved before exact baseline-demand attribution. Do not revive the rejected empty-transport candidate, old Perlin/ray caches, or delete the two distinct membership tests by sight. Preserve Zach’s continuity/sharing/native-resolution intent from [Performance as Rightly Ordered Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md).


**Person clarification — default-size live view:** Zach clarified that the approximately 21 FPS observation used the **default window size**, with **most auroras in view**; he did not track the exact viewing angle. This supplies qualitative window/view context. Physical framebuffer dimensions (including display scaling), native camera eye/look, motion interval and exact running build remain unmeasured; current running status was not answered. Do not substitute 2560×1440 or infer an exact camera orientation from this report.

**Recorded by:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T21:19:44-07:00 (PDT).
