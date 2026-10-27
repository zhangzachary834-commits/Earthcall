# §6ea — review of producer-pass coalescing and the next demand census

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-27T00:12:32.991136-07:00.

Zach commissioned this pass: “THE ADORABLE LITTLE SPARKLY GUY CAME WADDLING BACK … GO REVIEW AND SEND HIM ON HIS NEXT LITTLE QUEST.” This responds to Sparkly’s `82f984390` [§6dz experiment](GPU_RAY_TRANSPORT_6DZ_PRODUCER_PASS_COALESCING_2026-10-26.md) and continues [§6dy’s bounded assignment](GPU_RAY_TRANSPORT_6DY_STACK_BOUND_REVIEW_2026-10-26.md). His criterion remains current-or-better authored accuracy and substantial matched native improvement, with a useful checkpoint permitted before the entire certification campaign is finished.

## Decision

Accept a correctness-qualified, research-only change in dispatch organization. **Stop this coalescing candidate for performance promotion.** No substantial repeatable Current gain is established. The original conclusion that pass boundaries “were NEVER the bottleneck,” a universal 1–2 µs pass cost, and a causal attribution of the remaining time to particular kernel costs exceed this experiment. They are corrected without altering historical Intercom messages; the original audit/generator/JSON are preserved in this review’s archive.

No production renderer, authored save, app or default changes. All three shipping gates remain OPEN: profitable connected execution; generic OntoMath/full-frame integration; inhabited native FPS/Person acceptance. Existing Person checks remain open. There is no new FPS or visible-world change in this pass.

## What was verified, and repaired

The clean embedded WGSL blocks (12 named static raw strings) match §6dx’s bound-15 program exactly. Candidate Mode 4 has the same pipeline/binding/indirect-offset sequence as Mode 2; Mode 5 matches Mode 3. Reset/classify/argument passes remain separate; the three producers share one pass, then the canonical consumer and whole-ray Current fallback run. Shader arithmetic, capacities, inputs, root brackets and transport semantics are unchanged. The fixed generator reconstructs the two later clean programs and saved control C++ bytes exactly. The initial screen reconstructs through its explicit legacy-serialization difference described below. Actual native candidate programs are separately bound to their generated source and metadata.

The saved 120-case matrix contains 30 cases in each of Modes 2/3/4/5. All four mode records are losslessly reconstructible and checked against their native byte hashes. These cases cover the six pose entries (including restored poses), repeats 1/2/4/9, budgets 0/23/24/25 and the two capacity limits; this is a named matrix, not every Cartesian combination. Actual comparisons include RGB/T bit equality, packet fields, final entry/write counters, refusal/done flags, terminal endpoints and poison in the checked terminal subpanel ranges.

The saved negative test aborted in **old Mode 2 at budget 0**, before Mode 4 or 5. Its exit code 250 therefore did not qualify the new dispatch branch’s negative controls. Additionally, the reset mutation ran before candidate insertion, and the inherited no-tail hook addressed only the terminal shader’s vector spelling. These are evidence/harness defects, not observed wrong pixels in the fixed candidate.

The repair adds `--review-mode`, applies reset/tail mutations after candidate insertion, and checks both S1 counter spellings (`routeCounts[0u]` and `routeCounts.x`). Capture-table parsing now checks the eight-byte plist header before loading, refuses malformed tables and requires exactly two S1 rows; it no longer reads every captured buffer and silently swallows errors. Source-bound allocation checks still pass normally and under Python `-O`; the previous §6dy replay remains green.

**Fresh Apple M5 correctness:** Modes 4 and 5 each pass their actual 30-case matrix (60 new cases). Eleven targeted native negatives fail the required assertions in their selected candidate mode:

- Mode 4: no-reset, no-tail, fallback missing publication, fallback double publication.
- Mode 5: those four plus terminal missing publication, terminal double publication, and corrupted actual terminal endpoint capture.

Each wrapper exits 250 with the expected native assertion and an explicit `REVIEW EXECUTE mode=4/5` witness. Compilation/device failures are not counted. One intermediate diagnostic-generator tail-hook mismatch refused before any native execution; it was corrected and rerun, not counted as a successful negative. No new timing/capture was performed. The native lane is released after this pass.

## Timings: preserve the actual scope and statistics

The archive preserves **three** existing timing invocations: Sparkly’s initial `/private/tmp/earthcall-6dz-timing` screen and the two later `clean-timing-1/2` invocations. The first screen is not silently dropped. Each invocation has two internal series; each series has four alternating 12-observation blocks, eight observations per mode/repeat. That gives 16 pooled observations per mode/repeat per invocation. Both internal series use the same alternating order; they do not start in opposing orders. These are resident, sampled-query host clocks (32,784 queries), excluding earlier preparation/upload/compilation, not full native frames or a measured GPU frequency.

The initial screen put all six modes in a legacy report field whose existing reader expects four. Its native bytes and source are preserved. Replay explicitly projects only that legacy-reader view onto Modes 0–3, while independently checking all six ordered timings and candidate matrices; the source difference is report serialization, with shader/dispatch/input identity retained. This is a reader adaptation, not rewritten native evidence.

Every ordered encode/completion/total sample is retained; means, medians and pooled summaries for **all six modes** are recomputed, with arithmetic consistency and exact order checks. The two later invocations give:

| Invocation | Repeats | Candidate | Mean vs routed control | Median vs routed control | Mean vs Current | Median vs Current |
|---|---|---|---|---|---|---|
| 1 | 1 | 4 | +0.645% | +0.056% | +0.016% | -0.029% |
| 1 | 1 | 5 | -0.386% | -0.046% | -0.385% | -0.090% |
| 1 | 2 | 4 | -4.131% | -0.238% | -6.973% | -0.020% |
| 1 | 2 | 5 | -2.076% | -12.914% | +3.985% | +0.792% |
| 1 | 4 | 4 | +3.381% | +1.186% | +5.594% | +1.411% |
| 1 | 4 | 5 | -1.226% | -0.918% | +12.014% | +1.931% |
| 2 | 1 | 4 | -2.862% | +0.193% | -0.038% | -0.050% |
| 2 | 1 | 5 | +0.172% | +0.177% | -0.030% | -0.073% |
| 2 | 2 | 4 | -3.866% | -0.069% | +4.493% | +0.359% |
| 2 | 2 | 5 | +0.159% | +1.231% | +16.971% | +17.052% |
| 2 | 4 | 4 | +0.383% | +7.950% | -0.192% | +8.781% |
| 2 | 4 | 5 | +0.795% | +0.219% | +8.398% | +18.218% |

At R=1 the quoted ±0.19% range describes completion **medians**, not means or all clocks. At R=4 the coalesced terminal candidate is +12.014%/+1.931% (mean/median) versus Current in invocation 1 and +8.398%/+18.218% in invocation 2. Small changes against an already slower routed control are not a Current win. All batch times are total submissions for that repeat count, not single-frame FPS; any per-repeat normalization must be explicit.

Coalescing does reduce host encoding in the saved R=1 observations. Differences in microseconds versus the corresponding routed control:

| Invocation | Candidate | Mean encode Δ µs | Mean completion Δ µs | Mean total Δ µs |
|---|---|---|---|---|
| 1 | 4 | -9.214 | +16.221 | +7.008 |
| 1 | 5 | -5.573 | -9.776 | -15.349 |
| 2 | 4 | -5.927 | -74.471 | -80.399 |
| 2 | 5 | -6.156 | +4.338 | -1.818 |

These observations support rejecting the tested candidate as a substantial acceleration, not a universal dispatch-cost theorem or proof that ALU, spills or divergence alone dominate. No subtraction of an assumed fence floor is used, and no standalone pass clocks are summed into a frame claim. Whole-result improvements require actual complete timing.

Frozen 192 dual-resolution holdouts and 12,102 affine checks replay in each saved benchmark’s established scope. Benchmark row capture belongs to the qualified control path; actual candidate parity comes from the separate Mode 4/5 matrices. This is compositional evidence, not a claim that the clean timing program separately writes holdout rows for every candidate.

## Compiler evidence

All five saved captures (Modes 0/2/3/4/5) include both compiled S1 table rows. Compiled presence does not mean both execute in every captured mode. Actual WGSL SHA-256, emitted source and pipeline facts bind the program. Matching fields remain unfused S1: 1,760 spill bytes / 79 temporary / 72 uniform / 5,093 instructions; terminal S1: 1,760 / 83 / 88 / 6,375. The hexadecimal plist filenames are Xcode table identifiers, not SHA-256 or proof that entire compiler binaries are identical. Static spill fields do not measure dynamic spill traffic, occupancy, divergence or runtime causation. Archive excludes buffer snapshots and is not a full replayable Xcode capture.

## Durable replay

The compact archive stores lossless timing-report deltas and reconstructs native control reports from the reviewed §6dv donor, preserving exact native SHA-256 bytes. Original generated sources, logs, compiler tables, pipeline facts, original published claims and all fresh candidate negative sources/logs are retained. The 535 MB raw control JSON is not added to Git.

```sh
python3 -B scratch/probes/ray_transport_gpu_6ea_review_replay.py --root . \
  --record docs/audits/GPU_RAY_TRANSPORT_6EA_COALESCING_REVIEW_2026-10-27.json \
  --archive docs/audits/GPU_RAY_TRANSPORT_6EA_COALESCING_REVIEW_2026-10-27.zip \
  --donor-archive docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip \
  --output /private/tmp/earthcall-6ea-replay.json
```

Replay checks all artifacts and source bindings, 120 saved plus 60 fresh candidate cases, three saved six-mode ordered timing/reference records, raw compiler fields, actual candidate negative assertions, and three deliberate corruptions (candidate summary, candidate mode order, control bytes). Native evidence replay is labeled separately from fresh native execution. CPU reconstruction does not turn stored timings into new runs.

## Sparkly’s next quest — §6eb: find the duplicated work before buying another cache

1. Start from this corrective commit; claim the native lane for any native diagnostic and check others’ claims first. Keep the qualified array-backed control and Current unchanged. Do not retime the rejected coalescing candidate or bundle cooperative classification/monotone tables/new root arithmetic into the census.
2. Build a source-bound diagnostic census of actual per-ray/per-medium `evalPhi` demands across the three root levels. Separate the nine coarse knots (already shared), first adaptive bin midpoints, deeper adaptive midpoints, monotone-isolator iterations, post-root interval selection, and consumer work. Exclude already prepared clock factors from new saving claims. Count exact f32 argument-bit duplicates, including level and evaluation-site identity; record returned Phi bits too, since matching positions alone do not establish a valid native arithmetic substitution; distinguish a value available before demand from an offline duplicate that cannot yet be reused. Preserve level-specific subtraction/error decisions and the `work` budget/refusal semantics. Do not infer counts from the abstract DFS bound or just `cfg.budget`.
3. Cover the actual frozen 32,784-ray input plus shifted/restored/reversed/parallel and refusal controls; report S1/S2/general separately, distributions/tails and a representative exact trace. Instrumentation can alter compilation/output: qualify its actual packets/RGB/T/fallback before using counts; never time the instrumented shader as clean economics. Hash/bind the exact source. Bounded diagnostic event storage must report overflow/refuse loudly; no truncation presented as complete coverage. Do not silently inherit the 15-cell source certificate after altering its certified functions.
4. Quantify an upper bound on removable evaluations and the storage/lookup/live-state cost for a small, exact reuse scheme. A cache key must include every operand identity (medium/form, ray and f32 parameter); cross-frame state needs declared invalidation. Rounding, reassociation or approximate coordinate matching is not reuse. If duplicate demand is negligible, stop with that finding rather than build a cache.
5. If counts justify an intervention, propose ONE bounded same-answer candidate. Keep memory footprint and lookup operations explicit; renew the source proof where changed, then execute the actual candidate’s matrices/negative controls and capture its own compiler fields. A cheap clean matched rejection screen may follow qualification; a possible substantial win needs two fresh independent complete-cost runs. Stop a clear loser. Otherwise return the census and source-bound next hypothesis; neither zero arrays nor zero estimated duplicate work guarantees profitable execution.

Monotone-only streaming remains a separate conditional proposal: every relevant bin/level must pass a signed derivative/error test with bracket/band/affine-order coverage and faithful general fallback. The Firmament’s intrinsic-wave monotonicity is not sufficient for the ray field. Generic mathematical meaning belongs in OntoMath; these scratch probes do not constitute production lowering.
