# §6cy — review and correction of the work census

Zach commissions another review/analysis/feedback/next-steps pass after Sparkly commit `9d1e35882`. His criterion remains current-or-better authored-equation accuracy with a substantial native Northern Veil performance checkpoint. Parents: [Sparkly §6cx](GPU_RAY_TRANSPORT_WORK_CENSUS_2026-10-22.md), [§6cw review/assignment](GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22.md), [Firmament counsel](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CW_WORK_CENSUS_AND_ON_THE_FLY_SWEEP_2026-10-22.md), and [existing Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). The counsel file existed only in default; it is now relayed into the ray checkout so the plan link resolves there too.

## Review verdict and useful progress

**The capture qualification is real.** Independently replayed actual fused packets agree with the guarded schedule on32784 distinct native rays. All12102 matched affine endpoints remain and outer support unions are unchanged. Capacity64/16 is visible in the exact compiled shader. Normal headers are read before forced fallback; actual trace headers are now also read and compared. The corrected native run preserves guarded RGB/T outputs bit-for-bit, including instrumented trace/fused-census outputs; instrumented current outputs match the native current baseline. Three tile capacities retain complete normal/forced output parity.192 predetermined holdout rays pass both65536/32768 references against captured and contemporary baselines. These are compute captures, not full native frames.

**The main work counts survive correction.** Current and candidate do different work; fewer calls in one category do not prove a lower total cost.

|Counter across32784 rays|Corrected candidate|Current marcher|
|---|---:|---:|
|Noise function calls|496468|956301|
|Consumer envelope evaluations / current density evaluations|509044|2102076|
|Producer curve `evalPhi` evaluations|2606763|No corresponding root producer|

Noise calls fall48.1%; the consumer point-count ratio is4.13×. Current density and candidate envelope functions are not identical-cost units, and the candidate additionally performs2.607million curve evaluations, each doing transcendental arithmetic. These counters establish work composition, not that total mathematics is cheaper or which operation dominates elapsed time. The old nine-setting stream timings still lose; they were rerun unconditionally by the original census executable despite its prose saying no sweep. The corrected diagnostic disables that stream sweep. Inherited bulk/component timing code still executes; its numbers are not used as new candidate economics.

## Findings and implemented corrective repairs

1. **Failed controls were not admission failures.** Original C++ merely stored budget `pass` and capacity `fallback_bit_exact_pass`; original Python called an admission function that ignored both. The exact old admission still accepts an otherwise valid historical report with failed capacity parity injected. Controls now throw on failure/non-vacuity, and independent Python admission checks them. Capacity outputs are poisoned before dispatch and checked on EVERY ray against native current when refused and guarded candidate otherwise. Raw flags/results are exported; expected capacity reason16/32 is checked. Native controls retain budget23/24/25 refusals16/0/0, single-cap4 fallback26243, overlap-cap1 fallback412, with exact mixed results.

2. **Coarse reject and stack counters were wrong.** `coarseBinsBowRejected` incremented on refined cells too: a per-ray maximum105 exceeded the maximum96 coarse bins. Restricting increments to `depth==0` changes total2389859 to2332420 and maximum105 to93. Stack maxima previously updated only on splitting, incorrectly reporting0 for rays whose initial root stack held one item. They now observe actual stack use: minimum/p50 become1 rather than0; max remains6. Depth is observed when popping, bisection iterations are counted at loop entry separately from evaluations, and skipped-index counts are clamped to the accepted lattice without changing renderer step arithmetic. The main Phi/noise/density totals remain unchanged on this capture.

3. **Width was mislabeled as provenance.** The code classifies only `hi-lo`. Correct independent width counts are37808 at≤2cm,11515 at2–5cm,5238 at5–20cm and192765 at>20cm. That does not identify which root level, uncertainty tag, affine cut or subdivision created them. Firmament’s earlier54725 microinterval estimate is not this executed count; this capture has49323 intervals≤5cm. The corrected report/console calls these width bins. No apex knot or uncertainty-band boundary may be removed on the strength of width alone.

4. **Raw evidence and actual flag checks were incomplete.** Original reports kept only aggregate census statistics, no per-ray counters; forced fallback count was assigned `count` rather than read. Corrected reports export24 producer/consumer counters and4 current counters per ray, actual normal headers at all capacities, instrumented outputs, and capacity flags/results. Independent replay recomputes all statistics and histograms and verifies source/packet/consumer accounting. Counters remain per-ray, not per-member; the earlier claim of a complete per-member census is not established. Forced headers are now actually read. The64/16 check no longer substitutes a hardcoded shader when evidence is missing, and stale32/8/global-storage metadata are corrected.

5. **Harness stewardship and lifetime issues.** The old runner overwrote existing work and deleted existing output, and checked case identity only before compilation. It now refuses non-fresh work/output and unsafe labels, rechecks the pinned fixture around execution, and saves independent admission. Two duplicate diagnostic allocations leaked their first buffer handles; removed. Instrumentation arrays are Kernel diagnostic scratch initialized/exported per ray and dispatch, with no authored state or persistent cache.

6. **Physical causation and universal parity remain open.** Resource data were recorded unavailable; this pass independently observes `xcrun -sdk macosx --find metal` refusing the absent tool. No register/spill/occupancy measurement isolates the slowdown. Source-visible1280-byte schedule arrays do not establish physical register allocation or spill bytes, and removing them does not remove knots/pieces/root-stack/noise state. Firmament’s chronological-stream insight is sound as a representation proposal; its universal IEEE bit-equality claim must still be tested under the actual compiled shader. Sparkly’s comparison of fewer noise calls against current cannot explain why two candidates performing the same mathematics differ in time. Old buffered-stream versus bulk also changes chunk/pass organization, so it is not a clean buffer-only causal experiment.

## Executed verification and preserved evidence

One bounded corrected native diagnostic run, native timestamps off; no on-the-fly candidate implemented or timed. Both original and corrected compiled source bytes/identities and value-preserving raw reports are retained. Corrected source SHA256: `24f0b69217ed7349cef5e6261977d96ca225ae0e6e6a41c01f5b7b4778b94b3f`. [Machine provenance](GPU_RAY_TRANSPORT_CENSUS_REVIEW_2026-10-23.json), [portable bundle](GPU_RAY_TRANSPORT_CENSUS_REVIEW_2026-10-23.zip), [executed replay](GPU_RAY_TRANSPORT_CENSUS_REVIEW_2026-10-23_REPLAY.json), `scratch/probes/ray_transport_gpu_census_review.py`. Fifteen documents share12 arrays,13.85MB compressed; JSON values reconstruct losslessly and compiled source bytes are exact. Existing guarded archives are hash-checked dependencies. Three historical failed-control refusals and eight corrected raw/control corruption refusals pass. Native AST assertions are recorded and transfer through exact output equality; AST nodes are not independently reevaluated offline. Dense reference covers192 rays, not the full32784.

```sh
python3 -B scratch/probes/ray_transport_gpu_census_review.py --root . --record docs/audits/GPU_RAY_TRANSPORT_CENSUS_REVIEW_2026-10-23.json --output /private/tmp/census-review-fresh.json
```

A new native diagnostic from the ray checkout requires the pinned temporary case, configured build and exclusive native lane; use a fresh work directory:

```sh
python3 -B scratch/probes/ray_transport_gpu_work_census.py --root . --work /private/tmp/census-corrected-fresh --case /private/tmp/earthcall-wide-case/manifest.json --label diagnostic
```

Native lane is released. No production/default/save/live-data/app change, new FPS gain or Person acceptance. Existing inhabited movement checks remain open; there is no new app-visible behavior for Zach to verify in this pass.

## Next direction: approve one bounded on-the-fly experiment

Sparkly executes; Firmament reviews; Codex has completed the corrective pass. Test elimination of the two local schedule arrays and their re-merge, keeping root construction, sorted knots, active masks, protected boundaries, error model and64/16 capacity accounting unchanged. This targets source-visible redundant buffering, without claiming a proven spill cause.

**Critical tightening:** a single-member event span `[a,b]` is NOT necessarily one existing quadrature panel. Preserve `n=max(1,ceil((b-a)/4))`, the exact existing `k=0..n-1` endpoint arithmetic/selects, and one original single-panel transport update per emitted subpanel. Collapsing that entire span into one update changes the integration rule. For overlap use the exact existing `advanceOverlap` sequence, member order and shared attenuation. Preserve chronological L/T accumulation and small-depth gain arithmetic. Write result once only after success; late capacity/budget failure discards all partial L/T and leaves the unchanged fallback to write the whole result. Keep every flag writer/reader explicit.

First prove structural emission order through a separate untimed export and strict actual uninstrumented outputs, not just real-number equivalence. Test spans requiring n>1, interleaved single/overlap, touching/duplicate event knots, short/protected intervals, empty rays, final16-ray tail and late fallback. Retain all-query AST guards, both-resolution192-ray holdout against captured/contemporary baselines, poisoned normal/forced/mixed output checks, all12102 matched affine boundaries and outer support unions. No knot removal, quadrature retuning, culling/secant/noise/lighting change.

For cost, compare current; guarded bulk control; buffered fused64/16; on-the-fly fused64/16 in the SAME process with identical rays, chunking, fallback passes and preparation. Buffered versus on-the-fly must have identical pass/chunk organization to test this representation change. Start with the primary32768-capacity/two-chunk16-tail configuration and bounded paired order/repetitions. Do not reuse historical absolute ms as a target or subtract different processes. Stop on clear loss; a possible win needs a second clean paired series before other views/clocks/full frame. Beating buffered/bulk but losing current is not gate1 completion. Full directions are in both Intercom copies.

All three shipping gates remain **OPEN**: profitable repeatable connected distinct-ray execution; generic OntoMath source/density/root and revision/clock/device lifetimes plus full viewport/depth/composition/frame economics; moving inhabited p95 and Zach acceptance. OntoMath owns generic meaning; Screen lowers execution. The checkpoint remains frozen-fixture compute evidence.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-23T00:40:20.361400-07:00.
