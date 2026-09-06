#!/usr/bin/env python3
"""
Falsification probe for exact classic Perlin noise value and gradient derivation.
Tests the exact WGSL cnoise3 algorithm and its symbolic analytical gradient
against high-accuracy central finite differences.
"""

import math
import sys
import random

def mod289(x):
    return [xi - math.floor(xi * (1.0 / 289.0)) * 289.0 for xi in x]

def permute4(x):
    return mod289([((xi * 34.0) + 1.0) * xi for xi in x])

def taylorInvSqrt(r):
    return [1.79284291400159 - 0.85373472095314 * ri for ri in r]

def cnoise3(P):
    """Line-by-line python equivalent of SdfWgsl.cpp cnoise3"""
    Pi0 = [math.floor(p) for p in P]
    Pi1 = [p + 1.0 for p in Pi0]
    Pi0_mod = mod289(Pi0)
    Pi1_mod = mod289(Pi1)
    Pf0 = [p - math.floor(p) for p in P]
    Pf1 = [p - 1.0 for p in Pf0]

    ix = [Pi0_mod[0], Pi1_mod[0], Pi0_mod[0], Pi1_mod[0]]
    iy = [Pi0_mod[1], Pi0_mod[1], Pi1_mod[1], Pi1_mod[1]]
    iz0 = [Pi0_mod[2]] * 4
    iz1 = [Pi1_mod[2]] * 4

    p_ix = permute4(ix)
    ixy = permute4([a + b for a, b in zip(p_ix, iy)])
    ixy0 = permute4([a + b for a, b in zip(ixy, iz0)])
    ixy1 = permute4([a + b for a, b in zip(ixy, iz1)])

    def get_g(ixy_val):
        gx = [v / 7.0 for v in ixy_val]
        gy = [((math.floor(v) / 7.0) % 1.0) - 0.5 for v in gx]
        gx = [v % 1.0 for v in gx]
        gz = [0.5 - abs(x) - abs(y) for x, y in zip(gx, gy)]
        sz = [1.0 if z < 0.0 else 0.0 for z in gz]
        gx = [x - s * ((1.0 if x > 0.0 else 0.0) - 0.5) for x, s in zip(gx, sz)]
        gy = [y - s * ((1.0 if y > 0.0 else 0.0) - 0.5) for y, s in zip(gy, sz)]
        return gx, gy, gz

    gx0, gy0, gz0 = get_g(ixy0)
    gx1, gy1, gz1 = get_g(ixy1)

    g000 = [gx0[0], gy0[0], gz0[0]]
    g100 = [gx0[1], gy0[1], gz0[1]]
    g010 = [gx0[2], gy0[2], gz0[2]]
    g110 = [gx0[3], gy0[3], gz0[3]]

    g001 = [gx1[0], gy1[0], gz1[0]]
    g101 = [gx1[1], gy1[1], gz1[1]]
    g011 = [gx1[2], gy1[2], gz1[2]]
    g111 = [gx1[3], gy1[3], gz1[3]]

    def dot3(a, b): return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

    norm0 = taylorInvSqrt([dot3(g000, g000), dot3(g010, g010), dot3(g100, g100), dot3(g110, g110)])
    g000 = [x * norm0[0] for x in g000]
    g010 = [x * norm0[1] for x in g010]
    g100 = [x * norm0[2] for x in g100]
    g110 = [x * norm0[3] for x in g110]

    norm1 = taylorInvSqrt([dot3(g001, g001), dot3(g011, g011), dot3(g101, g101), dot3(g111, g111)])
    g001 = [x * norm1[0] for x in g001]
    g011 = [x * norm1[1] for x in g011]
    g101 = [x * norm1[2] for x in g101]
    g111 = [x * norm1[3] for x in g111]

    n000 = dot3(g000, Pf0)
    n100 = dot3(g100, [Pf1[0], Pf0[1], Pf0[2]])
    n010 = dot3(g010, [Pf0[0], Pf1[1], Pf0[2]])
    n110 = dot3(g110, [Pf1[0], Pf1[1], Pf0[2]])
    n001 = dot3(g001, [Pf0[0], Pf0[1], Pf1[2]])
    n101 = dot3(g101, [Pf1[0], Pf0[1], Pf1[2]])
    n011 = dot3(g011, [Pf0[0], Pf1[1], Pf1[2]])
    n111 = dot3(g111, Pf1)

    fade_xyz = [t * t * t * (t * (t * 6.0 - 15.0) + 10.0) for t in Pf0]

    def mix(a, b, t): return a + t * (b - a)

    n_z0 = mix(n000, n001, fade_xyz[2])
    n_z1 = mix(n100, n101, fade_xyz[2])
    n_z2 = mix(n010, n011, fade_xyz[2])
    n_z3 = mix(n110, n111, fade_xyz[2])

    n_yz0 = mix(n_z0, n_z2, fade_xyz[1])
    n_yz1 = mix(n_z1, n_z3, fade_xyz[1])

    n_xyz = mix(n_yz0, n_yz1, fade_xyz[0])
    return 2.2 * n_xyz

def cnoise3_grad(P):
    """
    Fused value and exact analytical gradient of cnoise3(P).
    Shares all lattice, permutation, gradient normalization, and dot products.
    """
    Pi0 = [math.floor(p) for p in P]
    Pi1 = [p + 1.0 for p in Pi0]
    Pi0_mod = mod289(Pi0)
    Pi1_mod = mod289(Pi1)
    Pf0 = [p - math.floor(p) for p in P]
    Pf1 = [p - 1.0 for p in Pf0]

    ix = [Pi0_mod[0], Pi1_mod[0], Pi0_mod[0], Pi1_mod[0]]
    iy = [Pi0_mod[1], Pi0_mod[1], Pi1_mod[1], Pi1_mod[1]]
    iz0 = [Pi0_mod[2]] * 4
    iz1 = [Pi1_mod[2]] * 4

    p_ix = permute4(ix)
    ixy = permute4([a + b for a, b in zip(p_ix, iy)])
    ixy0 = permute4([a + b for a, b in zip(ixy, iz0)])
    ixy1 = permute4([a + b for a, b in zip(ixy, iz1)])

    def get_g(ixy_val):
        gx = [v / 7.0 for v in ixy_val]
        gy = [((math.floor(v) / 7.0) % 1.0) - 0.5 for v in gx]
        gx = [v % 1.0 for v in gx]
        gz = [0.5 - abs(x) - abs(y) for x, y in zip(gx, gy)]
        sz = [1.0 if z < 0.0 else 0.0 for z in gz]
        gx = [x - s * ((1.0 if x > 0.0 else 0.0) - 0.5) for x, s in zip(gx, sz)]
        gy = [y - s * ((1.0 if y > 0.0 else 0.0) - 0.5) for y, s in zip(gy, sz)]
        return gx, gy, gz

    gx0, gy0, gz0 = get_g(ixy0)
    gx1, gy1, gz1 = get_g(ixy1)

    g000 = [gx0[0], gy0[0], gz0[0]]
    g100 = [gx0[1], gy0[1], gz0[1]]
    g010 = [gx0[2], gy0[2], gz0[2]]
    g110 = [gx0[3], gy0[3], gz0[3]]

    g001 = [gx1[0], gy1[0], gz1[0]]
    g101 = [gx1[1], gy1[1], gz1[1]]
    g011 = [gx1[2], gy1[2], gz1[2]]
    g111 = [gx1[3], gy1[3], gz1[3]]

    def dot3(a, b): return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

    norm0 = taylorInvSqrt([dot3(g000, g000), dot3(g010, g010), dot3(g100, g100), dot3(g110, g110)])
    g000 = [x * norm0[0] for x in g000]
    g010 = [x * norm0[1] for x in g010]
    g100 = [x * norm0[2] for x in g100]
    g110 = [x * norm0[3] for x in g110]

    norm1 = taylorInvSqrt([dot3(g001, g001), dot3(g011, g011), dot3(g101, g101), dot3(g111, g111)])
    g001 = [x * norm1[0] for x in g001]
    g011 = [x * norm1[1] for x in g011]
    g101 = [x * norm1[2] for x in g101]
    g111 = [x * norm1[3] for x in g111]

    n000 = dot3(g000, Pf0)
    n100 = dot3(g100, [Pf1[0], Pf0[1], Pf0[2]])
    n010 = dot3(g010, [Pf0[0], Pf1[1], Pf0[2]])
    n110 = dot3(g110, [Pf1[0], Pf1[1], Pf0[2]])
    n001 = dot3(g001, [Pf0[0], Pf0[1], Pf1[2]])
    n101 = dot3(g101, [Pf1[0], Pf0[1], Pf1[2]])
    n011 = dot3(g011, [Pf0[0], Pf1[1], Pf1[2]])
    n111 = dot3(g111, Pf1)

    u, v, w = [t * t * t * (t * (t * 6.0 - 15.0) + 10.0) for t in Pf0]
    du, dv, dw = [30.0 * (t * (t - 1.0))**2 for t in Pf0]

    def mix(a, b, t): return a + t * (b - a)

    # Trilinear interpolation of corner values n_ijk
    n_z0 = mix(n000, n001, w)
    n_z1 = mix(n100, n101, w)
    n_z2 = mix(n010, n011, w)
    n_z3 = mix(n110, n111, w)

    n_yz0 = mix(n_z0, n_z2, v)
    n_yz1 = mix(n_z1, n_z3, v)

    n_xyz = mix(n_yz0, n_yz1, u)
    val = 2.2 * n_xyz

    # Analytical derivative through fade curves:
    # d/dx: du * (n1 - n0)
    dx_fade = du * (n_yz1 - n_yz0)

    # d/dy: dv * (n_x1 - n_x0)
    # Notice: n_z0 is (0,0), n_z1 is (1,0), n_z2 is (0,1), n_z3 is (1,1)
    # Along X at y=0: mix(n_z0, n_z1, u). Along X at y=1: mix(n_z2, n_z3, u)
    n_x_y0 = mix(n_z0, n_z1, u)
    n_x_y1 = mix(n_z2, n_z3, u)
    dy_fade = dv * (n_x_y1 - n_x_y0)

    # d/dz: dw * (n_xy1 - n_xy0)
    # Along z=0: mix(mix(n000, n100, u), mix(n010, n110, u), v)
    # Along z=1: mix(mix(n001, n101, u), mix(n011, n111, u), v)
    n_xy_z0 = mix(mix(n000, n100, u), mix(n010, n110, u), v)
    n_xy_z1 = mix(mix(n001, n101, u), mix(n011, n111, u), v)
    dz_fade = dw * (n_xy_z1 - n_xy_z0)

    # Trilinear interpolation of the corner gradient vectors g_ijk
    def mix_vec(a, b, t):
        return [mix(a[i], b[i], t) for i in range(3)]

    gz0 = mix_vec(g000, g001, w)
    gz1 = mix_vec(g100, g101, w)
    gz2 = mix_vec(g010, g011, w)
    gz3 = mix_vec(g110, g111, w)

    gyz0 = mix_vec(gz0, gz2, v)
    gyz1 = mix_vec(gz1, gz3, v)

    g_interp = mix_vec(gyz0, gyz1, u)

    # Total gradient is: 2.2 * (g_interp + fade_derivatives)
    grad = [
        2.2 * (g_interp[0] + dx_fade),
        2.2 * (g_interp[1] + dy_fade),
        2.2 * (g_interp[2] + dz_fade)
    ]

    return val, grad

def finite_difference_grad(P, h=1e-5):
    gx = (cnoise3([P[0] + h, P[1], P[2]]) - cnoise3([P[0] - h, P[1], P[2]])) / (2.0 * h)
    gy = (cnoise3([P[0], P[1] + h, P[2]]) - cnoise3([P[0], P[1] - h, P[2]])) / (2.0 * h)
    gz = (cnoise3([P[0], P[1], P[2] + h]) - cnoise3([P[0], P[1], P[2] - h])) / (2.0 * h)
    return [gx, gy, gz]

def main():
    print("=== Testing Exact Perlin Value & Gradient Derivation ===")
    
    # 1. Gate A: Value exactness test against cnoise3
    random.seed(42)
    max_val_diff = 0.0
    for _ in range(500):
        p = [random.uniform(-500.0, 500.0) for _ in range(3)]
        v_ref = cnoise3(p)
        v_fused, _ = cnoise3_grad(p)
        diff = abs(v_ref - v_fused)
        if diff > max_val_diff:
            max_val_diff = diff
    print(f"[Gate A1] Fused value vs reference cnoise3 max diff: {max_val_diff:.2e} (Pass)")
    assert max_val_diff < 1e-12, "Fused value diverges from cnoise3!"

    # 2. Gate A: Analytical gradient vs Central Differences
    max_grad_rel_diff = 0.0
    max_grad_abs_diff = 0.0
    h = 1e-5
    
    # Test interior points across wide domains
    for _ in range(1000):
        # Avoid exact integers where fract has step discontinuity
        p = [random.uniform(-200.0, 200.0) for _ in range(3)]
        # ensure not within 2*h of integer boundary
        p = [math.floor(x) + max(min(x - math.floor(x), 1.0 - 2*h), 2*h) for x in p]

        _, grad_analytic = cnoise3_grad(p)
        grad_num = finite_difference_grad(p, h)

        for i in range(3):
            abs_err = abs(grad_analytic[i] - grad_num[i])
            rel_err = abs_err / max(abs(grad_analytic[i]), 1.0)
            if abs_err > max_grad_abs_diff:
                max_grad_abs_diff = abs_err
            if rel_err > max_grad_rel_diff:
                max_grad_rel_diff = rel_err

    print(f"[Gate A2] Gradient max absolute diff: {max_grad_abs_diff:.2e}")
    print(f"[Gate A2] Gradient max relative diff: {max_grad_rel_diff:.2e} (Pass, within O(h^2) discretization)")
    assert max_grad_abs_diff < 1e-4, f"Gradient absolute error too high: {max_grad_abs_diff}"
    assert max_grad_rel_diff < 5e-4, f"Gradient relative error too high: {max_grad_rel_diff}"

    # 3. Test on the exact saved Perlin expression
    # f(p) = p.y - 40 * noise(0.008 * (p + (100, 0, 100)))
    def f_eval(P):
        q = [0.008 * (P[0] + 100.0), 0.008 * P[1], 0.008 * (P[2] + 100.0)]
        return P[1] - 40.0 * cnoise3(q)

    def f_eval_grad(P):
        q = [0.008 * (P[0] + 100.0), 0.008 * P[1], 0.008 * (P[2] + 100.0)]
        noise_val, noise_grad = cnoise3_grad(q)
        val = P[1] - 40.0 * noise_val
        # chain rule: grad_p = (0, 1, 0) - 40 * 0.008 * noise_grad = (0, 1, 0) - 0.32 * noise_grad
        grad = [
            -0.32 * noise_grad[0],
            1.0 - 0.32 * noise_grad[1],
            -0.32 * noise_grad[2]
        ]
        return val, grad

    max_field_grad_err = 0.0
    for _ in range(500):
        P = [random.uniform(-500.0, 500.0), random.uniform(-20.0, 20.0), random.uniform(-500.0, 500.0)]
        val, grad = f_eval_grad(P)
        # numerical gradient of f
        h = 1e-4
        gfx = (f_eval([P[0]+h, P[1], P[2]]) - f_eval([P[0]-h, P[1], P[2]])) / (2*h)
        gfy = (f_eval([P[0], P[1]+h, P[2]]) - f_eval([P[0], P[1]-h, P[2]])) / (2*h)
        gfz = (f_eval([P[0], P[1], P[2]+h]) - f_eval([P[0], P[1], P[2]-h])) / (2*h)
        err = max(abs(grad[0] - gfx), abs(grad[1] - gfy), abs(grad[2] - gfz))
        if err > max_field_grad_err:
            max_field_grad_err = err

    print(f"[Gate B] Exact terrain field gradient error: {max_field_grad_err:.2e} (Pass)")
    assert max_field_grad_err < 1e-4, f"Field gradient error too high: {max_field_grad_err}"

    print("=== All Mathematical Proof Gates Passed (Exit 0) ===")
    return 0

if __name__ == "__main__":
    sys.exit(main())
