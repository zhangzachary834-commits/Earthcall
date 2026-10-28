# §6ei — review of §6eh and Firmament counsel: actual candidate parity and clock calibration

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-27T17:00:15.352035-07:00.

Zach reported new counsel and another Sparkly pass during completion of [§6eg](GPU_RAY_TRANSPORT_6EG_ROOT_CONTRACT_REVIEW_2026-10-27.md). This review continues [§6eh, `2b74c5ef8`](GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.md), the Firmament’s local204-line poll/radius/Terminal-S2 counsel, and Zach’s default-branch `b9b1afff3` addition to `PERFORMANCE_AS_TRUTH.md`. The new steering supersedes sending the already-completed §6eh assignment again.

**Outcome:** §6eh’s two runs are real source-bound **host-completion** measurements. Their code did not assert candidate parity to §6eg, and their time plateaus cannot establish equal GPU execution time. A fresh unchanged-shader experiment verifies all32,784 candidate outputs and headers plus available nonterminal packets against validated §6eg, rechecks selected authored accuracy, and demonstrates a large host-wait floor with an empty submission. The calibrated observations remain mixed and establish no substantial repeatable Current win. No new optimization, app change or shipped FPS is claimed.

## The new counsel and Zach’s criterion

Zach’s new statement says the internal process must be ordered to the authored being, beyond merely displaying a matching result. This review adopts that criterion: record the justified mathematical work, temporary storage/traffic and actual substrate cost separately. A matching image is not a minimum-work theorem; a smaller work count is not a hardware timing measurement.

The Firmament’s observation of timing plateaus is useful and testable. Its proposed radius contraction and Terminal S2 are research directions, not executed guarantees. The counsel and default philosophical text are snapshotted without rewriting their authorship; the untracked counsel file remains untouched.

## What §6eh did and did not establish

Both saved native sources reconstruct exactly. Their raw timing fields match the public compact dataset:240 observations per run,480 across two independent native invocations. Recomputed means/medians retain those host observations.

The original `candidate_verification` only counted terminal/fallback rays and differences **against Current**. Matching12,889 terminal rays does not assert agreement with validated §6eg. It neither compared expected output bits nor candidate packet words. The new preflight closes the observed part of this gap explicitly.

The reported timing clock is genuinely submit-to-completion as observed through one `wgpuDevicePoll(device,true,...)`. That includes a host wait. The local vendored API header documents the boolean return from `wgpuDevicePoll` as queue empty versus submissions still in flight. Hardware timestamp queries were not used. No register, occupancy or stall cause follows from these clocks.

§6eh’s “slower at every measured repeat” description also overstates its own table: run2/repeat1 candidate median is slightly lower than Current. The appropriate retained finding is that the chosen host screen did not establish a substantial repeatable gain. Equality of kernel times, universal slowdown and rejection of all future solver refinements are not established.

## Fresh native preflight against the validated candidate

The [new factory](../../scratch/probes/ray_transport_gpu_6ei_clock_review.py) leaves **every original literal WGSL block byte-identical** to §6eh. It changes host observation/reporting only. Expected outputs and actual packet metadata are derived from the losslessly archived §6eg native report, not synthesized from the timed implementation.

| Normal frozen fixture check | Fresh result |
|---|---:|
| Output bit differences, all32,784 rays | 0 |
| Subpanel counts/ownership header differences | 0 |
| Live nonterminal subpanel/overlap word differences | 0 |
| Output component differences after changing host wait | 0 |
| Actual clean terminal endpoints exported | **No** |

Full output/header arrays are exported for independent replay. The nonterminal packet check is an executed source-bound live assertion; those actual packet arrays are not exported again. Terminal endpoint parity remains unobserved in this clean program. Do not turn this normal-fixture witness into an all-pose/unsupported-state proof.

Fresh selected-reference replay also passes RGB pooled mean/p99/max and T mean/p99/max at both reference resolutions against captured and this clean run’s **contemporary Current**. The candidate still meets the same bounded numerical checkpoint; no visual retuning occurred.

## Direct clock calibration on Apple M5

One invocation collected416 ordered observations using identical shaders and dispatch work. It crosses blocking `devicePoll(true)` with active `devicePoll(false)` until the documented queue-empty return, at repeats0/1/2/4/8/16. Repeat0 submits an empty command buffer. Active polling consumes CPU and can change scheduling/power behavior; it is a diagnostic observer, not a renderer optimization or hardware GPU timestamp.

| Empty submission, median host completion | Measured |
|---|---:|
| Blocking wait | 1.2680835ms |
| Active nonblocking polling | 0.0311665ms |

This supports a substantial host-wait component in the blocking floor. It does **not** establish an exact universal1.2615ms internal wakeup cadence, or recover a true1.89ms GPU duration. In the old runs,213/240 and218/240 samples lie within25µs of the proposed1.2615ms grid; maximum residuals are356µs and245µs. “Every sample is an exact multiple” is too strong.

The new nonempty observations remain mixed. Selected **active-poll host total means**, entire batches:

| Repeats | Current | Unchanged terminal bisection | Conservative candidate |
|---|---:|---:|---:|
| 1 | 2.751531ms | 2.692219ms | 2.673609ms |
| 8 | 10.059620ms | 12.431234ms | 11.559193ms |
| 16 | 20.827271ms | 27.265630ms | 22.524589ms |

At repeat1 the candidate’s mean is slightly lower; at larger batches it is higher. Large host encoding outliers occur, particularly in terminal-control samples. No clean uncontended hardware-state claim is made. All clocks, encodes, totals, order and poll counts are preserved. This single crossover invocation is metrology evidence; neither it nor dividing batch totals by repeats establishes inhabited FPS. Stop retiming this conservative candidate without a materially different hypothesis.

## Reply to the radius and Terminal-S2 proposals

**Radius:** retaining the uncertain bracket was deliberate conservative reference behavior, explicitly documented in §6eg. The observed11.43% rise in terminal subpanel-pair checks and error regression are real. They do not quantify an11.43% total GPU-time penalty. Packet-comparison mismatch counts are not extra GPU work counts. A correctly bounded tighter radius is reasonable independent work; getting the old counts or31× selected max-error ratio back is a hypothesis.

The proposed two-ULP-expanded quotient addresses the demonstrated nearest-rounding idea, but no source-bound Metal division/subnormal error envelope or executed ten-case result was supplied. Its caller must retain/refuse unsupported nonfinite/underflow cases. Finite example success cannot supply universal error bounds. Keep this as a separate numerical candidate, rather than combining it with S2 fusion.

**Terminal S2:** source inspection supports the structural opportunity. The admission restricts the route to active members0/1 with conservative non-overlap and0-before1 order. Its existing global `count` is already shared across both members and limits the total to48; `memPieces` reuses the same array region per member. Retaining both members’ pieces in disjoint portions is plausible without enlarging the declared array. It does not prove zero added live registers, unchanged compiled spill, exact f32 output, or a native gain.

The counsel’s22,957 terminal rays is a nominal ceiling from the frozen route counts, not a measured terminal-S2 completion result. Removing some `sBuf` writes/consumer reads does not remove its entire33MB capacity or establish33MB less traffic. Exact chronological math still requires native arithmetic/order/capacity/refusal checks. The3 zero intervals proved in §6eg are scoped to their exact tree/time/placement/ray bindings; they are not permanent certificates after edits or camera/time changes.

## §6ej — next isolated quest for Sparkly

1. Reproduce §6ei replay and retain §6eh clocks as host observations. Do **not** implement a new radius/SBI rule in this quest. Freeze Current/bulk/routed/terminal-bisection controls and test **Terminal S2 with unchanged bisection only** as one distinct candidate.
2. Preserve the existing two-member order and shared48-piece/64-subpanel admission. Finish both members and capacity/refusal checks before publishing any final result. Use one shared `(L,T)` state in exact canonical order and original arithmetic/endpoint selection. Late member1 refusal must discard partial work and execute whole-ray Current fallback exactly once.
3. Qualify all30 temporal/repeat/budget/capacity cases and reverse/parallel/late-refusal inputs. Capture actual terminal-S2 endpoint **and member** data; the existing terminal recorder hardcodes member0 and cannot certify S2. Assert strict output and packet equality to unchanged bisection. A mathematical chronology argument is not a bit-parity result.
4. Execute actual missing-S1, missing-S2, missing-fallback, reset, double-publication and late-second-member refusal controls. Keep producer ownership and fallback counters explicit. Do not widen parity or capacities to hide a failure. A declared48-element array does not prove unchanged physical resource allocation.
5. Only if qualification passes, run one bounded complete-cost screen with the repaired observer/accounting and unchanged controls; include complete host encoding and completion, raw ordered repeats and means/medians. Retain observer overhead and CPU use explicitly. Optional **isolated** hardware timestamps require correct native outputs, supported/nonzero queries and explicit profile identity; never enable them in the ordinary app or treat zeros as zero cost. A clear loss stops. Credible gains need a second independent clean invocation and actual source-bound compiler/resource facts before promotion.

No cache/streamer/second solver, engine/default/save/app change or production integration. The full generic certification campaign remains separate from Zach’s bounded current-or-better checkpoint. After a real gain, broad full-frame/unsupported-state validation and generic integration still remain.

## Evidence and shipping account

[Index](GPU_RAY_TRANSPORT_6EI_CLOCK_AND_COUNSEL_REVIEW_2026-10-27.json), [compact evidence](GPU_RAY_TRANSPORT_6EI_CLOCK_AND_COUNSEL_REVIEW_2026-10-27.zip) and [replay](../../scratch/probes/ray_transport_gpu_6ei_review_replay.py) preserve actual source/report/expected data, bounded exact-native timing projections, original §6eh documents, counsel snapshots and all416 fresh observations. Expected-data provenance reconstructs the validated §6eg report from existing archives. The old88MB reports remain unchanged locally; this archive retains their relevant timing projections and full-file hashes, **not** a claim to reconstruct every unrelated old field. Historical blocking-clock CLI execution now requires explicit acknowledgement of its limitations.

```sh
python3 -B scratch/probes/ray_transport_gpu_6ei_review_replay.py --root . \
 --record docs/audits/GPU_RAY_TRANSPORT_6EI_CLOCK_AND_COUNSEL_REVIEW_2026-10-27.json \
 --archive docs/audits/GPU_RAY_TRANSPORT_6EI_CLOCK_AND_COUNSEL_REVIEW_2026-10-27.zip \
 --eg-archive docs/audits/GPU_RAY_TRANSPORT_6EG_ROOT_CONTRACT_REVIEW_2026-10-27.zip \
 --ee-archive docs/audits/GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.zip \
 --fixture docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip \
 --output /private/tmp/earthcall-6ei-replayed.json
```

All3 shipping gates remain **OPEN**: profitable connected execution; generic OntoMath/full-frame integration; inhabited native FPS/Person acceptance. Two review rounds in this turn now supply safer root behavior, a real frozen affine proof, actual clean-candidate parity and calibrated host observation. Production renderer source, live saves, app/defaults and visible rendering are unchanged. No shipped FPS gain. Existing Person checks remain open. Native lane **RELEASED**.
