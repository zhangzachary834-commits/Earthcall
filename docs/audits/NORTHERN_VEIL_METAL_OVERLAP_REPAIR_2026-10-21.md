# Northern Veil — complete overlap transport restored and native timing requalified

**Human origin:** Zachary Zhang transferred the next stretch to Codex while away at dinner.
**Implementation / witness:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a`.
**Starting ray HEAD:** `fdf26c7d8` (`claude/ray-domain-transport-rung0`).
**Scope:** scratch native diagnostic and evidence repairs; no production source, save, app, defaults or budgets changed. No visible app was launched.

Sparkly Guy (Gemini 3.8 Flash High) originated the frozen Metal reproducer, shared-cell Perlin arithmetic and A* shader. Prism Sun and Firmament supplied the reviewed diagnostic direction; Codex found the missing overlap flow and added ordered GPU/CPU execution, an original-authored-AST comparator, independent closed-form witnesses and batched timing. The original artifact remains historical evidence.

## Why the first Metal result did not qualify

In `ray_transport_metal_diagnostic.py`, both shaders only dummy-read `overlaps[overOffset]`; they never integrate its 253 intervals. The CPU float comparator also visits only single-medium subpanels. Their mutual agreement therefore does not establish complete A* execution. The original JSON's RGB max `0.0038751194` and T max `0.0060272964` exceed native maxima `0.0019582570` and `0.0030249109` by about 1.98× / 1.99×. Improved means alone did not clear the stated mean/p99/max gate.

The old 1.27 ms measurement includes one submission and blocking completion poll for 2,304 rays. It is not a pure GPU clock, an equivalent-work renderer measurement, or evidence that discovery accounts for 99.98% of a future frame. No register/occupancy counters were read.

## Repair and independent checks

The new reproducer transforms the preserved original source using checked hooks and builds a separate executable in a fresh external directory. `source.json` records the base and generated-C++ hashes. The ordinary timestamp-free WebGPU adapter was independently queried: **Apple M5, Metal, timestamps disabled**.

Both native variants merge the sorted single-medium and overlap streams in ray order. They consume all 23,776 single-medium subpanels and all 253 overlap intervals; overlap uses the same two-panel coupled CF4 update as the CPU preflight. Processing all overlaps after the singles would change colored emission under attenuation and is explicitly checked. `h*phi(tau)` uses a small-depth series for `abs(tau)<0.01`, avoiding cancellation at small positive depth, including intermediate signed CF4 stages.

The CPU comparator evaluates the **original MathNode density, extinction and emission** at f32 point coordinates. It does not reuse the shader's manually copied envelope/noise helper as its oracle. CPU scalar evaluation and GPU execution are distinct arithmetic paths. Closed-form cases independently check zero density, tiny positive floored emission, ordinary transfer, pure different-color overlap and single–overlap–single ordering. All five pass in both isolated processes; the tiny-depth case checks emission with a relative tolerance so a zero-emission cancellation bug cannot hide inside a loose absolute threshold.

Fixture hashes and all 2,304 pixel identities match. CPU stream counters reconcile to 23,776 single subpanels, 253 overlap intervals and 2,024 overlap member-sites. Native output is checked against both the independent AST consumer and the prior converged 2^16/2^15 reference. These are finite numerical witnesses, not universal error certificates or Person acceptance.

## Native arithmetic / independent-reference results

Both processes produce identical raw rows. Fused and unfused native T/RGB are bit-identical on these fixture rows. Maximum disagreement with the independent authored-AST path is `2.906e-7` RGB and `6.557e-7` T.

| Error measure | Current native baseline | Repaired A* on Metal |
|---|---:|---:|
| Pooled RGB mean | 5.22350e-5 | 6.92274e-6 |
| Pooled RGB p99 | 7.49103e-4 | 5.05613e-5 |
| RGB max | 1.95826e-3 | 9.15713e-5 |
| T mean | 1.33515e-4 | 2.02057e-5 |
| T p99 | 1.43029e-3 | 1.09961e-4 |
| T max | 3.02491e-3 | 1.39098e-4 |

All six aggregate gates pass. Per-ray regressions remain: RGB 474/2,304, maximum error increase `6.871e-5`; T 495/2,304, maximum increase `1.083e-4`. Aggregate improvement does not mean every ray improves.

## Batched ABBA: reject fusion, keep clock labels honest

Each independent process measures batches of 1, 8 and 64 dispatches per completion fence, after four warm batches per variant. Each scale has six ABBA blocks (12 measured A and 12 B batches), plus six empty-dispatch batches. Compilation, readback, CPU oracle and synthetic checks are outside those timed batches. Each dispatch processes the same 2,304 frozen rays. Repeated dispatches measure throughput on this sparse workload; they do not create a full-viewport scene witness.

| Series / 64 dispatches per fence | Unfused batch, ms | Fused batch, ms | Fused extra cost | Empty batch, ms |
|---|---:|---:|---:|---:|
| 1 | 8.89962 | 10.50065 | +17.99% | 1.54744 |
| 2 | 8.85242 | 10.40210 | +17.51% | 1.48065 |

At one dispatch, empty and real batches are all around 1.5 ms in the final runs: the fixed submission/poll floor obscures the mechanism. The earlier preliminary runs similarly measured about 1.27 ms empty. The 64-dispatch normalized host costs are approximately 0.139 ms/dispatch unfused and 0.163 ms/dispatch fused. **These include submission/poll and dispatch costs; no empty-time subtraction is presented as a GPU execution clock.** No hardware register-pressure or occupancy cause is asserted. The fused source also requests a two-value noise result when only one envelope node is positive; attributing its measured slowdown requires separate evidence.

Decision: **reject this fused variant on cost.** The complete unfused A* remains an accurate scratch consumer candidate. This benchmark does not compare it with the current renderer on equivalent work and does not authorize production use.

## Reproduce

From any directory (use a fresh external `--work` path):

```sh
python3 scratch/probes/ray_transport_metal_overlap_repair.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-overlap-new --label series1
python3 scratch/probes/ray_transport_metal_overlap_repair.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-overlap-new --run-only --label series2
```

The wrapper verifies frozen fixture hashes, requires the original reproducer's expected source hooks, and fails if synthetic, independent-AST or aggregate accuracy checks fail. It never initializes a visible app or writes saves. It uses the configured app's existing objects, so build configuration must already exist. Its CPU timing is an AST diagnostic, not comparable to the old copied-envelope CPU loop as a performance regression.

## Shipping ledger / next concrete work

- **Established:** complete ordered native A* arithmetic on the frozen rays; original-AST agreement; five closed-form GPU witnesses; current-or-better aggregate reference accuracy; reproducible rejection of the fused variant.
- **Open:** complete unfused A* versus the current renderer on identical ray/viewport/output work; full-image qualification, actual schedule-generation/lookup/upload costs and memory limits, broad views/clocks/source/overlap, production revision/device/fallback lifetime, inhabited movement and Zach's acceptance.
- **Next:** compare the accurate unfused consumer with the actual current-renderer baseline under matching work. Only then decide which concrete boundary producer/amortization experiment is worth implementing. The 902 range-evaluation CPU producer is not a shipping implementation; no broad query-space framework is commissioned by this result.
- **No visible behavior changed.** Existing inhabited/Person checks remain open; no new manual interaction is claimed or required by this isolated diagnostic.

Artifacts: [two-series JSON](NORTHERN_VEIL_METAL_OVERLAP_REPAIR_2026-10-21.json), [raw pixel-labeled output](NORTHERN_VEIL_METAL_OVERLAP_REPAIR_2026-10-21_ROWS.json), [reproducer](../../scratch/probes/ray_transport_metal_overlap_repair.py). Raw row layout is documented in the combined JSON. The original failed report is retained in `NORTHERN_VEIL_METAL_DIAGNOSTIC_2026-10-21.*` with a later qualification notice.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T19:18:52.575755-07:00`.
