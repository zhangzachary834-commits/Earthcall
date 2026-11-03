# §6dv — actual endpoint/ownership qualification and corrected §6du evidence boundaries

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-26T18:30:49.134534-07:00.

Zach asked: “CHECK IF SPARKLY GUY IS DONE … IF HE IS THEN DO UR OWN PASS.” Sparkly's completed `1f29da123` was present on `claude/ray-domain-transport-rung0`, so this is the next corrective pass, responding to [his §6du qualification](GPU_RAY_TRANSPORT_6DU_CORRECTIVE_QUALIFICATION_2026-10-26.md) and [the Firmament's review](FIRMAMENT_EVALUATION_OF_6DT_M5_PLIST_TELEMETRY_AND_6DU_DIRECTIONS_2026-10-26.md). No engine, authored world, installed app, default lane or rendering equation is changed.

## Accepted result and new native witness

Sparkly moved the final-publication atomic to the actual Current/fallback output sites, poisoned packet storage, and retained explicit completed/refused ownership. These improvements survive a rebuilt Apple M5 correctness run. The corrected driver passes all 30 cases: six pose entries × repeats 1/2/4/9, plus four budget and two capacity controls. Comparisons cover 32,784 sampled queries, with poisoned RGB/T, exactly-one producer entry and final-publication counters, mutually exclusive ownership flags, accepted packet comparisons and Current fallback output. Counters are read after the final repetition of each case; this is not an export of every intermediate repetition's write history.

A further endpoint gap was found: the clean benchmark reconstructs terminal S1 packet records from the reference because the real producer no longer materializes them. Those records alone cannot prove the candidate's actual protected endpoints. The controls now export the exact `s0`/`s1` consumed by the terminal integrator into a separate diagnostic tail of the existing audit buffer, and compare their bits directly with contemporary guarded packets. The original pose compares **81,087 actual S1 subpanels** on **12,889 nonempty terminal rays**. Across all 30 cases, **1,073,023 actual terminal subpanels** compare successfully. The separate `sBuf` region remains poisoned for completed S1. This diagnostic export is not added to clean shaders or clean timings.

Seven rebuilt negative variants are required to fail with their named native assertion: `no-reset`, `no-tail`, `double-publish`, `missing-publish`, `fallback-missing-publish`, `fallback-double-publish`, and the new `terminal-endpoint-corrupt`. The seventh changes only captured endpoint bits, leaving transport arithmetic untouched; its failure must report **zero output differences and nonzero packet differences**. This catches the precise endpoint blind spot even when rendered output still agrees.

The driver also refuses an existing work directory instead of recursively deleting it, bounds-checks terminal packet storage, requires zero overlap count for completed S1, and makes a missing fallback mutation hook fatal. My first additional check incorrectly required identical offsets in reduced-capacity cases; capacity changes intentionally alter the layout. It was corrected to check storage bounds, counts and actual endpoint values, and the final matrix was rerun.

## What the two saved timing invocations actually establish

Both `/private/tmp/rt_6du_bench` and `/private/tmp/rt_6du_bench2` contain separate native invocations of the same reconstructed 275,009-byte generated C++ program. Each invocation contains **two internal ordered blocks**, each with 32 total observations per repeat: **8 per mode**, pooled to **16 per mode**. The published §6du JSON/table uses only the second invocation. Its “pooled 64 samples” means 64 across all four modes, not 64 per mode. The portable replay retains and checks both invocations, complete raw reports, query outputs, reference data and source identity.

Percentages below compare each candidate with Current in the same invocation and repeat; negative means faster. These are submit-to-completion host clocks, including GPU execution and the host completion wait, excluding prior host preparation/upload/compilation. They are neither pure GPU timestamps nor full-frame/FPS measurements.

| Saved invocation | Repeats | Routed unfused mean / median | Terminal S1 mean / median |
|---|---:|---:|---:|
| first | 1 | +0.056% / +0.028% | +1.342% / −0.193% |
| first | 2 | +1.129% / +3.352% | +8.279% / +19.492% |
| first | 4 | −2.220% / +0.558% | +11.791% / +18.470% |
| second, published | 1 | +5.906% / +0.234% | +6.060% / +0.194% |
| second, published | 2 | −9.291% / −1.422% | +2.587% / +1.644% |
| second, published | 4 | −5.234% / +0.932% | +3.790% / +18.429% |

There is **no repeated mean-and-median win over Current**. Isolated favorable pooled means do not establish a shippable gain. The two saved invocations also pass the existing 32,784-query/192 dual-resolution holdout admission and 12,102 protected-affine checks within that harness's stated scope; reference-filled S1 packet entries are distinguished from the new actual endpoint capture. These remain sampled fixture checks, not a complete-image or universal expression guarantee. No new clean timing run was performed in §6dv.

## Reporting and causal corrections

1. The old generator unconditionally wrote `thirty_case_matrix_passed: true`, negative-control success strings and a compiler table, despite not running/loading those external controls or capturing a hardware profile. It also overwrote the committed audit path. The corrected generator writes `qualification.json` only in the external work directory, with actual validated results, source/artifact hashes and explicit **not run/not captured** external evidence statuses. Original §6du records are preserved in the archive.
2. The old `measurePass` repeats the classifier 16 times per submission and 16 samples without resetting its append counters. Later indirect dispatches therefore do not use the qualified 2,304-tile routing state. Its phase table is rejected. Separate phase completion clocks cannot be summed into connected cost. The broken phase loop is removed; cached-classification timing is also disabled until independently qualified for output and invalidation.
3. The corrected timing path retains chronological encode/completion/total observations for both internal blocks and states that they share one invocation. A lower Current completion time does not itself demonstrate unthrottled GPU frequency or identify scheduler causation.
4. Historical compiler tables report 2,000-byte fields on several producer kernels and zero on Current. This does not isolate all 2,000 bytes to four source arrays, prove quadrature added zero bytes in a controlled same-source comparison, measure dynamic spill traffic, or prove every winning representation must eliminate every array. The four named declarations total 1,344 logical bytes; adding the cited 36-byte knot samples and 12-byte depth bits yields 1,392 logical bytes, not a physical spill-layout proof. Other differences between kernels remain confounders. No new hardware counter/profile claim is made here.
5. The rewritten Mode 2 S1 producer has original-pose composed checks, but the 30-case terminal control driver executes Mode 3. The entire changed Mode 2 path must receive its own matrix before its generic control coverage can be claimed.

The corrected clean benchmark compiles against the explicit source-matching engine build. All **17 embedded WGSL blocks are byte-identical** to Sparkly's captured benchmark. Source tree identity remains `3603add4018961529a91bc9592d76f9a553500b1`; clean transport code is unchanged. Diagnostic endpoint export is separately instrumented and is not a clean performance witness.

## Reproduction and durable evidence

The adjacent [record](GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.json) indexes a lossless [archive](GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip); large raw JSON is compressed rather than committed as an oversized blob. The replay checks hashes, reconstructs the saved benchmark source, replays both native admission results and timing statistics, checks native matrix/negative artifacts, and rejects nine deliberate corruptions.

```sh
python3 -B scratch/probes/ray_transport_gpu_6dv_review_replay.py --root . \
  --record docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.json \
  --archive docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip \
  --output /private/tmp/6dv-replay.json --self-test
```

For a fresh correctness execution, use `ray_transport_gpu_6du_controls.py --root . --build-root <source-matching-checkout> --work <fresh-external-directory> --mutation fixed --run`. The seven negative mutation names above must each fail at their expected native assertion; device refusal/compilation failure is not an accepted mutation result. Keep these instrumented tests separate from clean timings.

## Next bounded direction for Sparkly and the Firmament

First qualify the changed Mode 2 across the same actual inputs and refusal matrix if its measurements are to be used as a fully qualified control. Freeze Current, authored arithmetic, tolerance, budgets, capacities, endpoint provenance and fallback behavior.

For the physical hypothesis, make **one controlled diagnostic lowering change** to the pending DFS stack representation while retaining the existing root decisions, endpoint evaluations and bands/knots/pieces. Compare the exact source-bound compiler output with the unchanged control. This tests a cause; replacing the solver, all scratch arrays and the consumer together would not. A scalar/state-machine or constant-index lowering is a proposal, not a promise of fewer physical registers or better occupancy. If the physical hypothesis does not materially change, stop before retiming it.

If a candidate does change the relevant physical work/state, pass actual RGB/T, endpoint, ownership and late-refusal controls before two fresh independent clean matched invocations, reporting both means/medians and ordered samples. Preserve the whole connected scope; host-prepared variants include producer and upload costs. Newton/Brent polishing alone cannot replace interval coverage: multiple/even roots, uncertain branches and finite-precision errors still require a proved covering mechanism or whole-ray Current fallback. OntoMath owns mathematical meaning; this remains execution-lowering research.

**Remaining shipping gates:** profitable connected execution remains OPEN; generic OntoMath/renderer/full-frame integration remains OPEN; inhabited native FPS and Person acceptance remain OPEN. This pass improves trustworthy evidence and a tested fixture checkpoint, and ships no new GPU optimization. No new visible behavior or Person-only verification item is introduced; existing movement/appearance checks remain open.


**Executed continuation:** [§6dw](GPU_RAY_TRANSPORT_6DW_STACK_ISOLATION_2026-10-26.md) subsequently closes the changed-Mode2 matrix gap, tests a controlled pending-stack lowering, and stops it on clean cost. Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-26T19:00:52.638639-07:00.
