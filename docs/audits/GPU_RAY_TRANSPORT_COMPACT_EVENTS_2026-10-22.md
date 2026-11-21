# Compact events: smaller packets preserve scoped accuracy, but cost more

Zach asked for the next pass toward a substantial Northern Veil speed checkpoint at **current-or-better accuracy against the authored equations**. This continues [§6cm's distinct-ray rejection](GPU_RAY_TRANSPORT_WIDE_QUERY_SCREEN_2026-10-22.md), screens the Firmament's [full-f32 unchopped-event proposal](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CK_UNCERTAINTY_AND_KNOT_STREAM_2026-10-22.md), and answers his [new12:38 counsel](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CM_WIDE_QUERY_DIAGNOSIS_AND_6CN_DESIGN_2026-10-22.md). Both the existing [Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) and live dashboard carry the result.

**Decision:** do not promote the compact consumer. The implemented packet is128 bytes/ray rather than1,280 bytes/ray of padded single/overlap storage. Its numerical candidate passes all-query original-AST and unchanged holdout-reference gates, but primary connected costs are **73% /45% slower than current**. Same-query component controls show essentially unchanged producer cost and a more expensive compact consumer. The original bit-lossless claim is also false for this lowering: explicit native endpoint and output counterexamples are preserved.

No production source, renderer defaults, sample settings, authored saves/data home or installed app changed. Certificates remain off. No PR/push or new Person-visible/FPS result. Native probes finished and ownership is released.

## Implemented representation and its numerical boundary

The producer retains the original8-bin, three-level root searches, curvature/error/derivative rules, uncertainty bands, work/capacity refusals and complete event ordering. It writes up to15 chronological **unchopped** intervals with their exact stored f32 endpoints and nonzero four-bit active-member masks. Gaps and internal knots remain distinct. The consumer expands single-member intervals using the same panel-count rule and applies the original overlap rule to multi-member intervals; density gating, extinction floors and complete-product/shared attenuation are retained.

The corrected128-byte encoding is `15 × vec2f` endpoints (120 bytes) plus two u32 words (8 bytes). Masks occupy60 bits; count occupies the last4 bits. Normal empty is controls `(0,0)`; count0/control0=1 is a disjoint fallback sentinel, and the existing ray header still carries the native-fallback flag/reason. Thus no65th independent bit is required. **241 codec round trips and six malformed-input refusals** exercise all counts/masks, empty and fallback records. A sixteenth span refuses; it is never truncated.

Moving arithmetic across a storage/kernel boundary did not preserve native f32 execution:

- Plain same-device reconstruction refused at ray3, span4, panel3 of5: stored parent `[25.873123168945313,43.049068450927734]` yields reconstructed start36.178688049316406, while the original GPU producer wrote36.17869186401367 (**one ULP**). Exact rational rounding reproduces this pair using sequential versus fused evaluation. That is evidence consistent with contraction differences, not Metal disassembly or a universal reconstruction rule.
- Explicit `fma(k,(b−a)/n,a)` passed that point, then strict output comparison refused at ray5: red0.17201413214206696 versus old0.17201410233974457. **The exact variants remain refused and reproducible.**
- A separate numerical variant follows Zach's actual current-or-better authored-reference criterion. It records every geometry/output difference and keeps the original-AST1e-5 and both-resolution six-metric gates. Native fallback stays bit-exact. This is not a quiet reinterpretation of the exact test: separate scripts, compiled-source hashes, refusal records and explicit report flags distinguish the variants.

The first CPU endpoint assertion was insufficient as a same-device witness. Replacing it with GPU decompression exposed a host readback sizing bug: the diagnostic allocated vectors for populated packets, then copied the larger padded buffers. That run exits−11. The repaired code sizes from actual byte counts with element-size checks. Failed sources/logs are archived; use the current generator, not those failed snapshots. Exact GPU comparisons and the later numerical checks run on the repaired path.

## Workload, accuracy and failure scope

The previous capture/reference is reused byte-for-byte:36,864 valid native pixel-centre records at2560×1440, fixed uniform selection of **32,784 distinct queries**, inside camera `(0,50,75)→(40,62,110)`, clock7.25. The **192 predetermined holdout rays** have independent original-AST references at65,536 and32,768 intervals; the other32,592 do not have dense references. No resampling, candidate-dependent selection or threshold changes.

All three completed configurations—8,192/32,768 rings plus the component control—produce the same numerical outputs. Across the whole query,2,453 output rays and8,165 endpoint packets differ from old A*. Max differences are RGB1.49e-7, T2.38e-7 and endpoint7.63e-6. The CPU evaluates the original AST at this candidate's actual GPU-decompressed panels on every accepted ray: maximum RGB3.73e-7 /T1.07e-6, within the unchanged1e-5 gate. Chronology, active-member structure and panel/overlap counts agree with old A*.

Both reference resolutions pass all six aggregate mean/p99/max gates. Finest candidate RGB mean6.445e-6 versus current5.208e-5; T mean1.877e-5 versus current1.313e-4. Those are holdout results, not a full-image or universal error certificate. The accuracy improvement largely belongs to the already established A* quadrature; tiny differences from the old numerical candidate do not justify the compact path's extra cost.

Every forced budget-zero output matches the actual production-generated native marcher at every f32 bit in all three configurations; outputs are NaN-poisoned first. Normal fallback is0. The observed maximum is14 unchopped intervals. Thus actual GPU15-entry overflow is **not exercised** here: codec16-entry refusal and budget-zero fallback do not substitute for a future native capacity-exhaustion witness. Generic source revisions, other views/clocks and execution-bound admission also remain open.

## Complete native cost and component controls

Apple M5/Metal; timestamps disabled. Matched ABCCBA/CBAABC supplies8 observations per mode at1/2/4 query repetitions. Each clock includes host encoding and submission-to-blocking-completion; no floor subtraction. Cold setup/compilation, CPU references, poison/readbacks, packet mirror copies and the decompression diagnostic are outside timing. The normal connected lane executes producer→consumer→native fallback, with no diagnostic dispatch.

Four-repetition means, milliseconds per complete32,784-query batch:

| Packet-ring capacity | Chunks | Schedule-only bytes | Current ms | Old bulk ms | Compact ms |
|---:|---:|---:|---:|---:|---:|
| 8,192 | 5 | 1,048,576 | 1.694 | 1.913 | 2.931 |
| 32,768 | 2 | 4,194,304 | 1.387 | 1.675 | 2.017 |

Both primary rings genuinely reuse their schedule buffers; each ends with a16-ray tail. Schedule storage is1/4MiB, **10× smaller than matching old padded rings**, not10× lower total process VRAM: input/output, old comparison buffers and diagnostic resources remain resident.

A separate65,536-capacity allocation (8MiB) holds the same32,784 inputs in **one partial chunk**, not65,536 distinct rays. It pairs seven modes: current, bulk connected, compact connected, old/new producer only, old/new consumer+fallback. Original consumer-only output matches old A* exactly; compact consumer-only output matches its own numerical output exactly. Zero fallback and single-chunk preconditions are enforced before schedule reuse.

| Repetitions | Old producer | Compact producer | Old consumer + fallback | Compact consumer + fallback |
|---:|---:|---:|---:|---:|
| 1 | 2.701 | 2.512 | 1.448 | 1.746 |
| 2 | 2.720 | 2.643 | 1.402 | 1.953 |
| 4 | 1.755 | 1.744 | 0.901 | 1.199 |

At four repetitions the producer clocks are1.755 versus1.744ms—no meaningful producer gain established—while the compact consumer is1.199 versus0.901ms (**33.1% more**); the other repetition counts also show a more expensive compact consumer. Connected current/bulk/compact are2.182/2.607/2.915ms in this run. **Component clocks are non-additive** and not percentages of a frame. Absolute times differ from the earlier process; compare contemporaneous paired controls, not cross-process rows as isolated treatments. Kernel clocks do not identify cache misses, register spills, occupancy or bus utilization.

The representation change leaves quadrature-site counts and the three independent root-search algorithms intact, while adding packed-mask decoding and panel regeneration to the consumer. The measured incremental loss sits in that consumer path; its precise hardware cause is still unknown. The old producer also remains substantial. The next hypothesis is shared level-set work with the cheaper existing consumer, rather than assuming smaller buffers create a faster renderer.

## Reply to the new12:38 Firmament counsel

The new review is read and preserved verbatim. It accepts both previous header/cone counterexamples and independently proposes shared Φ evaluations across the three level sets. That aligns with the next source-grounded hypothesis. Several claims need tighter scope before implementation:

- **Accept shared coarse knots as a candidate.** Three8-bin searches evaluate48 initial endpoints per hit member; caching9 unique coarse-knot values could reduce that subset of evaluations48→9. This is **not** a5.33× kernel or FPS prediction; subdivision/bisection work remains. Preserve per-level arithmetic slack, original subtraction/order and strict derivative enclosure—real-math slope independence of the level constant is not automatic f32 equivalence. New cached values can change contraction/materialization, as this pass demonstrates. Mathematical ownership belongs in OntoMath; the frozen offset decoder is still not generic admission.
- **Separate changes.** Test shared endpoints first with current capacities and the cheaper current consumer. Do not bundle new root traversal, smaller capacities, private-array packing, SoA indexing and fallback suppression into one causally ambiguous screen.
- **No inferred physical spill budget.** Source arrays and strides are observable; register allocation, spill residency, SLC/DRAM traffic and cache-line utilization were not measured. Multiplying nominal bytes by all invocations does not give simultaneous physical storage. Bounds/events already have disjoint source lifetimes, so their compiler reuse is unknown. Declared `array<vec3f,20>` also cannot simply be charged20×12 host-layout bytes without considering stride/lowering.
- **Merged spans are not temporary-piece capacity.** The maximum3 merged spans/member does not bound raw pieces with all internal kinks/uncertainty.96→64 bounds or48→24 pieces changes capacity; it needs correct overflow checks and actual native refusal coverage. Fallback can preserve correctness, but its frequency/cost must be measured.
- **Use sound input bounds and current fallback state.** All-ray interval reduction is a plausible repair to the normalized-corner error, with execution slack. Normalized direction components are not globally concave either; affinity along a fixed ray is the usable fact. Frozen zero fallback cannot authorize suppressing fallback on future frames. A current GPU counter/indirect path would require lifetime/correctness and complete-cost witnesses; earlier rejected indirect work is not automatically rescued.
- **Do not subtract the§6cm process from§6cn to claim a bandwidth share.** This transformation changes packing, loops, arithmetic materialization and shader compilation as well as bytes. The current paired component controls are the measured result. Cache/spill explanations and “100% utilization” remain hypotheses.

## Reproduction and remaining shipment gates

[Machine record](GPU_RAY_TRANSPORT_COMPACT_EVENTS_2026-10-22.json) pins32 source/raw artifacts,4 reused from§6cm to avoid duplicate capture/reference storage. [Final replay](GPU_RAY_TRANSPORT_COMPACT_EVENTS_2026-10-22_REPLAY.json) validates archive hashes, original capture selection, source identities, all3 configurations, actual GPU-expanded structures, original-AST values, exact forced fallback, both reference resolutions, every paired clock and10 corruption refusals. Codec controls and actual strict native refusal witnesses are replayed too. The primary raw report inherits one stale “2304” nested scope string; actual count/dispatch/input/output coverage is32,784 and explicitly checked. The phase report corrects that label; archived snapshots remain unaltered.

```sh
python3 scratch/probes/ray_transport_knot_evidence.py --root . --record docs/audits/GPU_RAY_TRANSPORT_COMPACT_EVENTS_2026-10-22.json --output /private/tmp/your-new-knot-replay.json
python3 scratch/probes/ray_transport_knot_evidence.py --root .
```

Live entrypoints are `ray_transport_gpu_knot_stream.py` (refused exact), `ray_transport_gpu_knot_fma.py` (refused exact-FMA), `ray_transport_gpu_knot_numerical.py` and `ray_transport_gpu_knot_phases.py`. Each uses `--root . --work /private/tmp/unique-directory --build-only`, then `--run-only --case /private/tmp/restored-wide-case/manifest.json --label unique-label`. Numerical entrypoints independently gate their own outputs after the shared runner's old-bulk checks. The case source/hash restoration follows§6cm. Exact compiled C++/source metadata is archived, including failed diagnostics; successful sources contain no readback overrun. Current generators also correct the legacy wording without changing GPU shader strings.

**Remaining:** (1) a profitable distinct-query connected path—next candidate shared level-set evaluations, with independent math/accuracy and paired costs; (2) generic OntoMath density/root derivation plus source/revision/clock/device lifetimes, then affordable unique full-viewport/depth/composition and complete frame economics; (3) moving inhabited p95 and Zach's appearance/feel acceptance. The existing Person checks remain open. No rendering improvement is shipped by this pass.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T13:06:29.632017-07:00`.
