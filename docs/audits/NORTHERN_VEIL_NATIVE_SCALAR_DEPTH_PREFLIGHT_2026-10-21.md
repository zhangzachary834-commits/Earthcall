# Northern Veil native scalar-depth preflight: the bridge executes, the candidates fail cost

Zach commissioned the next pass toward his current-or-better-accuracy shipping checkpoint. This continues the [Firmament review and checkpoint thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#codex-native-preflight-in-progress--sample-runs-not-box-exclusivity). Zach originated the authored-equation/performance goal and smaller shipping scope; Firmament proposed scalar-depth reduction, anisotropic allocation and Simpson sampling; Opus took the converged multi-medium reference. Codex implemented the native census and the isolated experiments below.

**Decision:** retain the evidence and reproducibility tools, **no production optimization**. A fixed-fixture raw proportional coefficient derived in OntoMath is now experimentally executed by the actual native volume path with single-contributor run batching, but both batching versions are slower. Literal-zero source specialization is flat. Fixed-budget grid redistribution does not give a repeatable primary gain. This is not the shipping checkpoint; native accuracy/reference/lifecycle admission and a profitable consumer remain open.

## Native work census: contributing overlap differs sharply from box overlap

The corrected authored-source fixture runs the production equations on actual 2560×1440 rays, selecting 64×36 stratified pixels. Only a scratch renderer is instrumented. Other pixels discard early; there are no per-step global atomics. Counters are private and copied once per selected ray. This instrumentation changes **8 of 9,216 selected RGBA bytes by one level** in the inside/time-7.25 comparison with the frozen uninstrumented capture. Counts describe the instrumented execution; they are not exact uninstrumented instruction counts, a mathematical admission certificate or timing evidence.

Each view has 2,304 selected rays. Density/positive/exponential counts agree at times 0 and 7.25, as expected for these density fields; that does not prove arbitrary clock independence. Zero incident-light contribution sites and zero legacy-cutoff rays are observed in these samples. A floor site is conservatively excluded whenever projected extinction is not strictly between 1e-6 and 1e30.

| View | Density calls | Positive medium calls | Current exp updates | Single member, nonfloor | Single-member box sites | Optimistic run exp updates |
|---|---:|---:|---:|---:|---:|---:|
| inside | 147,796 | 67,184 | 67,003 | 66,774 | 2,701 | 3,558 |
| spawn | 35,917 | 15,558 | 15,526 | 15,457 | 1,613 | 1,604 |
| reverse | 100,405 | 46,279 | 46,209 | 46,111 | 27,791 | 4,425 |
| side | 69,667 | 29,626 | 29,460 | 29,132 | 2,042 | 1,954 |

Inside has 181 mixed-contributor updates and 48 single-floor updates. The remaining 66,774/67,003 updates (99.66%) have one contributing member, but only 2,701 (4.03%) lie in single-member boxes. The optimistic 3,558 is 3,329 same-contributor runs plus 229 generic updates. Runs may continue through zero gaps and V5 segment boundaries. A segment-local version would need 3,842. Neither number includes coefficient/domain/rounding eligibility or new execution overhead. **A box-exclusive consumer would miss most of the observed opportunity.**

## Native numerical prototype, not production admission

The scratch compiler uses the existing `OntoMath::deriveProportionalFieldCoefficient` on the actual extinction and each emission component. It requires single unbounded channel pieces, a literal-zero authored source and coefficient dependencies limited to the medium clock. Screen lowers the returned coefficient arithmetic; there is no curtain-name match or manually supplied colour ratio. For this experiment only, coefficient constants are embedded in the generated shader. Numeric-edit/retained-parameter recipes and general device/lifecycle admission are unfinished; this representation must not be promoted as written.

Every original density/channel evaluation, sample location, box event, proof walk, medium order, floor and mixed-medium update remains. A run with one contributing member accumulates optical depth; its radiance/attenuation is reconstructed on a member change, fallback or final answer. Mixed, floor, nonfinite, nonzero-incident or invalid-colour sites flush first and use the old update. Pending runs require pre-run transmittance >0.1 and pending optical depth ≤1, keeping a deferred answer far above the legacy 0.01 cutoff. This is a conservative work-grouping guard, not a changed cutoff or error setting. The prototype changes floating arithmetic grouping and has no completed execution-error/continuum admission proof.

`batch` computes four coefficient vectors before the ray walk. `lazy` evaluates a coefficient only when needed at a run start and additionally specializes the proven literal-zero source. `dark` tests that source specialization alone, with no batching. Activation is printed during pipeline compilation; generated shader dumps verify all four coefficient functions and the batch state. Compatibility lighting has no literal-zero source admission: the lazy variant bypasses both changes and reproduces all **14,745,600** old compatibility bytes exactly. This is one useful negative witness, not complete structural/edit coverage.

First full-resolution inside/time-7.25 comparisons: both batching forms differ in **493 of 14,745,600 bytes, maximum one level**; source-only and allocation are exact in that capture. These are observed differences against the current renderer, not demonstrated current-or-better error against the independent authored reference. Cost rejection ends broader accuracy work on these candidates; the Opus reference is not claimed completed or consumed here.

## Paired native economics

Native desktop WebGPU/Metal, configured Debug host objects, corrected authored-source frozen volume fixture, 2560×1440, inside/time 7.25. Exactly two sequential ABBA series per candidate, 24 readback-free timed frames/run after four warm frames and a completion fence. All **32 runs / 768 timed frames** are included in the [numerical evidence](NORTHERN_VEIL_NATIVE_SCALAR_DEPTH_PREFLIGHT_2026-10-21.json). No concurrent GPU correctness workload or ordinary timestamp query runs during timing. Every final warm record has zero shader compiles, lowerings, parameter bytes and proof bytes uploaded.

| Candidate | Paired baseline → candidate mean ms | Separate series cost changes | Inside capture difference |
|---|---:|---:|---|
| batch | 86.213 → 97.283 | +11.02% / +14.53% | 493 bytes; max 1 |
| dark | 86.864 → 87.213 | +0.74% / +0.09% | 0 bytes; max 0 |
| lazy | 99.814 → 110.814 | +11.78% / +10.30% | 493 bytes; max 1 |
| allocation | 88.478 → 86.731 | +1.08% / -4.66% | 0 bytes; max 0 |

Positive percentages are slower. The preflight screen requires both primary series improve at least 5% before broader promotion work; the proposed substantial checkpoint target remains 20%, which is Codex's engineering target rather than a number Zach supplied. None passes the screen. Allocation's pooled improvement is not a repeatable win: its first series loses and its second is below the 5% screen. No retesting or best-run selection was used to manufacture a pass.

Reducing exponential updates is not sufficient evidence of lower native cost. Additional branches, live state and compiler scheduling are plausible costs; **no hardware occupancy/register counter measurement is claimed**. Neither does this prove every possible scalar-depth consumer must lose. The unchanged field work remains a major quantity a future strategy must address.

## Fixed-budget anisotropic allocation is distinct from older cropped-grid experiments

The 64×8×64 trial changes only the scratch CPU grid dimensions over the **original full box**. It keeps the existing proof theorem, 1% cell enlargement, packing, metadata and shader; no extra grid-domain/parent payload or shader branch is introduced. It differs from the rejected cropped-support, full-height cropped and parent-bank trials of October 15. The existing builder already proves whole blocks hierarchically. Its 32,768 logical cells occupy 5,464 bitmap bytes per medium before metadata under 24-bit-f32 packing.

| View | Density calls with new allocation | Change | Positive sites / exponentials |
|---|---:|---:|---|
| inside | 147,796 → 129,830 | -12.16% | unchanged |
| spawn | 35,917 → 29,119 | -18.93% | unchanged |
| reverse | 100,405 → 82,279 | -18.05% | unchanged |
| side | 69,667 → 70,296 | +0.90% | unchanged |

Thus redistribution reduces inside empty-site field work by 12.16%, not the forecast halving of the halo; side gets slightly worse. All original positive-member and exponential counts match the diagnostic baseline. Visited proof-walk work can rise. The full-resolution inside uninstrumented capture is exact, but cost fails; other-view performance/complete exact images/lifecycle checks are deliberately not expanded for this rejected experiment. No general allocator policy is installed.

## Reproduce and continue

Build the app's configured objects first (see BUILD_AND_ENVIRONMENT); keep GPU runs sequential. These drivers compile/link scratch copies, never add a CMake target or write saves:

```
python3 scratch/probes/ray_transport_native_preflight.py --output /private/tmp/new-native-census
python3 scratch/probes/ray_transport_batch_trial.py --variant batch --output /private/tmp/new-batch
python3 scratch/probes/ray_transport_batch_trial.py --variant dark --output /private/tmp/new-dark
python3 scratch/probes/ray_transport_batch_trial.py --variant lazy --output /private/tmp/new-lazy
python3 scratch/probes/ray_transport_allocation_trial.py --output /private/tmp/new-allocation --census /private/tmp/new-native-census
```

`ray_transport_native_preflight.py` prints diagnostic counts, never performance. For native cost use the resulting **uninstrumented** batch/allocation binary with the existing `volume_demand_probe` CLI and `cost 24`; freeze an authored-source baseline first and follow ABBA. The allocation driver builds its own corrected probe object; it does not need the historical compatibility probe. The batch driver’s inline constants are frozen experiment state; do not use it to claim edit/device admission. Generated sources/binaries/captures/logs are under the artifact roots in the evidence JSON; tracked drivers and all native run means remain even if temporary binaries are cleaned.

**Next work:** retain the actual-contributor census, avoid assuming box overlap blocks every reduction, and preflight an implementation that materially reduces expensive field work rather than only counting fewer transport operations. The unconverged/raw per-medium Simpson oracle is not an executable full-colour candidate. Opus's independent projected four-medium reference remains the accuracy dependency for a changed numerical method. A future intrinsic-reuse experiment would need a new mechanism addressing the old per-medium cache state and native failures, with hit-rate/state/cost preflight; none is activated or promised here. No generic CPU-integral campaign completion is imposed as a shipping prerequisite.

**Shipping ledger:** current-accuracy scope and corrected baseline available; scratch raw-coefficient/native-run bridge executes but fails speed; no admitted production reduced consumer or substantial gain; reference comparison, edit/device/lifecycle and inhabited-frame/Person evidence remain open. No source under src/, installed app, live data home, save, sample setting, output budget, lighting or ordinary timestamp setting changed. No new Person-visible behavior or Person check; existing movement checks remain. No PR or push.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T02:41:55.520921-07:00`.
