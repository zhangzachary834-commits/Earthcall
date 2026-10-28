# Northern Veil full-workload reference — every wide-inside ray, not 192

Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-27 (times below are PDT).

Zach asked me to rejoin the council and analyze after §6bn. Since then the council ran §6bo–§6ei: 145 commits, ~45 native qualifications and cost screens. Since §6cm, every accuracy gate in that run has been measured on **192 predetermined holdout rays** out of the 32,784-ray `wide-inside` workload. This audit runs the same independent converged reference (my §6bn probe, [`scratch/probes/veil_projected_reference.cpp`](../../scratch/probes/veil_projected_reference.cpp), unchanged) on **all 32,784 rays**. That turns "current-or-better accuracy" into a full-workload test. CPU only, 4 threads. No GPU work, no engine/save/app change.

## Inputs (read only)

| Input | Identity |
|---|---|
| Rays | `/private/tmp/earthcall-wide-case/rays.json`, sha256 `cd403a01…a402` (Codex's wide-case manifest; same file every §6c*–§6ei probe reads via `EC_POSE_RAYS`) |
| Current outputs | `fresh/baseline.json` in [§6ei's archive](GPU_RAY_TRANSPORT_6EI_CLOCK_AND_COUNSEL_REVIEW_2026-10-27.zip) — the contemporary clean Current run |
| Candidate outputs | `fresh/expected.json` in the same archive — the §6eg conservative candidate (Mode 5), which §6ei verified bit-identical in its clean run |
| Fixture | `saves/zones/Northern Veil/zone.ecform`, clock 7.25, authored zero root radiance (the probe checks and refuses otherwise) |

## The reference is the one the council already trusts

On the 192 holdouts, the rebuilt probe reproduces Codex's `reference.json` **bit for bit** (max difference 0.0). It also reproduces §6ei's `accuracy.json` exactly: Current RGB max 0.00149 / T max 0.00232, candidate RGB max 0.000859 / T max 0.00125. Each ray is integrated with 4th-order Simpson at 2¹⁶ and 2¹⁵ intervals. Full-workload agreement between the two: RGB 6.5e−8, T 1.1e−7 (both far below every error compared here).

## Result

Errors against the reference, all 32,784 rays (2¹⁶ intervals; 2¹⁵ gives the same verdicts to 7 digits):

| | RGB mean | RGB p99 | RGB max | T mean | T p99 | T max |
|---|---:|---:|---:|---:|---:|---:|
| **Current** (the bar on this workload) | 5.23e−5 | 7.19e−4 | **2.24e−3** (0.57 levels) | 1.34e−4 | 1.40e−3 | **3.47e−3** |
| §6eg candidate (Mode 5) | 4.38e−5 ✅ | 8.01e−4 ❌ | **6.93e−3** (1.77 levels) ❌ | 1.19e−4 ✅ | 3.10e−3 ❌ | **1.08e−2** ❌ |

**The candidate fails four of the six current-or-better gates** (RGB p99 and max, T p99 and max) and passes only the two means. **316 rays** exceed Current's worst RGB or T error. **None of them is among the 192 holdouts.** Current's own full-workload max (2.24e−3 RGB, 3.47e−3 T, worst ray at pixel 1785, 1115) is ~50% above its holdout max, so the holdouts also understated the bar.

## Where and why the candidate fails

The failing rays form one coherent region of the image, not scattered noise: the 316 rays lie in px 1895–2555, py 455–1205, the right third of the image. The worst ray, index 26392 (pixel 2375, 1155), shows the mechanism:

| | Cyan-ribbon (member 1) on ray 26392 |
|---|---|
| True positive-density support (reference, 2¹⁶ samples) | s ∈ [69.070, 74.136] |
| Candidate subpanels | [66.056, 69.782] · [69.782, 73.509] · [73.509, 77.235] — three **equal** 3.726-wide panels |
| Healthy neighbour 22965 (pixel 2235, 1005) | support [66.933, 71.897]; the candidate places apex/root brackets at 69.38–69.43 and 71.80–71.83 |

On the failing ray, neither support edge nor the tent apex is a panel boundary. Each fixed-node panel then straddles a kink of `max(D, 0)`. That is the error mode §6eg's conservative bracket retention accepted when it kept wide brackets instead of bisecting. The region overlaps where CF4 failed in §6bn (px 2260–2340, py 900–1180): the cyan ribbon is crossed at a grazing angle there.

The bisection modes do not share this. From §6eg's own `conservative/analysis.json`, Modes 2 and 3 have holdout max error 4.8e−5 RGB / 7.0e−5 T, about 31× better than Current. Their full-set deviation from Current (0.00223 RGB / 0.00348 T) is the size of Current's own error. Mode 5 deviates by 0.00721 / 0.01125. Modes 2/3 output bits were not in the archives I read, so their full-set check against these rows is left to the native owner (one comparison, no GPU).

## What this changes

1. **§6ei's "the candidate still meets the same bounded numerical checkpoint" holds only on the holdouts.** On the full workload the conservative candidate fails the current-or-better max gate. §6ej already returns to unchanged bisection. This result says that was the right call on accuracy as well as cost.
2. **Gate on all rows, not 192.** The holdouts are evenly spaced in ray order (stride 170–171) and missed all 316 failing rays. A uniformly random 192-ray sample would have caught at least one with only ~84% probability. Either way, a holdout pass is weak evidence about a max or p99. The rows below are reusable for any future candidate on this workload: compare outputs, no GPU needed.
3. **The bar on the full workload** is Current's full-set error (table above), not its holdout error.

## Rows

[`NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz`](NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz): 32,784 rows in ray-file order, `[px, py, T, Lr, Lg, Lb, T_half, Lr_half, Lg_half, Lb_half]`, at full double precision. [`…_SUMMARY.json`](NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_SUMMARY.json) holds the gate numbers. Replay: build the probe as in §6bn (Release flags/link line of `webgpu_line_quadrature_test`), then run `veilref rays.json 16 1000000 4 out.json` from the worktree root.

## Limits

One pose (wide-inside), one clock (7.25), 32,784 of 36,864 captured native rays (Codex's rank selection). This is a numerical reference for the projected model the native marcher implements: box-contained members, f32 sample positions, the 1e−6 extinction floor, authored zero source. It is not a certificate. I did not touch, rerun or time any GPU path.
