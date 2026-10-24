# Northern Veil: exact support traversal retained

Zach commissioned the highest likely value remaining optimization and explicitly asked to implement it before measuring before/after. This continues [the existing Sun thread](../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-support-traversal-retained) and the [saved-equation model](NORTHERN_VEIL_EQUATION_AND_OVERLAP_MODEL_2026-10-11.md). The model identified thin positive support inside large overlapping admission boxes; the benchmark's inside eye is actually in empty density. This pass implements generic compiler/proof machinery, with no Northern Veil-specific equation or drawing rule.

## What changed

The fused volume compiler computes each medium's certified support interval once per ray. Before support entry, traversal can jump directly toward that entry; after support exit or with wholly empty support, that medium imposes no further cell-by-cell walking in the current authored segment. When all held media certify zero, the loop jumps to the earliest possible contribution among them. Inside possible support, the established leaf-cell proof remains authoritative. Its result is reused for density demand, removing the second packed-grid lookup previously performed at an occupied sample.

Authored box events, chords, segment counts, midpoint positions, channel arithmetic and integration order stay intact. The index jump selects an existing sample; it does not stretch a step, move samples or replace box events with support bounds. Source/occluder domains and independent clocks remain unchanged. This is zero-contribution acceleration, separate from the positive output-error budget and unfinished contribution-tail certification.

Absent/unrevisioned proofs and near-parallel rays receive an unbounded interval for fused support exclusion. The original authored box events still delimit those domains. This prevents a rounded substitute-direction/box intersection from accidentally certifying zero. Existing leaf-cell fallback continues to apply. The last guard was added after the initial prototype, then native parity and both final ABBA series were rerun.

Six local support-bound floats now follow every proof payload in both camera lanes. Authored parameter order is unchanged; absolute proof offsets are rebuilt under the existing density revision/half-extent/membership identities. Stable parameters and proofs stay resident. Support intervals and proof flags are per-ray/sample GPU derived values, with no new authored state, serialized fields or cache lifetime. Existing prewarming and device teardown remain authoritative. Outside global prefix/tail trimming remains a separate camera specialization; this pass does not re-enable the rejected always-on global prepass inside boxes. The single-medium inside marcher is unchanged apart from the common proof payload format.

## Native fidelity and integration

The rebuilt app and six focused CPU/native targets pass, with no skips: parameter refresh; CPU sharing 55/55; CPU proof 24/24; native sharing 65/65; native quadrature 15/15; native proof 54/54. Added proof cases enter actual Emerald/Violet and Emerald/Crimson support, retain unknown temporal density inside a box, mix known/unknown support in both member orders, and compare proof-on/off in positive-budget full-lattice mode. Existing witnesses retain axis-parallel rays, all-empty support, zero-to-positive density revisions, camera-lane reuse, independent clocks, reload and device replacement.

All eight complete 2560×1440 RGBA captures (spawn/inside/reverse/side, t=0 and 7.25) match retained baseline captures byte for byte: 14,745,600 bytes each, max error 0. The frozen current baseline was already byte-identical to those captures in the preceding time-preparation stage. Finite captures supplement the conservative proof contract; they do not prove every possible expression or continuous integral. Original read-only `nv_ref.cpp`/probe sources and authored save bytes were untouched.

## Sequential before/after economics

Apple M5, native Metal/WebGPU, current Debug configuration; four curtains, fixed t=7.25, 2560×1440. Frozen baseline includes retained compilation, residency, camera prewarming and GPU time-only preparation. Constructor layouts match their linked renderer objects. One warm frame precedes each batch; admission prewarms both camera variants. Two A–B–B–A series per view, 24 timed frames/run, execute sequentially and separately from image readback and GPU correctness tests. Ordinary GPU timestamp features remain disabled. Batch-completion fences belong only to the benchmark.

This is completion-fenced **offscreen volume-scene frame cost**, including preparation, recording/submission and GPU completion, not separated volume GPU execution time, live surface acquisition, moving-world frame p95 or Zach's window FPS. Each table mean uses all four run means for that version; these are not minima or floor timings.

| View | Baseline mean ms | Candidate mean ms | Cost reduction | Separate series |
|---|---:|---:|---:|---|
| Spawn | 100.49 | 65.99 | 34.33% | 36.47% / 32.03% |
| Inside | 257.77 | 172.79 | 32.97% | 34.06% / 31.90% |
| Reverse | 231.98 | 158.28 | 31.77% | 32.86% / 30.68% |
| Side | 194.47 | 102.54 | 47.27% | 46.58% / 47.95% |


Every final series exceeds the 5% cost gate and improves all views; there is no repeated >3% other-view regression. Retain the candidate on these fixed-view results. All 32 warm runs report zero lowering, shader compilation and CPU parameter/proof upload bytes. The first final spawn baseline has a preparation outlier (8.222167 ms versus mostly hundredths of a millisecond); it is included rather than silently discarded. The second spawn series independently exceeds the retention gate. Absolute costs drifted upward during the session; compare paired final series, not earlier prototype/historical timings. The two optimizations were measured together; this pass does not assign isolated percentages to support jumps versus lookup reuse.

Scratch records: `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/inside-support-cost-results.json`, `inside-support-cost-summary.json`, `inside-support-default-results.json`, `inside-support-focused-tests.log`, `inside-support-provenance.json`, and `inside-support-baseline/provenance.json`. Initial prototype results before the final fallback guard are separately preserved as `inside-support-prototype-*` and are not this final table. Source and binary SHA256 values distinguish them.

## Person acceptance and remaining work

The ordinary app is rebuilt. Relaunch it, load Northern Veil and move from spawn through both curtain overlaps, reverse and side views. The aurora should keep the same detail, color and opacity, with less traversal cost; check continuous animation for flicker/disappearance and warm boundary crossings for hitches. Record framebuffer dimensions and frame-time/FPS distribution in the ordinary performance window. Offscreen images do not establish live temporal coherence.

Thirty FPS inhabitation remains unverified: these fixed-view batch times exceed 33.3 ms, the usual framebuffer size is not independently confirmed, and no new native moving route or Person acceptance was collected. Remaining work includes exact Perlin-cell intermediate reuse, direct retained parameter recipes, exact demand sharing and certified contribution-tail bounds. The legacy source-chroma fallback discrepancy stays a separate semantic issue; it was neither changed nor used to justify excluding scattering.

Save SHA256 remains `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. No save, codec, authority, resolution or sample-count change; no commits/pushes. Concurrent Storage/save-format edits were preserved.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T02:25:43-07:00
