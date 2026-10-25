# Make Northern Veil inhabitable: its volumetric frame cost

**Opened by:** Zach, 2026-10-09: Northern Veil is "beautiful but inhabitable bc of lag freezing the computer." The Zone was authored by Gemini (PR #346) with Zach ("THE AURORA IS HEREEEEEEEEEE"); the 2026-09-25 resident-parameter candidate gave no responsiveness gain there (see [Sunlit Mist task](../Sunlit_Mist_saved_world_responsiveness/Sunlit_Mist_saved_world_responsiveness.md)).

**Zach's constraint (2026-10-09):** "WE MUST NEVER TRY TO 'fix' THINGS BY LOWERING RESOLUTION OR WINDOW SIZE." Only work that cannot change the image may be removed — Prism Sun's "Keep the aurora. Kill only the work that reality cannot observe."

**Current continuation:** shared-channel guards were committed in `a5a8bdf6`; the [2026-10-10 proof-support pass](#sixth-one-sun-proof-support-with-cached-camera-paths) is verified in the working tree. Earlier sections retain dated historical measurements and do not describe every current loop or codec.

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


## Sixth-one Sun: proof support with cached camera paths

Continues [the commissioned finer-proof pass](../../../../../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-finer-proof-economics-commissioned-by-zach). Zach approved the next measured pass after committing the shared-channel work in `a5a8bdf6`; he then asked to resume after interruption. His governing aim remains the minimum necessary mathematical evaluation for a perfectly or nearly perfectly accurate image. This pass preserves the authored equations, native resolution and accepted sample lattice.

### What was retained

The existing conservative zero grid now also bounds every unproven cell, enlarged by its cell-proof margin. That derived support envelope is built once per density revision/half-extent, using the same proof that already admits grid walking. Outside the media, the shader can reject wholly empty rays and jump null prefixes/tails before grid walking; six bounds follow the existing bit words. A near-parallel ray or unknown proof takes the original interval. All-empty density has reversed support bounds. Support **never** replaces authored box entry/exit events, opaque clipping, chord length, overlap segment boundaries, or quadrature spacing. A retained extra prefix sample and arithmetic margin make the skip conservative.

The always-on prototype was byte-identical but about 25% slower inside the curtains: A=204.50/202.50 ms, B=256.15/254.98 ms at native resolution. It is not the retained path. `flushVolumeComposite` now selects two cached Kernel specializations: outside all admitted authored boxes with proofs enabled uses support trimming; inside any box, or with proofs disabled, uses the original marcher, with neither the support helper nor its extra payload. Both single and fused caches include this bool alongside their existing expression identities/membership. Numeric/structural revisions still refresh each lane through the established checks. A camera change changes no equation. Each lane compiles on first use; warmed outside→inside→outside revisits cause zero equation recompiles. Prewarming the second lane and reusing a lowered expression plan between variants remain future work; do not promise a hitch-free first boundary crossing.

The existing `volumeZeroProofEnabled` switch owns this behavior; there is no new setting or Zone-specific rule. The [Derived State Ledger](../../../../../architecture/law/DERIVED_STATE_LEDGER.md#proof-derived-volume-support--2026-10-10) records payload, cache and fallback contracts.

### Fresh native economics and pixels

Native WebGPU/Metal, Apple M5 (8 GPU cores), Debug. Baseline volume sources frozen from `5cf8c361`; the retained working-tree renderer is based on `ffe0681f668d91ce361bed8fb3f7de9bc21f89c6` (the intervening merge is documentation). Scratch `retained-provenance.json` identifies the exact source and binary SHA-256 values. Runs below completed 2026-10-10T23:53:20Z–23:55:37Z. All four Northern Veil media, 2560×1440, t=7.25, sequential A–B–B–A, one warmed frame followed by 40 timed GPU-synchronized frames at spawn or 30 inside/20 reverse/20 side. Camera eye/target: spawn `(0,2,0)→(0,60,90)`, inside `(0,50,75)→(40,62,110)`, reverse `(0,60,160)→(0,55,75)`, side `(90,55,90)→(-30,55,90)`.

| View | A runs ms | B runs ms | Two-run mean A → B ms | Reduction |
|---|---|---|---|---|
| spawn | 110.86, 114.64 | 97.36, 94.26 | 112.75 → 95.81 | +15.02% |
| inside | 231.71, 228.24 | 229.10, 228.31 | 229.98 → 228.70 | +0.55% |
| reverse | 189.12, 192.26 | 194.81, 195.11 | 190.69 → 194.96 | -2.24% |
| side | 153.39, 164.80 | 158.50, 163.86 | 159.09 → 161.18 | -1.31% |

These are means of the two runs of each version, **not minimum/floor timings**. Spawn improves ~15%; inside is effectively unchanged. Reverse and side are ~2.2%/~1.3% slower in these runs; this is recorded, not hidden inside a global speedup claim. Earlier support-only/specialized spawn repeats showed ~16–18% reductions, with load/thermal drift in absolute times. Every run in every retained view compared all 14,745,600 RGBA bytes with its baseline reference and was byte-identical. Finite image witnesses establish these cases, not a universal quadrature theorem or minimum-work bound. Deep inside remains ~229 ms in this witness: Northern Veil is still not established as smoothly inhabitable.

### Experiments rejected on economics

Larger uniform grids (65,536 and 262,144 target cells) and a four-level hierarchy preserved pixels but did not establish a net improvement. An initial native spawn series had baseline 92.68/88.79 ms, 65k 93.56 ms, 262k 120.68 ms, 32k hierarchy 91.93 ms and 262k hierarchy 101.68 ms. This is a screening series, not a balanced final ABBA. The primary medium's CPU proof increased from 31,920/33,120 empty cells (~235 ms, 5,520 bytes) to 258,264/264,960 (~740 ms, 44,160 bytes) at 262k. More empty proofs can add traversal/lookup and build cost. Those candidates remain scratch experiments; production retains the original ~32k grid budget and grid walk.

### Verification and full-app limits

`earthcall_webgpu` rebuilt. Six focused CTest targets passed on native Metal with no skips: parameter refresh, shared-expression codegen (51/51), CPU proof (24/24), native sharing (14/14), unified quadrature (15/15), native proof (38/38). Native proof compares RGBA vectors at two times, single/fused paths, odd-sized axis/inside/reverse/side views, an in-place zero→positive revision before proof eviction, distinct first-use kernels, and warmed camera-lane revisits. Unified quadrature still compares against its 6144-sample reference; no changed count or resolution was introduced. Final focused run: 21.79 seconds, `retained-focused-tests.log`.

Full-app evidence is deliberately separate. Three earlier prototype runs genuinely drew Northern Veil at 2560×1440 with two draws/131,860 proven cells: A1 mean 121.71 ms (median 116.68), B1 mean 110.29 (median 113.06), B2 mean 112.05 (median 112.46). The paired A2 had zero draws and is excluded; there is no complete valid full-app ABBA or retained-version FPS claim.

A scratch-only surface diagnostic explained a current no-draw run: status `196609 == 0x00030001` is **Occluded**, explicitly defined in `third_party/wgpu/include/webgpu/wgpu.h:78`, beyond the standard status enum. Its documented Metal behavior is to skip a covered/minimized window without reconfiguring. This is not evidence of an ABI mismatch or a volume-pipeline refusal. Two subsequent scratch app runs did render, but all their CSV rows remained in **Sanctum of Beginnings**, with 344 draws and no volume proofs; they failed the requested Northern Veil switch and are also excluded. Their Northern Veil fixture bytes still exactly matched the live save. The native UI selection tool timed out, so visible Northern Veil admission/appearance/motion remain Person checks. Do not count the fast wrong-Zone or occluded runs as an optimization. Inspect the fixture's Zone loading/identifiers before using it again; do not change the other agent's ongoing Storage work based on this unresolved symptom.

### Handoff toward the actual minimum

Reprofile the current inside view to separate sample math, noise and remaining grid-walk overhead; older noise percentages are historical. Native timing must justify the next elimination. Keep the two cached paths and preserve independent coordinate/source/occluder domains. Resident dirty uploads, per-medium lowering reuse and incremental structural repair remain open; none is proved to be the current dominant cost. Any error-bounded quadrature or integration-tail rule needs separately stated bounds and dense/pixel witnesses. The [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md#northern-veil-proof-derived-ray-support--october-10) covers actual loading, appearance, movement, and first versus warmed boundary crossings.

All probes/diagnostics/captures live in `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/`; inherited `nv_probe`/`nv_ref.cpp` were compiled unchanged and remain read-only. No `bash -c`, deletion, save authoring, full save-mutating suite, commits or pushes. Northern Veil save SHA-256 remains `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. Storage/Fmt edits and root `test_fmt`/`test_parse` scratch files belong to another agent and were left alone.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:08:05.843830+00:00


## Northern Veil missing from the Zone list: format discovery repair

Continues [the shared-checkout thread](../../../../../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-northern-veil-discovery-repair-commissioned-by-zach). Zach reported that Northern Veil was absent from Creator Console’s Zone list, explaining why earlier scratch runs stayed in Sanctum. Its live `zone.ecform` remained intact: 97,635 bytes, SHA-256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`.

The concurrent, uncommitted Storage migration could read `.ecform`, but its Zone/Home catalogs and identity-path resolvers searched only `.ecsave` and `.json`. Thus boot’s `hydrateFromZoneStore` never received Northern Veil’s record; Creator Console enumerates that live manager. The existing Zone/Home identity writers still emit `.ecform`, making compatibility necessary even for their newly written identities. This was a discovery regression, not disappearance of the authored world.

`SaveSystem.cpp` now shares one file-local resolver for nonempty regular files in `.ecsave` → `.ecform` → `.json` order. Catalogs pass the actual directory entry without reconstructing or sanitizing its key; direct reads use the same resolver. Empty newer files no longer hide a valid older identity. A nonempty malformed preferred file is still selected and reported by the decoder/admission boundary; this repair does not silently choose another document after decode failure. Other migration/codec changes are preserved; no save conversion, rename, replacement, or authoring occurred.

The isolated `stale_sidecar_cleanup_test` reproduced eight discovery/path failures before the repair; its empty-newer direct-read case also exposed the missing fallback. Final regression checks cover legacy JSON, `.ecform`-only discovery, newer-file precedence, empty-newer fallback, and byte preservation (31/31). `zone_identity_boundary_test` preserves mismatch/duplicate/nameless refusal behavior (10/10). The read-only Material corpus passes 118 checks across all 46 Zone identities in the existing isolated fixture, whose Northern Veil bytes match live. `earthcall_webgpu` rebuilt successfully. A scratch probe linked against the final app objects discovered Northern Veil, hydrated it at index 22 of 46 with 24 Objects, and successfully activated it through `switchTo`; exit status 0. `discovery-repair-provenance.json` records final source/binary hashes and the unchanged save hash. Scratch boot/activation witnesses, build and focused-test logs are under `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/`; live save files remain untouched. Visible Creator Console acceptance requires restarting the rebuilt app, clearing any Zone-list filter, and selecting Northern Veil. Rendering performance and smoothness remain the separate open task above.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:27:10.143532+00:00


### Person observation after recovery

Zach reports loading the Zone and seeing approximately **12–13 FPS** after the discovery repair (2026-10-10 PDT). This is Person-reported live-app evidence, approximately 77–83 ms per frame, not a controlled GPU timing or a new A/B result. Camera/view, window dimensions and timing distribution were not specified. Loading recovery is confirmed by his report; responsive inhabitation remains open. The next measured pass should isolate steady frame cost in the actual Zone, especially the inside view, before choosing further exact eliminations.

**Recorded by:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:33:19.514668+00:00


## Minimum-work continuation: reusable compilation and resident data

Zach commissioned [the minimum-work implementation plan](../../../../../plans/NORTHERN_VEIL_MINIMUM_WORK_IMPLEMENTATION_2026-10-10.md), requiring compile-once mathematics, incremental repair, native fidelity and a 30 FPS movement milestone. After the usage interruption he reports about **15 FPS from his view, sometimes 20 FPS**; he explicitly cautions that 20 FPS may also have occurred before these changes. This is useful Person evidence, not a matched comparison or achievement of the movement target.

### Implemented exact candidate

The volume compiler now lowers reusable evaluator fragments directly. Standalone and fused modules consume those fragments; they no longer compile whole standalone shaders and extract their functions. A Kernel-only typed DAG records channel/piece/child occurrences, parameter slots, independent coordinate/clock domains, reverse dependencies and exact typed structural/value identity. Sharing keys no longer call JSON serialization. On a revision, unchanged node fragments survive when their binding, parameter positions, temporary positions and sharing context agree. Sharing changes conservatively invalidate the affected medium's fragments; unrelated media retain theirs. Unchanged frames do no lowering. Stable numeric edits retain WGSL and update values; **the parameter-only refresh still uses the established collector, which traverses the changed medium and builds temporary emitter text**. A direct retained parameter-binding recipe remains open, so do not call all revision work minimal. Changed modules can still incur whole-module driver compilation.

Single/fused and inside/outside variants share the per-medium evaluator cache. Admission prewarms both existing camera lanes; warmed boundary crossings need zero equation lowering or shader compilation. This moves cost into admission, rather than eliminating driver work. Kernel GPU payload buffers retain parameter/proof data, coalesce adjacent dirty writes and skip stable packing/dirty scans. Camera, clocks and transforms remain frame data. Proof revisions/half-extents, unknown inputs, reordered/removed media, same-frame repeated composition, cache reload and device teardown have explicit paths. Stable and time-only frames upload zero parameter/proof bytes. No native resolution, sample-count control, save codec or authority boundary changed.

### Fidelity and rejected experiments

The final candidate matched all **14,745,600 RGBA bytes per capture** at 2560×1440 in spawn, inside, reverse and side views at t=0 and t=7.25 (eight comparisons, max error 0). Six focused CPU/native targets pass: parameter refresh, sharing, CPU proof, native sharing, unified quadrature and native proof. The DAG witness is 55/55; native proof is 42/42. Native sharing additionally witnesses numeric repair, one-medium structural edits, time-only/unchanged zero uploads, membership reorder/removal, reload and actual device replacement. These finite captures establish the tested cases; they do not establish a universal floating-point theorem or inhabited movement acceptance.

A lazy/nested demand-first sharing prototype passed the synthetic tests but changed one native output level at the spawn view. It was removed from production because the exact stage requires byte equality. The original eager largest-group evaluation remains. Same-arithmetic GPU time preparation and certified tails remain open.

### Person rejection of timestamp instrumentation

Zach saw severe jaggedness and curtains flickering out of existence during the timestamp-enabled frozen-baseline window diagnostic. That run is rejected, including all its performance numbers. The ordinary native window remains on its original timestamp-disabled feature negotiation; the temporary opt-in was removed. Ordinary offscreen witnesses now also request no timestamp features. The delayed timing machinery remains feature-gated and rejects all-zero query samples as unavailable. We have **no reliable separated volume GPU timing** on this host. Prior native timestamp/frame-coherence trouble and this report motivate the rejection; warmup camera jumps are another diagnostic effect, so exclusive causality is not proved by the report alone. Post-restoration live visual coherence remains a separate Person check.

### Matched economics in progress

The frozen comparison includes the Zone-discovery repair and retained proof support. Scratch constructor translation units use each renderer's matching header layout; frozen renderer objects must not be instantiated with the current private class layout. The new readback-free route warms four views for eight seconds, then joins them continuously over 60 seconds, with the renderer clock matched to route progress. Two sequential ABBA series at 2560×1440 are running separately from image/GPU correctness checks. Wrong-Zone, no-draw and compile frames are excluded. CSV `nonempty-frame` is a draw-status proxy, **not the raw native surface status**; no-draw frames are excluded, but fully auditable raw acquisition status remains open. Zach's exact usual framebuffer dimensions are not separately confirmed. No new retained performance gain or 30 FPS claim precedes these measurements.

The bounded-stage reference needs clarification: removing the existing uncertified 1% transmittance cutoff can change bright late emission by more than one level compared with the displayed legacy renderer. A one-level bound against the full existing sampling lattice and a one-level bound against the legacy cutoff renderer are different contracts. The budget property and certified termination are not yet implemented while that question remains unanswered.

All diagnostics/captures are isolated under `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/`. The inherited `nv_probe`/`nv_ref.cpp` remains read-only, and the actual Northern Veil save retains SHA-256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. No commits, pushes, stashes, deletions or live save authoring. Other agents' Storage/Fmt work remains theirs.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T22:32:41.495077-07:00


### Minimum-work economics: retain preparation and upload savings

Native WebGPU/Metal, Apple M5 (8 GPU cores), Debug, all four authored media, 2560×1440 and t=7.25. Each view ran two sequential ABBA series, 24 GPU-synchronized frames per run after warmup. A new scratch benchmark, cloned from the unchanged inherited probe, waits for completion only outside the timed frame batch and performs **no texture copy or image readback**. The table reports arithmetic means across four runs per version, **not floors or minimum timings**. Negative frame-cost change means faster.

| View | Four A runs, ms | Four B runs, ms | Mean A → B, ms | Frame-cost change |
|---|---|---|---|---|
| spawn | 69.95, 78.01, 80.68, 82.75 | 73.20, 77.87, 83.92, 83.84 | 77.85 → 79.71 | +2.39% |
| inside | 209.49, 223.79, 224.31, 226.78 | 215.46, 219.62, 227.99, 226.75 | 221.09 → 222.46 | +0.62% |
| reverse | 189.15, 192.34, 191.96, 194.76 | 188.69, 190.37, 193.79, 194.58 | 192.05 → 191.86 | -0.10% |
| side | 164.45, 169.21, 168.66, 173.02 | 165.12, 164.44, 167.19, 162.33 | 168.84 → 164.77 | -2.41% |

The CPU/upload candidate passes its retention gates: recurring parameter/proof upload bytes fall by 100%, and the first complete moving ABBA series reduces mean volume preparation from 0.04778 to 0.03866 ms (~19.1%). Neither fixed-view series repeats a >3% frame regression: spawn -2.10%/-2.65% improvement (i.e. slower), inside -0.42%/-0.81%, reverse +0.64%/-0.43%, side +1.23%/+3.56%. These are targeted preparation/upload savings; **no ≥5% GPU/frame improvement is established**. The inside cost remains ~222 ms in this isolated fixed-view witness. Do not convert this benchmark into Zach's live FPS.

The first matched window route produced mean frame times 118.58/126.00 ms in A and 122.17/128.05 ms in B (ABBA #1), then 123.02/121.52 ms in A and 81.96/124.27 ms in B (ABBA #2). The faster B5 changed its waiting pattern from surface acquisition to queue submission and has substantial between-run variance. We cannot attribute it to the candidate. Route p95 values were 181–262 ms, nowhere near the 33.3 ms milestone. A raw-status rerun is being prepared; the earlier route logged draw-status only. All-zero timestamps are unavailable, not a zero-cost volume pass.

**Correction to a diagnostic hypothesis:** inspecting `applyBeginFrame` alone suggested acquisition failure might leave stale draw counters. `Renderer::beginFrame` already resets them before invoking that helper, so that diagnosis was false. A proposed redundant reset was removed. Production behavior is unchanged. The existing native object test also passes after updating twelve cold module expectations from one to two camera variants; its warmed/numeric-edit zero-compilation checks remain unchanged. This makes seven focused CPU/native targets green, rather than treating old tests as disposable.

Raw performance records and provenance: `minimum-cost-results.json`, `minimum-cost-summary.json`, `matched-route-results.json`, `minimum-final-provenance.json` under the scratch stage above. Final renderer/compiler sources match that provenance; saves remain unchanged. The plan's direct parameter recipes, demanded/nested exact sharing, GPU time preparation and certified tails are still unfinished.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T22:58:25.385007-07:00


### Raw-status route admission correction

The first raw-status attempt returned Metal `Occluded` (`196609`) on 22,788 frames and `SuccessOptimal` on 70. Northern Veil first became visible at route t≈53.8 seconds, incurring a late cold compile/proof build, so the complete route/steady-state gate failed and the run is excluded. The public beginFrame reset worked: this is surface occlusion, not stale draw statistics. The scratch route now activates its own native app once at warmup and titles its window “Northern Veil read-only performance route”; it writes only the isolated fixture and scratch logs. Raw acquisition codes 1/2 admit frames; other statuses refuse them. A new sequential series preserves the excluded artifacts instead of overwriting them. Production Engine/window behavior is untouched.

Codex / GPT-6.1 Sol / `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:04:23.095760-07:00.


### Final raw-status moving-world record

Two sequential ABBA series completed at 2560×1440 with an eight-second four-view warmup and a 60-second continuous route, renderer clock t=7.25+route elapsed, native timestamps disabled, no image readback and no concurrent GPU correctness tests. Scratch-only native foreground activation allowed admission. Every accepted warmed route frame had raw `SuccessOptimal` status 1; no occluded route frames occurred in these completed runs. Startup/warmup occlusion was excluded. Every run prepared two camera modules and built four density proofs before the measured route. Warmed compiles/lowerings/proof builds were zero, and every candidate run uploaded zero parameter/proof bytes. The frozen baseline's first byte counter includes its proof subset; do not sum its two counters.

| Run | Warm frames | Mean frame ms | p95 frame ms | Mean volume preparation ms | Parameter/proof bytes |
|---|---|---|---|---|---|
| A0 | 496 | 121.07 | 261.15 | 0.04844 | 11,805,888 / 11,315,840 |
| B1 | 469 | 127.75 | 197.31 | 0.04197 | 0 / 0 |
| B2 | 480 | 124.83 | 251.69 | 0.04138 | 0 / 0 |
| A3 | 464 | 129.53 | 254.50 | 0.05320 | 11,044,032 / 10,585,600 |
| A4 | 474 | 126.57 | 283.74 | 0.05391 | 11,282,160 / 10,813,848 |
| B5 | 448 | 133.71 | 283.12 | 0.04545 | 0 / 0 |
| B6 | 445 | 134.89 | 298.13 | 0.04489 | 0 / 0 |
| A7 | 617 | 97.29 | 210.72 | 0.02812 | 14,681,928 / 14,072,332 |

ABBA #1 mean frame A→B: **125.30→126.29 ms** (+0.79% cost); mean preparation **0.05082→0.04168 ms** (~18.0% reduction). ABBA #2: **111.93→134.30 ms** (+19.99% cost); preparation **0.04101→0.04517 ms** (~10.14% worse). The A7 baseline has the same unusual acquire→submit wait shift seen in the earlier faster B5 candidate. Thus the large swing occurs in both versions, and the moving-window results do not establish a causal gain or a uniform preparation percentage. The separate fixed-view two-ABBA checks show no repeatable >3% regression and support retaining zero-upload/compiler groundwork, but movement performance remains **inconclusive**; report the second series, do not hide it. Surface acquisition remains the dominant CPU-side wait in ordinary runs. No valid separated volume GPU timing exists, so do not attribute that wait exclusively to measured volume execution.

The candidate route's p95 values are **197.31, 251.69, 283.13 and 298.13 ms**, all above the 33.3 ms target. **Thirty FPS inhabitation is not achieved**, and Zach's exact usual framebuffer dimensions/visual-motion acceptance are not independently confirmed. His 15–20 FPS report remains separate Person evidence at his viewpoints.

The earlier fully occluded raw attempt remains excluded. The public `Renderer::beginFrame` reset was correct; no redundant reset or production surface behavior change was retained. Foreground activation and raw-status logging exist only in scratch diagnostics. The completed series and summary are `visible-status-route-results.json` / `visible-status-route-summary.json`. The ordinary app is no longer running after diagnostic shutdown. Further repeated whole-route runs are not justified until the host/window wait-pattern variability is understood or a new candidate changes GPU mathematics. No new FPS claim, save authoring, commit or push.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:24:19.962220-07:00


### Zach selected the full lattice: opt-in opacity cutoff removed

After the reference-contract question, Zach said “remove the cutoff.” The accuracy reference for bounded work is therefore the existing sample lattice evaluated without the uncertified opacity stop. Default `volumeOutputErrorBudget = 0` retains compatibility. Positive values enable the full-lattice fallback; the renderer caps finite values at 1/255 and sends a separate GPU field. Single and fused marchers, in either camera lane, cannot use `T < 0.01` in opt-in mode. The exact zero-density switch and sample resolution stay independent. This can expose authored emission that the old renderer prematurely lost, and can increase cost.

The rebuilt app and eight focused targets pass. Native sharing 46/46 now covers bright late emission and overlapping media against a closed-form ramp and 6144 samples, background visibility, budget clamping, tiny-positive values, residency/no recompile on mode changes and exact zero-budget restoration. Eight 2560×1440 default captures at two times/four views match the retained baseline bytes. The property-path test initially hung in sandboxed GLFW initialization; stopped that test and reran with desktop access successfully. No save changes; Northern Veil SHA256 remains `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`.

**Remaining:** certified contribution bounds and numerical uncertainty before any bounded tail exit; unsupported/unknown cases must continue the full lattice. This is a reference/fidelity prerequisite, not a certified-tail optimization or new FPS result. Keep native timing features off, preserve the earlier inconclusive movement evidence and require Person acceptance. [Current plan](../../../../../plans/NORTHERN_VEIL_MINIMUM_WORK_IMPLEMENTATION_2026-10-10.md#cutoff-reference-decision-implemented).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:49:10-07:00


### Independent GPU time-only preparation retained

Zach commissioned the highest likely value independent remaining item. Implemented stage 3's GPU preparation for eligible time-only scalar transcendental expressions. Retained compiler recipes use the original arithmetic and parameter slots; source and medium/occluder clocks remain separate. Derived result slots live in the existing resident buffer. One ordered compute pass precedes volume rendering, with no ordinary-frame CPU wait. Camera lanes reuse preparation pipelines; layout-preserving numeric edits update inputs without shader regeneration; reload/device replacement rebuilds derived state. Spatial/directional expressions remain in their original evaluators. No Zone-specific rules or save changes.

Eight native 2560×1440 captures across four views/two times are byte-identical. CPU sharing/parameter refresh and four final native targets pass: sharing 65/65, proof 42/42, quadrature 15/15, object consumer green. New cases include independent inline arithmetic, distinct clocks, numeric edits, no warm uploads/compiles, reload and device replacement. App rebuilt.

Two sequential readback-free ABBA series per view, 24 frames/run, fixed t=7.25: spawn 87.485→84.3625 ms (3.57%), inside 219.3225→207.075 (5.58%), reverse 183.6925→180.0025 (2.01%), side 159.0075→155.46 (2.23%). Inside's separate gains are 4.14% and 7.01%. Retain on the pooled repeated inside cost gate and no other-view regression; smaller gains remain below 5%. This measures offscreen batch frame cost, with GPU execution timestamps unavailable. It does not establish live FPS, movement p95 or Person acceptance. [Full protocol, artifacts and remaining work](../../../../../plans/NORTHERN_VEIL_MINIMUM_WORK_IMPLEMENTATION_2026-10-10.md#independent-high-value-step-retained-gpu-time-only-preparation).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:15:29-07:00


### Saved-equation and overlap model — October 11

Zach requested a model of these particular aurora equations and their compiled overlaps. Read-only current-save decoding, production CPU lowering/proof construction and density evaluation establish four static folded shapes, each shared across its own D/extinction/scattering/emission channels; only emission pulses read time. All admission-box pairs overlap, but actual density intersections are Emerald/Violet and Emerald/Crimson. Cyan is separated from the other supports, and a real-valued derivative-bound model separates Violet/Crimson; that analysis is not yet an outward-rounded GPU certificate. The CPU evaluator confirms the inside benchmark eye has zero density in all four media even though it selects the conservative inside-box shader. Geometric support occupies at most 1.3–1.8% of each box; this is not an unremoved-work/time percentage. No authored occluder means no nested 24-step visibility march in this save.

[Full equations, coefficients, compiler counts, transport model, witnesses and next hypotheses](../../../../../audits/NORTHERN_VEIL_EQUATION_AND_OVERLAP_MODEL_2026-10-11.md). The model prioritizes testing generic support-aware inside traversal and Perlin-cell intermediate reuse; demand-first sharing is less compelling here because the one shared group is demanded by density. Current legacy source-chroma fallback behavior is recorded as a separate semantic discrepancy, not changed or used to infer zero illumination. No renderer/save changes or new GPU timings/FPS claims.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:52:28-07:00


### Exact support traversal retained — October 11

Zach asked to implement the highest likely value item first, then measure. Fused rays now retain each medium's certified support interval, jump over empty prefixes/tails inside admitted boxes, and reuse the grid-walk proof for density demand. Authored box events, midpoint lattice and channel/integration arithmetic stay intact. Unknown/unrevisioned and near-parallel inputs cannot certify support exclusion. Both cached lanes keep resident proof bounds, without warm uploads or recompilation; the single-medium inside traversal remains unchanged.

Eight final native captures match every RGBA byte; six focused targets pass, including expanded native proof 54/54. Two final sequential readback-free ABBA series per view pass retention with no other-view regression:

| View | Baseline mean ms | Candidate mean ms | Cost reduction | Separate series |
|---|---:|---:|---:|---|
| Spawn | 100.49 | 65.99 | 34.33% | 36.47% / 32.03% |
| Inside | 257.77 | 172.79 | 32.97% | 34.06% / 31.90% |
| Reverse | 231.98 | 158.28 | 31.77% | 32.86% / 30.68% |
| Side | 194.47 | 102.54 | 47.27% | 46.58% / 47.95% |


These are means of four run means each, including the documented first-spawn CPU preparation outlier; no minima/floor comparison. Costs include offscreen preparation/recording/submission/completion, with no reliable GPU timestamps. They are not live FPS or movement p95. App rebuilt; usual-size 30 FPS and temporal/movement acceptance remain open. [Full protocol, cache boundaries and artifacts](../../../../../audits/NORTHERN_VEIL_SUPPORT_TRAVERSAL_2026-10-11.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T02:25:43-07:00


### Perlin reuse implemented, measured and rejected — October 11

Zach commissioned the next minimum-work step after support traversal. Three exact ray caches (normalized corners, full-cell hashes and XY-only hash) matched all eight native captures, but repeated paired timings regressed; three GPU lookup variants passed compute bit checks but changed the inside image by one level. None was retained. All candidate code/resources/dependent test edits were removed, and the ordinary app rebuilt with retained support traversal. Seven focused checks pass using the existing Perlin ABC selector; eight fresh restored images match baseline bytes. The default terrain-upload assertion also fails in the unmodified baseline and remains open. Warm performance runs continue to show zero lowering/compiles/parameter/proof uploads. No new FPS claim or save edits. [Experiment details, paired means, baseline failure and handoff](../../../../../audits/NORTHERN_VEIL_PERLIN_REUSE_EXPERIMENTS_2026-10-11.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T10:29:00-07:00


### Selective folded-support index retained — October 11

Zach asked to implement the highest-value item grounded in the mapped aurora equations. The generic index follows unproven support through slices of the existing grid, with outside cost eligibility evaluated once per ray and an obligatory empty-leaf guard. Inside code stays original; existing residency/revision/extents/membership/device gates govern index lifetime. Eight full-resolution images match; six focused targets pass, including native proof 65/65 and a new 2560×1440 regression check.

Final paired offscreen means: spawn **61.35→55.36 ms (−9.77%)**, reverse −2.47%, side −0.27%; inside all sixteen runs **124.81→127.67 ms (+2.29%)**. Initial inside +7.76%/+1.02% prompted two additional predeclared series +0.57%/−0.13%; no runs removed, no repeatable >3% regression, no inside gain claimed. Warm lowerings, compiles and parameter/proof uploads remain zero. Direct-everywhere, interval caches and per-leaf cost selection fail exactness/cost/gain gates and are absent from the final path. [Proof/index contract, all runs and handoff](../../../../../audits/NORTHERN_VEIL_FOLDED_SUPPORT_2026-10-11.md).

App rebuilt, saves untouched, no commits/pushes. Person movement/fidelity and usual-size 30 FPS remain open; tighter proof contraction, demanded mathematics and certified tails are subsequent work. Fixed-view means establish none of those remaining milestones.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T13:27:06-07:00


### Whole-system counsel and CPU work screening — October 11

Zach asked to derive rendering, caching, compilation and evaluation together before another implementation round. [The signed counsel](../../../../../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#astra-counsel--derive-the-work-before-choosing-its-storage) ranks a backend-certified sample-local support guard ahead of further ray caches, conditional on production-kernel costs and sound numerical enclosures. [Fresh CPU screening](../../../../../audits/NORTHERN_VEIL_COUNSEL_WORK_MODEL_2026-10-11.md) found only 8.68% fewer modeled inside loop visits from a larger empty-block index, while a real-arithmetic 8 KiB support table could avoid 42.21% of modeled fold evaluations; neither is a GPU speedup or an admitted f32 certificate. The current host interval widening does not alone establish every GPU intrinsic error bound. Production code, authored saves and existing Person acceptance status are unchanged. Next: bounded kernel/certificate investigation, then one candidate with the existing exact-pixel and repeated-cost gates; follow the counsel for fallback, invalidation and alternatives.

**Signed:** Codex / GPT-6 / session `01a12cb8-ac95-7b13-827d-2c7f289aa560` / 2026-10-11T15:03:05-07:00.


## Sixth-one Sun implements Astra’s sample-local schedule

Following Zach’s request to reply, implement and reply again, retained the second-ranked Astra direction after deferring the uncertified projected guard. `composeVolumeSet` consumes each medium’s proof and contribution together and delays original direction math until positive density, preserving lattice/order/independent channels. Six focused targets pass with desktop native access; eight full-size images match every byte. Two ABBA/view show inside −9.77%, spawn −11.53%, reverse −6.44%, side −8.03%, zero warm uploads/compiles/lowering. [Audit, numerical admission issue, all means and future-agent handoff](../../../../../audits/NORTHERN_VEIL_SAMPLE_LOCAL_SCHEDULING_2026-10-11.md). Ordinary app rebuilt; live movement p95/30 FPS and Person appearance acceptance remain open. Projected support needs a justified backend numerical contract; direct parameter recipes/nested sharing/certified tails remain unfinished.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T17:22:37-07:00 (PDT).


### Person-reported live improvement after scheduling

Zach reports a “BIG IMPROVEMENT FROM THAT PASS” and approximately **21 FPS at `(-1.45, 0.3, -1.3)`** after the Sun's sample-local scheduling continuation. This is a Person observation, separate from the Sun's paired offscreen timing gains. Preserve the raw coordinates; framebuffer/view direction, sampling interval, moving/stationary state and running build were not specified, so neither a matched percentage gain nor movement p95 follows. [Person verification record](../../../For%20Zach/Person%20Verification%20List.md#northern-veil--zach-reports-approximately-21-fps-after-scheduling-pass); continuous appearance acceptance and the 30 FPS milestone remain open.

**Recorded by:** Codex / GPT-6 / session `01a12cb8-ac95-7b13-827d-2c7f289aa560` / 2026-10-11T18:05:55-07:00.


## Fragment demand testing pass — no production candidate retained

Implemented a reproducible read-only GPU fragment census, with paired pixels and counter-accounting checks. Its counters repeat, but instrumentation changes RGBA by at most one level; treat them as diagnostic rather than exact original-fragment evidence. Tested an independent sample-local empty-transport skip: eight exact full-size captures pass, but inside ABBA series are +8.02%/+1.04% slower, so it is rejected and production scheduling stays unchanged. No new support guard, cutoff, cache or save edit. [Full evidence and reproducible tool](../../../../../audits/NORTHERN_VEIL_FRAGMENT_DEMAND_2026-10-11.md). Actual live-view context, sound numerical support certificates and the movement milestone remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T20:53:25-07:00 (PDT).


**Person clarification — default-size live view:** Zach clarified that the approximately 21 FPS observation used the **default window size**, with **most auroras in view**; he did not track the exact viewing angle. This supplies qualitative window/view context. Physical framebuffer dimensions (including display scaling), native camera eye/look, motion interval and exact running build remain unmeasured; current running status was not answered. Do not substitute 2560×1440 or infer an exact camera orientation from this report.

**Recorded by:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T21:19:44-07:00 (PDT).


## Bounded certificate research — production admission remains zero

Implemented a generic AST-recognized scalar-support research tool with rational rounding/intrinsic enclosures, a conditional two-endpoint band and 18 passing witness groups. The complete production transformation/coordinate contract is unresolved, so **zero production skips are admitted** and the runtime candidate stops here. Conditional grid zeros are not remaining GPU demand or a performance result. AD-30–32 remain OPEN; retained rendering and saves are untouched. [Derivation, refusals, all coverage and next dependency](../../../../../audits/VOLUME_SUPPORT_NUMERICAL_CONTRACT_2026-10-12.md). Zach's default-window/most-auroras clarification remains qualitative live context; warmed movement and 30 FPS acceptance are still open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T00:43:07-07:00 (PDT).


## Native numerical profile follow-up — October 12

The retained wgpu-native v29.0.1.1 volume modules are now observed compiling with Metal Fast arithmetic and Fast functions on Apple M5. A scratch-only compiler observer preserves all arguments/options; eight paired 320×180 images match every byte. The rational tool passes 24 witness groups, including cancellation and uncertain coordinate-bin refusal. Complete Fast-mode transformation/producer enclosure remains unresolved, so no guard or new performance candidate is installed. Earlier local-grid figures remain conditional, with zero admitted production skips. [Native options, evidence limits and next dependency](../../../../../audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md). AD-30–36 remain open author review, not a blanket hold on independent engineering; existing movement/appearance acceptance remains open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T11:27:35-07:00 (PDT).


## Per-channel authored-math revisions (2026-10-12)

Zach asked which Prophetic / Formation Rete precedents map onto the renderer; he chose the third, Formation Rete Incremental Maintenance §2: "semantic deltas, not global revision panic."
- **Change:** `geom::FieldNode`'s single revision is now one revision per authored channel. Editing a curtain's emission (or chroma, phase…) no longer moves `densityRevision`, so the density zero-proof, its derived support envelope and the sharing plan for unchanged channels survive the edit. Identical reloads move nothing. The MCP `author_volume` path notes exactly the channels it wrote.
- **Witnesses:** `field_node_authored_revision_test` 30/30 and `webgpu_volume_zero_proof_test` 69/69. Through the real renderer, an emission edit builds 0 proofs and a density edit exactly 1, byte-identical to the exact path. Also passing: the volume, channel and save suite, including the Sun's `webgpu_volume_shared_subexpression_test` (65/65).
- **Not a frame-rate change:** this makes edits cheaper; steady frames are unaffected.
- **Uncommitted files:** `FieldNode.hpp/.cpp`, `VolumeDensity.hpp`, `EngineRender.cpp`, `WebSocketServer.cpp`, the two tests, and the ledger entry.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01NJy6VrPVNcHAnggwFyTsmF` · 2026-10-12



## Controlled arithmetic profiles rejected — October 12

Zach commissioned the next pass. A separate scratch driver tested volume-module Safe/Fast and Safe/Precise compilation against the retained renderer. Eight four-view/clock 320×180 captures per candidate fail exact fidelity; a 2560×1440 side-view comparison against the full accepted lattice also exceeds the one-level bounded limit. Maximum channel difference is 12 in both candidates. Unchanged-options controls match every byte. No cost benchmark follows failed fidelity; retained renderer, saves and Opus’s per-channel revisions remain unchanged. Twenty-four current numerical witnesses pass, but new support admission remains zero. [Candidates, measured failures, source/option checks and reproducible artifact paths](../../../../../audits/VOLUME_ARITHMETIC_PROFILE_FIDELITY_2026-10-12.md). Live movement/usual-framebuffer p95 and a complete retained Fast-profile support enclosure remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T17:13:57.815427-07:00 (PDT).


## Standalone evidence analysis — October 14

Zach commissioned a separate tracing/complexity-analysis executable. `scripts/earthcall-analyze` now joins imported timing, diagnostic demand, emitted WGSL and explicit model views in a searchable offline report, with 20 focused checks and real packaged imports of the saved eight-view census. This is generic developer tooling, adds no app instrumentation, and implements none of AD-30–36. It does not diagnose the current live bottleneck or claim a gain; fresh CPU/GPU capture and usual-size movement samples remain needed. [Separate task and input contracts](../Standalone_Performance_Analysis/Standalone_Performance_Analysis.md); [execution evidence](../../../../../audits/STANDALONE_PERFORMANCE_ANALYSIS_2026-10-14.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-14T22:37:18.690115-07:00 (PDT).


## Rendering/Intercom catch-up — October 15

Zach requested synchronization with Astra/Firmament/Opus. Existing CPU probe reruns reproduce the Sanctuary radial counts and the exact cloud-sharing output; current source confirms early chroma-group demand. Posted Northern Veil family/ray tables are not independently reproduced: the available probe skips a missing temporary input. Their operation counts do not by themselves prove economic rejection. A separate generic `ontoPow` source-contract discrepancy is tracked for CPU/native correctness follow-up. No renderer edit, new proof consumer, GPU measurement or gain; AD-30–36 remain open. [Full checked scope and reply](../../../../../audits/RENDERING_INTERCOM_CATCHUP_2026-10-15.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T01:39:25.380709-07:00 (PDT).

## Demand and support implementation plan — October 15

Zach requested a plan after Firmament supplied replacement probe provenance and accepted the earlier qualifications. He permits planning across **earlier** AD-30–36, with verdicts later; this does not pre-approve later discoveries or implement unresolved choices. The [new implementation plan](../../../../../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md) sequences power correctness and a compatible baseline, compiled per-group demand placement, product-local arithmetic experiments, qualified Euclidean proofs and dependency-directed repair. It separates Sanctuary's verified emitted color-demand opportunity from Northern Veil's one-group workload, keeps native omission conditional on the full contract, and specifies exact/native/economic/movement gates. All implementation stages remain pending; no new FPS or runtime gain. The replacement Northern Veil simulator source is inspected/fingerprinted but not rebuilt/rerun in this planning pass. Existing appearance and movement acceptance remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T03:21:44.030398-07:00 (PDT).

## Commissioned implementation — power retained, optimization trials rejected

The [October 15 implementation audit](../../../../../audits/VOLUME_POWER_AND_OPTIMIZATION_TRIALS_2026-10-15.md) records independent reproduction of Firmament's previously missing Northern Veil tables and three actual native trials. Demand scheduling was exact but missed the performance gate; the coarse AND index was exact but repeatably slower; product-local reuse reduced a cloud call but changed cloud bytes and was rejected before timing. All three production candidates were removed. Their measured results are not new aurora gains and the coarse trial is distinct from Firmament's finer XZ proposal.

Retained work is generic power route/domain correctness, CPU/native witnesses and an external repeatable experiment harness. Existing support traversal/residency are preserved. No new AD verdict, qualified proof consumer, certified tail, live FPS or 30 FPS acceptance. The next stages remain explicit in the [plan](../../../../../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md#12-implementation-results--october-15).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T05:56:25.134858-07:00 (PDT).


## Scoped Euclidean host analysis — next rung

Zach commissioned the next rung. [Implementation and executed evidence](../../../../../audits/VOLUME_EUCLIDEAN_RANGE_2026-10-15.md) add generic vector Length/Distance host range rules behind an explicitly named analysis entrypoint. Intermediate rounding, square/subtraction/sum overflow, types and unbound inputs are handled conservatively. The cloud/halo coverage gain is reproduced; these four auroras gain no extra cells. Existing volume proof payloads match the pre-change implementation and native proof/image regressions pass. The renderer does not consume the new theorem. No new FPS gain, movement acceptance or author verdict; native scope/arithmetic qualification remains the next dependent rung.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T10:45:48.355899-07:00 (PDT).
