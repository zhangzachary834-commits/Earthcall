# Derivation & Mathematical Audit: Exact Fused Perlin Value & Gradient (`ScalarJet`)

**Date**: 2026-09-05 22:45 PDT  
**Author**: Gemini Spark · session `c_9e6b76f2`  
**Supervision**: Antigravity Gemini 3.1 Pro  
**Parent Plan**: [`docs/plans/PERLIN_EXACT_VALUE_GRADIENT_IMPLEMENTATION_PLAN_2026-09-05.md`](../../plans/PERLIN_EXACT_VALUE_GRADIENT_IMPLEMENTATION_PLAN_2026-09-05.md)  
**Status**: Derivation complete; numeric falsification verified in `scratch/probes/test_exact_perlin_gradient.py` (Exit 0, max diff $< 2.02 \times 10^{-9}$)  

---

## 1. Actual WGSL `cnoise3` & CPU OntoMath `Noise` Semantics

### A. CPU Reference (`Singularity/OntoMath/ScalarForm.cpp`)
* CPU evaluation of `MathNode::Op::Noise` calls `glm::perlin(va)` (`ScalarForm.cpp:1722`).
* In `ScalarForm.cpp:1967-1975`, the 3D classic Perlin noise bound is documented as:
  $$\sup |glm::perlin| = 2.2 \times \frac{\sqrt{3}}{2} = 1.905255$$
* The scale factor $2.2$ is a hardcoded normalization constant in GLM's classic Perlin implementation.

### B. GPU Reference (`Singularity/Screen/WebGPU/SdfWgsl.cpp`)
* Lines 95–174 implement `cnoise3(P: vec3<f32>) -> f32`.
* Coordinate breakdown:
  * Integer lattice: $Pi_0 = \lfloor P \rfloor$, $Pi_1 = Pi_0 + 1$.
  * Modulo wrapping: $Pi_0^{\text{mod}} = \text{mod289}(Pi_0)$, $Pi_1^{\text{mod}} = \text{mod289}(Pi_1)$.
  * Fractional offset: $Pf_0 = \text{fract}(P)$, $Pf_1 = Pf_0 - 1$.
* Permutation hash:
  * Permute polynomial: $\text{permute4}(x) = \text{mod289}((34x + 1)x)$.
  * Permuted indices: $ixy = \text{permute4}(\text{permute4}(ix) + iy)$, $ixy_0 = \text{permute4}(ixy + iz_0)$, $ixy_1 = \text{permute4}(ixy + iz_1)$.
* Gradient vector generation & normalization:
  * 8 unnormalized corner vectors $g_{ijk}$ extracted from $ixy_0, ixy_1$.
  * Normalization factor via Taylor inverse square root: $\text{taylorInvSqrt}(r) = 1.79284291400159 - 0.85373472095314 \cdot r$.
* 8 corner projections:
  * $n_{ijk} = g_{ijk} \cdot v_{ijk}$, where $v_{ijk} \in \{Pf_0, Pf_1\}^3$.
* Quintic fade & trilinear interpolation:
  * $\text{fade}(t) = t^3 (t(6t - 15) + 10) = 6t^5 - 15t^4 + 10t^3$.
  * Interp along Z, then Y, then X: $n_{xyz}$.
  * Final return: $2.2 \times n_{xyz}$.

---

## 2. Closed-Form Analytical Derivative Derivation

Let $P \in \mathbb{R}^3$. The integer lattice $Pi_0 = \lfloor P \rfloor$ is locally constant almost everywhere, so $\frac{\partial Pi_0}{\partial P} = 0$. Consequently, the 8 corner gradients $g_{ijk}$ have $\frac{\partial g_{ijk}}{\partial P} = 0$.

The corner displacement vectors $v_{ijk} = P - \lfloor P \rfloor - c_{ijk}$ satisfy:
$$\frac{\partial v_{ijk}}{\partial P} = I_{3 \times 3}$$
Therefore, the spatial gradient of each corner projection $n_{ijk} = g_{ijk} \cdot v_{ijk}$ is identically the corner gradient vector:
$$\nabla_P (n_{ijk}) = g_{ijk}$$

### The Trilinear Interpolation Product Rule
Let $u = \text{fade}(Pf_0.x)$, $v = \text{fade}(Pf_0.y)$, $w = \text{fade}(Pf_0.z)$.
The trilinear interpolation $N$ over the unit cube $[0, 1]^3$ can be expressed as:
$$N = \sum_{i,j,k \in \{0, 1\}} B_i(u) B_j(v) B_k(w) n_{ijk}(P)$$
where $B_0(t) = 1 - t$, $B_1(t) = t$.

Applying the multivariable chain and product rules:
$$\nabla_P N = \sum_{i,j,k} B_i(u) B_j(v) B_k(w) \nabla_P (n_{ijk}) + \begin{pmatrix} \frac{\partial N}{\partial u} \frac{du}{dx} \\ \frac{\partial N}{\partial v} \frac{dv}{dy} \\ \frac{\partial N}{\partial w} \frac{dw}{dz} \end{pmatrix}$$

1. **Term 1 (Interpolated Gradients)**:
   $$\sum_{i,j,k} B_i(u) B_j(v) B_k(w) g_{ijk} = \text{trilinear}(g_{000}, \dots, g_{111}; u, v, w)$$
   This is simply the trilinear blend of the 8 normalized gradient vectors!

2. **Term 2 (Fade Derivatives)**:
   Since $\text{fade}(t) = 6t^5 - 15t^4 + 10t^3$:
   $$\text{fade}'(t) = 30t^4 - 60t^3 + 30t^2 = 30 t^2 (t - 1)^2 = 30 (Pf_0 \cdot Pf_1)^2$$
   Let:
   * $u' = 30 (Pf_0.x \cdot Pf_1.x)^2$
   * $v' = 30 (Pf_0.y \cdot Pf_1.y)^2$
   * $w' = 30 (Pf_0.z \cdot Pf_1.z)^2$

   The partial derivatives with respect to the fade weights are obtained directly from the intermediate values computed during standard trilinear interpolation:
   * $\frac{\partial N}{\partial u} = n_1 - n_0 = n_{yz1} - n_{yz0}$
   * $\frac{\partial N}{\partial v} = n_{x\_y1} - n_{x\_y0}$, where $n_{x\_y0} = \text{mix}(n_{z0}, n_{z1}, u)$ and $n_{x\_y1} = \text{mix}(n_{z2}, n_{z3}, u)$
   * $\frac{\partial N}{\partial w} = n_{xy\_z1} - n_{xy\_z0}$, where $n_{xy\_z0} = \text{mix}(\text{mix}(n_{000}, n_{100}, u), \text{mix}(n_{010}, n_{110}, u), v)$ and $n_{xy\_z1} = \text{mix}(\text{mix}(n_{001}, n_{101}, u), \text{mix}(n_{011}, n_{111}, u), v)$

### Complete Fused Result
$$\text{cnoise3\_grad}(P) = \begin{cases} \text{value} = 2.2 \times N \\ \nabla_P = 2.2 \times \left( \text{trilinear}(g_{ijk}; u, v, w) + \begin{pmatrix} u' \cdot (n_{yz1} - n_{yz0}) \\ v' \cdot (n_{x\_y1} - n_{x\_y0}) \\ w' \cdot (n_{xy\_z1} - n_{xy\_z0}) \end{pmatrix} \right) \end{cases}$$

---

## 3. OntoMath Differentiability Inventory

| Operation (`MathNode::Op`) | Classification | Derivative Rule (`ScalarJet`) |
|---|---|---|
| `ValueLeaf` (coord component $x, y, z$) | Fully Differentiable | $\text{grad} = e_x, e_y, \text{ or } e_z$ |
| `ValueLeaf` (ambient point $p$) | Fully Differentiable | Jacobian $J = I_{3 \times 3}$ |
| `ScalarLeaf` / Constants | Fully Differentiable | $\text{grad} = \vec{0}$ |
| `Add` / `Sub` | Fully Differentiable | $a \pm b \implies a.\text{grad} \pm b.\text{grad}$ |
| `Negate` | Fully Differentiable | $-a \implies -a.\text{grad}$ |
| `Scale` ($c \cdot a$) | Fully Differentiable | $c \cdot a.\text{grad}$ |
| `Mul` ($a \cdot b$) | Fully Differentiable | $a.\text{val} \cdot b.\text{grad} + b.\text{val} \cdot a.\text{grad}$ |
| `Div` ($a / b$) | Differentiable a.e. | Guarded: if $|b| < 10^{-6} \implies \text{fallback}$, else quotient rule |
| `Pow` ($a^k$, const $k$) | Differentiable on domain | $k \cdot a^{k-1} \cdot a.\text{grad}$ (guarded $a > 0$ for fractional $k$) |
| `Sin` / `Cos` | Fully Differentiable | $\cos(a) \cdot a.\text{grad}$, $-\sin(a) \cdot a.\text{grad}$ |
| `Exp` / `Ln` | Fully Differentiable on domain | $e^a \cdot a.\text{grad}$, $\frac{1}{a} \cdot a.\text{grad}$ (guarded $a > 10^{-12}$) |
| `Sqrt` | Differentiable on $(0, \infty)$ | $\frac{1}{2\sqrt{a}} \cdot a.\text{grad}$ (guarded $a > 10^{-7}$) |
| `Tan` | Differentiable on domain | $(1 + \tan^2(a)) \cdot a.\text{grad}$ |
| `Noise` | Differentiable a.e. | Fused `cnoise3_grad` + Chain rule $J_q^T \nabla_q$ |
| `Abs` | Piecewise Differentiable | $\text{sign}(a) \cdot a.\text{grad}$ (0 at 0) |
| `Clamp` ($a, lo, hi$) | Piecewise Differentiable | $\text{select}(0, 1, lo \le a \le hi) \cdot a.\text{grad}$ |
| `Union` (min) / `Intersection` (max) | Piecewise Differentiable | $\text{select}(b.\text{grad}, a.\text{grad}, a.\text{val} < b.\text{val})$ |
| `Floor` / `Round` | Non-Differentiable / Step | Derivative is 0 a.e.; trips fallback if composing distance |
| `Raycast` / `LineIntegral` | Unsupported | Explicit refusal; falls back to finite differences |

### Chain Rule Application to the Saved Perlin Floor
For $f(p) = p.y - 40 \cdot \text{Noise}(0.008 \cdot (p + \text{vec3}(100, 0, 100)))$:
* Entire expression consists strictly of Category A operations (Sub, Scale, Add, ValueLeaf, Noise).
* No discontinuities, no branch boundaries, no zero divisors.
* Closed-form analytical gradient:
  $$\nabla f(p) = \begin{pmatrix} 0 \\ 1 \\ 0 \end{pmatrix} - 0.32 \cdot \nabla_q \text{cnoise3}(q), \quad q = 0.008 \cdot (p + (100, 0, 100))$$

---

## 4. Operation Count & ALU Benchmark

* **Current finite-difference path (`sdfGrad`)**:
  5 complete calls to `cnoise3` per step.
  Each `cnoise3` call evaluates ~150 ALU ops.
  Total per march step: $\mathbf{5 \times 150 = 750\text{ ALU instructions}}$.
* **Proposed Fused `ScalarJet` path (`cnoise3_grad`)**:
  1 lattice setup, 1 hash, 1 gradient normalization, 8 dot products: ~130 ALU ops.
  Fade derivative: 3 muls per component = 9 ALU ops.
  Vector trilinear interpolation of $g_{ijk}$: 7 vector `mix` = 21 ALU ops.
  Combination: 6 ALU ops.
  Total per march step: $\mathbf{\sim 175\text{ ALU instructions}}$.
* **Net ALU Reduction**: **$4.3\times$ fewer ALU instructions per step**, completely eliminating the 4 extra sampling calls per iteration.

---

## 5. Numeric Verification Results (`scratch/probes/test_exact_perlin_gradient.py`)

A standalone mathematical verification probe was constructed and executed over 2,000 deterministic points across $[-500, 500]^3$:
1. **Gate A1 (Value Parity)**:
   Max difference between `cnoise3_grad.value` and reference `cnoise3`:
   $$\max |V_{\text{fused}} - V_{\text{ref}}| = \mathbf{0.00\text{e}+00}$$
   (Bit-identical value preservation).
2. **Gate A2 (Gradient Correctness)**:
   Tested against symmetric central finite differences with step $h = 10^{-5}$:
   * Maximum absolute difference: $\mathbf{2.02 \times 10^{-9}}$
   * Maximum relative difference: $\mathbf{1.92 \times 10^{-9}}$
   (Conforms perfectly to $O(h^2)$ discretization error).
3. **Gate B (Terrain Field Chain Rule)**:
   Tested full terrain field $f(p) = p.y - 40 \cdot \text{noise}(0.008 \cdot (p + (100, 0, 100)))$:
   * Maximum gradient error against central differences: $\mathbf{7.53 \times 10^{-10}}$.

The derivation is verified, exits 0, and is ready for Codex's review and compiler integration.
