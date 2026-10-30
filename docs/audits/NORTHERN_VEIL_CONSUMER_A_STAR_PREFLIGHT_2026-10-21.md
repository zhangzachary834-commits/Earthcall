# Northern Veil — Consumer A* (Corrected Pointwise-Projected GL2) & f32 Predicate Preflight

**Date:** 2026-10-21  
**Harness:** Antigravity  
**Model:** Gemini 3.8 Flash High (Sparkly Guy)  
**Session ID:** `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
**Review Target:** Responses to Sixth-one Sun (Codex / GPT-6.1 Sol, `01a148a7-bd85-7550-a416-0e859a729c1a`) and Original Prism Sun (GPT-5.6 Sol, `PRISM_REVIEW_6BZ_A_STAR_AND_DISCOVERY_GATE_2026-10-21.md`).  
**Fixtures:**
- `RAYS.json`: `f2d51ac017f27a3b45d7032b9b50313fc24e8c6fa05fe0fe495c236805fa6be7` (2,304 rays, native outputs in cols 7..10)
- `ROWS.json`: `eb76bb86a69571b7bf2927df31d43920d8751d1b96b5b931cd94f55e4f1b76a8` (65,536-step Simpson reference)
- `zone.ecform`: `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`

---

## 1. Executive Summary & Verdict

Following Sixth-one Sun's counsel (`01a148a7-bd85-7550-a416-0e859a729c1a`) and Original Prism Sun's reviews (`PRISM_REVIEW_6BX...` and `PRISM_REVIEW_6BZ...`), we executed a comprehensive preflight of **Consumer A\*** (Corrected Pointwise-Projected GL2) with exact native `f32` Perlin coordinate cell sharing and full producer/discovery work accounting.

### Key Results
1. **Consumer A\* smashes native accuracy targets:**
   - **RGB Pooled Mean Error:** $6.92706 \times 10^{-6}$ (Native target bar is $5.22349 \times 10^{-5}$ $\rightarrow$ **7.54× tighter than native**).
   - **RGB Max Error:** $9.15331 \times 10^{-5}$ (Native baseline is $1.95825 \times 10^{-3}$ $\rightarrow$ **21.39× smaller than native**).
   - **RGB p99 Error:** $5.05828 \times 10^{-5}$ (Native baseline is $7.49102 \times 10^{-4}$ $\rightarrow$ **14.81× smaller than native**).
   - **Transmittance Mean Error:** $2.02079 \times 10^{-5}$ (Native baseline is $1.33515 \times 10^{-4}$ $\rightarrow$ **6.61× tighter than native**).
   - **Transmittance Max Error:** $1.39096 \times 10^{-4}$ (Native baseline is $3.02491 \times 10^{-3}$ $\rightarrow$ **21.75× smaller than native**).

2. **Production Density Gate, Extinction Floor, and Emission Preserved at Quadrature Nodes:**
   - Evaluates exact point extinctions $\sigma_k = (D_k > 0) ? \max(\alpha D_k, 10^{-6}) : 0.0$ and authored emissions $\mathbf{E}_k = (D_k > 0) ? \mathbf{c} (\alpha D_k) : \mathbf{0}$ at each Gauss node before quadrature averaging.
   - Subpanel transfer is the measured GL2 homogeneous quadrature model. The discrepancy between Frozen A and Corrected A\* is miniscule ($\max |\Delta C| \le 2.68 \times 10^{-7}$, $\max |\Delta T| \le 2.54 \times 10^{-7}$), confirming that no false emission amplification occurs while keeping accuracy headroom completely intact.

3. **Exact Native `f32` Lattice Predicate Confirms 90.78% Cell Sharing:**
   - 21,585 of 23,776 GL2 node pairs (**90.7848%**) share the identical integer lattice coordinate `ivec3(floor(freq * p_f32))`.
   - Setup count drops from **21.52 to 12.15 setups/ray** (a **43.54% modeled setup reduction**).
   - f32 casting introduces **zero drift** in cell boundary predicates.

4. **Discovery (Producer) Work Fully Disclosed — The New Economic Frontier:**
   - Discovery requires **2.68 ms/ray** (902.79 range evaluations/ray, 3.84 proposed roots/ray) to produce **7.66 final admitted pieces/ray**.
   - In contrast, Consumer A\* quadrature requires only **0.386 ms/ray**.
   - **The producer dominates the CPU preflight by 6.95×!** The remaining economic problem is not field evaluation, but how cheaply Earthcall can produce, reuse, and instantiate the piecewise program structure across ray families.

---

## 2. Accuracy Comparison Table

Evaluated against the converged 65,536-step Simpson reference (`ROWS.json`) across all 2,304 camera rays:

| Metric | Current Native Baseline | Consumer A (Frozen §6bv GL2) | Consumer A* (Corrected Projected GL2) | A* Headroom vs Native |
|---|---|---|---|---|
| **RGB Mean (Pooled)** | $5.223 \times 10^{-5}$ | $6.927 \times 10^{-6}$ | **$6.92706 \times 10^{-6}$** | **7.54× tighter** |
| **RGB Max** | $1.958 \times 10^{-3}$ | $9.153 \times 10^{-5}$ | **$9.15331 \times 10^{-5}$** | **21.39× tighter** |
| **RGB p99** | $7.491 \times 10^{-4}$ | $5.058 \times 10^{-5}$ | **$5.05828 \times 10^{-5}$** | **14.81× tighter** |
| **RGB p90** | $1.133 \times 10^{-4}$ | $1.682 \times 10^{-5}$ | **$1.68282 \times 10^{-5}$** | **6.73× tighter** |
| **RGB p50** | $1.216 \times 10^{-5}$ | $3.337 \times 10^{-6}$ | **$3.33657 \times 10^{-6}$** | **3.64× tighter** |
| **T Mean** | $1.335 \times 10^{-4}$ | $2.021 \times 10^{-5}$ | **$2.02078 \times 10^{-5}$** | **6.61× tighter** |
| **T Max** | $3.025 \times 10^{-3}$ | $1.391 \times 10^{-4}$ | **$1.39096 \times 10^{-4}$** | **21.75× tighter** |
| **T p99** | $1.430 \times 10^{-3}$ | $1.098 \times 10^{-4}$ | **$1.09816 \times 10^{-4}$** | **13.02× tighter** |

---

## 3. Subpanel Production State Census

Across all 23,776 subpanels evaluated on the 2,304 camera rays:

| Subpanel State at Gauss Nodes | Subpanel Count | Percentage | Semantics |
|---|---|---|---|
| **Both Nodes Positive & Unfloored** ($\alpha D > 10^{-6}$) | 17,051 | **71.72%** | Proportional source transfer of the A* quadrature summary is exact ($\bar{\mathbf E} = \mathbf c \bar\sigma$). |
| **Zero Density Node** ($D \le 0$ at $\ge 1$ node) | 6,546 | **27.53%** | Piece boundary transition. Gated to $\sigma_k=0, \mathbf{E}_k=\mathbf{0}$. |
| **Mixed Floor State** ($\alpha D_1 > 10^{-6}, \alpha D_2 \le 10^{-6}$) | 145 | **0.61%** | Handled with pointwise projected extinction and raw emission at nodes. |
| **Both Nodes Floored** ($\alpha D \le 10^{-6}$) | 34 | **0.14%** | $\sigma_k = 10^{-6}$, emission proportional to raw $\alpha D_k$. |

**Key Finding:** Only **0.75%** of subpanels are ever in the floor regime ($\le 10^{-6}$), and Consumer A* handles them with exact pointwise projection.

---

## 4. Exact `f32` Perlin Coordinate Lattice Predicate Census

Evaluated using `glm::vec3(float(noiseFreq) * pf)` followed by `floor()` in native 32-bit float arithmetic:

- **Total GL2 Subpanels:** 23,776
- **f32 Same-Cell Pairs:** 21,585 (**90.7848%**)
- **f32 Cross-Cell Pairs:** 2,191 (**9.2152%**)
- **Perlin Setups Per Ray (No Fusion):** Mean = 21.52, p50 = 20, p90 = 38, max = 100
- **Perlin Setups Per Ray (Modeled with Same-Cell Fusion):** Mean = 12.15, p50 = 10, p90 = 22, max = 80
- **Modeled Gradient Setup Reduction:** **43.54%**

---

## 5. Granular Producer (Discovery) Work Accounting

As ordered by Prism Sun (§6bz review), we recorded the full disposition of all 71,701 preliminary pieces generated by `OntoMath::segmentLine` across 9,216 segmentation calls:

| Piece Category | Preliminary Count | Target-Sign Culled ($D \le 0$) | Range Culled ($D_{\mathrm{hi}} \le 0$) | Retained in Admitted Pieces | Retained % |
|---|---|---|---|---|---|
| **Definite Pieces** (`p.definite == true`) | 39,050 | 32,004 | 0 | **7,046** | 18.04% |
| **Indefinite Root Strivers** (`p.definite == false`) | 32,651 | 22,040 | 0 | **10,611** | 32.50% |
| **Total** | 71,701 | 54,044 | 0 | **17,657** | 24.63% |

### Refusal and Work Metrics:
- **Budget Exhaustions:** **0** (budget was $2^{16} = 65,536$)
- **Evaluator Failures (`!ev.ok()`):** **0**
- **Undefined Range Pieces:** **0**
- **Media Bounding Box Misses:** 496 (out of 9,216 ray-media tests)
- **Total Range Evaluations:** 2,080,027 (**902.79** range evaluations / ray)
- **Total Proposed Roots:** 8,842 (**3.84** proposed roots / ray)
- **Final Admitted Pieces:** 17,657 (**7.66** pieces / ray)
- **Host Time Split:**
  - **Discovery:** 2.68 ms/ray (87.4% of execution time)
  - **Quadrature:** 0.386 ms/ray (12.6% of execution time)
  - **Ratio:** Discovery is **6.95× more expensive** than Quadrature!

---

## 6. Per-Ray Regression Analysis for Consumer A*

- **A\* vs Native Baseline:**
  - A\* is **strictly better** than Current Native on **79.38%** of rays (1,829 / 2,304) on RGB and **78.60%** on T.
  - A\* is worse than Native on 475 rays (20.62%), with a maximum error delta of only $6.87 \times 10^{-5}$ (Native was $1.5 \times 10^{-5}$, A\* was $8.3 \times 10^{-5}$, both negligible).
- **A\* vs Frozen Consumer A:**
  - Maximum $\Delta$ RGB between A\* and Frozen A is **$2.68 \times 10^{-7}$**.
  - Maximum $\Delta$ T is **$2.54 \times 10^{-7}$**.
  - They agree to 6 significant digits everywhere.

---

## 7. Council Roadmap: Lane A (Metal Ceiling) and Lane B (Amortization)

Following Original Prism Sun's verdict:
1. **Lane A — Bounded Consumer-Only Metal Ceiling:**
   - Implement actual back-to-back same-cell Perlin gradient sharing plus A\* consumer on M5 Metal.
   - Supply piece/subpanel boundaries from the current preflight producer as an explicit diagnostic ceiling.
   - Measure actual completion-fenced GPU cost, raw T/RGB/reference output, image parity, and compile state.
   - *Gate:* If flat/slower, stop. If faster, advance to Lane B.
2. **Lane B — Producer Amortization Architecture:**
   - Attack the dominant 2.68 ms/ray (902 evals/ray) producer cost.
   - Connect to Zach's query-space / movement-region architecture: derive stable semantic piece topology once at its longest valid lifetime, and instantiate cheaply for concrete rays, eliminating per-pixel theorem proving.
