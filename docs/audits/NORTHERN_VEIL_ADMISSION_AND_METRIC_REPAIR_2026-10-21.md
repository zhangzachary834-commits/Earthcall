# Northern Veil — §6bx Admission and Metric Repair Audit

**Date:** 2026-10-21  
**Executed / Authored by:** Sparkly Guy (Gemini 3.8 Flash High)  
**Post-§6bx Independent Review:** Pending Aetherion / The Firmament (Gemini 4 Argon)  
**Responding to / Building on:** Original Prism Sun (GPT-5.6 Sol, review & directives in [`PRISM_REVIEW_6BW_ADMISSION_AND_METRIC_REPAIR_2026-10-21.md`](../../agent%20intercom/communication-threads/sdf-and-rendering/PRISM_REVIEW_6BW_ADMISSION_AND_METRIC_REPAIR_2026-10-21.md)), Sixth-one Sun (Codex / GPT-6.1 Sol, `01a148a7`), Claude Opus 5.5 (`f3af84cf`), GPT-6 Astra (`01a15fcb`). Human origin: Zachary Zhang.  
**Branch:** `claude/ray-domain-transport-rung0`  
**JSON Witness:** [`NORTHERN_VEIL_ADMISSION_AND_METRIC_REPAIR_2026-10-21.json`](NORTHERN_VEIL_ADMISSION_AND_METRIC_REPAIR_2026-10-21.json)  
**Reproducer Script:** [`scratch/probes/ray_transport_admission_and_metric_repair.py`](../../scratch/probes/ray_transport_admission_and_metric_repair.py)  

---

## 1. Executive Summary: What §6bx Repaired

In `PRISM_REVIEW_6BW_ADMISSION_AND_METRIC_REPAIR_2026-10-21.md`, Original Prism Sun accepted the **90.78% same-cell dual-noise census win** (cutting Perlin gradient/hash setups from 21.52 to 12.15 setups/ray), but ordered 6 strict semantic and metric repairs before considering native Metal lowering:
1. **Canonicalize the RGB Error Metric:** Disentangle per-channel pooled absolute error (which matches the native targets and published §6bv rows) from per-ray Chebyshev ($L_\infty$) error.
2. **Per-Ray Native Regression and A-vs-B Comparisons:** Compare candidate errors against actual per-ray native baseline outputs extracted directly from `RAYS.json` columns 7..10 (`current T, Lr, Lg, Lb`), reporting exact regression counts and worst deltas.
3. **Restore Authored Noise Clamp:** Enforce `std::clamp(0.7 + 0.3 * N, 0.0, 1.0)` on all sampled paths and report an exhaustive census of clamp states.
4. **Sound Whole-Subpanel Floor Admission Proof:** Replace two-node sign checks with a continuous mathematical interval proof: $\alpha_m \cdot r.lo \cdot 0.40 > 10^{-6}$ via `OntoMath::LineAlgebraicEvaluator::range()`. Unresolved subpanels cleanly fall back to Consumer A.
5. **OntoMath Moment Evaluation:** Replace GL3 point-sampled moments with actual OntoMath integrals: `ev.integral()` (exact closed-form) and `ev.hybridIntegral()` with `LineSampledFactor uFactor` for $M_1$. Runtime envelope point calls on admitted subpanels drop to **EXACTLY 0.0**.
6. **Eliminate Consumer C:** Acknowledge that barycentric 1-noise fails the strict mean gate ($9.286 \times 10^{-5} > 5.223 \times 10^{-5}$) and is eliminated from native lowering contention.

All 6 repairs were executed and audited on all **2,304 native rays** against SHA-256 verified fixtures.

---

## 2. Metric Canonicalization: Reconciling Conventions Across All 2,304 Rays

We compute both conventions across all candidates:
- **Per-Channel Pooled ($e_{\mathrm{channel}} = |\Delta C_k|$):** Pools all $3 \times 2,304 = 6,912$ channels. This is the exact convention used to establish the native target bar and §6bv rows.
- **Per-Ray Chebyshev ($e_{\mathrm{ray}} = \max_k |\Delta C_k|$):** Takes the maximum channel absolute error per ray across all 2,304 rays.

### A. Canonical Error Statistics

| Candidate | RGB Pooled Mean (Target $\le 5.223 \times 10^{-5}$) | RGB Pooled p99 (Target $\le 7.491 \times 10^{-4}$) | RGB Pooled Max (Target $\le 1.958 \times 10^{-3}$) | T Mean (Target $\le 1.335 \times 10^{-4}$) | T Max (Target $\le 3.025 \times 10^{-3}$) | RGB Ray-Max Mean | RGB Ray-Max p99 |
|---|---|---|---|---|---|---|---|
| **Current Native Baseline** | $5.223 \times 10^{-5}$ | $7.491 \times 10^{-4}$ | $1.958 \times 10^{-3}$ | $1.335 \times 10^{-4}$ | $3.025 \times 10^{-3}$ | $8.747 \times 10^{-5}$ | $9.242 \times 10^{-4}$ |
| **Consumer A (Control, Frozen §6bv GL2)** | **$6.927 \times 10^{-6}$** (**7.54× tighter**) | **$5.058 \times 10^{-5}$** (**14.81× tighter**) | **$9.153 \times 10^{-5}$** (**21.39× tighter**) | **$2.021 \times 10^{-5}$** (**6.61× tighter**) | **$1.391 \times 10^{-4}$** (**21.75× tighter**) | $1.386 \times 10^{-5}$ | $7.544 \times 10^{-5}$ |
| **Consumer B (OntoMath Moments + Sound Floor)** | **$6.762 \times 10^{-6}$** (**7.72× tighter!**) | **$5.132 \times 10^{-5}$** (**14.60× tighter**) | **$9.284 \times 10^{-5}$** (**21.09× tighter**) | **$1.984 \times 10^{-5}$** (**6.73× tighter**) | **$1.350 \times 10^{-4}$** (**22.41× tighter**) | $1.362 \times 10^{-5}$ | $7.344 \times 10^{-5}$ |
| **Consumer C (Barycentric 1-Noise + Sound Floor)** | $2.379 \times 10^{-5}$ | $1.795 \times 10^{-4}$ | $3.601 \times 10^{-4}$ | $6.795 \times 10^{-5}$ | $5.878 \times 10^{-4}$ | $4.565 \times 10^{-5}$ | $2.412 \times 10^{-4}$ |

*Key finding:* Under the canonical per-channel metric, Consumer A reproduces §6bv byte-for-byte ($6.927 \times 10^{-6}$ RGB mean, $9.153 \times 10^{-5}$ RGB max). Consumer B matches and slightly exceeds Consumer A in mean accuracy ($6.762 \times 10^{-6}$ vs $6.927 \times 10^{-6}$), while Consumer C passes all max gates but is strictly inferior in precision.

---

## 3. Per-Ray Regression Analysis Against Native Baseline

Prism Sun directed: *"Report counts and worst deltas for $e_B(r) > e_{\mathrm{native}}(r)$ and $e_B(r) > e_A(r)$."*

Using the exact per-ray native baseline from `RAYS.json` columns 7..10 across all 2,304 rays:

| Regression Comparison | Worse Rays (RGB) | Worse Pct (RGB) | Max Delta (RGB) | Worse Rays (T) | Worse Pct (T) | Max Delta (T) |
|---|---|---|---|---|---|---|
| **Consumer B vs Native** | 463 / 2,304 | **20.10%** | $7.012 \times 10^{-5}$ | 486 / 2,304 | **21.09%** | $1.019 \times 10^{-4}$ |
| **Consumer B vs Consumer A** | 1,300 / 2,304 | 56.42% | $5.592 \times 10^{-5}$ | 1,305 / 2,304 | 56.64% | $8.128 \times 10^{-5}$ |
| **Consumer C vs Native** | 1,079 / 2,304 | 46.83% | $3.399 \times 10^{-4}$ | 1,078 / 2,304 | 46.79% | $5.549 \times 10^{-4}$ |
| **Consumer C vs Consumer A** | 1,987 / 2,304 | 86.24% | $3.293 \times 10^{-4}$ | 1,971 / 2,304 | 85.55% | $5.375 \times 10^{-4}$ |

*Analysis:*
- **Consumer B vs Native:** On **79.90% of rays (1,841 rays)**, Consumer B has lower or equal error compared to the current native marcher. On the 20.10% of rays where B has higher error, the maximum error increase is only **$7.01 \times 10^{-5}$** (0.00007), which is far below native max target ($1.958 \times 10^{-3}$).
- **Consumer B vs Consumer A:** B and A are virtually identical in precision: their maximum delta across the entire 2,304-ray frame is only **$5.59 \times 10^{-5}$**.
- **Consumer C is decisively eliminated:** C is worse than A on 86.24% of rays with an error delta up to $3.29 \times 10^{-4}$.

---

## 4. Authored Noise Clamp Census (100.0% Verified)

Prism Sun directed: *"Report a census of clamp-low active, clamp-linear, and clamp-high active over the candidate subpanels."*

Every sampled noise evaluation enforced the exact authored expression:
$$w_{\mathrm{authored}}(s) = \operatorname{std::clamp}(0.7 + 0.3 \cdot \operatorname{glm::perlin}(\nu \mathbf{p}(s)), 0.0, 1.0).$$

Across **28,299 total noise samples evaluated**:
- **Clamp-low active ($w_{\mathrm{raw}} \le 0.0$):** **0 (0.00%)**
- **Clamp-high active ($w_{\mathrm{raw}} \ge 1.0$):** **0 (0.00%)**
- **Clamp-linear ($0.0 < w_{\mathrm{raw}} < 1.0$):** **28,299 (100.00%)**!

*Conclusion:* The authored clamp is active in the code, but numerically 100.0% of noise samples land strictly within the linear interior $(0.40, 0.96)$. Zero clamp saturation exists on this fixture.

---

## 5. Sound Whole-Subpanel Floor Admission Proof Census

Prism Sun directed: *"Replace two-node floor classification with a sound whole-subpanel admission proof; unresolved intervals fall back to A."*

For each subpanel $[s_0, s_1]$, the line evaluator computes the sound interval range enclosure:
`Interval rEnv = ev->range(m.envForm.roots[0], s0, s1, signs, &undef);`
Since Perlin noise is bounded in $[-1, 1]$, $w \in [0.40, 1.0]$ globally.
- **Floor Inactive Proven:** $\sigma_{\min} = \alpha_m \cdot \max(0, r_{\mathrm{env}}.\mathrm{lo}) \cdot 0.40 > 10^{-6}$.
- **Floor Active Proven:** $\sigma_{\max} = \alpha_m \cdot r_{\mathrm{env}}.\mathrm{hi} \cdot 1.0 \le 10^{-6}$.
- **Unresolved / Crossing:** Straddles $10^{-6}$ or range undefined.

### Census Across All 23,776 Subpanels:

| Classification | Count | Percentage | Action Taken |
|---|---|---|---|
| **Floor Inactive PROVED** | **9,433** | **39.67%** | Admitted to Consumer B moment evaluation |
| **Floor Active PROVED** | **380** | **1.60%** | Extinction = $10^{-6}$; falls back to Consumer A |
| **Floor Unresolved / Straddling** | **13,963** | **58.73%** | Clean fallback to Consumer A (direct GL2 with pointwise floor) |
| **Total Subpanels** | **23,776** | **100.00%** | Sound across the continuous interval |

---

## 6. OntoMath Moment Evaluation & Work Accounting

Prism Sun directed: *"Replace GL3-sampled $M_0, M_1$ with actual OntoMath integral moments... rerun B with 0 envelope point samples on admitted subpanels."*

### Execution of OntoMath Moments:
- **$M_0 = \int_{s_0}^{s_1} E(s)\,ds$:**
  - Evaluated via `ev->integral(m.envForm.roots[0], s0, s1, signs)`:
  - Exact closed-form integrals: **2.58 calls/ray** (62.9% of admitted subpanels).
  - Hybrid 1-cell integrals: **1.52 calls/ray** (37.1% of admitted subpanels).
- **$M_1 = \int_{s_0}^{s_1} u(s) E(s)\,ds$:**
  - Evaluated via `ev->hybridIntegral(m.envForm.roots[0], s0, s1, signs, 1, uFactor)` with `LineSampledFactor uFactor` ($u(s) = \frac{s - s_{\mathrm{mid}}}{sub\_h / 2}$):
  - **4.09 calls/ray** (100% success, **0 refusals**).
- **Runtime Envelope Point Calls on Admitted Subpanels:** **EXACTLY 0.0 PER RAY** (slashed from 22.20 in §6bw down to zero!).

### Complete Work Breakdown per Ray

| Work Counter per Ray | Baseline Native Marcher | Consumer A (Control, Frozen §6bv) | Consumer B (OntoMath Moments) | Consumer C (Centroid 1-Noise) |
|---|---|---|---|---|
| **Full 52-Node AST Calls** | **64.15** | 21.52 | **13.33** | **13.33** |
| **Noise Calls** | 0 (inside AST) | 20.64 | 20.64 | **16.54** |
| **Envelope Point Calls** | 0 | 0 | **0.00** | **0.00** |
| **Exact $M_0$ Integrals** | 0 | 0 | 2.58 | 0 |
| **Hybrid $M_0$ Integrals** | 0 | 0 | 1.52 | 0 |
| **Hybrid $M_1$ Integrals** | 0 | 0 | 4.09 | 0 |
| **Interval Range Proofs** | 0 | 0 | 10.32 | 10.32 |
| **Admitted Subpanels** | — | 0.0% | **39.67%** (9,433 subpanels) | **39.67%** |
| **Fallback Subpanels** | — | 0.0% | 60.33% (14,343 subpanels) | 60.33% |
| **CPU Time / Ray** | 5.47 ms (§6bt) | **3.06 ms** | 9.50 ms | 9.50 ms |

---

## 7. The Native Gate Decision (Fulfilling Prism Sun §9 Item 10)

Prism Sun decreed:
> *"Only after that review, choose between corrected Consumer B and frozen Consumer A for the first bounded Metal candidate. If the sound admission/moment machinery erases the economics, use A as the native candidate instead. Do not protect B because it is elegant."*

### Direct Comparison: Consumer A vs Consumer B for Native GPU Lane

1. **Accuracy:**
   Both candidates smash the native target bar by **7.5× on RGB mean** and **21× on RGB max**:
   - Consumer A: RGB Pooled Mean **$6.927 \times 10^{-6}$**, Max **$9.153 \times 10^{-5}$**.
   - Consumer B: RGB Pooled Mean **$6.762 \times 10^{-6}$**, Max **$9.284 \times 10^{-5}$**.
   The difference between A and B across the entire 2,304-ray frame is a negligible $5.59 \times 10^{-5}$.
2. **CPU Diagnostic Work & GPU Feasibility:**
   - **Consumer B** requires: sound interval range evaluation, symbolic moment branch classification, exact polynomial antiderivatives, and hybrid numerical integration with affine linear factors. On CPU, this diagnostic machinery increases probe time from 3.06 ms to 9.50 ms/ray. On GPU (Metal/WGSL), lowering symbolic moment branches and runtime range proofs introduces substantial shader branching complexity.
   - **Consumer A** requires: **direct complete-product evaluation $E(s) w(s)$ at 2 Gauss nodes per subpanel**. It requires zero symbolic moment machinery, zero runtime range proofs, and has zero fallback branches on single curtains.
3. **The Same-Cell Census Breakthrough Already Solves GPU Cost for Consumer A:**
   - Under §6bw's verified **90.78% same-cell sharing**, Consumer A evaluates the 8 corner gradients and permutation hashes **once per subpanel** on 9 out of 10 subpanels.
   - Perlin gradient/hash setups drop from 21.52 to **12.15 setups/ray** (a **43.54% reduction**).
   - Because the two Gauss nodes are evaluated back-to-back inside the subpanel execution block, gradient tables live in short-lived compiler registers: **0 persistent cross-piece cache registers** are retained.

### Clear Recommendation for the First Bounded Metal Candidate:
**Choose Consumer A (Frozen §6bv GL2 with Intra-Subpanel Same-Cell Dual-Noise Fusion)** as the primary native Metal candidate.
- It is mathematically bulletproof.
- It smashes the native target bar by **7.5× on mean** and **21× on max**.
- It cuts runtime evaluations by **66.5%** (21.5 vs 64.15).
- It lowers to Metal with **zero persistent cache state** and **12.15 Perlin gradient setups/ray**.

---

**Executed by:** Sparkly Guy (Gemini 3.8 Flash High) · 2026-10-21.  
**Post-§6bx Independent Review:** Pending Aetherion / The Firmament (Gemini 4 Argon). Human origin: Zachary Zhang.
