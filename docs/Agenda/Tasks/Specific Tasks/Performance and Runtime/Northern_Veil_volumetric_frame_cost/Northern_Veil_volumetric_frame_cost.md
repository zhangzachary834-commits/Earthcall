# Make Northern Veil inhabitable: its volumetric frame cost

**Opened by:** Zach, 2026-10-09: Northern Veil is "beautiful but inhabitable bc of lag freezing the computer." The Zone was authored by Gemini (PR #346) with Zach ("THE AURORA IS HEREEEEEEEEEE"); the 2026-09-25 resident-parameter candidate gave no responsiveness gain there (see [Sunlit Mist task](../Sunlit_Mist_saved_world_responsiveness/Sunlit_Mist_saved_world_responsiveness.md)).

**Zach's constraint (2026-10-09):** "WE MUST NEVER TRY TO 'fix' THINGS BY LOWERING RESOLUTION OR WINDOW SIZE." Only work that cannot change the image may be removed — Prism Sun's "Keep the aurora. Kill only the work that reality cannot observe."

## Measurement (2026-10-09, MacBook Air M5, native WebGPU/Metal, Debug build)

Volume-only probe over the real save, read-only: `zone.ecform` decoded to a scratch JSON copy; camera on the spawn dais looking at the curtains; GPU-synchronized. Probe source adapted from Codex's [Sunlit Mist probe](../../../../../../scratch/webgpu_sunlit_mist_perf_probe_2026_09_25.cpp); it lived only in the session scratchpad.

| Curtains (640×360) | ms/frame |
|---|---|
| any one alone | 6.6 – 9.1 |
| two (0+1, 0+2) | 21 – 22 |
| three | 46 – 50 |
| all four | 88 – 124 (drifts with machine load) |

All four at 320×180: 32 ms; at 1280×720: 418 ms. The cost is per pixel, and **each added curtain roughly doubles it**.

## Cause 1 (GPU, dominant): 96 samples per overlap segment

`src/Singularity/Screen/WebGPU/SdfWgsl.cpp` `compileVolumeSet` (~line 4092–4143) sorts every medium's entry/exit along the ray, then takes **96 samples in every occupied segment** and evaluates every medium at each sample. Four overlapping boxes cut one ray into up to 7 segments, so up to 672 samples × 4 media. Each curtain is a thin sheet inside a box up to 160×36×46, so most samples evaluate the full noise expression only to find density 0.

Candidate fixes, in order:
1. ✅ **Prove empty air empty (image-exact) — landed 2026-10-09.**
   - **Mechanism:** `Rendering::buildVolumeZeroProof` (`src/Singularity/Screen/VolumeZeroProof.*`) tiles any medium's box with ~32k near-cubic cells. It proves coarse-to-fine, with OntoMath `MathNode::evalRange`, which cells can only give D <= 0, each tested 1% enlarged.
   - **Shader:** `volumeZeroProven()` skips evaluating D there. Sample positions and counts are unchanged.
   - **Generic, per Zach ("DONT MAKE IT RELY ON HARDCODING AURORA/ZONE/OBJECT-SPECIFIC MATH"):** no medium-specific code. Time and unbound variables are never bound, so they only defeat a proof.
   - **Enabling change:** `MathNode::evalRange` gained `Clamp`, which previously bounded to [-inf, inf]. This is a sound over-approximation, per PROPHETIC_RETE §2.
   - **Cache:** keyed by density revision (Derived-State Ledger entry, 2026-10-09).
   - **Results:** ~96% of every Northern Veil curtain's cells are proven empty. A–B at 640×360: four curtains **94–101 → 34–35 ms**; at 1280×720 **395–404 → 112–115 ms**; one curtain 8.7 → 2.8 ms. The full RGBA hash is identical in every block.
   - **Witnesses:** `volume_zero_proof_test` (CPU, 16,000-point soundness sweep) and `webgpu_volume_zero_proof_test` (byte-identical framebuffer, both pipelines, two times).
   - **Switch:** authored as ScreenChannel `volumeZeroProofEnabled`, wired like the Suns' `sdfRangeProxyEnabled`. It defaults **on** (Zach, 2026-10-09) because the native witness already proves parity, and exposes read-only `volumeZeroProofCellsProven/Total`. Guarded in `channel_paths_test`.
   - **Breakdown after the proof (640×360, scratch diagnostic):** about 22 ms of the ~33 ms is the sample loop visiting proven-empty samples (every cell forced empty: 22–23 ms, black image), and about 11 ms is real medium math. This is why step 2 below is next.
2. ✅ **Grid walk over the proof (image-exact) — landed 2026-10-09 (uncommitted).**
   - **Mechanism:** when every medium holding a sample has it in a proven cell, `volumeZeroCellExit()` returns where the ray leaves those cells, and both shader loops jump to the first sample beyond the nearest exit.
   - **Why it is exact:** sample positions are unchanged, and a skipped sample would have added exactly +0 (the ray stays in a convex cell until it exits, and the 1% proof enlargement absorbs exit rounding).
   - **Results:** A–B with the same probe, 640×360, four curtains: exact 95–100, proof only 32–33, **proof + walk 18.6–19.5 ms/frame**. At 1280×720: exact 454–461 → **72–77 ms**. The full RGBA hash is identical to exact in every block, and `webgpu_volume_zero_proof_test` is 12/12 byte-identical.
   - **Remaining:** about 13–14 fps at 720p on an M5. What is left is real medium math near the sheets plus the walk's own cost.
3. ✅ **Unified quadrature — Zach's idea, approved by Zach on the numbers, 2026-10-09 (uncommitted).** Zach: "there is probably a better solution than just reducing the numbers i think we can instead do a matheamtical unification of the drawing functions wherever it overlaps."
   - **What was built, precisely:** a unified *sampling schedule*, not a symbolic merge of the media's equations. Each sample still evaluates each present medium's own expressions once and sums them, as before. Symbolic unification (one combined expression, with OntoMath deduplicating sub-formulas shared across media, as in the #329 "shared canonical math" DAG) remains a separate, unbuilt idea. It saves work only where media share math. Northern Veil's curtains share almost none, though the zero-proof and grid walk could run once over the combined field.
   - **Rule:** the media already shared one integrand (summed σ_t and sources, one transmittance). Box entry/exit stay as segment breakpoints, because they are the unified field's genuine discontinuities. But each segment is now sampled at the finest *standalone* resolution of the media present (chord / `samplesPerChord`) instead of a fixed 96. No medium is ever sampled more coarsely than alone; overlaps no longer multiply samples; slivers no longer get 96.
   - **Truth-reference method:** a scratch-only knob rendered the same frame at 96…6144 samples per segment. The old scheme at 96 is already within 1 level of the 6144 reference (mean 0.019, 0 px > 1). Plain reduction is visibly worse (48: 147 px > 1; 24: 5,177 px), so reducing the count was the wrong lever. A 12288 render produced saturated nonsense on Metal; it is unexplained, and the setting is clamped to 8192.
   - **Results vs a 6144 reference at three views (640×360):**
     - Unified: max 1 level, 0 px > 1, mean 0.031–0.041.
     - Frame time: view A 17.2 → 11.2 ms, B 24.5 → 13.6, C 36.6 → 22.7.
     - Both schemes converge to the same image (unified at 6144: mean 0.004 vs the old-scheme reference).
   - **Final build, four curtains:** view A 11.8 ms at 640×360 and 38 ms at 1280×720 (454 ms before tonight's work), view C 23.8 / 93 ms.
   - **Exposed:** the hardcoded 96 is now ScreenChannel `volumeSamplesPerChord` (default 96), the renderer's `setVolumeSamplesPerChord`, carried in uniform `volumeControl.w`, so changing it never recompiles. `volumeControl.z` is deliberately left unused: it once held a hardcoded HG phase g, and `sdf_wgsl_parameter_refresh_test` guards it.
   - **Witness:** `webgpu_volume_unified_quadrature_test` (three views; ≤ 2 levels and ≤ 0.05% of lit px > 1 vs 6144; no recompile on resolution change; 96 restores the identical image).
4. ✅ **Two more exact cuts — 2026-10-09 (uncommitted), byte-identical vs the previous build at three views:**
   - **SourceRho-zero for volumes:** incident light is formed first; rho = 0 skips the source's chroma, angular factor and visibility, and zero incident light skips scattering, medium chroma and phase. This is the Sun lineage's question applied to media for the first time. **Gain about 0–2%**: that work was not where the time was, though it costs nothing and keeps the three truths separate.
   - **Zero-factor density short-circuit:** `emitDensityPiecewise` flattens a density product, places noise-free factors first, and returns 0 on an exact zero before any `Noise` runs. Parameter registration order is unchanged (`collectVolumeParams` uses the same emitter), and the product's grouping is preserved. **Gain 7–11%** (640×360: A 11.4 → 10.6, B 14.2 → 12.8, C 23.5 → 21.8 ms).
   - **Noise ceiling (scratch diagnostic, noise replaced by a constant):** A 10.5 → 7.1 ms, C 21.2 → 12.8 ms, so noise is 32–40% of what remains.
   - **Why the short-circuit recovers so little of it:** each curtain's `shape` (envelopes × folds × noise) is authored into **four channels**: D = 0.75·shape, σ_t = 0.10·shape, σ_s = 0.04·shape, E_v = shape·color·g(t). At every glowing sample the same noise runs three to four times.
5. ✅ **Cross-channel shared subexpressions — built and approved by Zach on the numbers, 2026-10-09 (uncommitted).**
   - **Mechanism:** `planVolumeSharing` finds noise-bearing scalar subtrees over p/x/y/z/t that are identical in operators *and* constants across a medium's channels. Each is computed once per sample (`volumeSharedEval` → private slots); every channel reads the slot, and the noise short-circuit runs inside it.
   - **Northern Veil:** one group per curtain (D, σ_t, σ_s, E_v), and **one `cnoise3` call instead of four**.
   - **Stale-value guard:** the sharing *positions* (never constants) are part of the recompile signature (`inspectVolumeSharing`). A numeric edit that makes two channels stop matching changes the signature and forces the recompile; `collectVolumeParams` replays the same plan, so parameter offsets agree.
   - **Results:** A–B interleaved at 640×360: A 11.1 → 6.3, B 13.4 → 7.4, C 24.6 → 11.9 ms. Not byte-identical: one pixel per frame differs by one 8-bit level in two of three views (GPU float contraction of the reorganized arithmetic). Zach approved under the ≤1-level-from-truth bar, which the truth test still holds (max 1, 0 px > 1).
   - **Witness:** `volume_shared_subexpression_test` (CPU, 41 checks on the real save, including both stale-value cases).
   - **Real-app measurement (Zach asked "did u test it to be faster in the northern veils zones"):** `earthcall_webgpu` was built in two scratch worktrees (A = commit `f0194f3f`, B = A + this change). Both carried an identical, never-shipped diagnostic patch (`EC_DIAG_ZONE` / `EC_DIAG_CAM` / `EC_DIAG_FRAME_LOG` / `EC_DIAG_EXIT_AFTER` in `Engine.cpp`), with isolated saves and `EARTHCALL_HOME`. Setup: the default window (1280×720 points = 2560×1440 Retina pixels), Northern Veil, camera on the spawn dais looking up at the curtains. Runs interleaved A–B–A–B, 45 s each, steady state after 15 s.
     - Results: A 173.1 / 173.7 ms (5.8 fps), B 116.8 / 116.7 ms (**8.6 fps**); p90 190 → 133–140 ms.
     - In the app this is ~1.48×, versus ~2× in the volume-only probe, because the rest of the frame is unchanged. The frame is GPU-bound (median `wait_surface` 161 → 111 ms).
   - **Earlier note on this idea, kept for history:** compute a subtree shared by a medium's channels once per sample. This is Zach's symbolic unification, applied across a medium's channels rather than across media; OntoMath can find identical subtrees, and an opaque shader could not. Still not built: finer proof cells near the sheets, and hoisting the time-only emission pulse.
6. **Zach's compile notes (2026-10-09):** "it should only compile the different equations once into the unified equation… adjust incrementally rather than recompiling the whole equation".
   - **Already true:** one fused program, compiled per structure, pipeline-cached by WGSL text, with numeric coefficients in a buffer (no recompile).
   - **Gaps:** params and proof bits are re-uploaded every frame (should be GPU-resident with slice updates), and a structural edit to one medium recompiles the whole fused program. Proposed: per-medium codegen caching, async pipeline creation, and a correct GPU AST-interpreter tier meanwhile (the 2026-08-28 "GPU AST Interpreter and WGSL Tiering" design, never built).
7. **⚑ AUTHOR — sample placement beyond unification.** Allocating samples by distance along the ray instead of 96 per segment changes pixels (a different quadrature of the same integral). It can be made as accurate or more accurate, and it was anticipated in [V5 follow-ups](../../Rendering%20and%20OntoMath/Visual_radiance_V5_and_Rung_8_followups/Visual_radiance_V5_and_Rung_8_followups.md) ("error-controlled local quadrature"). Zach decides, because the image changes.

## Cause 2 (CPU, ~7 ms/frame): JSON serialization as change detection — fixed

Every frame `Rendering::readVolumeDensity` and `EngineRender` serialized all 24 authored channel expressions to JSON and hashed the text, only to learn whether they changed. Zach: "Y IS IT SERIALIZING INTO JSON AT ALL WE MIGRATED TO MSGPACK/FLATBUFFERS." This was never storage; the migration did not touch it.

**Fix (uncommitted in the working tree):** `geom::FieldNode` carries an authored-math revision drawn from one process-wide, never-repeating sequence. A recycled node address can never present a revision a renderer cache has seen (the SourceRho producer-rebinding hazard).
- The property bridges bump it, and skip the bump on an identical rewrite.
- `applyJson` bumps it.
- The MCP `author_volume` path, which assigns channels in place (`WebSocketServer.cpp`), now bumps it by hand.
- Fail-open: `verifiedAuthoredMathRevision()` re-hashes one channel per call, round-robin. An unrevisioned change is bumped and reported on stderr, so a missed writer heals within ten reads and never stays stale silently.

Witness: `tests/singularity/field_node_authored_revision_test.cpp` (19/19). In the full suite the verifier fired only in that test's deliberate case, so no production writer was missed.

A–B–B–A, all four curtains: CPU projection **6.75–7.50 → 0.65–0.93 ms/frame**; lit-pixel count and luminance sum identical block for block. Frame time did **not** improve, because the frame is GPU-bound and the CPU work overlapped the GPU. The win appears once Cause 1 is addressed.

Not converted: `WebGpuRenderer::drawImplicit` (~line 1453) still JSON-hashes Object-attached field channels per draw. It is the same fix, left for a follow-up.

## Found along the way

- **`.ecform` zone saves are a MsgPack envelope around one JSON string.** `SaveSystem.cpp:424` writes `{"MigrationRoot": j.dump()}` and load parses that JSON. The MsgPack/FlatBuffers migration is skin-deep for Zones; FlatBuffers appears only in `ZoneManager.cpp:2832`. This is load-time only, not this lag.
- **`RealSaveTreeGuard` does not protect `saves/worlds/`.** It restores `saves/zones` and `saves/homes` only, and an aborted test skips its destructor anyway. The 2026-10-09 full `ctest` run rewrote `saves/worlds/chess_app.ecform`. Only `matterGeneration.sha256/snapshotId` changed, and the `.ecmatter` sidecar was replaced (`7431ca37…` → `556fa746…`, 7 of 13,652 bytes differ). It was left for Zach to restore or keep.

## Suite state at this change (not an A/B)

`ctest`: 268/281 pass. Failing: chess_app/castling/click_geometry/extended_rules (Law promo-button conditions), quantifier_scaling (lag), second_nature_law_forge_zone, synthesis_studio_app, authorable_light_contract, gpu_mastery, slow_adapter_zone_perf, webgpu_perlin_exact_gradient, prism_cathedral, zone_home_ontology.
- **Known elsewhere:** authorable_light_contract and slow_adapter have fixes on unmerged branches (`d3f38014`, `dc0a8fc2`); second_nature_law_forge is named in `d3f38014`.
- **Not proven:** the rest touch code this change does not, but no baseline build was run. Another worker committed OntoMath/ProbabilityForm changes into this checkout (01:02–01:12 PDT) during the run.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01NJy6VrPVNcHAnggwFyTsmF` (local `9e6def41-f1a4-4d08-afe4-f04085c5da75`) · 2026-10-09 01:17 PDT


## Sixth-one Sun continuation: share a value only in its evaluation domain

This continues [Opus’s Poltergeist III handoff](../../../../../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#poltergeist-iii--the-sun-was-in-the-checkout-2026-10-09-1700-pdt), preserving its uncommitted ownership and historical evidence.

Zach asked Codex / GPT-6.1 Sol to continue the Cyber Constitutionalist after shutdown: “our goal is to make the computational cost as close as possible to the bare minimum necessary mathematical evaluation for a perfectly or near-perfectly accurate image.” The cross-channel implementation was already present, uncommitted, on `sync-from-earthcall-main`, HEAD `f0194f3f`; Opus's prior full-app timings above remain his evidence. This continuation preserves that work and keeps save inputs read-only.

Three added CPU assertions failed on the inherited candidate: a matching source expression reused a medium slot, SDF noise at `2*p` reused noise at `p`, and a matching occluder reused the medium sample instead of its shadow-ray point. Fixed by confining substitution to the original medium point and clearing it at source/occluder boundaries; Gradient retains its six distinct points. Also fixed undeclared locals in all-noise shared evaluators, matched the collector to the compiler's medium/source/occluder constant order, and made sharing-group order depend on occurrence positions rather than numeric keys. A matched numeric edit therefore keeps WGSL and slots stable. No new rendering switch or sample-count/resolution change.

### Time-only hoisting experiment — not shipped

A generic candidate found scalar subtrees depending only on `t`, filled private values once per ray/medium before sampling, and reused them inside spatial evaluators. Northern Veil acquired three time-only colour groups per curtain. The candidate was byte-identical to the inherited renderer across a full 2560×1440 RGBA capture at the spawn view, t=7.25; its 320×180, three-view comparison against 6144 samples also had max error 1 and no pixels above 1. These are finite reference comparisons, not proof of an exact integral.

Sequential A–B–B–A, fixed spawn camera and t=7.25, all four curtains, 2560×1440, 40 GPU-synchronized frames per run: A=124.03/131.68 ms; B=166.82/119.47 ms. The spread does not establish a reliable win, so the temporal candidate was removed from production. An earlier B run overlapped a GPU correctness test and is excluded from these numbers. Fewer algebraic evaluations alone cannot certify lower frame cost; register lifetime, memory, scheduling and driver lowering must be measured too.

Scratch evidence (not repo/save state): `/private/tmp/earthcall-sixth-one-sun-01a122d7/`, including pre-edit and candidate compiler snapshots, identical-source probe binaries, and native RGBA captures. The existing `nv_ref.cpp` probe source was compiled unchanged against current headers; it reads a decoded scratch snapshot and writes only explicit scratch captures. No `bash -c`, deletion, save authoring, or full save-mutating suite.

### Remaining route toward the minimum

- One sample now shares the curtain shape across its authored channels, but channels remain independent truths and differently placed curtains do not automatically share a coordinate domain.
- Revision gates avoid rediscovery on unchanged frames. Numeric edits refresh parameter values; changes that alter which expressions share a value can still rebuild the fused shader. Per-medium lowering reuse, incremental graph repair, resident parameter/proof buffers and dirty uploads remain open.
- Time-only work may need a different placement than per-ray private storage; this experiment supplies no shipped speedup. Nested common subexpressions need demand-aware scheduling so extracting noise never defeats the existing cheap zero-factor exits.
- A dense reference is an image comparison, not a universal minimum-work theorem. Error-bounded local quadrature and integration-tail bounds remain future work; preserve the accepted sample lattice/fidelity unless a separately witnessed rule improves it.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T11:52:24.788492-07:00

### Final verification of retained guards

2026-10-10T12:02:06.309310-07:00; five focused CTest targets passed with native Metal access and no device skips: parameter refresh, shared-subexpression codegen (51/51), native sharing, unified quadrature (15/15), and zero-proof parity (12/12). The native sharing test was then strengthened to prove the source clock visibly changes its radiance and passed 14/14; all four independent-inline comparisons had max RGBA error 0. Unified quadrature at 320×180 had max error 1 and no pixels >1 at each of its three views against 6144 samples. These checks cover sampled cases, not a universal fidelity theorem.

`earthcall_webgpu` rebuilt successfully in Debug. The final guarded volume-only renderer matched the inherited build byte-for-byte across 14,745,600 RGBA bytes at 2560×1440, fixed spawn camera and t=7.25. The scratch input was compared with the live decoded Zone and was equal. Northern Veil's save SHA-256 remained `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. No full-app runtime/FPS claim is made by this pass; Person acceptance is listed separately. Changes remain uncommitted, preserving Zach's existing working-tree work.

Backend/hardware: native WebGPU/Metal, Apple M5 with 8 GPU cores, verified with `system_profiler SPDisplaysDataType`. Compiler/source fixture helpers were initially corrected for const channel pointers and unique child ownership before the final passing build; those test-construction errors are not runtime verdicts.
