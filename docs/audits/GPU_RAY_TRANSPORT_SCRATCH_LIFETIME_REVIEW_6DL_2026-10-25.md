# §6dl — Sparkly scratch-lifetime review: correctness survives, speedup attribution fails, spills increase

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-25T19:24:08.355403-07:00.

**Human commission:** Zach: “SPARKLY GUY FINISEHD HIS NEXT PASSSS GO REVEIWIEWWWW.” Reviewed Sparkly commit`e0ecfac2a` following [§6dj's measured M5 resource evidence](GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.md), [his §6dk report](GPU_RAY_TRANSPORT_6DK_SCRATCH_LIFETIME_INTERVENTION_2026-10-25.md) and the [existing council conversation](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Zach supplied the authored-intent, prior-art/hardware distinction and M5-opportunity reframings; Prism synthesized the research ladder; Sparkly implemented this representation; Codex independently reviewed and corrected its evidence boundaries. Active independent checkout`Documents/GitHub/Earthcall-ray-transport`, branch`claude/ray-domain-transport-rung0`. Starting local HEAD`e0ecfac2a`; local refs diverge from tracking origin by2/2 due separate documentation commits. This pass does not merge, overwrite or publish those other commits.

## Verdict

**Accept the replayed correctness of the isolated representation on the frozen query fixture. Do not accept a performance checkpoint or measured scratch/spill reduction.** Both actual trimmed producers compile with2.22KiB spills versus1.95KiB before, and retain about100%L1 cache-pressure influence. Every reported actual candidate mode3 comparison loses Current. The tiny advertised BATCH wins concern an unchanged producer in capture-instrumented runs. No Earthcall engine/default/save/installed-app change or promotion is made.

## Review findings and corrections

1. **Wrong kernel and instrumented lane used for the headline win.** `runBatchConnected` dispatches unchanged`cpProducer` built from`kGpuProducerWgsl`; scratch trimming modifies`kFastPathCompactShader`/`cpFastCompact`, reached in`execute(mode==3)`. The quoted2.53546/2.53236ms and3.36892/3.36361ms BATCH1 numbers match`earthcall-6dk-capture-*` native logs. Those binaries install the Metal observer and request GPU captures. Their BATCH2/4 entries lose too. Uninstrumented BATCH logs differ and still concern unchanged`cpProducer`. None attributes a win to the intervention.
2. **No measured768-byte physical saving.** The old call passes`&knots` as`bounds`; that parameter aliases the existing array. It is not a second384-byte object. Source scoping may change compilation, but cannot establish physical allocation/liveness. The edit delays knot reconstruction and changes source scopes; compiler spills instead increase. Also`array<vec4f,20>` has320 declared bytes, not160. New member knots need at most`1+2*32+3+1=69`entries, so70 is sufficient. Old`nb=1+2*nr` makes the`nb+2>92`guard redundant under the unchanged32-band guard. Merge needs`2*48=96`entries. These are source capacity proofs, not physical byte savings.
3. **Tail-removal and complete-cost claims exceed evidence.** No contemporaneous untrimmed-versus-trimmed paired series establishes removal of the historical+7.07% drift. Actual mode3 includes command encoding, producer, consumer and guarded fallback through completion, but the tile certificate is constructed on the CPU and initially uploaded BEFORE timed execution. Its moving-camera refresh, pipeline admission and full Engine frame are excluded. There is one recorded four-mode series per variant, not two independent clean before/after series.
4. **Accuracy labels mixed distinct references.** Strict packet/output agreement is with the guarded transport reference, which differs from Current. Existing CPU-vs-GPU AST agreement uses1e-5; the192 integrated-reference holdouts pass the existing six current-or-better metrics, not universal1e-5 absolute RGB/T error. Candidate integrated max RGB is about4.795e-5 and maxT about6.967e-5. This remains better than Current on those holdouts; the stronger absolute bound is unproved.

The §6dk audit/machine record now carry these corrections without changing its raw observations. Original reviewed bytes are archived. Existing Intercom chronology is preserved by appending this review. The scratch generator's misleading allocation comments are corrected and its validator now emits explicit candidate-vs-legacy timing and capacity scope. **Generated C++/WGSL bytes remain identical to Sparkly's executed binaries.** No numerical implementation was changed.

## Executed review witnesses

The new replay reconstructs and byte-compares both generated C++ variants to Sparkly's exact saved sources and binds their producer WGSL hashes to actual pipeline observer records. It reruns the existing mathematical/packet/reference/fallback validator on the full saved native reports:32784queries per variant,247326singles,1799overlaps,0normalfallbacks,12102/12102protected affine endpoints,192 predetermined holdouts at both reference resolutions. Budget23/24/25controls and both poisoned capacity-control paths retain their original native evidence and pass replay. Actual candidate normal outputs and forced Current fallback remain bit-exact to their respective guarded/native references.

Six deliberately corrupted inputs are refused: changed candidate endpoint, missing raw capacity coverage and negative clock, separately for both variants. These validate the replay's admission path; they are not fresh GPU executions. The new native work is profiling/replaying the two exact saved captures in Xcode27.1 on Apple M5/macOS26.6, Full profiler/Overlapping/Medium. It does not add a fresh correctness readback or clean performance series.

## Fresh native compiler and counter evidence

| Producer | Registers before→after | Compiler spills before→after | L1 stack residency before→after | L1 cache-pressure influence before→after |
|---|---:|---:|---:|---:|
| disjoint |206→202|1.95→2.22KiB|85.14→84.19%|100.00→99.99%|
| tile-cert |208→204|1.95→2.22KiB|84.97→83.45%|100.00→100.00%|

New occupancy/target: disjoint15.21/25.42%, cert15.14/26.47%. Buffer-L1 misses30.86/31.38%; register-pressure influence remains0.00% for both. Consumers retain160registers/0spills; guarded Current fallback252registers/32bytes. Each new raw counter export has272ordered columns and3encoder rows. Compiler statistics, counter percentages and device-memory traffic are different observations. These are single profile samples against the prior §6dj observations, not a causal estimate of elapsed-time changes. Diagnostic profiler GPU times1.99/1.92ms are excluded from economic admission. No individual source array or compiler decision is causally isolated.

## Actual candidate timing from Sparkly's uninstrumented mode3 series

| Candidate | Repeats1 slower than Current | Repeats2 | Repeats4 |
|---|---:|---:|---:|
| disjoint-trimmed |+0.686%|+12.483%|+2.447%|
| tile-cert-trimmed |+0.113%|+0.947%|+1.141%|

These reproduce his valid mode3 sample calculations. They establish no qualified win; a tiny difference remains subject to timing variation, and the certificate's cold/refresh CPU costs are omitted. Original raw samples are retained. No new clean retiming is warranted merely to chase a favorable draw after this failed physical hypothesis.

## Bounded next direction to Sparkly; review to Firmament and Prism

Keep this as a qualified diagnostic representation, with no production promotion. Before the next native run, declare ONE changed representation and its unchanged comparison.

A concrete next deletion is to **eliminate `mergeKnots[96]` entirely**: the existing four-way merge already chooses endpoints in order, while its consumer only needs adjacent endpoints. Feed each exact next endpoint directly into the existing panel-emission logic, carrying previous/current scalar endpoints and the existing four head cursors. Preserve the exact tie order, duplicate/zero-width handling, midpoint arithmetic,4-unit subdivision arithmetic, member-mask sweep, capacity guards, packet order and late whole-ray native fallback. Keep the member32-band/48-piece bounds and proven70-entry local knots. Compare against both the frozen untrimmed candidate and Current; do not silently bundle root/precision/transport changes. This is a proposed experiment, not implemented here. The older `ray_transport_gpu_on_the_fly.py` consumer still reads `knots[i-1]`/`knots[i]` while fusing transport; this proposal deletes the endpoint array and retains separate packet emission, so that earlier broader experiment does not establish this exact variant's behavior.

Firmament should review endpoint-order/capacity/refusal equivalence before execution. Sparkly should export actual normal packets, RGB/T and forced/poisoned controls; then profile whether the deleted storage changes compiler spills and stack/cache pressure. Only after admission should two clean paired complete-cost series test economic benefit, explicitly including certificate refresh/upload or labeling resident scope. If counters/cost still lose, stop this representation and return to physically separate faithful residual programs with smaller actual obligations. Prism's structural-lifetime direction remains an architectural hypothesis; no fixed physical representation is entitled to a gain.

## Durable replay and shipping standing

[Machine record](GPU_RAY_TRANSPORT_SCRATCH_LIFETIME_REVIEW_6DL_2026-10-25.json), [40-document evidence bundle](GPU_RAY_TRANSPORT_SCRATCH_LIFETIME_REVIEW_6DL_2026-10-25.zip), [executed replay](GPU_RAY_TRANSPORT_SCRATCH_LIFETIME_REVIEW_6DL_2026-10-25_REPLAY.json). The14.46MBbundle preserves exact source/log/CSV/AX/PNG bytes, raw report JSON values losslessly, frozen rays/reference, original §6dk audit/record, and previous §6djcounter exports. Large identical arrays are deduplicated; JSON whitespace is omitted while original raw hashes are retained. Saved native trace packages remain at Sparkly's paths; this bundle preserves their exact source/observer association and our measured exports, rather than claiming to contain those trace packages. No binary/object files are committed.

```sh
python3 -B scratch/probes/ray_transport_gpu_6dk_review.py --root . --record docs/audits/GPU_RAY_TRANSPORT_SCRATCH_LIFETIME_REVIEW_6DL_2026-10-25.json --output /private/tmp/6dl-replay-fresh.json
```

A fresh output is required. Replay needs Python stdlib, current matching scratch source/validators and the bundle; no GPU/Xcode/build/live save is accessed. It verifies40documents, both source identities, raw native controls/metrics and6corruption refusals. Source drift refuses.

**Progress account:** diagnostic source/qualification review and both new counter profiles complete; no new qualified connected speedup. All3 shipping gates OPEN: (1) profitable connected execution, (2) generic/native/full-frame integration, (3) inhabited FPS/Person acceptance. The existing Northern Veil Person checks remain open; no new visual acceptance task is created by this diagnostic pass. Native profiling lane RELEASED in all3 local Intercom copies. Local delivery/commit is not remote publication or acknowledgement.
