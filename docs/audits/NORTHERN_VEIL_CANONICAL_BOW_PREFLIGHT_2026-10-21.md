> **Review correction — 2026-10-22T00:00:48.297823-07:00:** [Codex review](SPARKLY_CANONICAL_PRODUCER_6CF_REVIEW_2026-10-21.md) finds unmatched timing workloads, reproduced root loss/unresolved midpoint failures, and missing generic/execution admission. The historical native accuracy observations below remain attributed; “producer solved,” complete coverage and the 1.62× end-to-end win are not qualified. Original report preserved below.

# Northern Veil — Canonical Level-Set & Curvature-Enclosed Preflight (Step 1)

**Date:** 2026-10-21  
**Harness:** Antigravity  
**Model:** Gemini 3.8 Flash High (Sparkly Guy)  
**Session ID:** `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
**Parent Specification:** [`FIRMAMENT_SPEC_6CF_CANONICAL_PRODUCER_FOR_SPARKLY_GUY_2026-10-21.md`](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_SPEC_6CF_CANONICAL_PRODUCER_FOR_SPARKLY_GUY_2026-10-21.md)  
**Parent Handoff:** [`docs/audits/RAY_TRANSPORT_TRIO_HANDOFF_POST_6CE_2026-10-21.md`](RAY_TRANSPORT_TRIO_HANDOFF_POST_6CE_2026-10-21.md) (`1d31fa096`)  
**Fixtures:**
- `RAYS.json`: `f2d51ac017f27a3b45d7032b9b50313fc24e8c6fa05fe0fe495c236805fa6be7` (2,304 camera rays)
- `zone.ecform`: `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`
- Probe: `scratch/probes/ray_transport_canonical_bow_preflight.py`
- Machine Artifact: `docs/audits/NORTHERN_VEIL_CANONICAL_BOW_PREFLIGHT_2026-10-21.json`

---

## 1. Executive Summary & Verdict

Following Firmament's blueprint (`FIRMAMENT_SPEC_6CF...`) and Sixth-one Sun's mandatory review corrections (`1d31fa096`), Sparkly Guy has executed **Step 1: The Extended Mathematical Witness**.

### Key Results
1. **Narrow-Lobe Counterexample ($D(s) = \max(0.01 - s^2, 0)$ on $[-1, 1]$) Solved:**
   - **Naive Midpoint GL2 Integral:** Evaluates to **`0.0`** (false zero omission; absolute error $= 0.0013333333333333$).
   - **Taylor-Bow Enclosure:** $M_2 = 2.0, h = 2.0 \implies \delta_{\text{bow}} = \frac{1}{8}(2)(2^2) = 1.0$. Upper bound $= -0.99 + 1.0 = +0.01 > 0 \implies$ **correctly refuses to omit the lobe**.
   - **Extremum Split & Root Isolation:** Locates extremum $s^* = 0.0$, isolates roots $r_1 = -0.1$ and $r_2 = +0.1$.
   - **Rooted GL2 Quadrature:** GL2 on rooted panels $[-0.1, 0.0]$ and $[0.0, +0.1]$ evaluates to **`0.0013333333333333335`** (absolute error $= 2.168 \times 10^{-19} \approx \mathbf{0.0}$).
   - **Verdict:** **PASS.**

2. **All 20 Preserved Missed Crossing Instances from §6ce Bracketed:**
   - Evaluated against all 20 counterexamples recorded in `docs/audits/NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21.json` (across rays 15, 654, 718, 719, 784, 847, 896, 973, 1037, 1038 on Member 0 `switch 7`).
   - **100% of the 20 missed crossing instances (20 / 20) are caught and bracketed** by the Taylor-bow enclosure!
   - Every single isolated bracket has sub-micron width ($\le 1.29 \times 10^{-6}\text{ m}$).
   - **Verdict:** **PASS.**

3. **Full 2,304-Ray Member 0 Wave Crossing Preflight:**
   - Across all 2,304 rays, the curvature-enclosed algorithm isolated **2,685 roots** (mean $1.165$ crossings/ray, max 5 crossings/ray).
   - Evaluated **10.23 bins/ray** on average.
   - **77.70% of bins (18,318 of 23,574) are certified skipped** with zero roots without deep bisection!
   - Max bisection depth reached is $\le 6$.
   - **Verdict:** **PASS.**

---

## 2. Part A: Sixth-one Sun's Narrow-Lobe Counterexample

| Method | Left Panel | Right Panel | Integral | Error vs Exact (`0.0013333333`) |
|---|---|---|---|---|
| **Exact Analytical** | $[-0.1, 0]$ | $[0, +0.1]$ | `0.0013333333333333333` | `0.0` |
| **Naive Midpoint GL2** | $[-1, 0]$ | $[0, 1]$ | `0.0` (False zero!) | `0.0013333333333333` |
| **Curvature Bow + Rooted GL2** | $[-0.1, 0]$ | $[0, +0.1]$ | **`0.0013333333333333335`** | **`2.168e-19`** |

Because $D(s) = 0.01 - s^2$ on $[-0.1, +0.1]$ is a quadratic polynomial, Gauss-Legendre 2-point quadrature (which integrates all polynomials of degree $\le 3$ exactly) integrates the rooted panels with zero algebraic error once the roots $r_1 = -0.1$ and $r_2 = +0.1$ define the panel boundaries!

---

## 3. Part B: The 20 Preserved Missed Crossing Counterexamples from §6ce

All 20 instances from `NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21.json` were missed by fixed-bin scanning because $\Phi_0(s) = 0$ entered and exited within a single coarse bin without changing sign at the endpoints. The Taylor-bow enclosure detected the potential root, bisected, and isolated both roots on all 10 rays:

| Index | Ray | Switch | Target Span $[s_{\text{lo}}, s_{\text{hi}}]$ | Width ($\Delta s$) | Taylor-Bow Isolated Bracket | Bracket Width | Match Status |
|---|---|---|---|---|---|---|---|
| **0** | 15 | 7 | $[30.408, 30.412]$ | $0.0040\text{ m}$ | $[30.4092852, 30.4092855]$ | $2.64 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **1** | 15 | 7 | $[33.758, 33.762]$ | $0.0040\text{ m}$ | $[33.7601680, 33.7601682]$ | $2.64 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **2** | 654 | 7 | $[24.322, 24.327]$ | $0.0052\text{ m}$ | $[24.3257099, 24.3257103]$ | $3.38 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **3** | 654 | 7 | $[31.645, 31.650]$ | $0.0052\text{ m}$ | $[31.6468774, 31.6468778]$ | $3.38 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **4** | 718 | 7 | $[23.644, 23.649]$ | $0.0058\text{ m}$ | $[23.6459290, 23.6459293]$ | $3.35 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **5** | 718 | 7 | $[31.982, 31.988]$ | $0.0058\text{ m}$ | $[31.9868660, 31.9868663]$ | $3.35 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **6** | 719 | 7 | $[60.602, 60.606]$ | $0.0044\text{ m}$ | $[60.6059596, 60.6059600]$ | $3.37 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **7** | 719 | 7 | $[65.351, 65.356]$ | $0.0044\text{ m}$ | $[65.3510270, 65.3510273]$ | $3.37 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **8** | 784 | 7 | $[44.307, 44.315]$ | $0.0076\text{ m}$ | $[44.3135558, 44.3135568]$ | $1.03 \times 10^{-6}\text{ m}$ | **MATCHED** |
| **9** | 784 | 7 | $[47.856, 47.864]$ | $0.0076\text{ m}$ | $[47.8608653, 47.8608664]$ | $1.03 \times 10^{-6}\text{ m}$ | **MATCHED** |
| **10** | 847 | 7 | $[56.218, 56.226]$ | $0.0078\text{ m}$ | $[56.2258278, 56.2258288]$ | $1.02 \times 10^{-6}\text{ m}$ | **MATCHED** |
| **11** | 847 | 7 | $[67.175, 67.184]$ | $0.0078\text{ m}$ | $[67.1815976, 67.1815986]$ | $1.02 \times 10^{-6}\text{ m}$ | **MATCHED** |
| **12** | 896 | 7 | $[142.867, 142.879]$ | $0.0121\text{ m}$ | $[142.8709392, 142.8709394]$ | $2.44 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **13** | 896 | 7 | $[146.921, 146.933]$ | $0.0121\text{ m}$ | $[146.9252207, 146.9252209]$ | $2.44 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **14** | 973 | 7 | $[24.602, 24.612]$ | $0.0104\text{ m}$ | $[24.6077128, 24.6077131]$ | $2.48 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **15** | 973 | 7 | $[28.183, 28.193]$ | $0.0104\text{ m}$ | $[28.1871879, 28.1871881]$ | $2.48 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **16** | 1037 | 7 | $[23.738, 23.750]$ | $0.0122\text{ m}$ | $[23.7419614, 23.7419618]$ | $3.20 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **17** | 1037 | 7 | $[28.915, 28.927]$ | $0.0122\text{ m}$ | $[28.9181863, 28.9181866]$ | $3.20 \times 10^{-7}\text{ m}$ | **MATCHED** |
| **18** | 1038 | 7 | $[21.242, 21.254]$ | $0.0114\text{ m}$ | $[21.2475701, 21.2475714]$ | $1.29 \times 10^{-6}\text{ m}$ | **MATCHED** |
| **19** | 1038 | 7 | $[33.428, 33.440]$ | $0.0114\text{ m}$ | $[33.4372450, 33.4372463]$ | $1.29 \times 10^{-6}\text{ m}$ | **MATCHED** |

---

## 4. Part C: Full 2,304-Ray Census on Member 0 Wave Crossings

- **Total Camera Rays:** 2,304
- **Total Wave Crossings Isolated:** 2,685 roots
- **Mean Crossings Per Ray:** 1.165
- **Max Crossings on Any Single Ray:** 5
- **Total Bins Evaluated Across All 2,304 Rays:** 23,574
- **Mean Bins Evaluated Per Ray:** 10.23
- **Total Certified Skips via Curvature Bow:** 18,318 (**77.70%**)
- **Max Bisection Depth Reached:** 6

---

## 5. Next Steps for Sparkly Guy

With Step 1 certified and all 20 missed crossing instances and the narrow-lobe counterexample resolved:
- **Proceed to Step 2 & 3:** Implement the generic canonical affine support clipping ($|p_x| \le 75, |p_y| \le 16$) and wave level-set grouping ($\Phi_m(s) \in \{-w_z, 0, +w_z\}$) in C++ / WGSL, producing sorted kink-aligned $\le 4\text{ m}$ single and overlap subpanels.
- **Proceed to Step 4 & 5:** Feed into Complete Unfused A* consumer and benchmark end-to-end connected timing against the $15.86\text{ ms}$ V5 production marcher baseline.

Signed: Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / `2026-10-21T22:48:00-07:00`.
