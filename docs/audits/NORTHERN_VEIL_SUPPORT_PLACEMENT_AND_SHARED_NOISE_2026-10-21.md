# Northern Veil: support-aware placement and shared Perlin reuse do not earn promotion

Zach commissioned this next pass with his current-or-better accuracy and substantial native speed checkpoint as the goal. This continues [the existing Intercom conversation](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#codex-next-pass--support-aware-schedule-discovery-and-work-preflight), Opus's support-placement suggestion and [his independent projected reference](NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21.md). **No new production optimization is admitted.** Forty-eight CPU placement strategies give insufficient combined accuracy/work headroom. Two new shared-slot noise variants keep the tested full image exact but fail native cost. The existing renderer, authored math, saves, installed app, resolution, lighting, sample count, budgets and default certificate switch are unchanged. No PR or push.

## Executable support discovery, then numerical/work rejection

The CPU-only `scratch/probes/ray_transport_support_preflight.py` consumes the actual exported f32 native rays and clock 7.25. It verifies the fixture SHA and all 2,304 pixel coordinates against the independent reference. Existing `VolumeZeroProof` bitmaps yield leaf DDA intervals; existing column envelopes yield slab intersections. Neither discovery method samples a field to find truth. Over these rays there are **84,549 leaf visits** and **45,687 column packet queries**; every unknown leaf interval is enclosed by the column interval union. The geometry witnesses compare DDA against brute cell intersections on 240 random rays and check subset/progress on 16 face/axis/tie rays. The latter are not equality proofs for arbitrary closed-face synthetic bitmaps. This geometry screen does not grant any new backend numerical skip eligibility.

CF4 and shared-endpoint RK4 integrate the coupled T/RGB problem after splitting overlapping interval boundaries. The run screen tries 1,2,3,4,6,8,12,16 panels per conservative unknown span with leaf and column geometry (32 strategies). A second screen retains individual cell-face boundaries and tries 1–4 panels per interval (16 strategies). RK4 shares endpoints within and across adjacent equal-active intervals. An analytic constant-sigma/affine-source witness converges at fourth order; no negative RK stages occur in the candidates. CF4's independent smooth/thin-slab evidence remains in its parent trial. Panel counts are fixed per strategy, never selected per ray using reference answers.

Field evaluation uses the authored MathNodes with f32 field inputs, while ray geometry and transport accumulation use double. Literal zero authored incident source is checked; D>0 gating, 1e−6 extinction floor, nonnegative emission, boxes and shared attenuation are included. These are **optimistic CPU accuracy and executable-work screens**, not native f32 qualification or native timing. Support discovery is counted separately and not assumed free; exponentials, branch/packet scheduling and register costs would remain in a native implementation.

| Candidate | Density calls | Positive calls | RGB mean error | RGB max error | T mean error |
|---|---:|---:|---:|---:|---:|
| Current native reference bar |147,796|67,184|0.00005223|0.00195826|0.00013352|
| CF4 leaf runs,12 panels |133,456|47,465|0.00007027|0.00190263|0.00019557|
| CF4 leaf runs,16 |176,878|62,964|0.00005012|0.00154840|0.00014047|
| CF4 column runs,16 |250,978|62,751|0.00004851|0.00121948|0.00013529|
| CF4 leaf cells,1 |84,470|36,147|0.00051287|0.00731211|0.00139043|
| CF4 leaf cells,3 |227,662|99,667|0.00005466|0.00077855|0.00015281|
| CF4 leaf cells,4 |299,276|131,472|0.00002924|0.00070003|0.00007870|

The cheaper options worsen mean accuracy; recovering accuracy needs approximately current or much greater field work, before discovery/transport overhead. In particular, the accurate leaf-cell4 strategy doubles field work. This does **not** prove all support-aware methods unprofitable, and call counts are not native cost. It gives insufficient headroom to justify another GPU implementation of these 48 strategies. No support-placement shader was built. All48 results, including RK4 and error tails, are in the companion JSON; no favorable subset is omitted.

## A distinct exact-reuse mechanism, screened on Metal

The [earlier Perlin cache trials](NORTHERN_VEIL_PERLIN_REUSE_EXPERIMENTS_2026-10-11.md) kept state per medium and lost native cost. The new motivation is the actual contributor census: nearly all positive updates have one member. `scratch/probes/ray_transport_shared_noise_trial.py` therefore tests **one private invocation slot shared across every caller of the pure Perlin intrinsic**. Full mode stores eight normalized gradients, 28 logical scalar components total versus roughly 112 in the old four-member form; XY mode stores one vec4 permutation plus key/valid, 8 logical components total. Compiler allocation/occupancy is not measured and is not asserted as cause.

Keys use the bits of actual f32 floor(P), including signed zeros. Only cells with all |floor(P)|<2^20 are reusable; large/nonfinite inputs keep original computation, and the adjacent+1 is exact in this predicate. There is no cached final noise value: fractional positions, dot products, fade, final interpolation and original lattice/transport remain. All miss permutations/normalization arithmetic remain original. State is Kernel-only scratch for one shader invocation, not authored, persisted, shared between rays, or retained across clocks/devices. The experiment hooks the four-member program only; it is not production eligibility. Mathematical meaning is unchanged; no new renderer-private transport theorem is introduced.

A separate native census on 64×36 actual selected pixels of the 2560×1440 image records **67,184 calls**, **59,933 full-cell hits (89.21%)**, **60,290 XY hits (89.74%)**, zero out-of-range bypasses. Instrumentation changes shader behavior/cost and is used only to screen reuse, never for timing or uninstrumented parity claims.

Both uninstrumented variants match the frozen current inside/time 7.25 **full 14,745,600-byte RGBA image exactly**. This is a finite image check, not intrinsic bit equivalence at all inputs, edit/device lifetime admission or Person acceptance. The existing standalone Perlin test would not exercise this renderer-only scratch transform, so it was not used to claim coverage.

## Complete native cost: no repeatable gain

All table runs use the same compiled scratch fixture and program, with `EC_SHARED_NOISE_BASELINE=1` bypassing the transform for A. Viewport 2560×1440, inside camera (0,50,75) looking toward (40,62,110), clock 7.25, four media, original 96 samples/chord, authored incident source zero. Two ABBA series per mode, 24 timed frames after 4 warm, completion fences around the full submission batch, no native timestamps, capture, readback or correctness workload during cost. All 16 table runs/384 frames are retained; final warm compiles/lowerings/parameter/proof uploads are zero. Projection/preparation, submission and shader cost are included. Offscreen volume-only cost is not live inhabited FPS.

| Slot | Series | Baseline ms/frame | Candidate ms/frame | Change |
|---|---:|---:|---:|---:|
| full | 1 | 95.922 | 97.820 | +1.98% |
| full | 2 | 104.047 | 119.220 | +14.58% |
| xy | 1 | 110.853 | 111.275 | +0.38% |
| xy | 2 | 113.690 | 114.465 | +0.68% |

An initial full-cache pair of ABBA series (8 runs/192 frames) overlapped compilation of the XY executable. Those preliminary runs are preserved separately in JSON, with all logs, but excluded from the clean table. Compilation finished before the clean screen. No extra series was run after the completed clean results: full cache is slower and XY is flat. Device-wide interference/thermal drift cannot be excluded on a shared desktop; paired results provide no profitable checkpoint. No hardware-cause inference or significance claim is made.

## Reproduction and remaining shipping gates

From this branch, use fresh external output directories:

```sh
python3 scratch/probes/ray_transport_support_preflight.py --root "$PWD" --output /private/tmp/veil-support-run
python3 scratch/probes/ray_transport_support_preflight.py --root "$PWD" --output /private/tmp/veil-support-cells --cells
python3 scratch/probes/ray_transport_shared_noise_trial.py --root "$PWD" --output /private/tmp/veil-noise-hits --mode hits --capture
python3 scratch/probes/ray_transport_shared_noise_trial.py --root "$PWD" --output /private/tmp/veil-noise-full --mode full
python3 scratch/probes/ray_transport_shared_noise_trial.py --root "$PWD" --output /private/tmp/veil-noise-xy --mode xy
```

Builders use configured app compile/link recipes, replacing scratch renderer/entry objects; production source is never rewritten. Executables retain the generic parent builder's filename `cf4`, but the noise trial applies **no CF4 transform**. Its metadata removes the inherited unrelated quadrature witness. For a noise image, run the binary with `EC_CAM=0,50,75,40,62,110`, fixture `saves/zones/Northern Veil/zone.ecform`, arguments `0123 2560 1440 7.25 <output.rgba> 1`. For timing replace the last two arguments with `cost 24`; alternate baseline/shared/shared/baseline, twice, after compilation. The hit build additionally needs `EC_CENSUS=<output.bin>`; counters are u32 slots20–23 of each64-word ray record. Do not use another trial's panel-count interpretation for those words. Never time the capture build. `EC_SHARED_NOISE_WGSL=<file>` optionally dumps WGSL; unset it during cost.

**Shipping account:** the independent accuracy comparator is available and consumed. This pass closes two candidate avenues without changing shipped rendering. Still open: a consumer passing current-or-better accuracy and substantial complete native savings together; production eligibility/fallback and edit/device/revision lifetime where applicable; broader cameras/clocks/clipping; then inhabited movement timing and Person acceptance. Do not repeat these unchanged48 schedules or old/new caches because their node/hit counts look attractive. A future cell-polynomial/ray-reduction design must first count preparation, state and boundary work and respect native arithmetic; a high reuse rate is not a measured speedup. Any mathematical recipe belongs in OntoMath, with Screen lowering it. No new visible behavior or Person action is introduced by this pass.

**Evidence:** [machine-readable results and every cost log](NORTHERN_VEIL_SUPPORT_PLACEMENT_AND_SHARED_NOISE_2026-10-21.json); CPU source and two scratch drivers above. Production src/ diff remains empty. Python compilation of both new drivers passes. No production suite is claimed rerun for scratch-only rejected experiments.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T11:34:16.105280-07:00`.
