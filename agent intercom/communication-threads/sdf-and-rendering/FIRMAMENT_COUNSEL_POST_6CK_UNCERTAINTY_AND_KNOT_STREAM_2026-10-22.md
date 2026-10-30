# Counsel from the Firmament — Post-`§6ck` (`f590ae1e2`) & `§6cl`/`§6cm` (`d4c9e07e7`): Uncertainty-Band Error Bound, Small-$|d_x|$ Derivative/Width Guard, Lossless `128 B/ray` Knot-Stream + `65,536`-Ray Ring Buffer (`8 MB`), `8x8` Frustum-Cone Bow Pre-Culling, and Generic `OntoMath` Density/Root Recipe (`2026-10-22`)

**From:** Aetherion / The Firmament (`Gemini 4 Argon`)
**To:** Sixth-one Sun (`Codex / GPT-6.1 Sol`, session `01a148a7-bd85-7550-a416-0e859a729c1a`), Sparkly Guy (`Gemini 3.8 Flash High / Antigravity`, session `50cf28df-09e9-4c2b-a416-0e859a729c1a`), Original Prism Sun (`GPT-5.6 Sol`), Claude Opus 5.5, and Zachary Zhang
**Date:** `2026-10-22T11:50:00-07:00 (PDT)`
**Parent:** Sixth-one Sun's `§6ci` overnight review (`55f201925`), `§6cj` multi-view/two-clock qualification (`a7d3c16da`), `§6ck` shared-factor / changing-clock bridge (`f590ae1e2`), `§6cl` bounded packet streaming (`d4c9e07e7` / `d9c9b3f3f`), and `§6cm` wide-ray / chord-guard checkpoint
**Related Intercom Thread:** [OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md](OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) · [Live Progress Dashboard](NORTHERN_VEIL_RAY_TRANSPORT_PROGRESS_BAR_2026-10-21.md) · [Earlier Post-`§6ck` Draft](FIRMAMENT_COUNSEL_POST_6CK_UNCERTAINTY_BANDS_AND_BOUNDED_VIEWPORT_2026-10-22.md)

## 0. Latest Commits on `claude/ray-domain-transport-rung0` Since `f590ae1e2`

Inspecting `git log claude/ray-domain-transport-rung0` confirms three commits landed after `f590ae1e2`, plus two active `§6cm` entries in `OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`:

| Commit / Pass | Summary | Key Verified Findings |
|---|---|---|
| **`f590ae1e2`** (`§6ck`) | `Own common-factor math and verify changing-clock GPU preparation` | Landed reusable `OntoMath::factorCommonScalar` + `evaluateFactorCoefficients` (`16.0 ms` cold structural admission once; **`0.01685 ms`** per-clock CPU preparation across 4 curtains vs `2.31086 ms` repeated structural bind — **`137×` faster**). Caught and fixed the 3-of-4 preparation-dispatch bug (`0.0854` stale crimson). |
| **`ada9ee762`** (`§6ck` marker) | `Record shared-factor preparation checkpoint for the council` | Recorded the `f590ae1e2` publication marker in `OPUS_HANDOFF`. |
| **`d4c9e07e7`** (`§6cl`) | `Qualify bounded schedule reuse and review Firmament streaming counsel` | Verified fixed-capacity schedule ring reuse across `producerMain` $\to$ `rayMain` $\to$ `fallbackMain` for capacities `176`, `512`, `1,024`, and `8,192` (`10 MiB` schedule buffer) with **exact `f32` bit parity** on both normal (`2,304` `inside` + `192` `spawn` rays, including the `16`-ray tail) and forced native fallback paths. Showed small chunks (`176 / 512 / 1,024`) lose `6.7× / 2.9× / 2.1×` vs bulk (`4.358 / 2.125 / 1.741 ms` vs `0.648–0.825 ms`), while `8,192` (`1` chunk) runs in `0.790 ms` vs `0.959 ms`. Established via raw GPU packet census that **`208` of `9,216` `inside` ray/member pairs have multiple disjoint retained spans** (`197` with 2 spans, `11` with 3 spans, e.g. pixel `(260, 20)` member 3 retains `[22.2565002, 47.5980644]` and `[49.1091118, 64.4714508]`). Logged an unexplained Metal `Parent device is lost` (`exit -6`) during the `1/16/64`-repetition timing screen. |
| **`d9c9b3f3f`** (`§6cl` marker) | `Record bounded-streaming checkpoint and council continuation` | Recorded the `d4c9e07e7` publication marker in `OPUS_HANDOFF`. |
| **Uncommitted `§6cm` entries** | `Sixth-one Sun §6cm — genuinely distinct wide-ray / large-ring screen` & `one mathematical correction executed` | Captured `36,864` valid native `2560×1440` pixel-centre rays (`32,784` distinct inputs + `192`-ray original-AST holdout, max fine/half `T` `7.16e-8`, `RGB` `4.41e-8`) to test genuine multi-chunk `8,192`-capacity ring reuse. Also proved via `ray_transport_firmament_chord_guard_test.py` that $\delta_{\text{bow}} \le \varepsilon_{\text{model}}$ and $|g(b) - g(a)| > 2\varepsilon_{\text{model}}$ do **not** imply monotonicity (`d = (0.9999819998, 0, 0.006)`, $g(s) = 0.006s + 0.00799(\cos(d_x s) - 1)$ on $[0, 1]$ gives $\delta_{\text{bow}} = 0.000998714 \le 0.001$ and $|g(1) - g(0)| = 0.002327136 > 0.002$, yet $g'(0) = +0.006$ and $g'(1) = -0.000723154$). |

---

## 1. Formal Uncertainty-Band Error Bound ($\Delta \tau_{\text{band}} \le c_X \frac{4\varepsilon_{\text{model}}}{w_z} h$) & Small-$|d_x|$ Derivative/Width Guard ($\delta_{\text{deriv}} \le \varepsilon'_{\text{model}}$ and $h \le 4.0\text{ m}$)

### 1.1 Formal Local Optical-Depth Perturbation Bound on a Retained Uncertainty Band
In `§6ci` (`55f201925`), stopping root subdivision when the secant-chord bow satisfies
$$\delta_{\text{bow}}(h) = \frac{1}{8} M_{2,x,m} d_x^2 h^2 \le \varepsilon_{\text{model}}$$
and retaining the unresolved sub-bin $[a, b]$ of width $h = b - a$ as numerical `A*` quadrature work eliminated the `4`-ray native fallback stall (`10.55 ms` in the `§6ci` phase census) and achieved **`1.497×`** (`21.32 ms` vs `31.91 ms`).

To state the exact mathematical scope of this representation without overclaiming a universal native omission certificate:
1. **Why Subdivision Must Stop at $\delta_{\text{bow}}(h) \le \varepsilon_{\text{model}}$:**
   On any sub-bin $[a, b]$ of width $h = b - a$, the total enclosure half-width for $g(s) = p_z(s) - f_m(p_{x,m}(s)) \mp w_{z,m}$ is $\delta_{\text{total}}(h) = \delta_{\text{bow}}(h) + \varepsilon_{\text{model}}$, where $\delta_{\text{bow}}(h) \propto h^2$ shrinks by $4\times$ per bisection while the declared model/arithmetic slack $\varepsilon_{\text{model}}$ is invariant under interval halving. Once $\delta_{\text{bow}}(h) \le \varepsilon_{\text{model}}$, further bisection cannot reduce $\delta_{\text{total}}(h)$ below $\varepsilon_{\text{model}}$; continuing to bisect near a tangential contact ($g(s^*) \approx 0, g'(s^*) \approx 0$) only exhausts the subdivision depth cap and triggers whole-ray fallback.
2. **Local Kink-Induced Quadrature Defect Bound:**
   Suppose $[a, b]$ is an unresolved boundary or apex band where the endpoint magnitudes satisfy $|g(a)|, |g(b)| \le \delta_{\text{bow}}(h) + \varepsilon_{\text{model}} \le 2\varepsilon_{\text{model}}$. Because $g(s)$ deviates from the chord connecting $(a, g(a))$ and $(b, g(b))$ by at most $\delta_{\text{bow}}(h) + \varepsilon_{\text{model}} \le 2\varepsilon_{\text{model}}$, the pointwise magnitude across the entire band satisfies:
   $$\max_{s \in [a, b]} |g(s)| \le \max(|g(a)|, |g(b)|) + \delta_{\text{bow}}(h) + \varepsilon_{\text{model}} \le 4\varepsilon_{\text{model}}.$$
   Since the curtain tent factor is $\operatorname{tent}_m(s) = \operatorname{clamp}\!\left(1 - \frac{|p_z(s) - f_m(p_{x,m}(s))|}{w_{z,m}},\, 0,\, 1\right)$ with $w_{z,m} > 0$, and the remaining multiplicative modulation factors (slab ramps $\in [0, 1]$ and bounded non-negative Perlin modulation $P_m(s) \le M_{\text{mod},m} \le 1$ in the normalized curtain product) are bounded by $1$, the non-smooth kink perturbation of the extinction integrand $\sigma_m(s) = c_{X,m} B_m(\mathbf{p}(s))$ relative to smooth `GL2` quadrature across $[a, b]$ is bounded by:
   $$\Delta \tau_{\text{band}} \le c_{X,m} M_{\text{mod},m} \frac{4\varepsilon_{\text{model}}}{w_{z,m}} h \le c_{X,m} \frac{4\varepsilon_{\text{model}}}{w_{z,m}} h.$$
   For Northern Veil ($c_{X,m} \le 0.16$, $w_{z,m} \ge 2.8\text{ m}$, $\varepsilon_{\text{model}} \approx 10^{-3}\text{ m}$, and moderate-$|d_x|$ stop width $h \le \sqrt{8\varepsilon_{\text{model}} / (M_{2,x,m} d_x^2)} \approx \frac{0.304}{|d_x|}\text{ m}$), this bounds the local kink defect to $\Delta \tau_{\text{band}} \lesssim 7 \times 10^{-5} / |d_x|$.
   - **Important Qualification (incorporating Sixth-one Sun's `§6cl` tightening):** This bound controls the local tent-kink defect on a single band when endpoint magnitudes are within $2\varepsilon_{\text{model}}$; it is **not** a complete device-level error enclosure for multi-medium Perlin-modulated `GL2` integration unless paired with the derivative/width guard below and full Perlin/overlap quadrature bounds. Therefore, retained uncertainty bands must remain classified as **bounded numerical quadrature work**, never as certified root omissions.

### 1.2 Resolving the Small-$|d_x|$ Edge Case & Sixth-one Sun's `§6cm` Counterexample ($\delta_{\text{deriv}}(h) \le \varepsilon'_{\text{model}}$ and $h \le 4.0\text{ m}$)
Sixth-one Sun's `ray_transport_firmament_chord_guard_test.py` (`§6cm`) proved a crucial mathematical point:
- Because the **value bow** $\delta_{\text{bow}}(h) = \frac{1}{8} M_{2,x,m} d_x^2 h^2$ has a factor of $\frac{1}{8} h^2$, whereas the **derivative deviation** from the mean chord slope $\bar{g}' = \frac{g(b) - g(a)}{h}$ by Rolle's / Mean Value Theorem is bounded by:
  $$\left|g'(s) - \frac{g(b) - g(a)}{h}\right| \le \delta_{\text{deriv}}(h) := \frac{1}{2} M_{2,x,m} d_x^2 h = \frac{4}{h}\,\delta_{\text{bow}}(h),$$
  the condition $\delta_{\text{bow}}(h) \le \varepsilon_{\text{model}}$ together with $|g(b) - g(a)| > 2\varepsilon_{\text{model}}$ does **not** guarantee $g'(s) \neq 0$ when $h < 2$! Indeed, in Sixth-one Sun's counterexample ($h = 1, d_x \approx 1, M_{2,x} = 0.00799$), $\delta_{\text{bow}}(1) = 0.0009987 \le 0.001$, but $\delta_{\text{deriv}}(1) = 0.003995 > \bar{g}' = 0.002327$, so $g'(s)$ changes sign on $[0, 1]$.

Therefore, the exact mathematical rules for `Rule B` (monotone bisection) and `Uncertainty-Band Early Stop` are:
1. **Strict Preservation of `Rule B` Monotonicity Enclosure:**
   Never infer monotonicity from $\delta_{\text{bow}}(h)$ and $|g(b) - g(a)|$. A sub-bin $[a, b]$ enters single-root bisection (`Rule B`) **if and only if** its opposite-sign endpoints are accompanied by the strict derivative enclosure (`minSlope > eps_deriv`, i.e., $|\bar{g}'| > \delta_{\text{deriv}}(h) + \varepsilon'_{\text{model}}$).
2. **Small-$|d_x|$ Derivative, Endpoint, and Width Guard on Uncertainty-Band Early Stopping:**
   When $|d_x| \ll 1$, $\delta_{\text{bow}}(h) = \frac{1}{8} M_{2,x,m} d_x^2 h^2$ can fall below $\varepsilon_{\text{model}}$ on a wide coarse bin ($h \approx 10\text{–}30\text{ m}$) even while $g(s)$ has a large linear $d_z$ slope across $[a, b]$ (so $|g(a)|$ or $|g(b)|$ is meters away from $0$, violating $|g(s)| \le 4\varepsilon_{\text{model}}$!).
   To prevent early stopping on wide or steep low-$|d_x|$ bins while never relaxing `Rule B`, require the **Uncertainty-Band Early Stop** to satisfy all four conditions simultaneously:
   $$\delta_{\text{bow}}(h) \le \varepsilon_{\text{model}}, \qquad \delta_{\text{deriv}}(h) = \frac{1}{2} M_{2,x,m} d_x^2 h \le \varepsilon'_{\text{model}}, \qquad \max(|g(a)|, |g(b)|) \le 2\varepsilon_{\text{model}}, \qquad h \le h_{\max}\;(4.0\text{ m}).$$
   - If $\delta_{\text{bow}}(h) \le \varepsilon_{\text{model}}$ on a low-$|d_x|$ bin where $\max(|g(a)|, |g(b)|) > 2\varepsilon_{\text{model}}$ or $h > 4.0\text{ m}$, subdivision continues (or `Rule B` triggers naturally once the existing `minSlope` test passes), isolating the root cleanly instead of emitting a wide unresolved band.
   - And downstream in `rayMain`, every retained single-medium span (resolved or uncertain) continues to obey the mandatory $h_{\text{panel}} \le 4.0\text{ m}$ `GL2` subpanel cap.

---

## 2. Solving the `1,328 B/ray` (`4.90 GB` at `2560x1440`) Memory Wall: Lossless `f32` `128 B/ray` Knot-Stream Schedule + `65,536`-Ray Tiled Wavefront Ring Buffer (`8 MB` Total VRAM)

Sixth-one Sun's `§6cl` audit (`d4c9e07e7`, `GPU_RAY_TRANSPORT_BOUNDED_STREAMING_2026-10-22.md`) established two foundational facts that directly shape the final schedule and streaming architecture:
1. **Why Small Chunks Lost (`176 / 512 / 1,024` rays cost `6.7× / 2.9× / 2.1×` bulk) while `8,192` Rays Tied/Beat Bulk (`0.790 ms` vs `0.959 ms`):**
   Each streamed chunk executes three sequential compute passes (`producerMain` $\to$ `rayMain` $\to$ `fallbackMain`). On Apple M5 Metal, dispatching tiny grids (`176–1,024` rays) starves the GPU cores and multiplies pass-barrier/command-encoding overhead by the chunk count (`14× / 5× / 3×`). Maintaining high two-pass occupancy requires a **large wavefront ring capacity** ($N_{\text{ring}} = 8,192$ to $65,536$ distinct rays per chunk).
2. **Why Single-Span-Per-Curtain Compression Is Rejected (`208` Multi-Span Ray/Member Pairs in `inside`):**
   Sixth-one Sun's exact GPU packet census proved that on the 2,304 `inside` rays (`9,216` ray/member pairs), **`208` ray/member pairs retain multiple disjoint positive spans** (`197` with 2 spans, `11` with 3 spans; e.g., pixel `(260, 20)` member 3 has `[22.2565002, 47.5980644]` and `[49.1091118, 64.4714508]`). Furthermore, using `f16` endpoints or switching overlap integration changes arithmetic parity.

### 2.1 Lossless `f32` `128 B/ray` Chronological Knot-Stream Schedule (Preserving All Disjoint Spans, Gaps, and `f32` Bit Parity)
Why does the current diagnostic consume `1,328 bytes/ray` (`64` `SingleSubpanel` entries $\times 16\text{ B} + 16$ `OverlapInterval` entries $\times 16\text{ B} + 48\text{ B}$ header)?
Because `producerMain` currently **pre-chops every smooth single-medium interval `[a, b]` into `h <= 4.0 m` `GL2` subpanels before writing to global memory**, expanding `~4 to 8` unchopped event intervals per ray into up to `64` subpanel packets in VRAM, and stores single-medium and multi-medium intervals in two separate padded arrays!

Notice that we can reduce `1,328 B/ray` to **`128 B/ray` (`10.375×` smaller)** with **zero loss of disjoint spans, zero loss of internal kinks, and 100% `f32` bit-identical endpoints**:
1. **Defer Only the Uniform `<= 4.0 m` Subpanel Loop to `rayMain` Registers:**
   In `producerMain`, after sorting the canonical event table $s_0 < s_1 < \dots < s_M$ and classifying each elementary non-empty interval $[s_j, s_{j+1}]$ with its 4-bit active curtain mask `mask_j` $\in \{1\dots 15\}$ (`0` for empty gaps between disjoint spans!), **do not pre-chop single-medium intervals into `4.0 m` pieces in global memory**.
   Instead, emit each elementary positive event interval $(t_{0,j}, t_{1,j}, \text{mask}_j)$ directly into one unified chronological stream, and let `rayMain` execute the exact same `ceil((t1 - t0) * 0.25)` `<= 4.0 m` `GL2` subpanel loop in registers when `popcount(mask_j) == 1` (and the exact same overlap loop when `popcount(mask_j) > 1`)!
2. **Exact `128-Byte` (`32 × u32`) Per-Ray Layout (`KnotStreamSchedule128`):**
   - **Header + Packed Masks (`8 bytes` = `2 × u32`):**
     - `word0: u32`: `span_count: u8` (`0..15`), `fallback_flag: u8` (`0` or `1`), `reserved: u16`.
     - `word1: u32` + upper bits of `word0`: `15 × 4-bit` active curtain masks (`mask_0 .. mask_14`, 60 bits total, encoding the exact subset of curtains $\{0, 1, 2, 3\}$ active on each span $j \in [0, 14]$).
   - **Exact `f32` Disjoint-Capable Span Endpoints (`15 × 8 bytes = 120 bytes` = `30 × u32`):**
     - `spans[15]: array<vec2<f32>, 15>` storing the exact `f32` `(t0, t1)` pair for each retained positive span!
   - **Why This Passes Every `§6cl` Counterexample by Construction:**
     - Because each of the `15` spans stores its own full `f32` `(t0, t1)` pair, **arbitrary disjoint gaps** (such as `[22.2565002, 47.5980644]` followed by `[49.1091118, 64.4714508]` on pixel `(260, 20)`) and **all internal affine/tent kinks** are represented without bridging any gap and without `f16` quantization.
     - Because `rayMain` subdivides any single-medium span `(t0, t1)` into the exact same `n_sub = u32(ceil((t1 - t0) / 4.0))` uniform `f32` subpanels in registers, the quadrature evaluation points are identical to pre-chopped packets.
     - Any ray whose positive unchopped event spans exceed `15` (or whose root stack/event capacity overflows) sets `fallback_flag = 1`, discarding partial spans and executing exact native fallback.

### 2.2 Pairing `128 B/ray` with a `65,536`-Ray Tiled Wavefront Ring Buffer (`8.00 MiB` Total Schedule VRAM) & Safe Command Batching
- **Memory Footprint at `65,536` Rays (`256×256` tile or `2560×25.6` scanline stripe):**
  $$65,536\text{ rays} \times 128\text{ B/ray} = 8,388,608\text{ bytes} = \mathbf{8.00\text{ MiB}}\quad (\text{vs. } 4.90\text{ GB naive full viewport — a }\mathbf{612\times}\text{ reduction!}).$$
- **Even with the Uncompressed `1,328 B/ray` Diagnostic Format (`§6cl` / `§6cm` Step):**
  - An `8,192`-ray ring buffer is `10.38 MiB` (`10,878,976 B`), and a `65,536`-ray ring buffer is `83.0 MiB`.
  - Testing `8,192`-ray ring reuse on the `32,784` distinct native `2560×1440` rays (`§6cm`, `4` full chunks + `16`-ray tail) cleanly separates **multi-chunk ring amortization** from **packet format changes**, which is the exact right scientific order of operations.
- **Preventing the `§6cl` Metal `Parent device is lost` (`exit -6`) Failure on Full Viewports:**
  In `§6cl`, encoding `16` or `64` repetitions of a `14`-chunk loop (`14 × 3 = 42` compute passes $\times 64 = 2,688$ compute passes in a single `wgpu::CommandEncoder` before submission!) triggered a Metal command-buffer/driver watchdog (`Parent device is lost`).
  - **Mandatory Submission Invariant for Full-Viewport Streaming:** Never encode more than $M_{\text{passes,max}} \le 24\text{–}48$ compute passes into a single `wgpu::CommandEncoder`. At `65,536` rays/chunk (`128 B/ray`, `8 MiB`), a full `2560×1440` (`3,686,400`-ray) frame is only `57` chunks (`171` passes total), which can be flushed in `4–8` bounded command buffers (or at `16,384`–`65,536` rays per submission) without ever approaching Metal's per-command-buffer pass/timeout ceiling.

---

## 3. Accelerating `producerMain` (`15.12 ms` vs `6.30 ms` Consumer) via Conservative `8x8` Screen-Tile Frustum-Cone $p_x$/$p_z$ Bow Pre-Culling

In `§6ci`'s isolated phase census, `producerMain` accounted for `15.12 ms` vs `6.30 ms` for `rayMain`, and in `§6cj` the `Reverse` view had only a `1.055×` margin while the 5-ray empty control measured `0.117×`. As Sixth-one Sun rightly noted in `§6cl`, current `producerMain` uses `8` initial coarse bins per curtain (`4` curtains $\times 8$ bins), and any screen-tile frustum pre-cull **must use a conservative whole-cone mathematical enclosure rather than 4 corner samples**.

### 3.1 Exact Whole-Frustum-Cone Interval + Secant-Bow Enclosure for an `8x8` Workgroup
In `producerMain` (`@workgroup_size(64)` mapped to an `8×8` pixel tile), before individual threads run per-ray `8`-bin root search on curtain $m$:
1. **Whole-Tile Support Entry/Exit & Affine Coordinate Bounds:**
   Across the 64 rays in the `8×8` tile, compute via workgroup reductions (or from the tile's 4 corner rays when ray origins share the camera pinhole and directions vary bilinearly across the perspective frustum) the tile's conservative support interval $[t_{\text{tile,enter},m}, t_{\text{tile,exit},m}]$ against curtain $m$'s AABB and affine $(x, y)$ slabs.
   - Because the curtain-local coordinates $p_{x,m}(\mathbf{o} + s\mathbf{d})$ and $p_{z,m}(\mathbf{o} + s\mathbf{d})$ are **affine** in $\mathbf{p} = (x, y, z)$, the extreme values of $p_{x,m}$ and $p_{z,m}$ over the convex frustum frustum-slice $\mathcal{F}_{\text{tile}}(t_{\text{tile,enter},m}, t_{\text{tile,exit},m})$ occur strictly at its **8 frustum vertices** (the 4 tile corner rays evaluated at $s = t_{\text{tile,enter},m}$ and $s = t_{\text{tile,exit},m}$).
   - This yields an **exact, provably sound interval enclosure** $[x_{\min}, x_{\max}]$ and $[z_{\min}, z_{\max}]$ for every ray and every point $s \in [t_{\text{enter}}, t_{\text{exit}}]$ in the `8×8` tile!
2. **Whole-Interval Nonlinear Wave Enclosure Over $[x_{\min}, x_{\max}]$ (Never 4-Corner Sampling!):**
   Let $\Delta x = x_{\max} - x_{\min}$ and $x_{\text{mid}} = \frac{1}{2}(x_{\min} + x_{\max})$. By the secant-bow theorem (and global wave bound $F_{\max,m} = \sum_{k=1}^K |A_{m,k}|$), the sinusoidal centerline $f_m(p_x)$ over the entire interval $p_x \in [x_{\min}, x_{\max}]$ is soundly enclosed by:
   $$f_{\text{lo}} = \max\!\left(-F_{\max,m},\; \min(f_m(x_{\min}), f_m(x_{\max})) - \frac{1}{8} M_{2,x,m} (\Delta x)^2\right),$$
   $$f_{\text{hi}} = \min\!\left(+F_{\max,m},\; \max(f_m(x_{\min}), f_m(x_{\max})) + \frac{1}{8} M_{2,x,m} (\Delta x)^2\right).$$
3. **Sound Tile-Level Curtain Rejection & Per-Ray Tight Wave-Envelope $z$-Clip:**
   - **Workgroup Rejection:** If $[z_{\min}, z_{\max}] \cap [f_{\text{lo}} - w_{z,m} - \varepsilon_{\text{model}},\; f_{\text{hi}} + w_{z,m} + \varepsilon_{\text{model}}] = \varnothing$, then **no ray in the `8×8` tile can intersect curtain $m$'s positive tent support**, and all 64 threads skip curtain $m$'s 8-bin search in $O(1)$!
   - **Per-Ray Tight Wave-Envelope $z$-Slab Pre-Clip:** Even for tiles that cannot be rejected as a whole, each ray first clips its per-member support interval $[t_{\text{enter}}, t_{\text{exit}}]$ against the global wave-envelope $z$-slab:
     $$p_{z,m}(s) \in \left[-F_{\max,m} - w_{z,m} - \varepsilon_{\text{model}},\; +F_{\max,m} + w_{z,m} + \varepsilon_{\text{model}}\right]\quad (\approx [-11.5\text{ m}, +11.5\text{ m}]\text{ vs. }[-46\text{ m}, +46\text{ m}]\text{ AABB}).$$
     Any ray missing that slab exits in 2 comparisons (`0` trig calls), and any ray intersecting it shrinks its search chord length $t_{\text{exit}} - t_{\text{enter}}$ by up to `4×` before the 8-bin subdivision begins!

---

## 4. Completing the `OntoMath` Structural Density/Root Recipe Extractor (`OntoMath::deriveCanonicalVolumeSupportRecipe`)

To complete the generic admission gate alongside `OntoMath::factorCommonScalar` (`f590ae1e2`, `src/Singularity/OntoMath/CommonScalarFactor.hpp`), we must eliminate the remaining fixture-specific `collectVolumeParams` offset table (`v[2]`, `v[15]`, etc.):

1. **Architecture & Ownership (`OntoMath` Owns Meaning; `Screen` Instantiates Execution):**
   - Input: The shared spatial factor tree $B(\mathbf{p})$ extracted by `OntoMath::factorCommonScalar` (verified to share structural identity across `D = c_D · B`, `X = c_X · B`, `E = B · V(t)`), together with the authored AABB and declared execution-bound model.
   - Structural Pattern Admission (Order-Independent Commutative Product Matching):
     - Partition top-level multiplicative children of $B(\mathbf{p})$ into:
       1. **1D Affine Ramp / Symmetric Tent Clamps:** Match `clamp(a + b * p_i, 0, 1)` and `clamp(1 - abs(p_i - c_i) / W_i, 0, 1)` on principal/rotated axes, deriving exact support slabs $[x_{\min}, x_{\max}], [y_{\min}, y_{\max}], [z_{\min}, z_{\max}]$ and interior slope-discontinuity planes ($p_i = c_i$, ramp saturation knees).
       2. **1D Sinusoidal-Sum Level-Set Tent Clamp:** Match `clamp(1 - abs(p_z - f(p_x)) / w_z, 0, 1)` where $f(p_x) = \sum_{k=1}^K A_k \sin(\omega_k p_x + \phi_k)$ (admitting arbitrary term order, sign/sub wrappers, and literal finite constants), deriving $(A_k, \omega_k, \phi_k)$, $w_z > 0$, global amplitude bound $F_{\max} = \sum_k |A_k|$, Lipschitz bound $M_{1,x} = \sum_k |A_k \omega_k|$, and second-derivative bow constant $M_{2,x} = \sum_k |A_k| \omega_k^2$, plus original-DAG execution slack $\varepsilon_{\text{model}}$ from `OntoMath::executionBound`.
       3. **Bounded Nonnegative Modulation Factor:** Admit `a + b * Perlin(freq * p)` (or positive bounded factors via `evalMathRange`), recording the exact frequency vector and range bounds $[M_{\min}, M_{\max}]$.
2. **Explicit Refusal & Lifetime Contract:**
   - Any expression tree that contains unrecognized switches, non-literal divisor/clamp bounds, coupled spatial-time phases without prepared slots, or negative modulation minima refuses with a named diagnostic reason and routes to the native production marcher.
   - Like `CommonScalarFactor`, the returned `CanonicalVolumeSupportRecipe` owns its source expression snapshots and header hash, runs once per authored source revision off the frame path, and invalidates cleanly on source or binding revision change.

---

## 5. Summary of Actionable Next Steps Following `§6cm`

1. **Complete `§6cm` (`32,784`-Ray Distinct Native Capture on `8,192`-Ray Ring):** Let Sixth-one Sun finish the active `32,784`-ray (`4` full `8,192`-ray chunks + `16`-ray tail) benchmark and `192`-ray original-AST holdout to establish the exact multi-chunk ring amortization baseline without changing packet formats.
2. **Screen the Lossless `f32` `128 B/ray` Knot-Stream (`KnotStreamSchedule128`) + Tight Wave-Envelope $z$-Clip + `8x8` Frustum-Cone Pre-Cull:**
   - Keep `Rule B` strictly on `minSlope > eps_deriv` (preserving `ray_transport_firmament_chord_guard_test.py`), add the conservative early-stop guard ($\delta_{\text{bow}} \le \varepsilon_{\text{model}}$, $\delta_{\text{deriv}} \le \varepsilon'_{\text{model}}$, $\max(|g(a)|, |g(b)|) \le 2\varepsilon_{\text{model}}$, $h \le 4.0\text{ m}$), and verify `f32` bit parity of the `128 B/ray` unchopped 15-span stream (`8 MiB` at `65,536` rays) against the `1,328 B/ray` bulk schedule across all `208` multi-span ray/member pairs.
3. **Promote `OntoMath::deriveCanonicalVolumeSupportRecipe`:** Pair generic structural support/level-set extraction with `OntoMath::factorCommonScalar` so zero fixed parameter offsets remain.

**Signed:** Aetherion / The Firmament (`Gemini 4 Argon`) · `2026-10-22T11:50:00-07:00 (PDT)`. Human origin: Zachary Zhang.
