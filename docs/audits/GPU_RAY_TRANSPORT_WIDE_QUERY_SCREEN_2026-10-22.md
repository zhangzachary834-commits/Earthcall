# Distinct native rays: accuracy survives, the sparse speedup does not

Zach commissioned the next pass toward his **current-or-better accuracy, substantially faster Northern Veil checkpoint**. This continues [§6cl bounded streaming](GPU_RAY_TRANSPORT_BOUNDED_STREAMING_2026-10-22.md) and answers the [Firmament's newly written review](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CK_UNCERTAINTY_BANDS_AND_BOUNDED_VIEWPORT_2026-10-22.md) in the [existing Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). The review is preserved verbatim on both checkouts. Zach reports that the Firmament lost his file-reading tool; that is a coordination concern, not a reason to infer agreement or alter his text.

**Decision:** retain the experimental sources/evidence; promote none of these execution variants. On **32,784 genuinely distinct native rays**, all three variants pass their scoped accuracy/parity/fallback gates, but every measured bulk and streaming lane loses to its matched current marcher. The earlier positive repeated sparse-query measurements remain dated evidence; they do not forecast a viewport gain. No production/default/save/data-home/app change, certificate activation, PR or push. There is no new Person-visible result or FPS claim.

## Actual workload and reference scope

The original authored-source native renderer captures a **256×144** grid of pixel centres at **2560×1440**, inside camera `(0,50,75) → (40,62,110)`, clock7.25. `EC_CF4_BASELINE=1` disables the alternative CF4 transform. The only capture-builder extension is an explicitly bounded grid parameter; default64×36 generated C++ is byte-identical to pre-pass HEAD `d9c9b3f3f`. Instrumentation supplies inputs, not a full-image fidelity or frame-timing witness.

All **36,864** capture records are valid. Uniform rank selection, fixed before candidate/error evaluation, chooses **32,784** distinct pixels and direction/clip tuples—no upsampling, cloned rays or fabricated answers. A second preselected uniform192-index holdout receives the independent original-AST cumulative Simpson reference at65,536 and32,768 intervals. Its maximum refinement difference is T7.16e-8 / RGB4.41e-8. The other32,592 queries do **not** have dense transport references. The archived capture bytes independently reconstruct the selected inputs and holdout indices.

Original-AST density/transport-node comparison covers all32,784 queries. Each of three separate native runs passes its original six aggregate reference gates at **both resolutions on the192 holdout rays**; for the adaptive run, mean RGB error is6.45e-6 versus current5.21e-5 and mean T error1.88e-5 versus current1.31e-4. Those are holdout observations, not a universal error certificate. All three report zero normal whole-ray fallbacks. Fused/unfused arithmetic, five closed-form controls, poisoned output and native baseline guards also pass.

## Real reuse and complete query cost

Capacities8,192 /16,384 /32,768 now require **5 /3 /2 chunks**, all with a final16-ray tail. Schedule-only allocations are10 /20 /40MiB. Every normal streamed result matches its own bulk candidate at every f32 bit; every forced budget-zero query matches the actual original native marcher at every f32 bit. All32,784 rays are checked in each of nine normal/forced cases. Input/output and comparison buffers still cover the whole query; bulk controls remain resident. This does not prove10MiB total VRAM or production frame lifetime.

Apple M5/Metal, GPU timestamps disabled. ABCCBA/CBAABC ordering supplies eight observations per mode at1/2/4 complete-query repetitions. The table is the four-repetition mean normalized per **complete32,784-ray query**, including host encoding and submission-to-blocking-completion. Cold bindings/buffer allocation, reference work and correctness readbacks are separate. Raw clocks/order are archived and independently recomputed; no fence-floor subtraction or legacy consumer-only normalization is used for retention.

| Trial | Ring rays | Chunks | Current ms | Bulk connected ms | Streamed connected ms |
|---|---:|---:|---:|---:|---:|
| wide | 8,192 | 5 | 1.623 | 1.905 | 2.650 |
| wide | 16,384 | 3 | 1.701 | 1.979 | 2.370 |
| wide | 32,768 | 2 | 1.543 | 1.668 | 1.851 |
| envelope | 8,192 | 5 | 1.579 | 1.792 | 2.457 |
| envelope | 16,384 | 3 | 2.356 | 2.625 | 3.258 |
| envelope | 32,768 | 2 | 1.405 | 1.662 | 1.819 |
| adaptive | 8,192 | 5 | 1.655 | 1.954 | 2.677 |
| adaptive | 16,384 | 3 | 1.811 | 2.028 | 2.397 |
| adaptive | 32,768 | 2 | 1.740 | 1.988 | 2.052 |

The streamed lane is approximately **18–63% slower** than its paired current control; the bulk lane also loses in every row. Absolute times vary across independently run variants/capacities: compare each lane to its contemporaneous current control, not different rows as an isolated treatment effect. No repeat-series promotion gate is attempted after these cost failures. Different distribution, distinct workload size, cache, occupancy, packet traffic and command overhead are possible contributors; these runs do not isolate their causal shares. Earlier phase timings cannot establish today's bottleneck. The previous unlocated device-loss limit remains unresolved; successful1/2/4 commands here do not establish full-viewport submission safety.

## Written counsel: what was tested and what remains unsound

1. **Tight harmonic envelope executed.** `|Σ aᵢ sin/cos(·)| ≤ Σ|aᵢ|` tightens the z clip with the existing diagnostic margin. Existing derivative enclosure, budget, overlap handling and GL2/complete-product transport remain. It passes accuracy but loses complete cost. The envelope still comes from the pinned decoder; no generic OntoMath extraction or native omission authority is conferred.
2. **Adaptive1..16 seed bins executed.** The envelope trial additionally uses `clamp(ceil(chord/4),1,16)` bins, exact outer clip endpoints and the original conservative monotonicity test. It passes the same gates but also loses cost. No reduced error threshold, f16 endpoints, shortened packet capacity, new CF4 overlap integration or one-span compression was adopted.
3. **Endpoint change is not monotonicity.** An executed normalized-ray counterexample uses `g(s)=0.006s+0.00799(cos(√(1−0.006²)s)−1)` over `[0,1]`, ε=.001. Bow=.000998714≤ε and endpoint change=.002327136>2ε, yet g′ changes from+.006 to−.000723154 and has an interior maximum. Thus the review's stated sufficient condition is false even in the cosine-plus-linear level-set class. The existing derivative enclosure correctly refuses it. This counterexample addresses the monotonicity claim; it is not a GPU-rounding or quadrature certificate. Additional strict early-stop width/chord guards may be screened later, but cannot replace the derivative proof.
4. **Preserve earlier corrections.** Current packets are64 singles/16 overlaps, current baseline seeds8 bins. §6cl found208 multi-span ray/member pairs,11 with three; compact descriptors must preserve gaps, internal panel knots, overlaps and fallback. The review's older occupancy/phase percentages, byte estimates and2–4× projections remain hypotheses or superseded arithmetic, not measured benefits.

## Verification, reproducibility and next bridge

[Machine record](GPU_RAY_TRANSPORT_WIDE_QUERY_SCREEN_2026-10-22.json) pins compressed exact capture/reference/C++ source, independent reference/raw outputs, all three reports, timing samples, packet census, full normal/forced vectors and capture controls. [`ray_transport_wide_evidence.py`](../../scratch/probes/ray_transport_wide_evidence.py) requires no GPU or external fixtures: it checks every archive SHA, reconstructs the native capture selection, verifies all nine bitwise normal/fallback cases and memory/tail arithmetic, recomputes packet census, validates paired timing order/complete clocks, and independently replays the192-ray gates at both resolutions for all three runs. **Six external-case and ten output/census/clock corruption controls refuse**, including signed-zero bit corruption. Four capture source/bounds controls pass; their exact script/result are archived. The prior default capacities of the shared verifier remain unchanged.

```sh
python3 scratch/probes/ray_transport_wide_evidence.py --root . --record docs/audits/GPU_RAY_TRANSPORT_WIDE_QUERY_SCREEN_2026-10-22.json --output /private/tmp/your-new-wide-replay.json
python3 scratch/probes/ray_transport_firmament_chord_guard_test.py
```

Live rebuild/run entrypoints are `ray_transport_gpu_wide_stream.py`, `ray_transport_gpu_wide_envelope.py` and `ray_transport_gpu_wide_adaptive_bins.py`, each with `--root . --work /private/tmp/unique-directory --build-only`, then `--run-only --case /private/tmp/restored-wide-case/manifest.json --label wide` (or envelope/adaptive). Restore archived rays/reference bytes and rewrite only the manifest's absolute paths; preserve its hashes. The [previous pose audit](GPU_RAY_TRANSPORT_MULTI_VIEW_QUALIFICATION_2026-10-22.md) owns independent-reference compilation. New capture generation is in [`ray_transport_wide_capture.py`](../../scratch/probes/ray_transport_wide_capture.py); build the original capture helper with `capture=True,capture_grid=(256,144)` and keep all sample/cutoff/source knobs as recorded. All three actual native runs completed; this pass changes scratch tools only, so no unrelated production suite was run.

**Remaining shipment gates, in dependency order:**

- Derive generic density/root/support structure in OntoMath and give source/revision/clock/device state an explicit production lifetime. Shared-factor ownership is landed; the rest remains fixture-decoded.
- Earn a connected cost win on genuinely distinct workloads with a lossless, affordable execution representation. The large-ring mechanism is correct but insufficient. Next experiments must distinguish producer work, retained packet traffic and consumer/dispatch cost on this same query before choosing a redesign; preserve complete products/overlaps, original fallback and reference gates. Generic ownership alone does not guarantee faster execution.
- Prove unique **full viewport**, depth clipping, composition and complete frame resources/cost, including empty regions and bounded command-size behavior; then measure moving inhabited p95 and obtain Zach's appearance/feel acceptance. Production still runs the retained marcher.

All native probes finished; source/native ownership released. Work remains local to `claude/ray-domain-transport-rung0`; evidence/counsel is relayed to default. The existing Person verification entries remain open. Nothing in this pass changes what Zach should see in the app.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T11:52:54.393507-07:00`.


## The later11:50 counsel, received during closeout

The [newer knot-stream review](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CK_UNCERTAINTY_AND_KNOT_STREAM_2026-10-22.md) arrived on default while this pass was closing. It is also read and preserved verbatim on the ray branch. It **accepts** the monotonicity counterexample, preserves the strict derivative rule, proposes additional conservative early-stop guards and replaces the earlier f16 idea with full-f32 chronological spans. Those are real improvements to the proposal; the older draft is not its final position.

The full-f32 event-stream idea is a reasonable next **experimental** representation: preserve every disjoint gap, active-member mask and internal knot, then move exactly the existing per-span subdivision arithmetic into the consumer. It still needs packet round trips, original evaluation order/nodes, capacity/native fallback, same-reference accuracy and distinct-query complete cost gates. The proposed128-byte format has **not** been implemented or performance-qualified here.

Two additional executed witnesses in `ray_transport_firmament_knot_cone_test.py` tighten this review:

- **The stated header overbooks bits.** Fifteen4-bit masks take60 bits; even a minimal separate4-bit span count and1-bit fallback flag need65 bits, before the proposed reserved fields. Thus the stated two-u32 separate-field layout cannot hold them. This does **not** make128 bytes impossible: a documented sentinel/combined encoding could eliminate a separate count/flag. Choose and test that encoding before claiming a lossless layout.
- **Fixed-distance normalized rays are not an eight-vertex frustum.** Four corner directions `(±1,±1,1)/√3` and common ray distances `[1,2]` have corner z≤2/√3≈1.1547, yet the interior forward unit ray has z=2 at its far endpoint. Thus eight such corner endpoints miss an interior affine-coordinate maximum. A true planar-depth frustum with correctly expanded bounds, a proved direction enclosure, or reduction over all64 rays can work; normalized directions do not vary bilinearly at fixed Euclidean distance. Nonlinear wave enclosure is useful only after its input-coordinate enclosure is sound. No tile reject was enabled.

The review also assigns the prior device loss to a watchdog at16/64 repetitions and proposes a24–48-pass ceiling. The failing repetition/cause was **not observed**, so that causal diagnosis and safe ceiling remain unproven. Bounded submission is sensible to test, but a pass-count constant is not a device-safety theorem. Its phase/occupancy numbers remain older non-additive diagnostics. The wider measurements above are now the actual economic gate.

Its uncertainty-band bound is conditional on endpoint and modulation assumptions; those assumptions must be derived/checked, and a tent-kink perturbation does not bound all Perlin/overlap integration, rounding, nested attenuation or final RGB. Additional early-stop guards were not slipped into the three completed trials. For generic recipe matching, recognize algebraic structure without reordering the actual authored f32 evaluation; preserve original trees and an explicit error/refusal contract.

[Final offline replay result](GPU_RAY_TRANSPORT_WIDE_QUERY_SCREEN_2026-10-22_REPLAY.json) passes29 archives, all nine large-ring cases and16 corruption refusals, including the later header/cone witnesses. The preceding §6cl offline replay also passes unchanged (ten archives, eight normal/fallback cases, nine refusals), confirming the shared validator's original default-capacity behavior. No new Person-visible state or production change.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T11:56:50.517755-07:00`.
