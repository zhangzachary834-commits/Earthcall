# §6ee — SBI corrective review: independent controls, real deltas, and the remaining root contract

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-27T12:48:00-07:00.

Zach commissioned review, reply and the next pass after Sparkly’s `26fa5f976` and the Firmament’s new counsel. This continues [§6ed](GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.md) and [§6ec](GPU_RAY_TRANSPORT_6EC_CENSUS_REVIEW_2026-10-27.md). Zach’s acceptance criterion remains substantial native improvement at current-or-better authored accuracy. No lighting, quality setting or authored equation was retuned.

**Verdict:** withdraw §6ed’s mathematical qualification, bit-exact Current agreement, unchanged-control cost comparison and hardware-cause claims. Retain the candidate as unqualified research: a properly isolated Metal observation improves selected-reference accuracy versus Current, but changes packets and leaves the root/affine-boundary contract unresolved. No new shipping gain is established.

## What was wrong and what is repaired

The Firmament correctly identified reference contamination: the old generator patched `kGpuProducerWgsl`, used by its Mode1 bulk reference, as well as every candidate producer. Zero discrepancies were SBI versus SBI. It also inherited obsolete §6dt controls and the bound20 timing base, omitted the qualified bound15 unchanged routed lane, and supplied no source-bound SBI hardware profile or measured native call reduction.

The reported zero Current deltas have a more direct cause than the counsel’s suggested final-pose explanation: the inserted C++ logger indexes `currentReview` and `bulkReview` **before either empty vector is populated**. This is undefined host access. Its printed zeros are invalid evidence. The reported host `segmentLine` packet totals do not repair that comparison.

The original sources, report, actual matrix source/report, clean timing source/report, CPU log and the Firmament’s 11,824-byte counsel are preserved in the evidence archive. Its untracked source draft was left untouched. The old public generator now refuses execution and retains only the exact frozen SBI loop for reconstruction; its historical audit and JSON are explicitly marked withdrawn. Its original callable implementation remains archived, rather than silently rewritten into purported evidence.

The new [source-isolated diagnostic](../../scratch/probes/ray_transport_gpu_6ee_isolated_root_probe.py) starts from repaired §6dz bound15 controls. It verifies every original WGSL block is byte-identical. Only three **separate** terminal-route producer clones get SBI:

| Mode | Meaning |
|---|---|
| 0 | unchanged Current |
| 1 | unchanged bulk bisection reference |
| 2 | unchanged routed bisection control |
| 3 | unchanged terminal-S1 bisection control |
| 4 | unused; refuses |
| 5 | separate terminal-route SBI candidate |

Actual terminal endpoints, poisoned outputs, producer-entry counts, exactly-once final writes, reset state, indirect arguments and real Current fallback remain checked. Current comparison runs after populated readbacks. Changed-method comparisons are labeled observations with `math_qualified:false`; the separate `--strict-candidate` run demonstrates strict rejection. No count tolerance or relaxed comparator is called exact. There is no clean SBI timing branch; inherited bootstrap clocks are unqualified incidental output and are ignored.

## Fresh native evidence on Apple M5

All modes used the same frozen Northern Veil fixture and 32,784 actual rays. Each mode ran the existing 30-case temporal/repeat/budget/capacity matrix.

| Result | Observation |
|---|---|
| Modes2/3 | 60/60 strict control cases: identical output/packet bits to unchanged bulk reference |
| Mode5 | 30 interface/ownership observations; 7 have strict reference parity, 23 differ |
| Original pose, Mode5 | 32,736 differing ray outputs; 108,905 packet-comparison differences |
| Original pose, terminal completion | 12,889 completed rays; actual candidate endpoint checks85,507 |
| Max Mode5 versus Current over matrix | RGB0.0022332221269607544; T0.0034830570220947266 |
| Candidate terminal write omitted | native refusal:12,889 exactly-once errors |
| Actual Current fallback dispatch omitted | native refusal:32,784 exactly-once errors, poisoned output exposed |
| Candidate reset omitted | native counter-accumulation refusal |
| Strict SBI comparison | native refusal:32,736 outputs/108,905 packets |

All four deliberate failing runs returned250 at the intended native assertion. These establish the diagnostic’s ability to detect defects and differences; they do not qualify SBI mathematically.

### Authored-reference accuracy is promising, but bounded

Independently decode all output bits, then apply the established192 predetermined ray indices and32768/65536-interval reference. SBI passes RGB pooled-channel mean/p99/max and T mean/p99/max at both resolutions, versus **both captured and contemporary Current**. These are24 comparisons using the same fixed reference samples, not24 independent populations.

At the finer reference, using contemporary Current:

| Absolute error | Current | Unchanged bisection route | Isolated SBI |
|---|---:|---:|---:|
| RGB mean | 5.20834e-5 | 6.50112e-6 | 6.47849e-6 |
| RGB p99 | 8.62845e-4 | 4.56529e-5 | 4.56180e-5 |
| RGB max | 1.49085e-3 | 4.79485e-5 | 4.79746e-5 |
| T mean | 1.31261e-4 | 1.89158e-5 | 1.88411e-5 |
| T p99 | 1.45452e-3 | 6.90643e-5 | 6.91239e-5 |
| T max | 2.31798e-3 | 6.96594e-5 | 6.97786e-5 |

SBI is slightly worse than the bisection route on some tail measures while remaining much better than Current on this selected sample. This supports Zach’s current-accuracy research lane. It supplies neither a whole-frame error theorem nor inhabited performance.

### Protected affine endpoints require a decision backed by proof

Unchanged controls retain all12,102 matched original GPU affine endpoints. SBI omits3 previously retained endpoints: ray4167/member2/yknee, ray29382/member3/x0 and ray30133/member0/y0. No candidate interval crosses those missing endpoints; intervals begin beyond them. Therefore this observation is **not** proof of crossing a branch incorrectly or visible image damage. It fails the existing exact-retention check and needs proof that the omitted support is irrelevant, or preservation of the necessary boundary. Selected-reference success cannot silently waive that requirement. The three raw packet counterexamples are preserved.

## Host witnesses and mathematical limits

The old ten-case script reproduces54→20 host calls. Its callbacks evaluate in binary64 and only partly round operations; “device-f32” was overstated. Root acceptance padded by1e-4. Its supposed adjacent-float witness had equal-sign endpoints and never entered the bracket branch. Its next-float helper was wrong for negatives. The audit’s functions, denominator threshold and printed WGSL differed from executable source. The actual SBI uses1e-30, strict0<w<1 and clamp[.125,.875]; it is not the printed Illinois/Brent implementation.

The replacement [host witness](../../scratch/probes/ray_transport_root_witness.py) explicitly rounds host steps, separates ideal and observed oracles, brackets reference roots independently without acceptance padding, tests positive/negative adjacent brackets, oracle bias, denominator overflow/tiny denominators and zero-budget refusal. Thirteen positive finite cases, explicit budget refusal and the expected-gap detector pass. This remains a finite host model; it does not reproduce Metal’s FMA/library execution envelope or establish a universal enclosure theorem.

The gap detector matters: with ideal `f(s)=1000*(s-(10-1e-9))`, error1e-5 and lower slope999, **both** baseline bisection and SBI can nearest-round the nonzero uncertainty band to `[10,10]`, excluding the ideal root. This is a shared scratch-primitive contract gap, not evidence that this new optimization introduced a production rendering bug. It must be addressed before generic soundness is asserted.

The Firmament is also right to reject the claimed bisection-rate guarantee. Clamp[.125,.875] alone only guarantees a weaker real-arithmetic per-step contraction. However `.875^32` is relative width, whereas0.0001 is an absolute threshold; an initial width is needed to compare them. A wider **conservative** returned bracket is not inherently unsound. ITP is one possible bounded strategy, not a prerequisite for every sound method. The immediate task is an honest enclosure/refusal contract, not adopting a new algorithm name.

## Timing and hardware claims are withdrawn

The saved clean report reproduces the published timing table. At repeat1 Current mean2.928776ms versus terminal SBI mean3.132737ms is about6.97% slower, while pooled median is0.26% lower. Two internal blocks are one invocation, not two independent runs. More fundamentally, no unchanged routed bisection lane exists in that comparison, its correctness admission was contaminated, and no actual SBI compiler capture/call census supports the claimed cause. Do not infer that stack spill or divergence dominated this candidate, or that native SBI calls fell63%, from an old profile and ten host functions. No new qualifying cost screen was run here.

The Firmament’s chronological streamer remains a proposal. A percentage of call events does not establish a percentage of bins or admitted rays; count the denominator and correlate actual records. Pending depth15 bounds neither total bands nor knots/pieces. Same-sign endpoints alone cannot certify no interior root. Removing arrays does not guarantee no spills, higher occupancy or a native gain. Resolve these proof/measurement obligations before replacing the traversal.

## §6ef — bounded next quest for Sparkly

1. Start from this isolated generator, frozen Current/bulk/Mode2/Mode3 and bound15. Keep engine/defaults/saves unchanged. Claim/release the native lane through the existing Intercom thread.
2. Repair **one root-enclosure contract** before another timing or representation experiment. Prove the observed-versus-ideal error and lower-slope assumptions, outward-safe band endpoints including sub-ULP radii/rounding in intermediates, nonfinite/adjacent/budget behavior and the32-step cap’s actual returned-bracket/refusal semantics. Turn this pass’s shared rounding counterexample into a passing enclosure witness without acceptance padding. Keep host proof and native execution-envelope evidence distinct; refuse unsupported cases.
3. Explain the3 missing affine endpoints with raw local support/error evidence. Preserve required exact boundaries or prove their absence harmless under an explicit contract. Do not delete the check or widen it to force green. Test any change against original/reverse/parallel/temporal inputs and actual capacity/fallback mutations; report method differences separately from control parity.
4. Recheck selected RGB/T mean/p99/max against both reference resolutions and both Current baselines. No generic mathematical qualification from finite examples alone. If enclosure or required boundary preservation remains unresolved, stop and report that precise blocker.
5. Only after admission, measure the isolated candidate’s actual tagged GPU Phi demands with the observer validated against the same uninstrumented candidate. Supply actual denominators before a shallow-bin/ray routing claim. Then capture its own source-bound compiler facts, and permit **one** complete resident native cost screen against Current and unchanged routed/terminal controls. A clear loser stops; repeated independent clean runs are required before a gain claim.

This quest does not require completing the full generic certified-integration campaign before Zach’s authorized numerical checkpoint. Its admission must state the actual bounded primitive/fixture/native arithmetic scope and leave broader certification unclaimed. Unresolved checks cannot be renamed soundness or silently waived.

No chronological streamer, cache or production integration in this quest. Mathematical meaning remains OntoMath’s office; this scratch probe is a substrate experiment, not a second definition of authored optics.

## Evidence, replay and remaining shipping account

The [machine index](GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.json) binds source hashes, native report bytes, logs and the [compact lossless archive](GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.zip). Whole repeated native field values are content-addressed while preserving numeric lexemes and indentation. Replay reconstructs the501,183,504-byte fresh report and119,135,589-byte original matrix exactly; no oversized raw JSON is added to Git. Fixture/reference donor is the already-published §6dv archive.

```sh
python3 -B scratch/probes/ray_transport_gpu_6ee_review_replay.py --root . \
  --record docs/audits/GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.json \
  --archive docs/audits/GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.zip \
  --donor docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip \
  --output /private/tmp/earthcall-6ee-replayed.json
```

All three shipping gates remain **OPEN**: profitable connected execution; generic OntoMath/full-frame connection; inhabited FPS/Person acceptance. This pass improves evidence integrity and supplies real candidate accuracy observations. It changes no renderer source, authored save, installed app, native visible appearance or shipped FPS. Existing Person movement/appearance checks remain open. Native lane **RELEASED** after these diagnostics.
