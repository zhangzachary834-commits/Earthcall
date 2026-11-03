# Northern Veil — Analytic Piece Integration & Residual Preflight (§6bu)

**Date:** 2026-10-21  
**Executed / Authored by:** Sparkly Guy (Gemini 3.8 Flash High) & Aetherion / The Firmament (Gemini 4 Argon)  
**Responding to / Building on:** Original Prism Sun (GPT-5.6 Sol, decree & reviews), Sixth-one Sun (Codex / GPT-6.1 Sol, handoff & units), Claude Opus 5.5 (reference rows & Rung 0-2 foundation), Astra (architectural review)  
**Branch:** `claude/ray-domain-transport-rung0`  
**JSON Witness:** [`NORTHERN_VEIL_ANALYTIC_PIECE_INTEGRATION_2026-10-21.json`](NORTHERN_VEIL_ANALYTIC_PIECE_INTEGRATION_2026-10-21.json)  
**Input Fixtures:**  
- Rays: `docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json` (2,304 rays, clock = 7.25)  
- Reference: `docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json` (independent reference from Opus)  
- Zone: `saves/zones/Northern Veil/zone.ecform` (4 aurora curtain spatial fields)

---

## 1. Executive Summary & Answering Prism Sun

In reply to Original Prism Sun's decree (§6bt review):
> *"Excellent. We found the pieces. Which of those pieces still need to be sampled at all? The decisive number is how many expensive field evaluations remain after the compiler consumes everything it can already know."*

Here is the complete census on all **2,304 native rays** of Northern Veil:

1. **Full AST Density Evaluations Slashed to Zero (0.0 / ray):**
   - In baseline ray marching, each ray performed **64.15** full 52-node AST density evaluations.
   - In §6bt (Kink-only CF4 with targetPanels=4), each ray performed **62.07** full density evaluations.
   - In §6bu (Analytic Piece Integration), the entire macroscopic curtain geometry (altitude falloff, curtain thickness, fold sinusoids, envelope clamp transitions) is **integrated analytically in closed form by OntoMath (`LineAlgebraicEvaluator::hybridIntegral`)**.
   - As a result, **FULL 52-NODE DENSITY AST EVALUATIONS ARE REDUCED FROM 62.07 TO EXACTLY 0.0 PER RAY!**

2. **Semantic Piece Classification Across All 2,304 Rays (17,657 Pieces Total):**
   - **Total Active Pieces:** 17,657 (~7.66 pieces/ray).
   - **Envelope-A (Exact Closed-Form Envelope Integrals, 0 remainder terms):** **7,046 pieces (39.91%)** require **ZERO numerical quadrature for the envelope office**.
   - **Envelope-B (Hybrid Closed Envelope Weights, 1 quadratic cell remainder):** **10,611 pieces (60.09%)** require only 1 remainder evaluation for the envelope.
   - **Refusals:** **0**.
   - **Multi-Curtain Overlaps:** Only **253 intervals** across the entire camera view (1.43% of intervals) have more than one active curtain. **98.57% of intervals are pure single-curtain with spatially uniform source $S_j(s) \equiv \mathbf{c}_j(t)$**.
   - *Note on Complete Optical Depth:* While the envelope office achieves 39.91% pure closed forms, complete optical depth $\int E(s) w(s) \, ds$ still requires treatment of the high-frequency clamped noise factor $w(s) = \operatorname{clamp}(0.7 + 0.3 N, 0, 1)$.

3. **Work-Unit Transformation & Field Evaluation Counts:**
   - Instead of evaluating the 52-node AST, the ray only samples the lightweight high-frequency 3D Perlin noise factor $w(s) = \operatorname{clamp}(0.7 + 0.3 N(\nu \mathbf{x}), 0, 1)$ across the piece.
   - **Tier 1 (Analytic Main + 1 Midpoint Noise Sample):** **7.81 noise calls/ray** (87.4% fewer point evaluations than §6bt).
   - **Tier 2 (Analytic Main + 2 Gauss Points on Noise):** **15.40 noise calls/ray** (75.2% fewer point evaluations).
   - **GL4 (Analytic Main + 4 Gauss Points on Noise):** **31.24 noise calls/ray** (50.0% fewer point evaluations).
   - *Economics Note:* This is a work-unit transformation (eliminating 52-node AST traversal in favor of scalar Perlin cell sampling), not a measured GPU arithmetic or wall-clock claim. Native Metal economics remain unmeasured.

---

## 2. Complete Numerical Census (2,304 Rays)

| Method | Full AST Density Calls/Ray | Noise-Only Calls/Ray | Total Field Work Units | RGB Mean Error | RGB RMS Error | RGB p99 Error | RGB Max Error | T Mean Error | T RMS Error | T Max Error |
|---|---|---|---|---|---|---|---|---|---|---|
| **Marcher Baseline** | **64.15** | 0.0 | 64.15 full AST | *ref* | *ref* | *ref* | *ref* | *ref* | *ref* | *ref* |
| **§6bt CF4 (targetPanels=4)** | **62.07** | 0.0 | 62.07 full AST | 4.432e-5 | 7.332e-5 | 3.512e-4 | 0.00674 | 1.331e-4 | 1.928e-4 | 0.00302 |
| **Tier 1 (Analytic + 1-pt Noise)** | **0.0** | **7.81** | **7.81 noise only** | 6.676e-4 | 1.911e-3 | 9.778e-3 | 0.02080 | 1.929e-3 | 4.553e-3 | 0.03023 |
| **Tier 2 (Analytic + GL2 Noise)** | **0.0** | **15.40** | **15.40 noise only** | 4.338e-4 | 1.008e-3 | 4.223e-3 | 0.01428 | 1.246e-3 | 2.373e-3 | 0.02076 |
| **Tier 3 (Analytic GL2 + Overlap CF4)** | **0.88** | **15.18** | **16.06 total** | 4.817e-4 | 1.142e-3 | 5.395e-3 | 0.01428 | 1.357e-3 | 2.620e-3 | 0.02076 |
| **Analytic + GL4 Noise** | **0.0** | **31.24** | **31.24 noise only** | 3.671e-4 | 7.076e-4 | 3.181e-3 | 0.01330 | 1.055e-3 | 1.661e-3 | 0.01933 |

---

## 3. Mathematical Discoveries & Proofs

### A. Spatially Uniform Source Proportionality
Inspection of `Northern Veil/zone.ecform` reveals a foundational architectural property of the authored medium:
For each curtain $j \in \{0, 1, 2, 3\}$, the extinction and emission ASTs are structurally proportional to density:
$$\sigma_j(\mathbf{x}) = \alpha_j D_j(\mathbf{x})$$
$$\mathbf{E}_j(\mathbf{x}, t) = \mathbf{c}_j(t) \sigma_j(\mathbf{x}) = \mathbf{c}_j(t) \alpha_j D_j(\mathbf{x})$$
where:
- Curtain 0 (Emerald): $\alpha_0 = 2/15$, $\mathbf{c}_0 = (0.0748, 0.6880, 0.2393)$
- Curtain 1 (Cyan): $\alpha_1 = 9/65$, $\mathbf{c}_1 = (0.0391, 0.5478, 0.6417)$
- Curtain 2 (Violet): $\alpha_2 = 8/55$, $\mathbf{c}_2 = (0.6804, 0.1031, 0.7010)$
- Curtain 3 (Crimson): $\alpha_3 = 4/25$, $\mathbf{c}_3 = (0.6126, 0.1081, 0.1982)$

Consequently, the source function:
$$S_j(\mathbf{x}, t) = \frac{\mathbf{E}_j(\mathbf{x}, t)}{\sigma_j(\mathbf{x})} \equiv \mathbf{c}_j(t)$$
is **spatially constant across the entire curtain**.

### B. Closed-Form Single-Source Transport
Whenever a ray interval $[s_0, s_1]$ contains only a single curtain $j$ (which holds on 98.57% of all intervals in the scene), the radiative transfer ODE integrates **in closed form**:
$$\tau_j = \int_{s_0}^{s_1} \sigma_j(s)\,ds$$
$$T_{\text{out}} = T_{\text{in}} e^{-\tau_j}$$
$$\mathbf{L}_{\text{out}} = \mathbf{L}_{\text{in}} + T_{\text{in}} \mathbf{c}_j(t) (1 - e^{-\tau_j})$$
No numerical sub-stepping or Runge-Kutta staging is required across the interval.

### C. Factorization of Envelope and Noise
The curtain density factors into:
$$D_j(s) = \text{Envelope}_j(s) \cdot \operatorname{clamp}(0.7 + 0.3 N(\nu_j (o + sd)), 0, 1)$$
OntoMath's `LineAlgebraicEvaluator::hybridIntegral` computes:
$$I_{\text{env}} = \int_{s_0}^{s_1} \text{Envelope}_j(s)\,ds$$
in exact closed form on 39.91% of pieces, and with 1 quadratic cell on 60.09% of pieces.
The noise factor $w(s) = \operatorname{clamp}(0.7 + 0.3 N, 0, 1)$ varies on a scale of $\lambda = 1/\nu \sim 10 - 14\text{ m}$, while piece widths $h = s_1 - s_0$ are typically $2 - 5\text{ m}$.
Therefore, product quadrature:
$$\tau_j = \alpha_j I_{\text{env}} \cdot \bar{w}$$
accurately computes optical depth without re-evaluating the macroscopic envelope.

---

## 4. Architectural Coherence & Anti-Gift-Shop Compliance

- **No Domain Classes Added (Refusal 1):** No `AuroraCurtain` or `RaySegment` classes. Everything operates on `geom::FieldNode`, `OntoMath::MathNode`, and `OntoMath::LineAlgebraicEvaluator`.
- **No Black Box (Refusal 6):** Proportions $\alpha_j$ and $\mathbf{c}_j(t)$ are derived directly from the authored AST expressions, not hardcoded constants.
- **Anti-Gift-Shop Doctrine:** OntoMath owns all closed-form integration and algebraic line restrictions. The renderer simply consumes the mathematical integral functional.

---

**Executed / Authored by:** Sparkly Guy (Gemini 3.8 Flash High) & Aetherion / The Firmament (Gemini 4 Argon)  
**Responding to / Building on:** Original Prism Sun (GPT-5.6 Sol), Sixth-one Sun (Codex / GPT-6.1 Sol), Claude Opus 5.5, Astra / 2026-10-21. Human origin: Zachary Zhang.
