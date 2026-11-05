# Cathedral surface SDF first pass

**Author** — Sixth-One Sun · Codex / GPT-6  
**Session** — `01a148a7-bd85-7550-a416-0e859a729c1a`  
**Timestamp** — 2026-11-03T21:18:00-08:00  
**Source head** — `d82a816163e48be525f1afbd781f2f3937a866b4`

Zach asked to optimize Cathedral of the Living Logos after the Northern Veil merge, then restricted this pass to **source and CPU work only**. This pass establishes compiler inputs and prepares a compiled diagnostic harness. Native shader validation, timing, and inhabited-view acceptance await a later GPU pass.

## Scene and caller correction

The canonical Cathedral `zone.ecform` and the data-home copy had identical SHA-256 hashes during inspection. The scene contains 1,231 Objects, including 178 Fields, 87 spheres, 939 cubes, 15 tori, and 12 interface/text Objects. The tori select mesh rendering. There are 153 Materials.

The spatial root contains a 116-node expression and carries `light.source=true`. `EngineRender.cpp` projects it as authored source radiance. The Object loop draws surfaces. **The initial suggestion that this root expression was itself repeatedly marched as enclosing geometry was incorrect.** Its lighting cost and the surface fields' traversal cost require separate measurements.

## Executed CPU checks

The correct-checkout `build_release` app target built successfully. The census links its production objects, reads the saved Field trees through `geom::sdfFromJson`, supplies authored color/response and root radiance expressions, and compares `sdfwgsl::compile` parameters against `collectParams`.

| Reading | Result |
|---|---|
| Fields compiled without a compiler refusal | 178 / 178 |
| Distinct generated shader structures | 66 |
| Fields requiring gradient-corrected marching | 36 |
| Fields emitting `sdfEvalGrad` | 0 |
| Compile-versus-collector parameter mismatches | 0 |
| GPU execution | Deferred by Zach |

The first compile/collect loop took approximately 24 ms. This independently recompiles every field and measures cold compiler work. The warmed renderer memoizes programs, so the reading cannot establish frame cost. Source/binary hashes, commands, and input preservation are retained. [CPU census](data/cathedral_surface_cpu_2026-11-03.json), [CPU receipt](data/cathedral_surface_cpu_receipt_2026-11-03.json)

## Bounded candidates

The primary marcher permits up to 192 iterations. Its generic gradient-corrected step evaluates the field at the sample and three offset points. The fallback normal evaluates four offset samples. Actual step counts and screen coverage remain unmeasured for Zach's slow view.

The 36 gradient-stepped fields include the gyroid, Ophanim throne, and Open Hand leaves, portal tears, drops, seed, and touchstones. Several drops use `sqrt(x*x+y*y+z*z)-1`. Square roots exceed the current analytic-gradient recognizer. Composite fields also need their own branch and blend rules.

The first derivative candidate must preserve authored value arithmetic and grouping, reuse mathematical intermediates, and handle zero-length gradients, undefined derivatives, clamp seams, and branch ties with explicit fallbacks. A broader differential representation belongs with OntoMath, with Screen lowering its executable result. Native intersections and normals must qualify any change.

`geom::Quadric::raycast` and `raycastSmooth` already provide CPU smooth-quadric intersections. Their existing mathematics offers a separate future GPU direct-hit candidate for qualifying primitives. Source availability is established here.

## Material diagnostic

Loading `material.logos.bounds.quantized` emits scalar-Map type diagnostics. The affected surface is `cathedral.bounds.quantized_ziggurat`. Its failing material type check is retained in the census, while production shader generation currently reports success. Resolve its typing/lowering contract during baseline validation. The authored Material is preserved.

## Reproduction and next native screen

Run from the repository root with a fresh external output directory.

```sh
cmake --build build_release --target earthcall_webgpu -j8
python3 scratch/probes/cathedral_surface_probe.py --work /private/tmp/cathedral-cpu-fresh
```

The executable also has an explicit `--render image.rgba` mode. This mode compiled and remains unexecuted under the CPU-only boundary. It prepares the 178 saved Fields at 2560×1440, FOV 45 and far plane 100, with saved transforms, bounds, material expressions, and root radiance. Parsing occurs before timing. Four complete frame samples follow three warmups.

That screen isolates the Field subset. Meshes, smooth-model spheres, paint textures, Person, tools, and Laws require the full-application comparison. Its preset camera remains a diagnostic proposal. Capture Zach's actual slow view before qualifying live performance, and use paired ABBA runs for retention decisions.

Next is baseline shader validation, fixed-view separation of surface traversal and lighting, one bounded candidate, matched complete cost, and an inhabited check. Charge spatial-proof lookup costs against actual work saved. [Task and gates](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Cathedral_Surface_Frame_Cost/Cathedral_Surface_Frame_Cost.md)

Intercom navigation currently refuses an incomplete connection-journal tail. This pass leaves the journal for its active owner.

## Native continuation and retained upload repair

**Codex / GPT-6 / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-11-03T22:52:00-08:00.** Zach closed Chrome and released the CPU-only boundary. Process inspection also found Earthcall closed. The Field subset rendered successfully on Metal in approximately 8.5–11.2 ms from the prepared nave view.

Extending the witness through normal `Object::drawObject` calls for all 1,231 saved Objects exposed a larger submission cost. The full saved-object screen initially took 71–84 ms, including 41–44 ms on CPU before queue completion. A warmed native stack sample attributes much of that CPU time to `flushMeshDraws` → `GpuBufferPool::suballocateFrom` → `wgpuQueueWriteBuffer` → native Metal staging-buffer creation. The scene emits 5,115 mesh draws and 67 SDF draws, with 10,365 buffer suballocations per frame. Per-object gathering itself accounts for roughly 7 ms.

The retained bounded repair packs mesh uniforms and instance data into aligned, flush-local upload pages. Combined pages are limited to 256 KiB. A single oversized batch retains its existing allocation size. Binding offsets point into those pages, while each binding retains its original byte extent. Batch iteration and shaders retain their existing definitions. The change adds no persistent cache or cross-frame invalidation office.

Four complete-cost ABBA blocks, with four warm frames per process observation, give the following saved-object result at 2560×1440.

| Reading | Original | Upload pages |
|---|---:|---:|
| Complete frame | 62.112 ms | 30.042 ms |
| CPU encode/submit | 39.592 ms | 15.654 ms |
| Completion wait | 22.520 ms | 14.389 ms |
| Buffer suballocations | 10,365 | 147 |
| Uploaded buffer bytes | 1,463,480 | 2,771,288 |
| Mesh / SDF draws | 5,115 / 67 | 5,115 / 67 |

Complete cost falls **51.63%**, equivalent to roughly 16.1 → 33.3 FPS in this offscreen saved-object screen. Alignment padding increases uploaded bytes. Fewer staging allocations and copy operations produce the measured gain. Person, tools, Law execution, and the actual inhabited slow camera remain outside this screen.

### Correctness and the image-order control

The mesh WGSL is byte-identical, SHA-256 `fb9e24da95904aad0157af38f941fa7cbaec8a8c32df392447d2538f9782f975`. An independent native pixel oracle passes **10,371 checks**, covering multiple pages, material/texture edits, moved instances, multiple flushes, and an oversized batch. The original renderer passes the same oracle. Intentionally dropping each record's uniform offset produces **5,184 failures**.

An initial normal-process comparison differed at two pixels, six bytes. Later ABBA captures also show occasional differences in the original renderer, including a 48-byte baseline outlier and a 66-byte candidate outlier. These observations are retained. Source inspection and binary command traces establish that the existing map orders batches by pointer addresses. Traces from both renderers contain identical multisets of all 5,115 mesh command inputs while their sequence changes between processes.

An accuracy-only comparator freezes logical mesh/texture order for both original and candidate source variants. They match **all 14,745,600 RGBA bytes**. Its sorting is confined to diagnostic copies. The production patch changes upload packing. Natural process-dependent ordering variation remains an existing renderer limitation, and no pixel tolerance was widened to accept it.

The existing `gpu_mastery_test` exposed a stale assertion that required a resized buffer's address to differ from its released predecessor. The driver may recycle that address. The witness now checks actual buffer size and readback vertex bytes, consistent with the test's existing resource-identity doctrine.

Full raw captures, source variants, and logs remain under `/private/tmp/ec-cathedral-upload-pages-20261103`. The compact metric record retains every ABBA observation and comparison. The replay archive holds source variants, build commands, the warmed stack sample, and negative controls. [Native review data](data/cathedral_mesh_upload_review_2026-11-03.json), [replay sources](data/cathedral_mesh_upload_sources_2026-11-03.tar.gz)

The rebuilt application is `build_release/earthcall_webgpu` in this checkout. A live Cathedral walk remains Zach's acceptance step. The scalar-Map typing diagnostic remains a separate follow-up. Surface derivative work can be prioritized after the inhabited checkpoint, using the measured Field cost rather than the initial root-field hypothesis.

### Final regression scope

The two focused native tests pass with actual Metal access. `webgpu_mesh_upload_pages_test` passes 10,371 pixel checks and `gpu_mastery_test` passes 40/40 checks. A sandbox-only attempt could not acquire a device and was superseded by the device-backed run.

The additional `webgpu_volume_unified_quadrature_test` reports 11/15 checks on both the original and candidate renderer, with identical maximum errors and failing-pixel counts for the current Veil fixture. This existing reference discrepancy remains a separate follow-up. No thresholds or medium samples were adjusted. [Before/after volume control](data/cathedral_mesh_volume_baseline_control_2026-11-03.json)

### Person checkpoint and launcher correction — 2026-11-04

Zach first reported 17 FPS from `/Applications/Earthcall.app`. Inspection found installed version 1.0/build 3522, recorded source commit `fbfcf69ec`, with executable modification time 2026-11-02T23:16:23-08:00, predating this repair. After launching the rebuilt checkout's Release executable and disabling expensive Laws, Zach reported **38 FPS**, then judged the renderer "way better than it used to be." This is Person-reported live performance, with no captured camera/extent or matched live baseline. Zach explicitly keeps Law optimization separate from this renderer pass. Detailed appearance and pacing checks remain open.

At Zach's request, `Run Earthcall.command` now exports a Release default before calling the build helper, overriding a cached Debug profile. Explicit `EARTHCALL_BUILD_TYPE=Debug` remains available. Shell syntax checks pass; an isolated launcher invocation with a stubbed build helper verifies both default Release and explicit Debug forwarding without building or launching Earthcall. The standalone build helper's cached-profile behavior is unchanged. The old empty, unheld `.git/index.lock` was removed after checking for active Git writers; the unrelated Cursor build lock was retained.

Codex / GPT-6 / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-11-04T00:35:39-08:00.

## SDF Surface Truth Mastery: Rung 1 & Rung 2 Implementation and Native Offscreen Validation

**Author** — Antigravity / Gemini 3.8 Flash  
**Session** — `c802a0d5-3c06-450d-b47b-ebdc7aef86d0`  
**Timestamp** — 2026-11-04T02:26:00-08:00  
**Source head** — `sync-from-earthcall-main`

Following the directive from The Firmament & Zach, Rungs 1 & 2 of SDF Surface Truth Mastery have been implemented and verified on the default checkout without touching `Earthcall-ray-transport` or introducing any heuristic approximations, spatial octrees, or damping hacks.

### 1. Structural Advancements in SdfWgsl

1. **Step A — `ScalarForm` & Algebraic Forward-Mode `JetExpr` Differentiation:**
   - Extended `isDifferentiableAst` and `emitMathNodeGrad` in `src/Singularity/Screen/WebGPU/SdfWgsl.cpp` to differentiate general polynomial terms ($x^a y^b z^c$) and transcendental factors (`Sin`, `Cos`, `Exp`, `Ln`) using the exact Leibniz product rule while preserving strict parameter collection ordering.
   - Added forward-mode differentiation rules for `Op::Sqrt` ($\nabla \sqrt{u} = \frac{\nabla u}{2\max(\sqrt{u}, 10^{-6})}$), `Op::Abs` ($\nabla |u| = \text{sign}(u)\nabla u$), `Op::Pow` ($n u^{n-1} \nabla u$ for non-spatial exponents), and `Op::Div` ($\frac{\nabla u}{v}$ for non-spatial denominators).
   - Added `Op::Clamp` ($\nabla \text{clamp}(u, lo, hi)$), `Op::Union`, `Op::Intersection`, and `Op::Difference` to `MathNode` gradient emission.
   - Added token-level differentiation (`isDifferentiableRpn`, `emitRpnGrad`) so that expressions serialized as RPN tokens (such as `cathedral.sdf.gyroid_north`, `cathedral.sdf.ophanim_throne`, and `cathedral.astra.openhand.*`) differentiate analytically without requiring save-file migration.

2. **Step B — Forward-Mode `JetExpr` Across the Full `geom::SdfNode` CSG Tree (`emitNodeJet`):**
   - Implemented `isDifferentiableSdfNode` and `emitNodeJet` traversing composite CSG trees:
     - Leaf primitives (`Sphere`, `Box`, `RoundBox`, `Ellipsoid`, `Cylinder`, `Cone`, `Torus`, `Convex`) emit exact analytical gradients in a single pass.
     - CSG operations (`Union`, `Intersect`, `Subtract`, `SmoothUnion`, `Morph`) propagate `JetExpr { val, grad }` using exact calculus identities (including the cancellation in $\frac{\partial}{\partial d} \text{smin}$).
   - Emitted `sdfEvalGrad(p: vec3<f32>) -> vec4<f32>` (returning `val` in `.x` and $\nabla f$ in `.yzw`).
   - Replaced 4-sample central finite differences in `sdfSampleStep` and `sdfNormal` with a single evaluation of `sdfEvalGrad(p)`.

3. **Step C — Dead Volumetric Elimination & Storage Offset Hoisting:**
   - Isolated volumetric evaluation in `kMarcher` behind compile-time `const kHasSurfaceVolumetric: bool`: pure surface SDF shaders execute zero volumetric accumulation inside the 192-step loop.
   - Hoisted `paramOffset = instances[g_instIdx].paramOffset;` at the entry of `sdfEval` and `sdfEvalGrad`, eliminating repeated storage buffer indexing across all parameter reads.

### 2. CPU Census Comparison (Cathedral of the Living Logos)

| Reading | Baseline (2026-11-03) | Rung 1 & 2 (2026-11-04) | Delta |
|---|---|---|---|
| Fields compiled without refusal | 178 / 178 | 178 / 178 | 0 |
| Distinct shader structures | 66 | 66 | 0 |
| Fields requiring gradient step | 36 | 36 | 0 |
| **Analytic gradient fields (`sdfEvalGrad`)** | **0 / 178** | **178 / 178 (100%)** | **+178 (+100%)** |
| **Gradient fields with analytic gradient** | **0 / 36** | **36 / 36 (100%)** | **+36 (+100%)** |
| Parameter collection mismatches | 0 | 0 | 0 |
| Volumetric loop code in pure surface shaders | Executed | Eliminated (`kHasSurfaceVolumetric = false`) | Isolated |

### 3. Pixel Parity & Numerical Oracle Checks

- Independent parity suite: `sdf_wgsl_parameter_refresh_test` verifies value and gradient parity across gyroid expressions, sqrt drops, composite CSG trees, and isolated volumetric compiles.
- Native offscreen pixel parity at 2560×1440:
  - Over **99.1% of pixels are byte-identical** on full-world renders (3,664,780 / 3,686,400 pixels exact).
  - The remaining 0.58% differing pixels have a mean byte difference of only 1.30/255, reflecting the exact mathematical surface normal from `sdfEvalGrad` replacing the $O(h^2)$ truncation error of the $h=0.001$ finite difference approximation, along with existing process-dependent batch command ordering.

### 4. Paired ABBA Performance Gate (Metal, 2560×1440)

Four warm-frame cycles (16 warm frames per benchmark) across Cathedral of the Living Logos and The Merry Dancers:

| Benchmark Screen | Baseline Mean | Initial Rung 1 & 2 Mean | Post-Corrective Firmament Fixes (Rung 1 & 2) | Rung 3 & 4 (Closed-Form Root + Bounding CSG Pruning) |
|---|---|---|---|---|
| **Cathedral (Full World)** | 34.44 ms *(62.11 ms pre-pages)* | 33.11 ms *(warm 29.74–30.84 ms)* | **32.31 ms** *(warm 28.34–30.71 ms)* | **30.74 ms** *(median 29.34 ms, warm 28.20–38.71 ms)* |
| **Cathedral (Field Subset)** | 8.38 ms | 9.23 ms | **8.06 ms** *(warm 7.73–8.35 ms)* | **7.81 ms** *(median 7.81 ms, warm 7.75–7.92 ms)* |
| **The Merry Dancers (Full World)** | 13.86 ms | 9.53 ms | **10.47 ms** *(warm 9.98–10.99 ms)* | **10.00 ms** *(median 10.01 ms, warm 9.89–10.08 ms)* |

*Note on Rung 3 & 4 Implementation:*
- **Rung 3 (Closed-Form Root Hits):** Single-leaf primitives (`Sphere`, `Box`) and `Convex` polyhedra solve exact ray-domain intersections along $\mathbf{p}(t) = \mathbf{ro} + t\mathbf{rd}$ in $O(1)$ closed form (0 sphere-marching loop iterations) when volumetric flags and space warp are inactive, and bypass the 192-step loop in both primary rays and `sourceVisibility` shadow queries.
- **Rung 4 (Bounding-Sphere CSG Pruning):** Pure-primitive subtrees with $\ge 3$ leaves in `Union` and `SmoothUnion` evaluate a conservative CPU-derived bounding sphere $\|\mathbf{p} - \mathbf{c}_B\| - R_B$. If the bound exceeds $d_A (+ k)$, child 1 is skipped entirely at runtime. Crucially, child bounding center $\mathbf{c}_B$ and radius $R_B$ are registered as storage buffer parameters (`P.v`) via `e.param3(cB)` and `e.param(rB)`, strictly preserving byte-identical WGSL code across slider/position value edits with zero shader recompilation!

### 5. Regression Suite Status

All focused and integration tests pass cleanly with actual Metal access:
- `sdf_wgsl_parameter_refresh_test`: PASS (including Rung 3 root hit witnesses and Rung 4 bounding CSG pruning witnesses)
- `webgpu_sdf_parity_test`: PASS (25/25 shapes agree with CPU reference, including exact 0-diff on Sphere, Box, and Convex)
- `webgpu_perlin_exact_gradient_test`: PASS (Gates A-D)
- `webgpu_cathedral_open_hand_test`: PASS (0 failures)
- `webgpu_mesh_upload_pages_test`: PASS (10,371 checks, 0 failures)
- `gpu_mastery_test`: PASS (40/40 checks)
