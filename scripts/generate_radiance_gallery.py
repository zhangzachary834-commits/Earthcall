#!/usr/bin/env python3
"""
Radiance Gallery — 28-Room OntoMath Radiance Showcase (Rung 3, 4, 5, 6 & Combined)

Grid Architecture (6 Columns, 28 Rooms Total):
- EXISTING COLUMN (3 rooms, Z = 0, X in [-40, 0]): PRESERVED UNTOUCHED
- RUNG 3 COLUMN   (5 rooms, Z = 35,  X in [-40, 40]): Authored spatial scalar radiance rho(p)
- RUNG 4 COLUMN   (5 rooms, Z = 70,  X in [-40, 40]): Time-aware authored radiance rho(p,t)
- RUNG 5 COLUMN   (5 rooms, Z = 105, X in [-40, 40]): Independently authored chroma chi(p,t)
- RUNG 6 COLUMN   (5 rooms, Z = 140, X in [-40, 40]): Authored angular emission alpha(p,omega,t)
- COMBINED COLUMN (5 rooms, Z = 175, X in [-40, 40]): Synthesis rho(p,t) * chi(p,t) * alpha(p,omega,t)
"""

import json
import os
import math

# -----------------------------------------------------------------------------
# OntoMath AST Construction Helpers
# -----------------------------------------------------------------------------

def scalar_node(val):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(val), "factors": {}}]}}

def scalar_term_node(c, factors=None, trans=None):
    term = {"c": float(c), "factors": factors or {}}
    if trans:
        term["trans"] = trans
    return {"op": 0, "scalarForm": {"terms": [term]}}

def poly_node(terms):
    return {"op": 0, "scalarForm": {"terms": terms}}

def vec3_node(x, y, z):
    return {
        "op": 2,
        "children": [scalar_node(x), scalar_node(y), scalar_node(z)]
    }

def vec3_expr_node(rx, gy, bz):
    return {
        "op": 2,
        "children": [rx, gy, bz]
    }

def var_node(name):
    return {"op": 1, "var": name}

def dist_node(pt_node, center_vec3):
    return {"op": 15, "children": [pt_node, center_vec3]}

def add_node(a, b):
    return {"op": 4, "children": [a, b]}

def sub_node(a, b):
    return {"op": 5, "children": [a, b]}

def scale_node(scalar, expr):
    return {"op": 6, "children": [scalar, expr]}

def div_node(num, denom):
    return {"op": 23, "children": [num, denom]}

def pow_node(base, exp):
    return {"op": 24, "children": [base, exp]}

def clamp_node(val, lo, hi):
    return {"op": 26, "children": [val, lo, hi]}

def noise_node(vec_expr):
    return {"op": 29, "children": [vec_expr]}

def p_node():
    return {"op": 1, "var": "p"}

def time_node():
    return {"op": 1, "var": "t"}

def omega_x_node():
    return {"op": 1, "var": "omega.x"}

def omega_y_node():
    return {"op": 1, "var": "omega.y"}

def omega_z_node():
    return {"op": 1, "var": "omega.z"}

def room_weight_node(room_x):
    """
    Bell envelope centered at room_x along the X axis:
    weight = 1.0 / (1.0 + 0.12 * (x - room_x)^2)
    """
    # (x - rx)^2 = x^2 - 2*rx*x + rx^2
    c_const = 1.0 + 0.12 * (room_x ** 2)
    c_x = -0.24 * room_x
    c_x2 = 0.12
    denom_terms = [
        {"c": float(c_const), "factors": {}},
        {"c": float(c_x), "factors": {"x": 1.0}},
        {"c": float(c_x2), "factors": {"x": 2.0}}
    ]
    return div_node(scalar_node(1.0), poly_node(denom_terms))


# -----------------------------------------------------------------------------
# COLUMN 0: EXISTING COLUMN (3 Rooms at Z = 0) — PRESERVED UNTOUCHED
# -----------------------------------------------------------------------------

def make_col0_scalar_ast():
    # 3 pieces partitioned along x inside the existing row:
    # r1: x <= -30.0
    r1_terms = [
        {"c": 1.05, "factors": {}},
        {"c": 0.12, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.3, "shift": 0.0}]},
        {"c": 0.08, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.3, "shift": 12.0}]}
    ]
    r1_ast = poly_node(r1_terms)

    # r2: x in [-30, -10]
    r2_terms = [
        {"c": 221.6125, "factors": {}},
        {"c": 18.0, "factors": {"x": 1.0}},
        {"c": 8.55, "factors": {"y": 1.0}},
        {"c": 0.45, "factors": {"x": 2.0}},
        {"c": 0.45, "factors": {"y": 2.0}},
        {"c": 0.45, "factors": {"z": 2.0}}
    ]
    r2_ast = div_node(scalar_node(4.2), poly_node(r2_terms))

    # r3: x > -10
    c = vec3_node(0.0, -9.5, 0.0)
    d = dist_node(p_node(), c)
    d1 = sub_node(d, scalar_node(2.5))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(3.2), pow_node(d1, scalar_node(2.0))))
    shell1 = div_node(scalar_node(2.4), denom1)

    d2 = sub_node(d, scalar_node(5.5))
    denom2 = add_node(scalar_node(1.0), scale_node(scalar_node(2.2), pow_node(d2, scalar_node(2.0))))
    shell2 = div_node(scalar_node(2.0), denom2)
    r3_ast = add_node(add_node(shell1, shell2), scalar_node(0.15))

    # Composite using smooth weighting
    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)

    term1 = scale_node(r1_ast, w1)
    term2 = scale_node(r2_ast, w2)
    term3 = scale_node(r3_ast, w3)

    return add_node(add_node(term1, term2), term3)


# -----------------------------------------------------------------------------
# COLUMN 1: RUNG 3 COLUMN (5 Rooms at Z = 35) — Authored Spatial Scalar rho(p)
# -----------------------------------------------------------------------------

def make_col1_scalar_ast():
    # Room 3.1: Soft Gaussian-like radial pill (-40, 35)
    c1 = vec3_node(-40.0, -9.5, 35.0)
    d1_sq = pow_node(dist_node(p_node(), c1), scalar_node(2.0))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(0.08), d1_sq))
    r3_1 = div_node(scalar_node(3.0), denom1)

    # Room 3.2: Directional Dual-Gradient Wall (-20, 35)
    # rho = clamp(1.2 + 0.18*(x + 20), 0.15, 2.6)
    ramp_term = poly_node([
        {"c": 4.8, "factors": {}},
        {"c": 0.18, "factors": {"x": 1.0}}
    ])
    r3_2 = clamp_node(ramp_term, scalar_node(0.15), scalar_node(2.6))

    # Room 3.3: High-Frequency Caustic Interference Grid (0, 35)
    caustic_terms = [
        {"c": 1.3, "factors": {}},
        {"c": 0.85, "factors": {}, "trans": [
            {"kind": 1, "var": "x", "scale": 1.2, "shift": 0.0},
            {"kind": 1, "var": "z", "scale": 1.2, "shift": -42.0}
        ]},
        {"c": 0.40, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 2.4, "shift": -84.0}]}
    ]
    r3_3 = poly_node(caustic_terms)

    # Room 3.4: Bipolar Elliptical Lobes (20, 35)
    cL1 = vec3_node(16.5, -9.5, 35.0)
    cL2 = vec3_node(23.5, -9.5, 35.0)
    dL1_sq = pow_node(dist_node(p_node(), cL1), scalar_node(2.0))
    dL2_sq = pow_node(dist_node(p_node(), cL2), scalar_node(2.0))
    lobeA = div_node(scalar_node(2.6), add_node(scalar_node(1.0), scale_node(scalar_node(0.9), dL1_sq)))
    lobeB = div_node(scalar_node(2.6), add_node(scalar_node(1.0), scale_node(scalar_node(0.9), dL2_sq)))
    r3_4 = add_node(add_node(lobeA, lobeB), scalar_node(0.12))

    # Room 3.5: Concentric Harmonic Ripple Disks (40, 35)
    c5 = vec3_node(40.0, -9.5, 35.0)
    d5 = dist_node(p_node(), c5)
    denom5 = add_node(scalar_node(1.0), scale_node(scalar_node(0.04), pow_node(d5, scalar_node(2.0))))
    base5 = div_node(scalar_node(2.4), denom5)
    cos_d = poly_node([
        {"c": 1.0, "factors": {}},
        {"c": 0.55, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.5, "shift": -60.0}]}
    ])
    r3_5 = scale_node(base5, cos_d)

    # Spatial weights along X
    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    t1 = scale_node(r3_1, w1)
    t2 = scale_node(r3_2, w2)
    t3 = scale_node(r3_3, w3)
    t4 = scale_node(r3_4, w4)
    t5 = scale_node(r3_5, w5)

    return add_node(add_node(add_node(add_node(t1, t2), t3), t4), t5)


# -----------------------------------------------------------------------------
# COLUMN 2: RUNG 4 COLUMN (5 Rooms at Z = 70) — Time-Aware Radiance rho(p,t)
# -----------------------------------------------------------------------------

def make_col2_scalar_ast():
    # Room 4.1: Breathing Radiant Heart (-40, 70)
    # rho = (1.8 + 1.4*sin(1.5*t)) / (1.0 + 0.05*d^2)
    c1 = vec3_node(-40.0, -9.5, 70.0)
    d1_sq = pow_node(dist_node(p_node(), c1), scalar_node(2.0))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(0.05), d1_sq))
    t_pulse = poly_node([
        {"c": 1.8, "factors": {}},
        {"c": 1.4, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.5, "shift": 0.0}]}
    ])
    r4_1 = div_node(t_pulse, denom1)

    # Room 4.2: Expanding Traveling Ripple Wave (-20, 70)
    # rho = 1.2 + 0.9 * cos(1.5*x - 2.5*t)
    r4_2 = poly_node([
        {"c": 1.2, "factors": {}},
        {"c": 0.9, "factors": {}, "trans": [
            {"kind": 1, "var": "x", "scale": 1.5, "shift": 30.0},
            {"kind": 1, "var": "t", "scale": -2.5, "shift": 0.0}
        ]}
    ])

    # Room 4.3: Migrating Focal Beacon (0, 70)
    # Uses sine wave modulation across x:
    r4_3_terms = [
        {"c": 1.5, "factors": {}},
        {"c": 1.2, "factors": {}, "trans": [
            {"kind": 1, "var": "x", "scale": 0.6, "shift": 0.0},
            {"kind": 1, "var": "t", "scale": 1.4, "shift": 0.0}
        ]}
    ]
    r4_3 = poly_node(r4_3_terms)

    # Room 4.4: Counter-Phase Alternating Lobes (20, 70)
    # Lobe A at (16.5, 70) with weight (1 + sin(2t)), Lobe B at (23.5, 70) with weight (1 - sin(2t))
    cA = vec3_node(16.5, -9.5, 70.0)
    cB = vec3_node(23.5, -9.5, 70.0)
    dA_sq = pow_node(dist_node(p_node(), cA), scalar_node(2.0))
    dB_sq = pow_node(dist_node(p_node(), cB), scalar_node(2.0))
    lobeA_static = div_node(scalar_node(2.2), add_node(scalar_node(1.0), scale_node(scalar_node(0.8), dA_sq)))
    lobeB_static = div_node(scalar_node(2.2), add_node(scalar_node(1.0), scale_node(scalar_node(0.8), dB_sq)))
    wA = poly_node([{"c": 0.5, "factors": {}}, {"c": 0.5, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 2.0, "shift": 0.0}]}])
    wB = poly_node([{"c": 0.5, "factors": {}}, {"c": -0.5, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 2.0, "shift": 0.0}]}])
    r4_4 = add_node(add_node(scale_node(wA, lobeA_static), scale_node(wB, lobeB_static)), scalar_node(0.15))

    # Room 4.5: Evolving Cellular Turbulence (40, 70)
    # Procedural 3D noise modulated with time
    c5 = vec3_node(40.0, -9.5, 70.0)
    d5_sq = pow_node(dist_node(p_node(), c5), scalar_node(2.0))
    base5 = div_node(scalar_node(2.4), add_node(scalar_node(1.0), scale_node(scalar_node(0.03), d5_sq)))
    time_wave = poly_node([{"c": 1.0, "factors": {}}, {"c": 0.45, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.2, "shift": 0.0}]}])
    noise_part = scale_node(scalar_node(0.35), noise_node(scale_node(scalar_node(0.18), p_node())))
    r4_5 = scale_node(base5, add_node(time_wave, noise_part))

    # Spatial weights along X
    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    t1 = scale_node(r4_1, w1)
    t2 = scale_node(r4_2, w2)
    t3 = scale_node(r4_3, w3)
    t4 = scale_node(r4_4, w4)
    t5 = scale_node(r4_5, w5)

    return add_node(add_node(add_node(add_node(t1, t2), t3), t4), t5)


# -----------------------------------------------------------------------------
# COLUMN 3: RUNG 5 COLUMN (5 Rooms at Z = 105) — Authored Chroma chi(p,t)
# -----------------------------------------------------------------------------

def make_col3_scalar_ast():
    # Constant baseline radiance across Rung 5 column so chroma reads purely
    return scalar_node(1.35)

def make_col3_chroma_ast():
    # Room 5.1: Crimson & Cyan Bipolar Hemispheres (-40, 105)
    r1_r = clamp_node(poly_node([{"c": 0.5, "factors": {}}, {"c": -0.12, "factors": {"x": 1.0}}]), scalar_node(0.08), scalar_node(1.2))
    r1_g = scalar_node(0.15)
    r1_b = clamp_node(poly_node([{"c": 0.5, "factors": {}}, {"c": 0.12, "factors": {"x": 1.0}}]), scalar_node(0.08), scalar_node(1.2))
    chi5_1 = vec3_expr_node(r1_r, r1_g, r1_b)

    # Room 5.2: Spectral Rainbow Radial Rings (-20, 105)
    r2_r = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.8, "shift": 16.0}]}])
    r2_g = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.8, "shift": 13.91}]}])
    r2_b = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.8, "shift": 11.82}]}])
    chi5_2 = vec3_expr_node(r2_r, r2_g, r2_b)

    # Room 5.3: Harmonic Chromatic Tapestry Lattice (0, 105)
    r3_r = poly_node([{"c": 0.7, "factors": {}}, {"c": 0.55, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.9, "shift": 0.0}]}])
    r3_g = poly_node([{"c": 0.7, "factors": {}}, {"c": 0.55, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.9, "shift": -94.5}]}])
    r3_b = poly_node([{"c": 0.7, "factors": {}}, {"c": -0.55, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.9, "shift": 0.0}]}])
    chi5_3 = vec3_expr_node(r3_r, r3_g, r3_b)

    # Room 5.4: Traveling Chromatic Wave (20, 105)
    r4_r = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": 0.0}]}])
    r4_g = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": -2.09}]}])
    r4_b = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": -4.18}]}])
    chi5_4 = vec3_expr_node(r4_r, r4_g, r4_b)

    # Room 5.5: Celestial Aurora Borealis Plasma (40, 105)
    # Emerald green & amethyst violet plasma
    r5_r = poly_node([{"c": 0.5, "factors": {}}, {"c": 0.45, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 0.9, "shift": 0.0}]}])
    r5_g = poly_node([{"c": 0.9, "factors": {}}, {"c": 0.45, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.5, "shift": -20.0}]}])
    r5_b = poly_node([{"c": 0.8, "factors": {}}, {"c": 0.45, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.3, "shift": 1.0}]}])
    chi5_5 = vec3_expr_node(r5_r, r5_g, r5_b)

    # Spatial weights along X
    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    # Weighted sum per color channel
    r_sum = add_node(add_node(add_node(add_node(scale_node(w1, r1_r), scale_node(w2, r2_r)), scale_node(w3, r3_r)), scale_node(w4, r4_r)), scale_node(w5, r5_r))
    g_sum = add_node(add_node(add_node(add_node(scale_node(w1, r1_g), scale_node(w2, r2_g)), scale_node(w3, r3_g)), scale_node(w4, r4_g)), scale_node(w5, r5_g))
    b_sum = add_node(add_node(add_node(add_node(scale_node(w1, r1_b), scale_node(w2, r2_b)), scale_node(w3, r3_b)), scale_node(w4, r4_b)), scale_node(w5, r5_b))

    return vec3_expr_node(r_sum, g_sum, b_sum)


# -----------------------------------------------------------------------------
# COLUMN 4: RUNG 6 COLUMN (5 Rooms at Z = 140) — Authored Angular alpha(p,omega,t)
# -----------------------------------------------------------------------------

def make_col4_scalar_ast():
    # Steady baseline scalar radiance across Rung 6 column
    return scalar_node(1.35)

def make_col4_angular_ast():
    # Room 6.1: Focused Forward Spotlight Cone (-40, 140)
    # alpha = 3.5 * clamp(0.7*omega.z - 0.7*omega.y, 0.0, 1.0)^3
    aim_expr = poly_node([
        {"c": 0.7, "factors": {"omega.z": 1.0}},
        {"c": -0.7, "factors": {"omega.y": 1.0}}
    ])
    clamped_cone = clamp_node(aim_expr, scalar_node(0.0), scalar_node(1.0))
    a6_1 = scale_node(scalar_node(3.5), pow_node(clamped_cone, scalar_node(3.0)))

    # Room 6.2: Opposing Bipolar Twin Lobes (-20, 140)
    # alpha = 3.0 * omega.x^2
    a6_2 = scale_node(scalar_node(3.0), poly_node([{"c": 1.0, "factors": {"omega.x": 2.0}}]))

    # Room 6.3: Horizontal Planar Fan Sheet (0, 140)
    # alpha = 2.8 * clamp(1.0 - 8.0 * omega.y^2, 0.0, 1.0)
    fan_term = poly_node([
        {"c": 1.0, "factors": {}},
        {"c": -8.0, "factors": {"omega.y": 2.0}}
    ])
    a6_3 = scale_node(scalar_node(2.8), clamp_node(fan_term, scalar_node(0.0), scalar_node(1.0)))

    # Room 6.4: Quad-Lobed Cloverleaf Slices (20, 140)
    # alpha = 1.0 + 1.8 * (omega.x^2 - omega.z^2)^2
    clover_diff = poly_node([
        {"c": 1.0, "factors": {"omega.x": 2.0}},
        {"c": -1.0, "factors": {"omega.z": 2.0}}
    ])
    a6_4 = add_node(scalar_node(0.6), scale_node(scalar_node(2.2), pow_node(clover_diff, scalar_node(2.0))))

    # Room 6.5: Dynamic Rotating Lighthouse Beam (40, 140)
    # alpha = 3.8 * clamp(-(omega.x * sin(1.5*t) + omega.z * cos(1.5*t)), 0.0, 1.0)^3
    # Use the proven native Rung-6 rotating beam formula:
    # lobe = -(omega.x * sin(1.5*t) + omega.z * cos(1.5*t))
    rot_x = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.5, "shift": 0.0}]}]), var_node("omega.x"))
    rot_z = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.5, "shift": 0.0}]}]), var_node("omega.z"))
    rot_sum = clamp_node(add_node(rot_x, rot_z), scalar_node(0.0), scalar_node(1.0))
    a6_5 = scale_node(scalar_node(3.8), pow_node(rot_sum, scalar_node(3.0)))

    # Spatial weights along X
    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    t1 = scale_node(a6_1, w1)
    t2 = scale_node(a6_2, w2)
    t3 = scale_node(a6_3, w3)
    t4 = scale_node(a6_4, w4)
    t5 = scale_node(a6_5, w5)

    return add_node(add_node(add_node(add_node(t1, t2), t3), t4), t5)


# -----------------------------------------------------------------------------
# COLUMN 5: COMBINED COLUMN (5 Rooms at Z = 175) — rho * chi * alpha
# -----------------------------------------------------------------------------

def make_col5_scalar_ast():
    # Room C.1: Solar Lighthouse (-40, 175) — Breathing spatial beacon
    c1 = vec3_node(-40.0, -9.5, 175.0)
    d1_sq = pow_node(dist_node(p_node(), c1), scalar_node(2.0))
    breathe = poly_node([{"c": 1.6, "factors": {}}, {"c": 0.9, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.4, "shift": 0.0}]}])
    rc_1 = div_node(breathe, add_node(scalar_node(1.0), scale_node(scalar_node(0.04), d1_sq)))

    # Room C.2: Aurora Caustic Curtain (-20, 175) — Rippling caustic curtain
    rc_2 = poly_node([
        {"c": 1.2, "factors": {}},
        {"c": 0.8, "factors": {}, "trans": [
            {"kind": 1, "var": "x", "scale": 1.2, "shift": 24.0},
            {"kind": 1, "var": "t", "scale": -2.0, "shift": 0.0}
        ]}
    ])

    # Room C.3: Celestial Pulsar (0, 175) — Dual focal lobes
    cA = vec3_node(-3.0, -9.5, 175.0)
    cB = vec3_node(3.0, -9.5, 175.0)
    dA_sq = pow_node(dist_node(p_node(), cA), scalar_node(2.0))
    dB_sq = pow_node(dist_node(p_node(), cB), scalar_node(2.0))
    lA = div_node(scalar_node(2.5), add_node(scalar_node(1.0), scale_node(scalar_node(0.8), dA_sq)))
    lB = div_node(scalar_node(2.5), add_node(scalar_node(1.0), scale_node(scalar_node(0.8), dB_sq)))
    rc_3 = add_node(add_node(lA, lB), scalar_node(0.12))

    # Room C.4: Resonant Harmonic Temple (20, 175) — Nested halo shells
    c4 = vec3_node(20.0, -9.5, 175.0)
    d4 = dist_node(p_node(), c4)
    sh1 = div_node(scalar_node(2.4), add_node(scalar_node(1.0), scale_node(scalar_node(3.0), pow_node(sub_node(d4, scalar_node(2.5)), scalar_node(2.0)))))
    sh2 = div_node(scalar_node(2.0), add_node(scalar_node(1.0), scale_node(scalar_node(2.0), pow_node(sub_node(d4, scalar_node(5.5)), scalar_node(2.0)))))
    rc_4 = add_node(add_node(sh1, sh2), scalar_node(0.15))

    # Room C.5: Living Singularity Heart (40, 175) — Living cellular turbulence
    c5 = vec3_node(40.0, -9.5, 175.0)
    d5_sq = pow_node(dist_node(p_node(), c5), scalar_node(2.0))
    base5 = div_node(scalar_node(2.6), add_node(scalar_node(1.0), scale_node(scalar_node(0.03), d5_sq)))
    turb = scale_node(scalar_node(0.35), noise_node(scale_node(scalar_node(0.16), p_node())))
    rc_5 = scale_node(base5, add_node(scalar_node(1.0), turb))

    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    t1 = scale_node(rc_1, w1)
    t2 = scale_node(rc_2, w2)
    t3 = scale_node(rc_3, w3)
    t4 = scale_node(rc_4, w4)
    t5 = scale_node(rc_5, w5)

    return add_node(add_node(add_node(add_node(t1, t2), t3), t4), t5)

def make_col5_chroma_ast():
    # Room C.1: Sunset gold/crimson
    c1_r = scalar_node(1.2)
    c1_g = poly_node([{"c": 0.65, "factors": {}}, {"c": 0.45, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.2, "shift": 0.0}]}])
    c1_b = scalar_node(0.2)

    # Room C.2: Emerald & violet plasma
    c2_r = poly_node([{"c": 0.45, "factors": {}}, {"c": 0.35, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.5, "shift": 0.0}]}])
    c2_g = poly_node([{"c": 0.95, "factors": {}}, {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.5, "shift": 0.0}]}])
    c2_b = scalar_node(0.7)

    # Room C.3: Bipolar sapphire & amber
    c3_r = clamp_node(poly_node([{"c": 0.6, "factors": {}}, {"c": 0.15, "factors": {"x": 1.0}}]), scalar_node(0.1), scalar_node(1.2))
    c3_g = scalar_node(0.35)
    c3_b = clamp_node(poly_node([{"c": 0.6, "factors": {}}, {"c": -0.15, "factors": {"x": 1.0}}]), scalar_node(0.1), scalar_node(1.2))

    # Room C.4: Traveling spectral wave
    c4_r = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.5, "shift": 0.0}]}])
    c4_g = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.5, "shift": -2.09}]}])
    c4_b = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.5, "shift": -4.18}]}])

    # Room C.5: Celestial polychrome
    c5_r = poly_node([{"c": 0.7, "factors": {}}, {"c": 0.4, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 0.8, "shift": 0.0}]}])
    c5_g = poly_node([{"c": 0.8, "factors": {}}, {"c": 0.4, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.1, "shift": 1.0}]}])
    c5_b = poly_node([{"c": 0.9, "factors": {}}, {"c": 0.4, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.4, "shift": 2.0}]}])

    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    r_sum = add_node(add_node(add_node(add_node(scale_node(w1, c1_r), scale_node(w2, c2_r)), scale_node(w3, c3_r)), scale_node(w4, c4_r)), scale_node(w5, c5_r))
    g_sum = add_node(add_node(add_node(add_node(scale_node(w1, c1_g), scale_node(w2, c2_g)), scale_node(w3, c3_g)), scale_node(w4, c4_g)), scale_node(w5, c5_g))
    b_sum = add_node(add_node(add_node(add_node(scale_node(w1, c1_b), scale_node(w2, c2_b)), scale_node(w3, c3_b)), scale_node(w4, c4_b)), scale_node(w5, c5_b))

    return vec3_expr_node(r_sum, g_sum, b_sum)

def make_col5_angular_ast():
    # Room C.1: Rotating beam sweeping in azimuth
    rot_x = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.8, "shift": 0.0}]}]), var_node("omega.x"))
    rot_z = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": 0.0}]}]), var_node("omega.z"))
    ac_1 = scale_node(scalar_node(3.6), pow_node(clamp_node(add_node(rot_x, rot_z), scalar_node(0.0), scalar_node(1.0)), scalar_node(3.0)))

    # Room C.2: Downward curtain fan
    ac_2 = scale_node(scalar_node(2.8), clamp_node(poly_node([{"c": -1.0, "factors": {"omega.y": 1.0}}]), scalar_node(0.0), scalar_node(1.0)))

    # Room C.3: Twin spinning jet lobes
    # alpha = 3.2 * (omega.x * cos(2.5*t) - omega.z * sin(2.5*t))^2
    jet_x = scale_node(poly_node([{"c": 1.0, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 2.5, "shift": 0.0}]}]), var_node("omega.x"))
    jet_z = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 2.5, "shift": 0.0}]}]), var_node("omega.z"))
    ac_3 = scale_node(scalar_node(3.2), pow_node(add_node(jet_x, jet_z), scalar_node(2.0)))

    # Room C.4: Cloverleaf directional petals
    clover = poly_node([{"c": 1.0, "factors": {"omega.x": 2.0}}, {"c": -1.0, "factors": {"omega.z": 2.0}}])
    ac_4 = add_node(scalar_node(0.6), scale_node(scalar_node(2.4), pow_node(clover, scalar_node(2.0))))

    # Room C.5: Dynamic sweeping fan
    ac_5 = scale_node(scalar_node(2.6), clamp_node(poly_node([{"c": 0.8, "factors": {"omega.z": 1.0}}, {"c": -0.6, "factors": {"omega.y": 1.0}}]), scalar_node(0.0), scalar_node(1.0)))

    w1 = room_weight_node(-40.0)
    w2 = room_weight_node(-20.0)
    w3 = room_weight_node(0.0)
    w4 = room_weight_node(20.0)
    w5 = room_weight_node(40.0)

    t1 = scale_node(ac_1, w1)
    t2 = scale_node(ac_2, w2)
    t3 = scale_node(ac_3, w3)
    t4 = scale_node(ac_4, w4)
    t5 = scale_node(ac_5, w5)

    return add_node(add_node(add_node(add_node(t1, t2), t3), t4), t5)


# -----------------------------------------------------------------------------
# Complete Piecewise Containers Partitioned Along Z
# -----------------------------------------------------------------------------

def make_full_gallery_scalar_ast():
    return {
        "input": "z",
        "pieces": [
            # Piece 0: Existing Column (Z <= 17.5) — Preserved Untouched
            {"hasLo": False, "hasHi": True, "hi": 17.5, "includeHi": True, "mathNode": make_col0_scalar_ast()},
            # Piece 1: Rung 3 Column (17.5 < Z <= 52.5) — Spatial Scalar rho(p)
            {"hasLo": True, "lo": 17.5, "includeLo": False, "hasHi": True, "hi": 52.5, "includeHi": True, "mathNode": make_col1_scalar_ast()},
            # Piece 2: Rung 4 Column (52.5 < Z <= 87.5) — Time-Aware rho(p,t)
            {"hasLo": True, "lo": 52.5, "includeLo": False, "hasHi": True, "hi": 87.5, "includeHi": True, "mathNode": make_col2_scalar_ast()},
            # Piece 3: Rung 5 Column (87.5 < Z <= 122.5) — Steady scalar rho for Chroma showcase
            {"hasLo": True, "lo": 87.5, "includeLo": False, "hasHi": True, "hi": 122.5, "includeHi": True, "mathNode": make_col3_scalar_ast()},
            # Piece 4: Rung 6 Column (122.5 < Z <= 157.5) — Steady scalar rho for Angular showcase
            {"hasLo": True, "lo": 122.5, "includeLo": False, "hasHi": True, "hi": 157.5, "includeHi": True, "mathNode": make_col4_scalar_ast()},
            # Piece 5: Combined Column (Z > 157.5) — Synthesized rho(p,t)
            {"hasLo": True, "lo": 157.5, "includeLo": False, "hasHi": False, "mathNode": make_col5_scalar_ast()}
        ]
    }

def make_full_gallery_chroma_ast():
    white_vec = vec3_node(1.0, 0.98, 0.95)
    return {
        "input": "z",
        "pieces": [
            # Pieces 0, 1, 2: Neutral white for Existing, R3, R4
            {"hasLo": False, "hasHi": True, "hi": 87.5, "includeHi": True, "mathNode": white_vec},
            # Piece 3: Rung 5 Column (87.5 < Z <= 122.5) — Authored Chroma Field chi(p,t)
            {"hasLo": True, "lo": 87.5, "includeLo": False, "hasHi": True, "hi": 122.5, "includeHi": True, "mathNode": make_col3_chroma_ast()},
            # Piece 4: Neutral white for Rung 6 Column
            {"hasLo": True, "lo": 122.5, "includeLo": False, "hasHi": True, "hi": 157.5, "includeHi": True, "mathNode": white_vec},
            # Piece 5: Combined Column (Z > 157.5) — Synthesized Chroma Field chi(p,t)
            {"hasLo": True, "lo": 157.5, "includeLo": False, "hasHi": False, "mathNode": make_col5_chroma_ast()}
        ]
    }

def make_full_gallery_angular_ast():
    iso_unit = scalar_node(1.0)
    return {
        "input": "z",
        "pieces": [
            # Pieces 0, 1, 2, 3: Isotropic identity (alpha = 1) for Existing, R3, R4, R5
            {"hasLo": False, "hasHi": True, "hi": 122.5, "includeHi": True, "mathNode": iso_unit},
            # Piece 4: Rung 6 Column (122.5 < Z <= 157.5) — Authored Angular Emission alpha(p,omega,t)
            {"hasLo": True, "lo": 122.5, "includeLo": False, "hasHi": True, "hi": 157.5, "includeHi": True, "mathNode": make_col4_angular_ast()},
            # Piece 5: Combined Column (Z > 157.5) — Synthesized Angular Emission alpha(p,omega,t)
            {"hasLo": True, "lo": 157.5, "includeLo": False, "hasHi": False, "mathNode": make_col5_angular_ast()}
        ]
    }


# -----------------------------------------------------------------------------
# 3D Physical Architecture Generators
# -----------------------------------------------------------------------------

def mat4_identity_at(x, y, z):
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        float(x), float(y), float(z), 1.0
    ]

def make_sdf_sphere_object(object_id, display_name, material_id, pos, radius=0.8, role="witness"):
    x, y, z = pos
    return {
        "authoritativeAxis": [0, 1, 0],
        "center": [float(x), float(y), float(z)],
        "faceColors": [[1, 1, 1]] * 6,
        "geometryType": 10,
        "materialId": material_id,
        "objectID": object_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeKind": 10,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": mat4_identity_at(x, y, z),
        "x2D": 100, "y2D": 100, "zOrder2D": 0,
        "field": {
            "op": 0,
            "prim": 0,
            "dims": [float(radius), float(radius), float(radius)],
            "offset": [0, 0, 0],
            "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(radius + 0.2), float(radius + 0.2), float(radius + 0.2)],
        "authoredProperties": {
            "displayName": {"t": "string", "v": display_name},
            "radianceWitness.role": {"t": "string", "v": role}
        }
    }

def make_sdf_box_object(object_id, display_name, material_id, pos, half_extents, role="architecture"):
    x, y, z = pos
    hx, hy, hz = half_extents
    return {
        "authoritativeAxis": [0, 1, 0],
        "center": [float(x), float(y), float(z)],
        "faceColors": [[1, 1, 1]] * 6,
        "geometryType": 10,
        "materialId": material_id,
        "objectID": object_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeKind": 10,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": mat4_identity_at(x, y, z),
        "x2D": 100, "y2D": 100, "zOrder2D": 0,
        "field": {
            "op": 0,
            "prim": 1,
            "dims": [float(hx), float(hy), float(hz)],
            "offset": [0, 0, 0],
            "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(hx + 0.1), float(hy + 0.1), float(hz + 0.1)],
        "authoredProperties": {
            "displayName": {"t": "string", "v": display_name},
            "radianceWitness.role": {"t": "string", "v": role}
        }
    }

def make_sdf_cylinder_object(object_id, display_name, material_id, pos, radius, half_height, role="pedestal"):
    x, y, z = pos
    return {
        "authoritativeAxis": [0, 1, 0],
        "center": [float(x), float(y), float(z)],
        "faceColors": [[1, 1, 1]] * 6,
        "geometryType": 10,
        "materialId": material_id,
        "objectID": object_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeKind": 10,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": mat4_identity_at(x, y, z),
        "x2D": 100, "y2D": 100, "zOrder2D": 0,
        "field": {
            "op": 0,
            "prim": 4,
            "dims": [float(radius), float(half_height), 0.0],
            "offset": [0, 0, 0],
            "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(radius + 0.1), float(half_height + 0.1), float(radius + 0.1)],
        "authoredProperties": {
            "displayName": {"t": "string", "v": display_name},
            "radianceWitness.role": {"t": "string", "v": role}
        }
    }

def make_sdf_torus_object(object_id, display_name, material_id, pos, major_r, minor_r, role="witness_ring"):
    x, y, z = pos
    return {
        "authoritativeAxis": [0, 1, 0],
        "center": [float(x), float(y), float(z)],
        "faceColors": [[1, 1, 1]] * 6,
        "geometryType": 10,
        "materialId": material_id,
        "objectID": object_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeKind": 10,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": mat4_identity_at(x, y, z),
        "x2D": 100, "y2D": 100, "zOrder2D": 0,
        "field": {
            "op": 0,
            "prim": 6,
            "dims": [float(major_r), float(minor_r), 0.0],
            "offset": [0, 0, 0],
            "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(major_r + minor_r + 0.1), float(minor_r + 0.1), float(major_r + minor_r + 0.1)],
        "authoredProperties": {
            "displayName": {"t": "string", "v": display_name},
            "radianceWitness.role": {"t": "string", "v": role}
        }
    }


# -----------------------------------------------------------------------------
# Main Generator
# -----------------------------------------------------------------------------

def build_full_gallery():
    # 1. Load existing zone to preserve the 3 existing rooms (38 objects)
    repo_root = "/Users/zacharyzhang/Documents/GitHub/Earthcall"
    existing_zone_path = os.path.join(repo_root, "saves", "zones", "Radiance Gallery", "zone.json")
    with open(existing_zone_path) as f:
        existing_doc = json.load(f)

    # Filter to only the 3 preserved existing rooms & their portals
    preserved_objects = [
        o for o in existing_doc["world"]["objects"]
        if any(o["objectID"].startswith(p) for p in [
            "r1-", "r2-", "r3-", "gallery-promenade-floor",
            "portal-col-1-2", "portal-lintel-1-2", "portal-col-2-3", "portal-lintel-2-3"
        ])
    ]
    print(f"[Preservation] Successfully identified {len(preserved_objects)} existing objects from the 3-room column.")

    objects = list(preserved_objects)

    # 2. Build the 5 NEW columns
    # Column definitions: (ColName, Prefix, Z_coord, RoomDescriptions)
    columns_spec = [
        ("Rung 3 Column", "col1", 35.0, [
            ("Radial Pill", "r3_1"),
            ("Dual Gradient Wall", "r3_2"),
            ("Caustic Grid", "r3_3"),
            ("Bipolar Lobes", "r3_4"),
            ("Harmonic Disks", "r3_5")
        ]),
        ("Rung 4 Column", "col2", 70.0, [
            ("Breathing Heart", "r4_1"),
            ("Expanding Waves", "r4_2"),
            ("Migrating Beacon", "r4_3"),
            ("Alternating Lobes", "r4_4"),
            ("Living Turbulence", "r4_5")
        ]),
        ("Rung 5 Column", "col3", 105.0, [
            ("Bipolar Hemispheres", "r5_1"),
            ("Rainbow Radial Rings", "r5_2"),
            ("Tapestry Lattice", "r5_3"),
            ("Traveling Color Wave", "r5_4"),
            ("Aurora Borealis Plasma", "r5_5")
        ]),
        ("Rung 6 Column", "col4", 140.0, [
            ("Spotlight Cone", "r6_1"),
            ("Bipolar Twin Lobes", "r6_2"),
            ("Planar Fan Sheet", "r6_3"),
            ("Cloverleaf Slices", "r6_4"),
            ("Rotating Lighthouse", "r6_5")
        ]),
        ("Combined Column", "col5", 175.0, [
            ("Solar Lighthouse", "rc_1"),
            ("Aurora Caustic Curtain", "rc_2"),
            ("Celestial Pulsar", "rc_3"),
            ("Harmonic Temple", "rc_4"),
            ("Living Singularity Heart", "rc_5")
        ])
    ]

    room_x_coords = [-40.0, -20.0, 0.0, 20.0, 40.0]

    for col_title, prefix, z_center, room_infos in columns_spec:
        # A. Colonnade Promenade Floor for this column: X in [-52, 52], Z in [z_center - 10, z_center + 10]
        floor_id = f"{prefix}-promenade-floor"
        objects.append(make_sdf_box_object(
            floor_id, f"{col_title} Promenade Floor", "gallery.pedestal.stone",
            pos=[0.0, -0.2, z_center], half_extents=[52.0, 0.2, 10.0], role="architecture"
        ))

        # B. Dividing portal arches between the 5 rooms in this column (at X = -30, -10, 10, 30)
        for px in [-30.0, -10.0, 10.0, 30.0]:
            p_tag = f"portal-x{int(px)}"
            objects.append(make_sdf_box_object(f"{prefix}-{p_tag}-north", f"{col_title} Arch North ({px})", "gallery.arch.gold", [px, 3.0, z_center + 5.0], [0.5, 3.0, 0.5]))
            objects.append(make_sdf_box_object(f"{prefix}-{p_tag}-south", f"{col_title} Arch South ({px})", "gallery.arch.gold", [px, 3.0, z_center - 5.0], [0.5, 3.0, 0.5]))
            objects.append(make_sdf_box_object(f"{prefix}-{p_tag}-lintel", f"{col_title} Lintel ({px})", "gallery.arch.gold", [px, 5.8, z_center], [0.5, 0.3, 5.5]))

        # C. End-wall backdrops for each column (West at X = -51.5, East at X = +51.5)
        objects.append(make_sdf_box_object(f"{prefix}-backdrop-west", f"{col_title} West Wall", "gallery.marble.white", [-51.5, 2.5, z_center], [0.4, 2.5, 7.5], role="backdrop"))
        objects.append(make_sdf_box_object(f"{prefix}-backdrop-east", f"{col_title} East Wall", "gallery.marble.white", [51.5, 2.5, z_center], [0.4, 2.5, 7.5], role="backdrop"))

        # D. The 5 rooms in this column
        for r_idx, (r_name, r_code) in enumerate(room_infos):
            rx = room_x_coords[r_idx]
            room_tag = f"{prefix}-rm{r_idx + 1}"

            # Central Pedestal
            objects.append(make_sdf_cylinder_object(
                f"{room_tag}-pedestal-center", f"{col_title} Room {r_idx + 1} ({r_name}) Pedestal",
                "gallery.pedestal.stone", [rx, 0.8, z_center], radius=1.1, half_height=0.8
            ))

            # Primary Witness Sphere at room center
            objects.append(make_sdf_sphere_object(
                f"{room_tag}-witness-center", f"{col_title} Room {r_idx + 1} ({r_name}) Center Witness",
                "gallery.marble.white", [rx, 2.5, z_center], radius=0.85, role=f"{prefix}_room{r_idx+1}_center"
            ))

            # North and South flanking witness spheres (to observe spatial gradients and angular falloff)
            objects.append(make_sdf_cylinder_object(
                f"{room_tag}-pedestal-north", f"{col_title} Room {r_idx + 1} North Pedestal",
                "gallery.pedestal.stone", [rx, 0.6, z_center + 4.5], radius=0.75, half_height=0.6
            ))
            objects.append(make_sdf_sphere_object(
                f"{room_tag}-witness-north", f"{col_title} Room {r_idx + 1} North Witness",
                "gallery.marble.white", [rx, 2.0, z_center + 4.5], radius=0.75, role=f"{prefix}_room{r_idx+1}_flank_north"
            ))

            objects.append(make_sdf_cylinder_object(
                f"{room_tag}-pedestal-south", f"{col_title} Room {r_idx + 1} South Pedestal",
                "gallery.pedestal.stone", [rx, 0.6, z_center - 4.5], radius=0.75, half_height=0.6
            ))
            objects.append(make_sdf_sphere_object(
                f"{room_tag}-witness-south", f"{col_title} Room {r_idx + 1} South Witness",
                "gallery.marble.white", [rx, 2.0, z_center - 4.5], radius=0.75, role=f"{prefix}_room{r_idx+1}_flank_south"
            ))

            # For specific rooms, add iconic geometric witnesses (torus, lobes, columns):
            if "Lobe" in r_name or "Pulsar" in r_name:
                # Add dual sub-altar pedestals
                objects.append(make_sdf_cylinder_object(f"{room_tag}-lobe-east", f"{room_tag} East Altar", "gallery.pedestal.stone", [rx + 3.5, 0.7, z_center], 0.6, 0.7))
                objects.append(make_sdf_sphere_object(f"{room_tag}-witness-east", f"{room_tag} East Lobe Witness", "gallery.marble.white", [rx + 3.5, 2.0, z_center], 0.65, role="lobe_sub_east"))
                objects.append(make_sdf_cylinder_object(f"{room_tag}-lobe-west", f"{room_tag} West Altar", "gallery.pedestal.stone", [rx - 3.5, 0.7, z_center], 0.6, 0.7))
                objects.append(make_sdf_sphere_object(f"{room_tag}-witness-west", f"{room_tag} West Lobe Witness", "gallery.marble.white", [rx - 3.5, 2.0, z_center], 0.65, role="lobe_sub_west"))
            elif "Ring" in r_name or "Temple" in r_name:
                # Add concentric SDF torus
                objects.append(make_sdf_torus_object(f"{room_tag}-torus-halo", f"{room_tag} Halo Torus", "gallery.marble.white", [rx, 1.2, z_center], major_r=3.0, minor_r=0.25, role="halo_ring"))
            elif "Turbulence" in r_name or "Heart" in r_name:
                # Add rotunda colonnade
                for angle, name_suf in [(0.785, "ne"), (2.356, "nw"), (-2.356, "sw"), (-0.785, "se")]:
                    cx = rx + 3.8 * math.cos(angle)
                    cz = z_center + 3.8 * math.sin(angle)
                    objects.append(make_sdf_cylinder_object(f"{room_tag}-rotunda-{name_suf}", f"{room_tag} Rotunda Col {name_suf}", "gallery.marble.white", [cx, 2.2, cz], 0.4, 2.2, role="colonnade"))

    print(f"[Assembly] Total physical objects generated for 28-room gallery: {len(objects)}")

    # 3. Materials
    materials = [
        {
            "name": "gallery.marble.white",
            "baseColor": [0.95, 0.95, 0.96],
            "opacity": 1.0,
            "shininess": 32.0,
            "specular": 0.95,
            "ambient": 0.20,
            "diffuse": 0.85
        },
        {
            "name": "gallery.pedestal.stone",
            "baseColor": [0.72, 0.74, 0.78],
            "opacity": 1.0,
            "shininess": 16.0,
            "specular": 0.50,
            "ambient": 0.25,
            "diffuse": 0.80
        },
        {
            "name": "gallery.arch.gold",
            "baseColor": [0.92, 0.84, 0.62],
            "opacity": 1.0,
            "shininess": 48.0,
            "specular": 1.00,
            "ambient": 0.20,
            "diffuse": 0.90
        }
    ]

    # 4. Authored Radiance Fields (Rungs 3, 4, 5, 6)
    ast_scalar = make_full_gallery_scalar_ast()
    ast_chroma = make_full_gallery_chroma_ast()
    ast_angular = make_full_gallery_angular_ast()

    zone_doc = {
        "identifier": "Radiance Gallery",
        "name": "Radiance Gallery",
        "owner": "Zach",
        "parentZone": "",
        "scope": "Local",
        "qualities": {
            "ownerKind": "person"
        },
        "deletable": {
            "Zach": True
        },
        "spatialRoot": {
            "id": "radiance-gallery.light-field",
            "origin": [0.0, 12.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "authoredProperties": {
                "displayName": {"t": "string", "v": "Radiance Gallery 28-Room Light Field"},
                "light.source": {"t": "bool", "v": True},
                "light.enabled": {"t": "bool", "v": True},
                "light.color": {"t": "vec3", "x": 1.0, "y": 0.98, "z": 0.95},
                "light.intensity": {"t": "float", "v": 1.0},
                "light.ambient": {"t": "float", "v": 0.20},
                "light.diffuse": {"t": "float", "v": 0.85},
                "light.specular": {"t": "float", "v": 1.0},
                "light.attenuation.constant": {"t": "float", "v": 1.0},
                "light.attenuation.linear": {"t": "float", "v": 0.0},
                "light.attenuation.quadratic": {"t": "float", "v": 0.0}
            },
            "field": {
                "mode": "AST",
                "baseDensity": 1.0,
                "frequency": 1.0,
                "amplitude": 1.0,
                "astDefinition": ast_scalar
            },
            "lightChroma": ast_chroma,
            "lightAngular": ast_angular,
            "vectorField": {
                "mode": "Procedural",
                "baseFlowX": 0.0,
                "baseFlowY": 0.0,
                "baseFlowZ": 0.0,
                "frequency": 1.0,
                "amplitude": 1.0
            }
        },
        "materials": materials,
        "world": {
            "objects": objects,
            "laws": [],
            "relations": []
        },
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Zach"],
        "injected_by": "Gemini Spark & Blep Dragon — Expanded OntoMath Radiance 28-Room Demonstration (Rungs 3-6 & Combined)"
    }

    world_doc = {
        "identifier": "radiance_gallery",
        "name": "radiance_gallery",
        "owner": "Zach",
        "description": "Expanded OntoMath Radiance Demonstration — 28 rooms across 6 columns (Preserved 3-room column, Rung 3, Rung 4, Rung 5, Rung 6, and Combined)",
        "spatialRoot": zone_doc["spatialRoot"],
        "materials": zone_doc["materials"],
        "world": zone_doc["world"],
        "authoredLaws": {
            "laws": [],
            "triggers": {},
            "formationMembers": [],
            "rete": {"alphaNodes": [], "betaNodes": [], "facts": [], "agenda": []}
        },
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Zach"],
        "injected_by": "Gemini Spark & Blep Dragon — Expanded OntoMath Radiance Demonstration"
    }

    return zone_doc, world_doc

def main():
    repo_root = "/Users/zacharyzhang/Documents/GitHub/Earthcall"
    zone_doc, world_doc = build_full_gallery()

    # 1. Write to saves/zones/Radiance Gallery/zone.json
    zone_dir = os.path.join(repo_root, "saves", "zones", "Radiance Gallery")
    os.makedirs(zone_dir, exist_ok=True)
    zone_file = os.path.join(zone_dir, "zone.json")
    with open(zone_file, "w") as f:
        json.dump(zone_doc, f, indent=2)
        f.write("\n")
    print(f"[Radiance Gallery] Wrote Zone to: {zone_file}")

    # 2. Write to saves/worlds/radiance_gallery.json
    worlds_dir = os.path.join(repo_root, "saves", "worlds")
    os.makedirs(worlds_dir, exist_ok=True)
    world_file = os.path.join(worlds_dir, "radiance_gallery.json")
    with open(world_file, "w") as f:
        json.dump(world_doc, f, indent=2)
        f.write("\n")
    print(f"[Radiance Gallery] Wrote World to: {world_file}")

    num_objects = len(zone_doc["world"]["objects"])
    scalar_pieces = len(zone_doc["spatialRoot"]["field"]["astDefinition"]["pieces"])
    chroma_pieces = len(zone_doc["spatialRoot"]["lightChroma"]["pieces"])
    angular_pieces = len(zone_doc["spatialRoot"]["lightAngular"]["pieces"])

    print(f"[Radiance Gallery] Complete: 28 rooms assembled with {num_objects} objects.")
    print(f"  Scalar rho(p,t) pieces:   {scalar_pieces}")
    print(f"  Chroma chi(p,t) pieces:   {chroma_pieces}")
    print(f"  Angular alpha(p,w,t) pcs: {angular_pieces}")

if __name__ == "__main__":
    main()
