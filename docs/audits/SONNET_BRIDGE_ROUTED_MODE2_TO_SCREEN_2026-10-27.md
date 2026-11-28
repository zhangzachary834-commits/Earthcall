> **Sun integration review (Codex, 2026-10-27):** the CPU probe was independently rebuilt/run with zero failures. P1–P5 remain unimplemented proposals; this merge admits no Screen connection or `OntoMath::VolumeRoutedPlan` API. OntoMath's reusable contract must express general mathematical execution structure rather than acquire renderer/volume meaning. A necessary reset/clear is legitimate measured production work; §6em excluded diagnostic clearing, not all possible correctness-required initialization. Native WGSL/pixel/depth/clock/dirty witnesses remain pending.

# Sonnet Bridge — routed-unfused Mode2 → real Screen rendering (Quest 3): map, prototype, proposal

Claude Code · Claude Sonnet 5.5 (`claude-sonnet-5-5`) · session `b2a4a59b-a34e-406b-9999-98f2292363f1` · 2026-10-27T23:29 PDT · worktree `.claude/worktrees/sonnet-bridge-gpu-ray-406844`, branch `claude/sonnet-bridge-gpu-ray-406844`, base checkpoint `46ebde497`. Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) on Zachary Zhang's instruction. This is a response to [§6em](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md), whose "next bounded bridge" paragraph asks for the smallest qualified transport route connected to the real Screen path, with unsupported cases returning to Current. It also builds on Sparkly's [§6el](GPU_RAY_TRANSPORT_6EL_CLEAN_MULTI_SCALE_ABLATION_2026-10-27.md) harness. Machine record of the executed probe: [JSON](SONNET_BRIDGE_ROUTED_MODE2_TO_SCREEN_2026-10-27.json). Probe: [sonnet_bridge_adapter_probe.cpp](../../scratch/probes/sonnet_bridge_adapter_probe.cpp).

**No GPU device was created, no frame rendered, no timing taken, no engine source, save, default, lighting or resolution touched.** Only `scratch/probes/sonnet_bridge_*` and this record were added.

## Verdict in five lines

1. The production consumer is a **single V5 fused fragment shader per pixel**, drawn from a union-bounds proxy, blended `One / OneMinusSrcAlpha`. It already contains the **whole-ray Current path**, the **native viewport ray**, the **authoritative opaque-depth clip** and the **ordered shared (T,L)** accumulation. The smallest bridge therefore leaves all of that in place and adds *one structural lane*: before the lattice loop, read a per-pixel **routed publication** `vec4(L, 1−T)`; if published return it unchanged, if refused (`alpha < 0`) fall through to Current. Measured fact, not a plan: all six patch anchors are **unique** in the real `composeVolumeSet` output (text patch only; not compiled, see Limits).
2. Mode2's kernels are **not liftable as they stand**. They are a Northern-Veil-structured family (member masks {0} and {0,1}, positional reads of `collectVolumeParams(density)` slots, a fixed tent×clamp×wave envelope) running as *compute over a ray list* with fixed capacities (2,304 tiles, 2,305 offsets, 32,784 rays). I could not find, and did not invent, a generic kernel. The missing interface is OntoMath-owned schedule derivation on device (proposal P4).
3. What **is** connectable today with existing APIs and is now connected in the prototype: production bindings → real lowering → program contract; production bindings → transport coefficients (α, colour at the Person's clock) through the **same OntoMath office** the harness uses; native pixel → ray/clip/tile records with the fs's own geometry and opaque-depth rule.
4. The `P` buffer the production fs reads is **not** the buffer Mode2's kernels index. The harness reads the 39-float density-only block; production `P` holds a 64/64/63/64-float all-channel segment per member. They share **no common prefix and no contiguous subsequence**. A routed lane needs its own kernel-derived payload (or an explicit remap), never raw reuse of `P` offsets.
5. Nothing here is a profitability claim. Mode2's 18.84% / 12.94% saving remains a native synthetic result over 32,784 replicated sparse rays; connected-frame cost is untested.

## Executed evidence vs. plans

| Claim | Status | Evidence |
|---|---|---|
| Probe compiles and links against source-matched engine (`HEAD:src` = `3603add4…` in both trees) with no GPU init | **Executed** | `ray_transport_probe_build.py` recipe; probe run exit 0, `failures: 0` |
| Real lowering of the 4 Northern Veil media: `lowerVolumeEvaluator`×4 → `composeVolumeSet(trim=true)` ok; 69,374 B WGSL (fnv64 `c5fedd23291a42f0`), 255 param floats, 0 kernel time slots, bindings `u,P,sceneDepthTex` (group 0), `instances` (group 1) | **Executed** | JSON §A |
| Per-member evaluators `volume{Density,Extinction,Emission}Eval_n` all present; none of the six channel evaluators reads `u.volumeTime` for this fixture | **Executed** | JSON §A/§B |
| Density channel is `algebraic`-admitted, not polynomial-admitted (`no exact line restriction for Clamp`); emission not line-admitted | **Executed** | JSON §A `ray_channel_forms` |
| α and colour re-derived from the production bindings via `OntoMath::factorCommonScalar` + `evaluateFactorCoefficients` at clock 7.25 match the harness's reference tables (|Δα| ≤ 7e-9; colours equal to 6 digits) for all 4 members | **Executed** | JSON §G |
| Harness `noiseFreq = {0.08,0.09,0.07,0.10}[field order]` equals authored all-channel slot 35 for all 4 members | **Executed** | JSON §B |
| Scratch density-only block ⊄ production set `P`: offset −1 and common prefix 0 for all 4 members | **Executed** | JSON §B |
| Synthetic 256×144 frame: 36,864 rays, direction unit error ≤1.8e-7, 26,458 opaque-clipped rays equal the analytic plane distance within 1.7e-4 relative (f32 NDC depth), every ray in exactly one 4×4 tile, ≤16 rays/tile | **Executed** | JSON §C |
| Premultiplied blend identity `L + T·scene` (15 cases, max err 1.8e-8); fs-discard vs write delta ≤ 8.1e-6 | **Executed** | JSON §D |
| fs patch anchors unique (6/6) in real program text | **Executed** | JSON §E |
| Recorded 32,784 native rays (`/private/tmp/earthcall-wide-case/rays.json`): every ray crosses ≥2 media boxes and **every ray has overlapping box chords**; 0 box-entry-order inversions | **Executed** | JSON §F |
| The patched WGSL compiles; the routed publication reproduces Current pixels; any speedup | **NOT done / proposed** | no GPU, no WGSL validator available offline (`naga`, `tint` absent) |

Reproduce (CPU only; builds under a fresh scratch directory, links but never launches the app):

```bash
python3 -B scratch/probes/ray_transport_probe_build.py --root "$PWD" --build-root /Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport --source scratch/probes/sonnet_bridge_adapter_probe.cpp --work /path/to/fresh/bridge_build
/path/to/fresh/bridge_build/probe "saves/zones/Northern Veil/zone.ecform" /private/tmp/earthcall-wide-case/rays.json out.json
```

## Call / data-flow map

### Production path (what Screen does today; line numbers at `46ebde497`)

| Stage | Entry point | Data and lifetime |
|---|---|---|
| Authored → binding | `Rendering::readVolumeDensity(FieldNode, clock, dt, VolumeDensityBinding&)` ([VolumeDensity.hpp:126](../../src/Singularity/Screen/VolumeDensity.hpp)) | Pointers to six authored `Piecewise` channels + per-channel revisions; `temporalCoordinate` is the **medium's own clock**. Lifetime: per frame, revision-keyed. |
| Ray-domain forms | `Rendering::VolumeRayFormCache::formsFor` / `lowerVolumeRayChannels` ([VolumeRayTransport.hpp:87–112](../../src/Singularity/Screen/VolumeRayTransport.hpp)) | OntoMath `LineRestriction` per channel. **Authored-revision lifetime**; camera, ray, extent and clock are *not* dependencies. Today **only tests, scratch and certificate code consume it** — `WebGpuRenderer` includes the header but never calls the planner/scheduler. |
| Evaluator lowering | `sdfwgsl::lowerVolumeEvaluator` / `repairVolumeEvaluator` (called from `flushVolumeComposite`, [WebGpuRenderer.cpp:3277](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)) | Kernel-derived per-medium WGSL fragments + param slots. Lifetime: structure-keyed, reused across frames. |
| Program composition | `sdfwgsl::composeVolumeSet(inputs, evaluators, trim, certs, switches)` ([SdfWgsl.hpp:447](../../src/Singularity/Screen/WebGPU/SdfWgsl.hpp)); cached in `_volumeSetProgramCache` | One program: header record (union bounds) + N medium records at `instances[1..N]`. `P.v` = concatenated per-member param segments, then zero-proof / certificate payloads. |
| Per-frame payload | `flushVolumeComposite` ([:2987](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)): `VolumeInstanceData{origin, halfExtent, time, paramOffset, zeroProof*, supportCert*}`; optional `volumeTimePrepare` compute (`kernelVolumePreparationWgsl`) | Clocks/camera stream; equations stay on the FieldNodes. Preparation is a compute pass **inside the same encoder**, ordered before the render pass ([:3860–3890](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)). |
| Draw | `wgpuRenderPassEncoderDraw(36, drawInstanceCount=1)` ([:3907–3930](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)) over the union proxy; **no depth attachment**, opaque depth read as `texture_depth_2d` (binding 2) | `_frameColorView` is `Load`ed; the ordinary pass reopens afterwards with colour+depth `Load` ([:3946+](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)). |
| Per-pixel consumer | V5 `fs` ([SdfWgsl.cpp:6173–6545](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp)) | Unprojects the **far plane** through `u.invViewProj` for the native ray; union-box chord; **opaque clip** `t1=min(t1,dot(worldAtDepth−ro, rd))` ([:6205–6211](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp)); sorts box events; per segment `stepLength` from `samplesPerChord`; **one shared** `totalExtinction`/`totalSource` per sample; `T*=exp(−Στ·Δ)`, `L += S·(Told−T)/Στ` ([:6511–6520](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp)). |
| Composition | return `vec4(L, 1−T)`; blend `One / OneMinusSrcAlpha` ([:1377–1392](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)) | `C_out = L + T·C_scene`. Discards when `alpha ≤ 1e-5 && max|L| ≤ 1e-6`. |

### Scratch Mode2 path (6el/6em harness; generated source regenerated and inspected on the CPU)

`zone.ecform → FieldNode::fromJson → readVolumeDensity(*field, 0, 0, …)` → per medium: `single()` MathNodes → `deriveOriginalCoefficients` (`factorCommonScalar`) + `prepareOriginalCoefficients(clock)` → `GpuMedium{color_alpha, freq_offset}`; `collectVolumeParams(densityExpr)` appended to a private `globalParamPool`; `GpuCurtainSpec` from **positional** `v[2], v[5], v[6], v[15], v[16..24]` (guard is only `values.size() >= 34`); recorded rays (`rays.json`: pixel, direction, clip a/b) → 48 B `GpuRay{ro,t0 | rd,t1 | counts}` → 16 rays/tile in 2,304 tiles → passes `cpReset → cpClassify → cpArgs → cpS1Unfused → cpS2 → cpFallback → cpUnfused → fallbackCP` → `results[rayIdx] = vec4(L, T)` in **ray-list order**, read back by the host.

### Where the two flows differ (the actual bridge gap)

| Aspect | Production fs | Mode2 harness | Consequence |
|---|---|---|---|
| Stage | render pipeline, per pixel | compute over ray list | publication must bridge buffer → fragment |
| Ray source | derived in-shader from `invViewProj`, `u.viewport` | recorded sparse rays (`rays.json`) | need a device ray-setup pass, or an exact host mirror (done as oracle) |
| Clip | union box ∩ opaque depth | pre-baked `a`,`b` | opaque clip must be *the fs's* — mirrored and checked (§C) |
| Params | `P` all-channel segments | private density-only pool | different layout; no shared prefix (§B) |
| Clock | `u.volumeTime` per medium instance; NV evaluators read none | `EC_POSE_CLOCK` → coefficient prep only; `readVolumeDensity(…,0,0)` | clock plumbing must follow `VolumeInstanceData.time`; **Northern Veil cannot witness clocks** |
| Capacity | dynamic | 2,304 tiles / 32,784 rays hard-coded; native 2560×1440 needs 230,400 tiles, 3,686,400 rays, 176.9 MB of 48 B rays | capacities must be derived from viewport, with a memory bound (§6em gate 1) |
| Membership | any N, shared sum | S1 = member 0 only; S2 = members {0,1} with `!tileCanOverlap && exit[0] <= enter[1]`; else fallback | safe (falls back) but profitability is fixture-ordered; the order guard exists in the 6el classifier |
| Live literal | none | `noiseFreq = {0.08,0.09,0.07,0.10}[field order]` | equals authored slot 35 (§B) — derivable, but by position |

## What the prototype actually connects

[sonnet_bridge_adapter_probe.cpp](../../scratch/probes/sonnet_bridge_adapter_probe.cpp), built and run on the CPU:

* **A — production contract (real APIs).** Lowers the four media through `readVolumeDensity → lowerVolumeEvaluator → composeVolumeSet` and extracts bindings, evaluator presence, param count and line-form admission. This is the producer side of the bridge.
* **G — coefficients (real OntoMath office).** α and colour from the *production* bindings, at the Person's clock, with `factorCommonScalar`; identical to what the harness uploads. A routed lane may take its transport coefficients from here and never bake them; non-literal or non-factorable channels refuse → Current.
* **C — ray/clip/tile adapter (verification oracle).** `nativeRay()` is the fs ray setup and opaque-depth clip written once in C++; `buildTiles()` emits the scratch tile contract (≤16 rays/tile) for a dense frame. It is a **mirror used as an oracle**, deliberately not proposed as the production path (see P3).
* **D/E — publication protocol and the fs patch.** Models blend identity and the refusal sentinel; applies the proposed fs hunk to the *real* program text, verifying all anchors are unique.

**Not connected:** the Mode2 kernels themselves. There is no executable path from production bindings into `cpClassify/S1/S2` that does not go through Northern-Veil positional slots, and I did not write one.

## Proposed minimal patch (not applied; each with witnesses)

**P1 — fs routed-publication lane (structural, like `trimProofSupport`).** In `composeVolumeSet`, behind a bool, add `@group(0) @binding(3) var<storage, read> routedPublication: array<vec4<f32>>;` and, immediately before `var transmittance = 1.0;` (anchor unique):

```wgsl
{
    let routed = routedPublication[u32(px.y) * u32(u.viewport.x) + u32(px.x)];
    if (routed.w >= 0.0) {
        if (routed.w <= 1e-5 && max(max(abs(routed.x), abs(routed.y)), abs(routed.z)) <= 1e-6) { discard; }
        return routed;
    }
}
```

**Order trap, found by executing the model:** the refusal sentinel `(0,0,0,−1)` *satisfies* the fs discard rule, so the refusal test **must come first** or every refused pixel is silently discarded instead of rendered by Current. Witness W1: an all-refused buffer must be pixel-identical to the unpatched fs.

**P2 — layout and lifetime.** One more group-0 entry in `volumePipeline`'s `globalBgl` (the lane is a separate cached pipeline, so the existing lane's layout and pixels are untouched); buffer sized `W×H×16 B` (2560×1440 → 59 MB; bound it, refuse above the bound → Current). Publication **completeness by construction**: the routed passes write *every* pixel of the proxy's screen rectangle (published value or sentinel), so no per-frame `ClearBuffer` is needed (§6em's clean timing forbids clears in the timed region). Witness W2: a poisoned buffer outside the written rectangle is never read.

**P3 — device ray setup instead of a host ray list.** A compute pass reading `sceneDepthTex` and `u` that runs the fs's own unprojection/clip and writes the 48 B ray record (or feeds tiles directly). This is the only option that preserves *authoritative* opaque depth with no readback. The C++ `nativeRay()` is its oracle. Witness W3: device rays equal `nativeRay` bitwise on a fixed depth/camera, including `sceneDepth ≥ 0.999999` and `t1 ≤ t0` discard cases.

**P4 — the missing interface (the real blocker).** A generic routed generator is not expressible with today's APIs without renderer-private meaning:

```cpp
// proposed, SdfWgsl.hpp — Kernel derived data only; no domain class, no enum, no knob
Program composeRoutedVolumeSet(const std::vector<VolumeProgramInput>& media,
                               const std::vector<KernelVolumeEvaluator>& evaluators,
                               const OntoMath::VolumeRoutedPlan& plan,   // OntoMath-owned
                               const RoutedLaneLimits& limits);          // memory/tile capacity from viewport
```

`OntoMath::VolumeRoutedPlan` must come from OntoMath (extending `restrictAlgebraicToLine`/`segmentLine`/`factorCommonScalar`, which already exist) and state: which switch/bin boundaries partition a ray, which pieces are certified empty/constant/needing lattice, the quadrature the *accepted reference* admits, and the structure identity that gates the payload. The kernel then calls the production `volumeDensityEval_n` / `volumeExtinctionEval_n` / `volumeEmissionEval_n` it already emitted, so authored meaning stays in the evaluators. Until this exists, Mode2's positional kernels may only run as a **fixture-gated experiment** keyed by an exact `inspectDensityExpression(...).structure` hash, refusing everything else.

**P5 — fallback and publication discipline (carry Sparkly's §6em rules to the connection).** Whole-ray granularity only: a ray is either fully published by a routed path or refused whole (Current). Preflight poison, exactly-once publication and zero partial writes are the same checks as 6em; add W4: refused pixels equal Current bitwise.

**Witness list before any promotion.** W1 all-refused ≡ Current; W2 poison outside rectangle; W3 device rays ≡ oracle; W4 refused ≡ Current bitwise; W5 published pixels vs Current under 6em's accuracy statistics on **distinct dense recorded rays** (not replicated sparse); W6 composition: routed result over an opaque scene with depth clip, vs Current, including `discard` threshold pixels; W7 clock: change a medium's `temporalCoordinate` and a time-only coefficient, republish — requires a fixture whose evaluators read the clock (Northern Veil's do not); W8 edit/revision: bump one channel revision, payload and structure hash refresh, stale payload refused; W9 overlap: tiles where member supports interleave must refuse to Current.

## Limitations and open items

* The patched WGSL (`fnv64 2c34866b192cfd91`) is **not validated**: no GPU permitted, no offline WGSL validator present. Treat P1 as a design with verified anchors, not shader code that compiles.
* The ray/clip/tile adapter was verified on a **synthetic** perspective camera and plane; the recorded-ray file was used only for the box-overlap statistic. It has not been compared with a real `sceneDepthTex` readback.
* My first scan of Mode2 read the older 6dg kernels (which do carry per-curtain literal tables). The executed 6el/6em lineage derives α/colour through OntoMath; I corrected the claim after regenerating the 6el source. I did not diff the other mode variants; the positional-slot and member-mask conclusions come from the regenerated 6el text (lines near the classifier and `extractCurtainSpecs`).
* Overlap: all recorded rays have overlapping **box** chords. Whether *supports* interleave on any ray is a classifier question the 6el classifier answers per tile; I did not re-prove it.
* `all_channel_values`/offsets describe the Northern Veil fixture at this checkpoint only.
* No Person-visible change; nothing added to the Person Verification List.

## Next native invocation (for the Sun to sequence — I ran none)

1. Do **not** run Mode2 again for this bridge. The next GPU-useful step is the cheapest one: implement P1+P2 behind a scratch-only flag and run **W1 and W4 only** (all-refused ≡ Current pixels, offscreen, `webgpu_*` lane): it exercises the entire connection with zero routed math and zero speed claim.
2. Only then P3 (W3 on device), then the fixture-gated Mode2 publication for W5/W6 on dense distinct rays with a proven memory bound.
3. P4 is design work (OntoMath) that can proceed on the CPU in parallel; it is the real distance to shipping gate 2.

## Intent trace

Direction came from **Zach/the Sun**: faithful authored rendering, Mode2 routed-unfused as the bridge candidate, native viewport rays, authoritative opaque clips, ordered shared (T,L), medium clocks/revisions, whole-ray Current fallback, mathematical meaning in OntoMath and Screen only binding/executing (§6em, CLAUDE.md refusals 1, 3, 6, 7). **Sparkly** supplied the Mode2 harness and its publication discipline. **Mine:** the call/data-flow map, the buffer-layout finding, the refusal-sentinel order trap, the structural-lane form of P1, the coefficient derivation from production bindings, and the P4 interface sketch — the last is a proposal for the Sun and Zach to accept or redirect.

*Claude Code / Claude Sonnet 5.5 / session `b2a4a59b-a34e-406b-9999-98f2292363f1` / 2026-10-27T23:29 PDT.*
