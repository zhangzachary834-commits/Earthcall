# Firmament Specification for `§6cf` — Canonical Level-Set & Curvature-Enclosed Producer Blueprint for Sparkly Guy (`2026-10-21`)

**From:** Aetherion / The Firmament (`Gemini 4 Argon`)
**To:** Sparkly Guy (`Antigravity`, Implementation & Native Harness Owner), Original Prism Sun (`GPT-5.6 Sol`, Architecture/Provenance & Dashboard Owner), Sixth-one Sun (`GPT-6.4 Sol`), and Zach
**Date:** `2026-10-21T22:28-07:00 (PDT)`
**Parent Handoff:** [RAY_TRANSPORT_TRIO_HANDOFF_POST_6CE_2026-10-21.md](../../../docs/audits/RAY_TRANSPORT_TRIO_HANDOFF_POST_6CE_2026-10-21.md) (`1d31fa0961413ceaf1a90a43999e90b66c349737` on `claude/ray-domain-transport-rung0`)
**Thread:** [OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md](OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md)

## 1. Mission & Trio Division of Labor for `§6cf`

Sixth-one Sun (`GPT-6.4 Sol`) reviewed our post-`§6ce` canonical bow counsel in commit `1d31fa096`, **accepted the core mathematical architecture** (canonical shared level sets + affine support/root derivation + 2nd-derivative secant-bow and monotonicity enclosures), contributed **six mandatory corrections** (including the narrow-lobe counterexample $D(s) = \max(0.01 - s^2, 0)$ on $[-1, 1]$), and handed the torch to the Trio (`Firmament` + `Sparkly Guy` + `Prism Sun`) before setting at 4% usage.

Per the [Trio Handoff](../../../docs/audits/RAY_TRANSPORT_TRIO_HANDOFF_POST_6CE_2026-10-21.md):
- **Firmament (`Gemini 4 Argon`):** Mathematical derivation of the generic canonicalization, curvature/extremum/root coverage, narrow-lobe resolution, and multi-layer error admission under all six corrections (this document).
- **Sparkly Guy (`Antigravity`):** Single bounded implementation and native harness owner on `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop` (`claude/ray-domain-transport-rung0`, starting from `1d31fa096`).
- **Original Prism Sun (`GPT-5.6 Sol`):** Architecture/provenance/lifecycle review, independent scrutiny of the accuracy and cost account, and maintenance of [NORTHERN_VEIL_RAY_TRANSPORT_PROGRESS_BAR_2026-10-21.md](NORTHERN_VEIL_RAY_TRANSPORT_PROGRESS_BAR_2026-10-21.md).

## 2. How We Mathematically Resolve All Six of Sixth-one Sun's Corrections

Before writing any code, lock in these six exact mathematical rules so `§6cf` passes every coverage, provenance, and accuracy gate on the first try:

| # | Sixth-one Sun's Correction (`1d31fa096`) | Exact Mathematical & Engineering Resolution in `§6cf` |
|---|---|---|
| **1** | **Saved AABB `(160, 36, 46)` $\ne$ Density Support `(75, 16, ...)`:** The probe's outer slab clip uses `spec.h = (160, 36, 46)`, so `x = ±75` and `y = ±16` were not clipped by the outer box. | **Derived Affine Support Clip Stage:** In OntoMath, extract top-level multiplicative `Clamp`/`Max` factors whose arguments are affine in local $\mathbf{p}$ (`1 - |p_x|/W_x >= 0`, `p_y - y_min >= 0`, `y_max - p_y >= 0`). Intersect their positivity half-spaces (padded outward by explicit f32 affine slack $\varepsilon_{\text{aff}}$) with the outer AABB interval $[s_{\text{box},0}, s_{\text{box},1}]$ to form the **certified affine density-support window** $[s_{\text{supp},0,m}, s_{\text{supp},1,m}]$. Only after this admitted clip do `switch 1, 3, 5, 6` collapse into the window endpoints, leaving `switch 0` ($p_{x,m}=0$) and `switch 4` ($p_{y,m}=y_{\text{knee},m}$) as the two interior affine kinks. |
| **2** | **Distinct Per-Member World Origins (`x0 = [0, 15, -5, 22]`, `wz = [3.2, 2.8, 3.8, 3.0]`):** Local `p_x = 0` is 4 distinct world planes, not 1 shared root. | **Shared Reciprocal, Per-Member Numerator & Parallel Guard:** Compute `inv_dx = 1.0 / d_x` (for $|d_x| > 10^{-7}$) and `inv_dy = 1.0 / d_y` (for $|d_y| > 10^{-7}$) once per ray. Each member $m$ uses its own local origin $o_m = o - \text{origin}_m$ to evaluate $s_{x0,m} = -o_{x,m} \cdot \text{inv\_dx}$ and $s_{y0,m} = (y_{\text{knee},m} - o_{y,m}) \cdot \text{inv\_dy}$ with member-specific error brackets. When $|d_x| \le 10^{-7}$, evaluate the constant/near-constant interval $p_{x,m}([s_a, s_b])$ directly without dividing; note that when $d_x = 0$, $M_{2,m} = M_{2,x,m} d_x^2 = 0$, so $\Phi_m(s)$ is purely affine! |
| **3** | **Narrow-Lobe Counterexample (`D(s) = max(0.01 - s², 0)` on `[-1, 1]`):** Splitting an non-monotone interval at its midpoint $s=0$ leaves the entry/exit kinks $s = \pm 0.1$ inside `[-1, 0]` and `[0, 1]`, where all 4 `GL2` nodes land in $D=0$ and return `0` vs. exact `0.00133333`. | **Extremum Split ($g'(s^*) = 0$) $\to$ Monotone Root Isolation $\to$ Explicit Unresolved Fallback:** Never treat an arbitrary midpoint as an extremum or relabel an unresolved leaf as a certified `GL2` panel! Instead, when $0 \in g'([a,b])$ on a bin whose value bow overlaps $[-w_{z,m}, +w_{z,m}]$, isolate the **actual interior extremum $s^*$ where $g'(s^*) = 0$** (on $[-1, 1]$ for $0.01-s^2$, $g'(s) = -2s = 0 \implies s^* = 0$ with $g(s^*) = +0.01 > 0$). Splitting $[a,b]$ at $s^*$ turns the equal-sign double crossing ($g(-1)<0, g(1)<0$) into **two opposite-sign monotone single crossings** ($g(-1) < 0 < g(s^*)$ on $[-1, 0]$ and $g(s^*) > 0 > g(1)$ on $[0, 1]$), which then isolate the exact entry/exit roots $r_1 = -0.1$ and $r_2 = +0.1$! Then `[-1, -0.1]` and `[+0.1, 1]` are discarded as certified negative, and `GL2` integrates **only** $[-0.1, 0]$ and $[0, +0.1]$, returning **`0.0013333333333333` (zero error!)**. Any leaf where $s^*$ or the root bracket remains within execution uncertainty is flagged `UNRESOLVED` and routed to explicit fallback, never silently passed to `GL2`. |
| **4** | **Multi-Layer Execution Error (`ea, eb`, interior `e_int`, `1/h` finite-difference blow-up, and sensitivity scaling):** Only $\frac{1}{8}M_2 h^2$ quarters on bisection; finite-difference slope error $(e_a + e_b)/h$ grows as $h \to 0$; dimensionless clamp switch errors must scale by $W$. | **Separate Error Layers + Analytic Derivative Enclosure:** (a) Track endpoint error $e_a, e_b$, interior execution error $e_{\text{int}}$, and sensitivity scaling $W \cdot \epsilon_{\text{clamp}}$ when unfolding $1 - |u|/W = 0 \to u = \pm W$. (b) For coarse bins, use secant-slope derivative radius $\frac{1}{2}M_2 h + (e_a + e_b)/h$. For fine bins or when $(e_a + e_b)/h$ becomes comparable to $\frac{1}{2}M_2 h$, evaluate the **analytic ray derivative** $g'(s) = d_z - d_x f_m'(p_{x,m}(s))$ directly with 3rd-derivative bow $\frac{1}{8} M_{3,x,m} |d_x|^3 h^2 + e'_{a,b}$, which has **no $1/h$ division**! |
| **5** | **Ray-Relative Extrema (`f'(x) = dz/dx`, not `f'(x) = 0`):** Pullback extrema along a ray depend on the ray slope $d_z / d_x$. | **Ray-Instantiated Derivative $g'(s) = d_z - d_x f_m'(p_{x,m}(s))$:** All monotonicity tests and extremum splits operate on the ray pullback derivative $g'(s) = d_z - d_x f_m'(o_{x,m} + s d_x)$, never on world-space $f_m'(x) = 0$. |
| **6** | **Qualified Cost & Capacity Accounting:** Do not assume `1 thread = 1 ray` or `<= 256 B/ray` wins without measuring combined producer + consumer cost and overflow behavior. | **Explicit Capacity/Overflow Guard & End-to-End Timing:** Track per-ray event/panel counts, overflow flags, unresolved leaf counts, and fallback work separately; benchmark connected producer + consumer against the verified `§6cc` current-consumer baseline (`15.86 ms`). |

## 3. Step-by-Step Implementation Plan for Sparkly Guy (`§6cf`)

### Step 1 — Extend the Mathematical Witness First (`scratch/probes/ray_transport_curvature_review_witness.py` or a new `ray_transport_canonical_bow_preflight.py`)

Before touching WebGPU/Metal kernels, verify in Python/C++ that **Extremum Split ($g'(s^*) = 0$) + Monotone Root Isolation** resolves **both**:
1. **Sixth-one Sun's narrow-lobe counterexample:** $g(s) = 0.01 - s^2$, $D(s) = \max(g(s), 0)$ on $[-1, 1]$ with $M_2 = 2, M_3 = 0$:
   - Coarse test on $[-1, 1]$ ($h=2$): $g(-1) = -0.99$, $g(1) = -0.99$. Bow upper bound is $-0.99 + \frac{1}{8}(2)(2^2) = +0.01 > 0 \implies$ **cannot discard**; derivative range is $[0 - 2, 0 + 2] = [-2, 2] \ni 0 \implies$ **not monotone**.
   - Extremum isolation on $[-1, 1]$: $g'(-1) = +2 > 0$, $g'(1) = -2 < 0$, and $g''(s) = -2 < 0$ is strictly negative on $[-1, 1]$. Solve $g'(s^*) = 0 \implies s^* = 0$, $g(s^*) = +0.01 > 0$.
   - Monotone root isolation on $[-1, s^*] = [-1, 0]$: $g(-1) = -0.99 < 0 < g(0) = +0.01$, strictly increasing $\implies$ isolate $r_1 = -0.1$.
   - Monotone root isolation on $[s^*, 1] = [0, 1]$: $g(0) = +0.01 > 0 > g(1) = -0.99$, strictly decreasing $\implies$ isolate $r_2 = +0.1$.
   - Resulting partition of $[-1, 1]$:
     - $[-1, -0.1]$: certified negative ($D=0$), discarded.
     - $[-0.1, 0]$ and $[0, +0.1]$: certified positive smooth panels; `GL2` integral = **`0.0013333333333333333` (`0.0` error vs. exact!)**.
     - $[+0.1, 1]$: certified negative ($D=0$), discarded.
2. **All `20` preserved missed crossings from `docs/audits/NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21.json`** (such as Ray 15 on Member 0 `switch 7` at `s ∈ [30.410, 33.760]` and Ray 784 at `s ∈ [44.311, 47.860]`):
   - Verify that on every single one of those 20 rays, the secant-bow upper/lower enclosure $\left[\min(\Phi_a, \Phi_b) - \frac{1}{8}M_2 h^2 - e_{\text{val}},\; \max(\Phi_a, \Phi_b) + \frac{1}{8}M_2 h^2 + e_{\text{val}}\right]$ catches the double crossing of $\Phi_m(s) = c$, isolates the ray-relative extremum $\Phi_m'(s^*) = d_z - d_x \operatorname{wave}_m'(p_{x,m}(s^*)) = 0$, and brackets both roots $r_1, r_2$ on $[a, s^*]$ and $[s^*, b]$!

### Step 2 — Generic Canonical Switch Grouping & Derivative Bounds in `OntoMath`

In `OntoMath` (or as a generic helper over `OntoMath::LineAlgebraicForm` in `LineRestriction.hpp/.cpp` that inspects the restricted form without any Northern Veil hardcoding):
1. **Extract Canonical Affine Support Constraints (`CanonicalAffineSupport`):**
   - Walk top-level multiplicative factors of `envForm` (or `envAST`). Identify affine boundary switches (`role` corresponding to `clamp(a(p), 0, 1)` or `max(a(p), 0)` where `a(p)` has degree 1 in `p`).
   - Emit each affine constraint as $(n_x, n_y, n_z, c_{\text{lo}}, c_{\text{hi}}, \varepsilon_{\text{aff}})$ together with its original switch index and provenance.
   - Identify interior affine zero switches (degree 1 in `p`, such as `Abs(p_x)` at $p_x = 0$ and internal $y$-knees at $p_y = -12$).
2. **Collapse Folded `Abs` / `Clamp` Switches into Shared Canonical Level-Set Groups (`CanonicalLevelSetGroup`):**
   - When a switch tests `Abs(u) = 0` (`switch 7`), `1 - Abs(u)/W = 0` (`switch 8`, for $W > 0$), and/or `(1 - Abs(u)/W) - 1 = -Abs(u)/W = 0` (`switch 9`), group them under the **single underlying smooth child node `u`** with sorted target levels:
     $$L_u = \{-W,\; 0,\; +W\}$$
     and per-level execution-error bands:
     $$\delta_0 = e_u + W \cdot e_{\text{switch9}}, \qquad \delta_{\pm W} = e_u + e_W + W \cdot e_{\text{switch8}}.$$
   - If `u` is not $C^2$ on the support box or its second-derivative bound $M_{2,u}$ (via OntoMath's existing `secondDerivativeRange`) is not finite, refuse canonical grouping for `u` and retain the existing fallback path.
   - For separated wave nodes $u(\mathbf{p}) = p_z - f(p_x)$ (detected structurally from the DAG, not by zone name), store the structural derivative bounds computed by OntoMath over the affine support box:
     $$M_{2,x} = \sup_{|x| \le W_x} |f''(x)|, \qquad M_{3,x} = \sup_{|x| \le W_x} |f'''(x)|$$
     (for Northern Veil's 4 curtains, verify OntoMath computes $M_{2,x,0} = 0.086772$ from the authored sin/cos terms).

### Step 3 — Per-Ray Canonical Curvature-Enclosed Producer Algorithm

For each ray $i$ and each member $m \in \{0, 1, 2, 3\}$:
1. **Outer Box + Certified Affine Density-Support Clip:**
   - Intersect $[s_{\text{clip},A}, s_{\text{clip},B}]$ with `spec.h = (160, 36, 46)` and then with member $m$'s `CanonicalAffineSupport` (`|p_{x,m}| <= 75 + eps_aff`, `|p_{y,m}| <= 16 + eps_aff`), using member-local origin $\mathbf{o}_m = \mathbf{o}_{\text{world}} - \mathbf{O}_m$ and safe reciprocals (`|d_k| > 1e-7`, else constant-interval check).
   - If the clipped interval $[s_{\text{lo}}, s_{\text{hi}}]$ is empty ($s_{\text{hi}} \le s_{\text{lo}}$), mark member $m$ as `CERTIFIED_EMPTY` and continue to $m+1$.
2. **Interior Affine Split Points:**
   - For each interior affine switch of member $m$ ($p_{x,m} = 0$ and $p_{y,m} = -12$), if $|d_k| > 10^{-7}$, compute $s_{\text{root}} = (c - o_{k,m}) \cdot \text{inv\_d}_k$ and uncertainty $\delta_s$. If $s_{\text{root}} \in (s_{\text{lo}}, s_{\text{hi}})$, insert $[s_{\text{root}} - \delta_s,\; s_{\text{root}} + \delta_s]$ into member $m$'s split list. If $|d_k| \le 10^{-7}$ and $|o_{k,m} - c| \le \varepsilon_{\text{aff}}$, mark the member interval as `UNRESOLVED_AFFINE_GRAZING`.
3. **Curvature-Enclosed Wave Level-Set Isolation on $[s_{\text{lo}}, s_{\text{hi}}]$:**
   - Partition $[s_{\text{lo}}, s_{\text{hi}}]$ into $N_{\text{bins}} = 8$ initial bins $[a, b]$ of width $h = (s_{\text{hi}} - s_{\text{lo}}) / 8$, evaluating $\hat{g}(s) = \Phi_m(s)$ and its analytic ray derivative $\hat{g}'(s) = d_z - d_x f_m'(p_{x,m}(s))$ at the 9 fencepost nodes.
   - For each bin $[a, b]$ (using a small explicit stack of max depth `4`):
     - Compute the value bow radius:
       $$R_{\text{val}}(h) = \frac{1}{8} M_{2,x,m} d_x^2 h^2 + e_g + e_{\text{round}},$$
       and value enclosure:
       $$I_{\text{val}}(a,b) = \left[\min(\hat{g}(a), \hat{g}(b)) - R_{\text{val}}(h),\;\; \max(\hat{g}(a), \hat{g}(b)) + R_{\text{val}}(h)\right].$$
     - **Case 1 — Certified Outside Support:** If $I_{\text{val}}(a,b) \cap [-w_{z,m} - \delta_{w},\; +w_{z,m} + \delta_{w}] = \emptyset$, and $\hat{g}(a), \hat{g}(b)$ have the same strict sign outside $[-w_{z,m}, +w_{z,m}]$, the entire bin $[a, b]$ is **provably outside the curtain** ($D_m(s) \equiv 0$). Discard $[a, b]$ (`certified_empty_bins++`).
     - **Case 2 — Certified Strictly Inside One Smooth Half-Tent:** If $I_{\text{val}}(a,b) \subseteq (-w_{z,m} + \delta_w,\; -\delta_0)$ or $I_{\text{val}}(a,b) \subseteq (+\delta_0,\; +w_{z,m} - \delta_w)$, then $[a, b]$ crosses **zero** wave thresholds and lies strictly inside one smooth half-tent! Retain $[a, b]$ as an active positive span (`certified_interior_bins++`).
     - **Case 3 — Overlaps a Threshold $\{-w_{z,m}, 0, +w_{z,m}\}$:**
       Compute the derivative enclosure on $[a, b]$ using the tighter of the secant-slope bound and the analytic-derivative 3rd-order bow bound:
       $$I_{\text{deriv}}(a,b) = \left[\min(\hat{g}'(a), \hat{g}'(b)) - \frac{1}{8} M_{3,x,m} |d_x|^3 h^2 - e'_{g},\;\; \max(\hat{g}'(a), \hat{g}'(b)) + \frac{1}{8} M_{3,x,m} |d_x|^3 h^2 + e'_{g}\right].$$
       - **Subcase 3a (`0 ∉ I_deriv(a,b)` — Certified Strictly Monotone):**
         For each threshold $c \in \{-w_{z,m}, 0, +w_{z,m}\}$:
         - If $(\hat{g}(a) - c)$ and $(\hat{g}(b) - c)$ have **certified opposite signs** ($|\hat{g}(a)-c| > \delta_c$ and $|\hat{g}(b)-c| > \delta_c$ with opposite signs), isolate the unique crossing $r_c \in (a, b)$ using 6 secant/bisection steps to produce a tight root bracket $[r_{c,-}, r_{c,+}]$.
         - If $|\hat{g}(a)-c| \le \delta_c$ or $|\hat{g}(b)-c| \le \delta_c$ (endpoint lands inside the threshold uncertainty band), bisect or emit the uncertain endpoint bracket explicitly (`unresolved_leaves++`).
       - **Subcase 3b (`0 ∈ I_deriv(a,b)` — Potential Local Extremum / Equal-Sign Double Crossing):**
         Check whether $\hat{g}''(s)$ has definite sign on $[a, b]$ (via $|g'''| \le M_{3,x,m}|d_x|^3$) and $\hat{g}'(a), \hat{g}'(b)$ have opposite signs:
         - If **yes** ($g'$ is strictly monotone and changes sign on $[a, b]$): isolate the unique ray-relative extremum $s^* \in (a, b)$ where $\hat{g}'(s^*) = 0$ (4–6 secant/bisection steps on $\hat{g}'$), and push the two monotone sub-intervals $[a, s^*]$ and $[s^*, b]$ onto the work stack! (This is what solves $D(s) = \max(0.01 - s^2, 0)$ and all 20 grazing wave double crossings!)
         - If **no** (or if $|\hat{g}(s^*) - c| \le \delta_c$ at the extremum so the crest grazes the threshold within execution error): if stack depth $< 4$, bisect $[a, b]$ at $(a+b)/2$ and push both halves; if stack depth $== 4$, mark $[a, b]$ as **`UNRESOLVED_LEAF`** (`unresolved_leaves++`, `unresolved_width_sum += (b-a)`) — **never** relabel an unresolved leaf as a certified `GL2` panel!

### Step 4 — Verification Gate 1: Independent Coverage & Narrow-Lobe Qualification (Before Timing)

Run the independent CPU interval coverage verifier from `ray_transport_gpu_root_proposals.py` against the `§6cf` canonical producer across all `2,304` frozen rays plus the synthetic edge-case suite:
1. **All 20 missed-crossing examples from `§6ce`** (`missedCrossings` must drop from `2,192` to **`0`** outside emitted root brackets/unresolved spans!).
2. **Narrow-lobe witness** $D(s) = \max(0.01 - s^2, 0)$ on $[-1, 1]$ (`GL2` integral on the producer's emitted active panels must equal `0.0013333333333333`, NOT `0.0`).
3. **Edge-case witnesses required by Sixth-one Sun:**
   - Tangency where $\Phi_m(s^*) = c$ within $\delta_c$ (must report `UNRESOLVED_LEAF` / fallback, never silent omission).
   - Equal-sign double crossing on $\Phi_m(s) = \pm w_{z,m}$ and $\Phi_m(s) = 0$.
   - Parallel rays ($d_x = 0$ and $d_y = 0$).
   - Member-origin shift (`x0 = [0, 15, -5, 22]`).
   - Cancellation / computed-constant witness from earlier passes.
   - Artificial capacity exhaustion (e.g. cap max panels per ray to `4` and verify the overflow bit routes the ray to baseline fallback without corrupting output).

### Step 5 — Verification Gate 2: Connected Producer $\to$ Complete Ordered Single/Overlap `A*` Consumer Accuracy & M5 Metal Timing

Once Gate 1 passes:
1. **Feed the actual `§6cf` producer output** (positive member spans split at all isolated affine and wave roots $\to$ sorted event sweep $\to$ `<= 4 m` single-member `GL2` subpanels + multi-member `OverlapInterval` records, with any `UNRESOLVED` / overflow rays routed to explicit fallback and counted separately) into the repaired complete consumer from `ray_transport_metal_overlap_repair.py` / `ray_transport_native_march_comparison.py`.
2. **Check all 6 aggregate accuracy gates against the 65,536-step Simpson reference (`ROWS.json`) AND original authored AST comparator:**
   - Pooled RGB mean, p99, max error (`<= 5.224e-5` native mean baseline; target matching `§6cb`/`§6cc`'s `~6.92e-6` mean / `~9.16e-5` max).
   - Transmittance $T$ mean, p99, max error (`<= 1.335e-4` native mean baseline).
   - Report per-ray regressions vs. current native (`RAYS.json`) and separate counts for `unresolved_leaves`, `fallback_rays`, and `schedule_bytes`.
3. **Measure Connected Native M5 Timing (with GPU lane reserved):**
   - Measure **both** the producer pass and the complete consumer pass on Apple M5 Metal (`timestamps off`, completion-fenced ABBA batches at `repeat = 1, 4, 16, 128`, matching `§6cc` `175988faf` and `§6ce` `243add9b1`), comparing **total `Producer + Consumer + Fallback` time** against the verified production-generated V5 marcher (`15.86 ms` at `repeat = 128`, `1.33 ms` at `repeat = 8`).

**Signed:**
Aetherion / The Firmament (`Gemini 4 Argon`)
`2026-10-21T22:28-07:00 (PDT)`
