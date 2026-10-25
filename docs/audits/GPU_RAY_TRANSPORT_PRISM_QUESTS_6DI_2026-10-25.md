# §6di — Prism precision quests: native pipeline facts, traces, routing repair and a common AST consumer

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-25T15:31:27.770627-07:00.

**Human commission:** Zach: “NOW DO THE QUESTS PRISM SUN HAS FOR U.” Starting HEAD`17ef9b4a6`, active independent checkout`/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, branch`claude/ray-domain-transport-rung0`. This follows [Prism's immediate twelve directions](EARTHCALL_NATIVE_GPU_MASTERY_M5_ANCIENT_ARCHIVES_DOSSIER_2026-10-25.md#19-immediate-directions-to-sixth-one-sun), his [semantic-contract review](PRISM_REVIEW_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-25.md), and the [existing council thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Source work remains in standalone probes: no engine/default/save/app change or promotion.

## Quest standing

| Quest | This pass establishes | Remaining |
|---|---|---|
| M5-0 pipeline/device truth | Actual pipeline objects identify Apple M5; execution width32, maximum1024 threads/group, static threadgroup memory0 for Current and both candidate producers. Exact WGSL hashes bind source to native properties; short real workloads captured. | No occupancy implication from group size or static memory. |
| M5-1 Current counter baseline | A short actual resident Current Metal trace is captured and archived. | **Counter-backed resource profile unavailable**: selected toolchain is Command Line Tools; `xctrace` and standalone`metal` are unavailable. |
| M5-2 candidate comparison | Matching actual disjoint and tile-certificate traces captured; both real harnesses pass their existing correctness/reference controls. | Same occupancy/register/spill/cache counters still require a profiler; no causal attribution or clean timing comparison from instrumented runs. |
| M5-3 state census | Exact shader bytes, declared arrays/root/sort structures and per-pipeline metadata are archived. | Declared arrays are not compiler live-register, spill, occupancy or binary-size measurements. |
| Routing contract | GPU-generated indirect dispatch arguments, per-classification resets, actual-family-length tail guards and diagnostic ownership pass executable controls. | Faithful root/transport executor must still be connected; this routing proof retains the explicitly labeled sibling math. |
| Generic math contract | Both expression families actually consume ONE analyzer recipe and OntoMath's existing mathematical range office. | Host mathematical proof only: finite-execution enclosure, GPU recipe lowering and production admission remain separate. |
| Shipping | Every claim above is scoped and replayable. | All three shipping gates OPEN; no new qualified speedup or FPS. |

## Actual pipeline facts — observed, not inferred

The observer is an Objective-C++ **child-process-only diagnostic**. It wraps the native Metal synchronous pipeline factories, forwards the original arguments/options unchanged, and reads the actual returned`MTLComputePipelineState`. The WebGPU call site supplies the exact WGSL source hash/entry point, letting the record distinguish identically named kernels. Role association requires both the observed entry name and exact source hash to match; extra internal/policy pipeline events with stale or absent call-site context are not used as source associations. It neither modifies shader text/options nor adds a setting to Earthcall. Observation state is named Kernel state in source comments.

| Actual pipeline role | WGSL SHA256 prefix | Execution width | Max threads/group | Static threadgroup bytes |
|---|---|---:|---:|---:|
| Frozen resident Current | `3668454316e8` |32|1024|0|
| Guarded producer | `5667ef1630e2` |32|1024|0|
| Disjoint candidate producer | `50a2ea7b8abd` |32|1024|0|
| Tile-certificate + disjoint producer | `7b3138291940` |32|1024|0|

The executed WGSL declares64-thread workgroups. The32,784-query normal dispatch uses513 groups, with48 surplus invocation slots handled by the existing bounds checks. That is execution geometry, **not100% occupancy or proof of a specific limiter**. Private arrays may still use registers or spilled storage even when static threadgroup memory is0. The source state census in the bundle reports declarations only.

The runtime exposes only`timestamp/GPUTimestamp` through its public counter-set enumeration in this process. No counter sampling or WebGPU timestamp feature was requested. `MTLCaptureManager` reports GPU trace documents unsupported by default but supported with`METAL_CAPTURE_ENABLED=1`; enabling that variable only in the diagnostic child successfully captures all three workloads. An initial sandboxed adapter refusal is preserved as an environment result; authorized native runs succeed.

Full profiler availability is a real external dependency, not evidence that the bottleneck is unknowable in principle. [Apple's counter-statistics workflow](https://developer.apple.com/documentation/xcode/analyzing-apple-gpu-performance-using-counter-statistics) measures resource behavior through the Metal debugger; the selected local toolchain currently cannot perform that inspection. Do not substitute guessed counters or the instrumented wall clocks.

## Source-faithful real-harness captures

The §6dh generator's original C++ is retained beside the observed version. The configured old ray build is explicitly reused only after both committed`src` trees match`3603add4018961529a91bc9592d76f9a553500b1` and both working`src` trees are clean. No engine headers or objects were grafted. The observer/capture hooks are separate translation-unit instrumentation; exact compiled C++/observer/WGSL identities and raw qualification reports are in the bundle.

Both captured variants pass the existing independent replay:32,784 native queries,247,326 singles,1,799 overlaps, zero normal fallbacks; guarded packet/output agreement, forced original-native fallback,192 predetermined holdouts at both reference resolutions, exact-budget boundaries and poisoned capacity controls. Native AST assertions are recorded; raw AST node values are not independently recomputed offline. Captures use one frozen inside view at clock7.25. This is a selected-query resident execution contract, not the full Engine frame or moving Person.

Three short traces are retained: **Current**, **disjoint**, **cert**. They represent one complete query batch each, not all timing warmups. Programmatic captures succeeded and their exact files/internal buffer-alias symlinks are archived. Xcode opening/counter inspection has not been observed. Trace-backed observations and clean economics are different lanes; the instrumented timing arrays are explicitly excluded from speedup admission.

## Routing correctness repair — executable results

The new routing generator derives a **correctness-only** witness from the historical S1/S2 prototype. Its sibling8-bin integration math is retained and explicitly labeled. It makes no Current accuracy claim and exports no performance matrix.

- A GPU finalizer reads current classifier counts and writes three indirect dispatch triples.
- Routed kernels guard against their **actual family count**, rather than2304, before loading a queue tile.
- Each cold classification clears append counters in GPU command order, inside EVERY repetition; dispatch uses current indirect arguments without CPU count readback.
- Diagnostic write counters are reset for each repetition. Output is poisoned, and queue tails intentionally contain stale **valid tile0** entries, so invalid tail work cannot hide behind sentinel indices.
- The12 native cases are original input, far-shifted input, restored original input, each at1/2/4/9 repetitions. Counts change`906/707/691 → 0/0/2304 → 906/707/691`. Every one of32,784 rays is written exactly once in every case; outputs are finite and within the declared sibling-only tolerance. Actual argument triples match current counts.
- Two rebuilt native mutations fail for the intended reason: removing the reset produces **counter accumulation**; weakening the tail guard produces **a ray written other than exactly once**. They compile and pass initial pipeline setup before the fatal control assertion. These are executable negative witnesses, not string checks.

The durable generator reconstructs the executed C++ bytes for fixed and both negative variants. Diagnostic atomics/readbacks alter workload cost, so this witness is never a performance comparison. Queue buffers live only in the process; no moving-camera cache or cross-frame proof persistence is admitted.

## Common OntoMath recipe — an actual shared consumer

The former two-field witness had two defects beyond disconnected analysis arrays: its radial AST builder supplied a scalar-zero center instead of the provided vector center, and ignored its noise-scale parameter. A typed clone/load exposed the error. The new authored mathematical fixture uses`VectorConstruct(center)` and`Noise(scale*p)`; it is type-checked. Radial fixtures are placed in the actual admitted query frustum to require non-vacuous tests. These are scratch mathematical expressions authored by Codex under Zach's test commission, not injected world beings or save edits.

Both families call the same analyzer and the same`certifiedZero(recipe, box)` consumer. The recipe owns an authored AST snapshot, recognizes a bounded grammar of typed arithmetic/product/clamp support factors, and calls the **existing OntoMath`evalMathematicalRange`**. There is no geometry-class switch, wave-spec side channel or shell intersection in the classifier. Unknown symbols, unsupported operations, invalid domains and zero divisors refuse; no arbitrary default bound is invented. Noisy clamp clauses are left undecided rather than paying for irrelevant lattice range discovery. A noisy residual can coexist with a non-noisy zero factor.

Across four saved wave expressions and four newly authored radial expressions, sampling every17th captured ray with eight clipped bins: **62,688 query boxes**, **52,381 mathematical zero proofs**, **471,429 authored evaluator point checks**, zero disagreements above the explicit1e-7 CPU corroboration threshold (not native bit identity). All32 negative controls pass: edited expression invalidates the old recipe, unknown expression retains work, unbounded query refuses, and zero-times-undefined refuses. Point samples corroborate but do not replace the existing interval theorem.

This repairs the **shared host analyzer-to-consumer witness**. It does not establish a generic GPU certificate, arbitrary AST admission, or a complete f32 error enclosure. Clock7.25 is an explicit frozen test binding. The mathematical recipe and native GPU execution contract must remain separate until the latter is derived and tested.

## Reproduce and inspect

[Machine record](GPU_RAY_TRANSPORT_PRISM_QUESTS_6DI_2026-10-25.json), [executed offline replay](GPU_RAY_TRANSPORT_PRISM_QUESTS_6DI_2026-10-25_REPLAY.json), [lossless evidence and traces](GPU_RAY_TRANSPORT_PRISM_QUESTS_6DI_2026-10-25.zip). The bundle contains145 source/data/trace documents, deduplicated to about19.3MB. Sources and trace bytes are exact; JSON values reconstruct losslessly with original raw hashes retained, while JSON whitespace is omitted. Internal relative trace symlinks are restored only after their targets are validated. Binaries/object files are not committed.

```sh
python3 -B scratch/probes/ray_transport_gpu_mastery_replay.py --root . --record docs/audits/GPU_RAY_TRANSPORT_PRISM_QUESTS_6DI_2026-10-25.json --output /private/tmp/6di-replay-fresh.json --restore-traces /private/tmp/6di-traces-fresh
```

Fresh facts/capture: `ray_transport_gpu_pipeline_facts.py disjoint` or`cert`, with explicit`--root`, source-matching`--build-root`, fresh external`--work`, pinned`--case` and optional`--capture`. Never reinterpret its clocks as clean economics. Fresh routing build: `ray_transport_gpu_routing_contract.py --root ... --build-root ... --work ... --mutation fixed` (or`no-reset`/`no-tail`); run the resulting probe with frozen zone, rays JSON, and fresh output JSON. Generic probe: build`ray_transport_generic_family_contract.cpp` with`ray_transport_probe_build.py`, then pass the same three input/output paths; it is CPU-only. The build/facts drivers require fresh external work locations and refuse mismatched engine-source provenance. Supply a fresh JSON output path to each resulting executable.

## Remaining precision and shipping work

1. **External tooling:** make the full Xcode GPU profiler available; open the restored Current trace, then inspect the same counters for both candidates. Capture occupancy/target, live registers/spills and cache/memory/ALU influence where the actual tool exposes them. Counter data from isolated replay must stay distinct from connected clean timings.
2. **Semantic executor:** connect the faithful existing root/subpanel/consumer fragments to physically separate residual kernels. The repaired scheduler currently carries sibling semantics; it is not that rebase. Preserve exact panels, shared attenuation, late whole-ray fallback and original guards.
3. **Generic/native contract:** carry the common mathematical recipe through finite-execution error accounting and GPU lowering; retain real active-bin/residual information with explicit source/domain/profile dependencies. Unsupported inputs keep the original path.
4. **Lifetime authority:** any future persistent region is derived from authored revision/clock dependence, domain/camera/depth, coordinate transform, proof margin and device/execution profile. Name writers/invalidation in the Derived-State Ledger; never reuse a stale mask merely because object pointers or buffer bytes survive.
5. **Economics and life:** after correctness and profiler discrimination, require two clean paired complete-cost series including classification/rebuild/upload/dispatch/fallback, then full viewport/depth/composition and moving inhabited p95. Thirty FPS permits33.33ms/frame. No selected-query microbenchmark closes that gate.

**Final standing:** M5-0 facts and exact captures delivered; source/diagnostic repairs above executed. M5-1/2 counter inspection is tooling-blocked. Full exact residual execution, generic GPU admission and shipping remain OPEN. Native timing lane RELEASED at close in all three local Intercom copies. No default, installed-app, save or new visible rendering result to accept; existing Northern Veil Person acceptance stays pending.
