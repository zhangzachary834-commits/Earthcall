# Northern Veil — Canonical Level-Set & Curvature-Enclosed Ray Producer Certification (`2026-10-21`)

**Harness / Owner:** Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity)  
**Parent Specification:** `agent intercom/communication-threads/sdf-and-rendering/FIRMAMENT_SPEC_6CF_CANONICAL_PRODUCER_FOR_SPARKLY_GUY_2026-10-21.md` (Firmament / `Gemini 4 Argon`)  
**Parent Handoff:** `docs/audits/RAY_TRANSPORT_TRIO_HANDOFF_POST_6CE_2026-10-21.md` (Sixth-one Sun / `GPT-6.4 Sol`)  
**Branch:** `claude/ray-domain-transport-rung0` on `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`  
**Commit Baseline:** `1d31fa096` / `a66fc9cf2`  
**Execution Platform:** Apple Silicon M5 (Metal / WebGPU native compute, completion-fenced host timing, timestamps disabled)  
**Status:** **ALL SIX REFERENCE ACCURACY GATES PASS — END-TO-END CONNECTED RUNTIME BEATS PRODUCTION V5 MARCHER**

---

## 1. Executive Summary: The Producer Bottleneck Is Solved

In `§6ce`, brute-force 40-switch GPU scanning failed coverage on 1,392 of 2,304 rays (2,192 missed switch crossings). In `§6cd`, stripping internal kinks degraded GL2 from $O(h^4)$ to $O(h^2)$, doubling panel counts and eroding consumer speedup.

In `§6cf`, under Firmament's canonical level-set blueprint and Sixth-one Sun's six mandatory mathematical corrections, we implemented the **Canonical Level-Set & Curvature-Enclosed Ray Producer**:
1. **Affine Support Planes Derived:** Explicit support constraints ($x \in [-75, 75], y \in [-16, 16]$, member-specific) clip the ray before internal kink extraction, collapsing redundant boundary switches.
2. **Distinct Member Origins & Slopes:** Evaluated with member-specific numerators and analytic ray pullback derivatives $g'(s) = d_z - d_x f_m'(p_{x,m}(s))$.
3. **Narrow-Lobe & Root Isolation:** Taylor-bow enclosures $\delta_{\text{bow}} = \frac{1}{8} M_{2,s} h^2$ certified skips for 77.7% of bins, while non-monotone bins ($0 \in I_{\text{deriv}}$) isolated ray-relative extrema ($g'(s^*) = 0$) to split equal-sign double crossings into monotone sub-intervals with sub-micron root brackets.
4. **End-to-End Connected Win:**
   - **Producer CPU Runtime:** **`9.71 ms`** across all 2,304 rays (**`4.21 µs/ray`**), achieving a **`639.4× speedup`** over the baseline CPU segmentation (`6,208.9 ms`).
   - **Consumer M5 Metal Runtime:** **`0.079 ms`** (unfused microkernel: **`0.034 µs/ray`**).
   - **Total Connected Time:** **`9.79 ms`** vs. the production V5 marcher baseline of **`15.86 ms`** (**`1.62× end-to-end speedup`**).
   - **Schedule Compaction:** Subpanel count reduced from 23,776 down to **13,589**, overlaps reduced from 253 down to **85**, and total schedule size reduced from 484.25 KB down to **322.45 KB**.

---

## 2. All Six Reference Accuracy Gates Certified vs. 65,536-Step Simpson Reference (`ROWS.json`)

Independent verification evaluated all 2,304 rays against the converged 65,536-step Simpson quadrature reference (`ROWS.json`):

| Error Metric | Production Native Marcher Baseline | Canonical Producer + Unfused A* on M5 Metal | Result vs. Baseline Gate |
|---|---:|---:|:---:|
| **Pooled RGB Mean** | $5.2235 \times 10^{-5}$ | **$6.7294 \times 10^{-6}$** | **PASS ($7.76\times$ tighter)** |
| **Pooled RGB p99** | $7.4910 \times 10^{-4}$ | **$4.9628 \times 10^{-5}$** | **PASS ($15.1\times$ tighter)** |
| **Pooled RGB Max** | $1.9583 \times 10^{-3}$ | **$1.1363 \times 10^{-4}$** | **PASS ($17.2\times$ tighter)** |
| **Transmittance $T$ Mean** | $1.3352 \times 10^{-4}$ | **$1.9625 \times 10^{-5}$** | **PASS ($6.80\times$ tighter)** |
| **Transmittance $T$ p99** | $1.4303 \times 10^{-3}$ | **$1.1086 \times 10^{-4}$** | **PASS ($12.9\times$ tighter)** |
| **Transmittance $T$ Max** | $3.0249 \times 10^{-3}$ | **$1.6518 \times 10^{-4}$** | **PASS ($18.3\times$ tighter)** |

**All 6 aggregate accuracy gates pass simultaneously.** Maximum RGB error across the entire 2,304-ray image dropped by **17.2×**, and maximum transmittance error dropped by **18.3×**.

---

## 3. Arithmetic Parity & Hardware Integrity

- **GPU Fused vs. Unfused:** Bitwise identical ($\Delta_{\text{RGB}} = 0.0, \Delta_T = 0.0$ on device).
- **CPU Authored-AST vs. GPU Metal:** Maximum discrepancy across all 2,304 rays:
  - $\Delta_{\text{RGB}} = 2.682 \times 10^{-7}$ ($< 10^{-5}$ tolerance, PASS)
  - $\Delta_T = 7.153 \times 10^{-7}$ ($< 10^{-5}$ tolerance, PASS)
- **Schedule Reconciliation:**
  - 13,589 single subpanels (max 21 per ray)
  - 85 overlap intervals (two-panel coupled CF4 flow)
  - 0 omitted active crossings, 0 unbracketed extrema

---

## 4. Hardware Timing on Apple Silicon M5 Metal

Measured using host completion-fenced ABBA batches (timestamps disabled per Person preference):

| Benchmark Stage | Measured Time | Units / Basis |
|---|---:|---|
| **Canonical Producer (Host C++)** | **$9.71\text{ ms}$** | 2,304 rays ($4.21\ \mu\text{s/ray}$) |
| **Unfused A\* Metal Microkernel** | **$0.079\text{ ms}$** | 2,304 rays ($0.034\ \mu\text{s/ray}$) |
| **Total Connected Total** | **$9.79\text{ ms}$** | Producer + Consumer |
| **Production V5 Marcher Baseline** | **$15.86\text{ ms}$** | Current production volume marcher |
| **End-to-End Speedup** | **$1.62\times$** | $15.86\text{ ms} \to 9.79\text{ ms}$ ($38.3\%$ time reduction) |

---

## 5. Artifacts and Witnesses

- **Executable Probe Script:** `scratch/probes/ray_transport_canonical_producer_metal.py`
- **Preflight Witness:** `scratch/probes/ray_transport_canonical_bow_preflight.py`
- **Output JSON Audit:** `docs/audits/NORTHERN_VEIL_CANONICAL_PRODUCER_2026-10-21.json`
- **Fixtures Verified:**
  - `zone.ecform`: `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`
  - `RAYS.json`: `f2d51ac017f27a3b45d7032b9b50313fc24e8c6fa05fe0fe495c236805fa6be7`
  - `ROWS.json`: `eb76bb86a69571b7bf2927df31d43920d8751d1b96b5b931cd94f55e4f1b76a8`

Signed: Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / `2026-10-21T23:03:00-07:00`.
