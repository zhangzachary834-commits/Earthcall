#!/usr/bin/env python3
"""
Prism Cathedral — Comprehensive Showcase of Authored Light & Volumetrics in Earthcall
=====================================================================================
Pass 2: Complex-Shaped Mathematical Light Fields & Volumetric Media

Zone: saves/zones/Prism Cathedral/zone.json
Authors: Zachary Zhang, GPT-5.6 Sol

Key Additions:
  - Visually diverse collection of non-box, non-radial mathematical shapes.
  - Luminous Radiance Fields rho(p,t):
      * Spherical Halos
      * Hollow Luminous Shells
      * Toroidal / Ring Fields
      * Multi-Lobed Clover / Flower Fields
      * Noisy Asymmetric Organic Fields
      * Time-varying animated shape variations (breathing radius, pulsing rings, crawling noise, rotating lobes)
  - Source Chroma chi(p,t):
      * North/South Bipolar Chroma
      * Toroidal Azimuthal Chroma Gradient
      * Concentric Shells with Distinct Chromatic Identities
  - Angular Emission alpha(p,omega,t):
      * Radially outward emission from toroidal ring
      * Shell with directional polar openings
      * Rotating directional lighthouse beam on asymmetric lobe
  - Multiple Coexisting Independent Shapes (Rung 7):
      * Source A: Spherical Luminous Shell
      * Source B: Toroidal Ring Field
      * Source C: Noisy Organic Lobe
      * Source D: Narrow Directional Filament / Vertical Pillar Beam
  - Visibility Interactions (Rung 8):
      * Architectural occluders interrupting complex shells
      * Source-selective occluders on filament vs toroidal source
  - Volumetric V0 Participating Media (D(p,t)):
      * B1: Soft Spherical Cloud
      * B2: Hollow Nebular Shell / Bubble Membrane
      * B3: Toroidal / Ring Medium (Donut fog ring)
      * B4: CSG-Like Complex Medium (Cavity-subtracted crescent & disconnected twin islands)
      * B5: Noise-Warped Organic Cloud (Wispy puff with bounded envelope)
      * B6: Time-Varying Breathing Hollow Nebula
      * B7: PROXY-PROOF DIAGNOSTIC: Huge Rectangular Proxy (20x8x20m) containing tiny slender fog ring!
      * B8: Dual Sovereign Being (rho_source != D_medium) + Opaque Depth Truncation Pillar
  - The Summit (THE PRISM):
      * Simultaneous coexistence of multiple shaped suns (shell, torus, noisy pulsar)
      * Encircling Toroidal Participating Medium ring around the Colossal Crystal Prism
      * Celestial Atmospheric Veil
      * Moving geometric occluders and authored surface color fields

Constitution:
  rho_source != V_transport != D_medium
  E_i = rho_i(p,t) * chi_i(p,t) * alpha_i(p,omega,t)
  L_direct = sum_i [ E_i * V_i(source,p,omega) ]
  D_medium(p,t) belongs to participating medium
  surface appearance = sdfColor(p)
"""

import json
import os
import sys
import shutil

# -----------------------------------------------------------------------------
# OntoMath AST Construction Helpers (Adhering strictly to ScalarForm.hpp Op enums)
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

def p_node():
    return {"op": 1, "var": "p"}

def x_node():
    return {"op": 1, "var": "x"}

def y_node():
    return {"op": 1, "var": "y"}

def z_node():
    return {"op": 1, "var": "z"}

def time_node():
    return {"op": 1, "var": "t"}

def omega_x_node():
    return {"op": 1, "var": "omega.x"}

def omega_y_node():
    return {"op": 1, "var": "omega.y"}

def omega_z_node():
    return {"op": 1, "var": "omega.z"}

def add_node(a, b):
    return {"op": 4, "children": [a, b]}

def sub_node(a, b):
    return {"op": 5, "children": [a, b]}

def scale_node(scalar, expr):
    return {"op": 6, "children": [scalar, expr]}

def dot_node(a, b):
    return {"op": 7, "children": [a, b]}

def length_node(v):
    return {"op": 11, "children": [v]}

def dist_node(pt_node, center_vec3):
    return {"op": 15, "children": [pt_node, center_vec3]}

def union_node(a, b):
    return {"op": 20, "children": [a, b]}  # min(a, b) in OntoMath

def intersection_node(a, b):
    return {"op": 21, "children": [a, b]}  # max(a, b) in OntoMath

def difference_node(a, b):
    return {"op": 22, "children": [a, b]}  # max(a, -b) in OntoMath

def div_node(num, denom):
    return {"op": 23, "children": [num, denom]}

def pow_node(base, exp):
    return {"op": 24, "children": [base, exp]}

def abs_node(scalar_expr):
    return {"op": 25, "children": [scalar_expr]}

def clamp_node(val, lo, hi):
    return {"op": 26, "children": [val, lo, hi]}

def sqrt_node(scalar_expr):
    return {"op": 27, "children": [scalar_expr]}

def noise_node(vec_expr):
    return {"op": 29, "children": [vec_expr]}

def room_weight_node(room_x, width_factor=0.12):
    """Bell envelope centered at room_x along the X axis."""
    c_const = 1.0 + width_factor * (room_x ** 2)
    c_x = -2.0 * width_factor * room_x
    c_x2 = width_factor
    denom_terms = [
        {"c": float(c_const), "factors": {}},
        {"c": float(c_x), "factors": {"x": 1.0}},
        {"c": float(c_x2), "factors": {"x": 2.0}}
    ]
    return div_node(scalar_node(1.0), poly_node(denom_terms))

# -----------------------------------------------------------------------------
# Specialized Shape Functions (Deriving Density and Radiance from Spatial Math)
# -----------------------------------------------------------------------------

def make_torus_distance_node(major_r, minor_r):
    """
    Computes distance to a torus tube in the XZ plane:
      pxz = sqrt(x^2 + z^2)
      u = pxz - major_r
      dtube = sqrt(u^2 + y^2) - minor_r
    """
    x_sq = pow_node(x_node(), scalar_node(2.0))
    z_sq = pow_node(z_node(), scalar_node(2.0))
    pxz = sqrt_node(add_node(x_sq, z_sq))
    u = sub_node(pxz, scalar_node(major_r))
    u_sq = pow_node(u, scalar_node(2.0))
    y_sq = pow_node(y_node(), scalar_node(2.0))
    dtube = sub_node(sqrt_node(add_node(u_sq, y_sq)), scalar_node(minor_r))
    return dtube

def make_torus_density_node(major_r, minor_r, max_density):
    """
    Volumetric density inside a torus tube:
      d = distance to tube center circle = sqrt((sqrt(x^2+z^2) - R)^2 + y^2)
      D = clamp(1.0 - d / r, 0.0, 1.0) * max_density
    Density is strictly zero in the donut hole and outside the tube.
    """
    x_sq = pow_node(x_node(), scalar_node(2.0))
    z_sq = pow_node(z_node(), scalar_node(2.0))
    pxz = sqrt_node(add_node(x_sq, z_sq))
    u = sub_node(pxz, scalar_node(major_r))
    u_sq = pow_node(u, scalar_node(2.0))
    y_sq = pow_node(y_node(), scalar_node(2.0))
    dist_tube = sqrt_node(add_node(u_sq, y_sq))
    normalized_falloff = sub_node(scalar_node(1.0), div_node(dist_tube, scalar_node(minor_r)))
    clamped_falloff = clamp_node(normalized_falloff, scalar_node(0.0), scalar_node(1.0))
    return scale_node(scalar_node(max_density), clamped_falloff)

def make_hollow_shell_density_node(center_r, thickness, max_density):
    """
    Density concentrated in a hollow spherical shell:
      u = abs(length(p) - center_r)
      D = clamp(1.0 - u / thickness, 0.0, 1.0) * max_density
    Zero density in the core and outside the shell.
    """
    d = length_node(p_node())
    u = abs_node(sub_node(d, scalar_node(center_r)))
    falloff = sub_node(scalar_node(1.0), div_node(u, scalar_node(thickness)))
    return scale_node(scalar_node(max_density), clamp_node(falloff, scalar_node(0.0), scalar_node(1.0)))

def make_noise_warped_cloud_density_node(base_r, noise_amp, noise_freq, max_density):
    """
    Noise-warped organic cloud density:
      r_eff = length(p) - noise_amp * cnoise3(noise_freq * p)
      D = clamp(1.0 - r_eff / base_r, 0.0, 1.0) * max_density
    Strictly bounded outside (base_r + noise_amp).
    """
    d = length_node(p_node())
    n = noise_node(scale_node(scalar_node(noise_freq), p_node()))
    r_eff = sub_node(d, scale_node(scalar_node(noise_amp), n))
    falloff = sub_node(scalar_node(1.0), div_node(r_eff, scalar_node(base_r)))
    return scale_node(scalar_node(max_density), clamp_node(falloff, scalar_node(0.0), scalar_node(1.0)))

# -----------------------------------------------------------------------------
# Spatial Radiance Expressions for Stations Along the Great Nave
# -----------------------------------------------------------------------------

# --- Station 1: FOUNDATION 1 (Z in [25, 55]) ---
def make_station1_scalar_ast():
    c = vec3_node(0.0, 0.0, 40.0)
    d_sq = pow_node(dist_node(p_node(), c), scalar_node(2.0))
    denom = add_node(scalar_node(1.0), scale_node(scalar_node(0.04), d_sq))
    return div_node(scalar_node(2.2), denom)

# --- Station 2: FOUNDATION 2 / PHASE 2 (Z in [55, 90]) ---
def make_station2_scalar_ast():
    # 5 exhibits spaced along X: -10, -5, 0, 5, 10
    c1 = vec3_node(-10.0, 0.0, 75.0)
    d1_sq = pow_node(dist_node(p_node(), c1), scalar_node(2.0))
    e2_1 = div_node(scalar_node(2.5), add_node(scalar_node(1.0), scale_node(scalar_node(0.08), d1_sq)))

    ramp_term = poly_node([{"c": 1.4, "factors": {}}, {"c": 0.20, "factors": {"x": 1.0}}])
    e2_2 = clamp_node(ramp_term, scalar_node(0.2), scalar_node(2.8))

    c3 = vec3_node(0.0, 0.0, 75.0)
    d3 = dist_node(p_node(), c3)
    base3 = div_node(scalar_node(2.4), add_node(scalar_node(1.0), scale_node(scalar_node(0.05), pow_node(d3, scalar_node(2.0)))))
    ripple3 = poly_node([{"c": 1.0, "factors": {}}, {"c": 0.6, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.8, "shift": 0.0}]}])
    e2_3 = scale_node(base3, ripple3)

    cL1 = vec3_node(3.5, 0.0, 75.0)
    cL2 = vec3_node(6.5, 0.0, 75.0)
    lobeA = div_node(scalar_node(2.2), add_node(scalar_node(1.0), scale_node(scalar_node(0.9), pow_node(dist_node(p_node(), cL1), scalar_node(2.0)))))
    lobeB = div_node(scalar_node(2.2), add_node(scalar_node(1.0), scale_node(scalar_node(0.9), pow_node(dist_node(p_node(), cL2), scalar_node(2.0)))))
    e2_4 = add_node(lobeA, lobeB)

    c5 = vec3_node(10.0, 0.0, 75.0)
    d5 = dist_node(p_node(), c5)
    sh1 = div_node(scalar_node(2.4), add_node(scalar_node(1.0), scale_node(scalar_node(3.0), pow_node(sub_node(d5, scalar_node(2.0)), scalar_node(2.0)))))
    sh2 = div_node(scalar_node(1.8), add_node(scalar_node(1.0), scale_node(scalar_node(2.0), pow_node(sub_node(d5, scalar_node(4.5)), scalar_node(2.0)))))
    e2_5 = add_node(sh1, sh2)

    w1 = room_weight_node(-10.0, 0.3)
    w2 = room_weight_node(-5.0, 0.3)
    w3 = room_weight_node(0.0, 0.3)
    w4 = room_weight_node(5.0, 0.3)
    w5 = room_weight_node(10.0, 0.3)

    return add_node(add_node(add_node(add_node(scale_node(w1, e2_1), scale_node(w2, e2_2)), scale_node(w3, e2_3)), scale_node(w4, e2_4)), scale_node(w5, e2_5))

# --- Station 3: RUNG 3 (Z in [90, 125]) — COMPLEX SPATIAL RADIANCE SHAPES ---
def make_station3_scalar_ast():
    # West: Hollow Luminous Shell (X = -8, Z = 110)
    c_shell = vec3_node(-8.0, 0.0, 110.0)
    d_sh = dist_node(p_node(), c_shell)
    u_sh = abs_node(sub_node(d_sh, scalar_node(3.0)))
    rho_shell = scale_node(scalar_node(2.8), clamp_node(sub_node(scalar_node(1.0), div_node(u_sh, scalar_node(0.9))), scalar_node(0.0), scalar_node(1.0)))

    # Center: Near/Far Truthful Contrast Field (X = 0, Z = 110)
    c_ctr = vec3_node(0.0, 0.0, 110.0)
    d_ctr_sq = pow_node(dist_node(p_node(), c_ctr), scalar_node(2.0))
    rho_near_far = div_node(scalar_node(2.8), add_node(scalar_node(1.0), scale_node(scalar_node(0.12), d_ctr_sq)))

    # East: Toroidal Ring Radiance Field (X = 8, Z = 110)
    # Local coords relative to (8, 0, 110)
    x_rel = sub_node(x_node(), scalar_node(8.0))
    z_rel = sub_node(z_node(), scalar_node(110.0))
    pxz_ring = sqrt_node(add_node(pow_node(x_rel, scalar_node(2.0)), pow_node(z_rel, scalar_node(2.0))))
    u_ring = sub_node(pxz_ring, scalar_node(2.5))
    dtube_ring = sqrt_node(add_node(pow_node(u_ring, scalar_node(2.0)), pow_node(y_node(), scalar_node(2.0))))
    rho_ring = scale_node(scalar_node(3.0), clamp_node(sub_node(scalar_node(1.0), div_node(dtube_ring, scalar_node(0.8))), scalar_node(0.0), scalar_node(1.0)))

    w_west = room_weight_node(-8.0, 0.2)
    w_ctr = room_weight_node(0.0, 0.2)
    w_east = room_weight_node(8.0, 0.2)

    return add_node(add_node(scale_node(w_west, rho_shell), scale_node(w_ctr, rho_near_far)), scale_node(w_east, rho_ring))

# --- Station 4: RUNG 4 (Z in [125, 160]) — ANIMATED SPATIAL SHAPES ---
def make_station4_scalar_ast():
    # West: Expanding/Contracting Breathing Shell (X = -7, Z = 145)
    # Radius breathes: R(t) = 2.8 + 1.2 * sin(1.6 * t)
    c_w = vec3_node(-7.0, 0.0, 145.0)
    d_w = dist_node(p_node(), c_w)
    r_t = poly_node([{"c": 2.8, "factors": {}}, {"c": 1.2, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.6, "shift": 0.0}]}])
    u_sh_t = abs_node(sub_node(d_w, r_t))
    rho_breath_sh = scale_node(scalar_node(2.8), clamp_node(sub_node(scalar_node(1.0), div_node(u_sh_t, scalar_node(0.8))), scalar_node(0.0), scalar_node(1.0)))

    # Center: Breathing Luminous Heart with traveling wave
    c_ctr = vec3_node(0.0, 0.0, 145.0)
    d_ctr_sq = pow_node(dist_node(p_node(), c_ctr), scalar_node(2.0))
    breathe = poly_node([{"c": 1.8, "factors": {}}, {"c": 1.2, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.6, "shift": 0.0}]}])
    wave = poly_node([{"c": 0.6, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.2, "shift": 0.0}, {"kind": 1, "var": "t", "scale": -2.0, "shift": 0.0}]}])
    rho_heart = div_node(add_node(breathe, wave), add_node(scalar_node(1.0), scale_node(scalar_node(0.04), d_ctr_sq)))

    # East: Rotating Quadrant Lobes in (x, z)
    # Lobes rotate with time: cos(2*x - 1.5*t) * sin(2*z - 1.5*t)
    c_e = vec3_node(7.0, 0.0, 145.0)
    d_e_sq = pow_node(dist_node(p_node(), c_e), scalar_node(2.0))
    rot_lobes = poly_node([{"c": 1.0, "factors": {}}, {"c": 0.8, "factors": {}, "trans": [
        {"kind": 1, "var": "x", "scale": 1.5, "shift": 0.0},
        {"kind": 0, "var": "t", "scale": -1.5, "shift": 0.0}
    ]}])
    rho_rot = scale_node(div_node(scalar_node(2.4), add_node(scalar_node(1.0), scale_node(scalar_node(0.08), d_e_sq))), clamp_node(rot_lobes, scalar_node(0.1), scalar_node(2.0)))

    w1 = room_weight_node(-7.0, 0.2)
    w2 = room_weight_node(0.0, 0.2)
    w3 = room_weight_node(7.0, 0.2)

    return add_node(add_node(scale_node(w1, rho_breath_sh), scale_node(w2, rho_heart)), scale_node(w3, rho_rot))

# --- Station 5: RUNG 5 (Z in [160, 195]) — CHROMA FOLLOWING SPATIAL STRUCTURE ---
def make_station5_scalar_ast():
    return scalar_node(1.6)

def make_station5_chroma_ast():
    # West (X = -8): North/South Bipolar Chroma on a Spherical Field
    # y > 0: Warm Radiant Gold, y < 0: Cyan Azure
    gold_r = scalar_node(1.35)
    gold_g = scalar_node(0.85)
    gold_b = scalar_node(0.20)
    cyan_r = scalar_node(0.15)
    cyan_g = scalar_node(0.70)
    cyan_b = scalar_node(1.40)
    # Blend factor along Y: clamp(0.5 + 0.35 * y, 0.0, 1.0)
    y_blend = clamp_node(add_node(scalar_node(0.5), scale_node(scalar_node(0.35), y_node())), scalar_node(0.0), scalar_node(1.0))
    inv_y_blend = sub_node(scalar_node(1.0), y_blend)
    chi1_r = add_node(scale_node(y_blend, gold_r), scale_node(inv_y_blend, cyan_r))
    chi1_g = add_node(scale_node(y_blend, gold_g), scale_node(inv_y_blend, cyan_g))
    chi1_b = add_node(scale_node(y_blend, gold_b), scale_node(inv_y_blend, cyan_b))

    # Center (X = 0): Traveling Chromatic Wave
    chi2_r = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": 0.0}]}])
    chi2_g = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": -2.09}]}])
    chi2_b = poly_node([{"c": 0.75, "factors": {}}, {"c": 0.65, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.8, "shift": -4.18}]}])

    # East (X = 8): Concentric Shells with Distinct Chromas
    # Core (dist < 2.0): Emerald, Outer Shell (dist >= 2.0): Amethyst Violet
    c_east = vec3_node(8.0, 0.0, 180.0)
    d_east = dist_node(p_node(), c_east)
    shell_blend = clamp_node(scale_node(scalar_node(0.8), sub_node(d_east, scalar_node(2.0))), scalar_node(0.0), scalar_node(1.0))
    inv_shell_blend = sub_node(scalar_node(1.0), shell_blend)
    # Emerald: (0.2, 1.3, 0.4), Amethyst: (1.3, 0.2, 1.2)
    chi3_r = add_node(scale_node(inv_shell_blend, scalar_node(0.2)), scale_node(shell_blend, scalar_node(1.3)))
    chi3_g = add_node(scale_node(inv_shell_blend, scalar_node(1.3)), scale_node(shell_blend, scalar_node(0.2)))
    chi3_b = add_node(scale_node(inv_shell_blend, scalar_node(0.4)), scale_node(shell_blend, scalar_node(1.2)))

    w1 = room_weight_node(-8.0, 0.15)
    w2 = room_weight_node(0.0, 0.15)
    w3 = room_weight_node(8.0, 0.15)

    r_sum = add_node(add_node(scale_node(w1, chi1_r), scale_node(w2, chi2_r)), scale_node(w3, chi3_r))
    g_sum = add_node(add_node(scale_node(w1, chi1_g), scale_node(w2, chi2_g)), scale_node(w3, chi3_g))
    b_sum = add_node(add_node(scale_node(w1, chi1_b), scale_node(w2, chi2_b)), scale_node(w3, chi3_b))
    return vec3_expr_node(r_sum, g_sum, b_sum)

# --- Station 6: RUNG 6 (Z in [195, 230]) — ANGULAR EMISSION + COMPLEX SPATIAL FORM ---
def make_station6_scalar_ast():
    # Source strength is a Toroidal Ring Field
    # pxz = sqrt(x^2 + (z-215)^2), u = pxz - 3.0, dtube = sqrt(u^2 + y^2)
    z_rel = sub_node(z_node(), scalar_node(215.0))
    pxz = sqrt_node(add_node(pow_node(x_node(), scalar_node(2.0)), pow_node(z_rel, scalar_node(2.0))))
    u = sub_node(pxz, scalar_node(3.0))
    dtube = sqrt_node(add_node(pow_node(u, scalar_node(2.0)), pow_node(y_node(), scalar_node(2.0))))
    return scale_node(scalar_node(2.8), clamp_node(sub_node(scalar_node(1.0), div_node(dtube, scalar_node(1.0))), scalar_node(0.0), scalar_node(1.0)))

def make_station6_angular_ast():
    # West: Directed Spotlight Cone (aimed toward negative X)
    cone_aim = poly_node([{"c": -0.85, "factors": {"omega.x": 1.0}}, {"c": 0.35, "factors": {"omega.z": 1.0}}])
    a_cone = scale_node(scalar_node(3.6), pow_node(clamp_node(cone_aim, scalar_node(0.0), scalar_node(1.0)), scalar_node(3.0)))

    # East: Dynamic Rotating Lighthouse Beam in (omega.x, omega.z) with time t
    rot_x = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.5, "shift": 0.0}]}]), var_node("omega.x"))
    rot_z = scale_node(poly_node([{"c": -1.0, "factors": {}, "trans": [{"kind": 1, "var": "t", "scale": 1.5, "shift": 0.0}]}]), var_node("omega.z"))
    a_rot = scale_node(scalar_node(3.8), pow_node(clamp_node(add_node(rot_x, rot_z), scalar_node(0.0), scalar_node(1.0)), scalar_node(3.0)))

    # Center: Radial Outward Emission from the Torus Ring
    # alpha peaks when omega is horizontal (omega.y ~ 0)
    fan_term = poly_node([{"c": 1.0, "factors": {}}, {"c": -8.0, "factors": {"omega.y": 2.0}}])
    a_fan = scale_node(scalar_node(2.8), clamp_node(fan_term, scalar_node(0.0), scalar_node(1.0)))

    w1 = room_weight_node(-7.0, 0.15)
    w2 = room_weight_node(0.0, 0.15)
    w3 = room_weight_node(7.0, 0.15)

    return add_node(add_node(scale_node(w1, a_cone), scale_node(w2, a_fan)), scale_node(w3, a_rot))

# --- Station 7: RUNG 7 (Z in [230, 265]) — MULTIPLE INDEPENDENT SOURCE FORMS ---
def make_station7_source1_scalar_ast():
    # Source A (Sun Golden Hearth): Spherical Luminous Shell
    # rho = max(0, 2.8 * (1.0 - (length(p) - 2.8)^2 / 0.8^2))
    c = vec3_node(0.0, 0.0, 250.0)
    d = dist_node(p_node(), c)
    u = abs_node(sub_node(d, scalar_node(2.8)))
    return scale_node(scalar_node(2.8), clamp_node(sub_node(scalar_node(1.0), div_node(u, scalar_node(0.8))), scalar_node(0.0), scalar_node(1.0)))

# --- Station 8: RUNG 8 (Z in [265, 305]) — VISIBILITY AGAINST COMPLEX SOURCES ---
def make_station8_red_scalar_ast():
    # Red source: Narrow Directional Filament / Vertical Column
    c = vec3_node(-6.0, 0.0, 285.0)
    d = dist_node(p_node(), c)
    return div_node(scalar_node(2.6), add_node(scalar_node(1.0), scale_node(scalar_node(0.08), pow_node(d, scalar_node(2.0)))))

# --- The Summit: THE PRISM (Z in [305, 400]) — COMPLETE SYNTHESIS ---
def make_the_prism_sol1_scalar_ast():
    # Sol Primus: Elevated Golden Sun with concentric breathing halo shells
    c = vec3_node(0.0, 0.0, 355.0)
    d = dist_node(p_node(), c)
    breathe = poly_node([{"c": 2.0, "factors": {}}, {"c": 0.8, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.2, "shift": 0.0}]}])
    sh1 = div_node(breathe, add_node(scalar_node(1.0), scale_node(scalar_node(0.03), pow_node(d, scalar_node(2.0)))))
    sh2 = scale_node(scalar_node(2.2), clamp_node(sub_node(scalar_node(1.0), div_node(abs_node(sub_node(d, scalar_node(4.0))), scalar_node(1.0))), scalar_node(0.0), scalar_node(1.0)))
    return add_node(sh1, sh2)

def make_the_prism_sol1_chroma_ast():
    c_r = scalar_node(1.35)
    c_g = poly_node([{"c": 0.85, "factors": {}}, {"c": 0.35, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.0, "shift": 0.0}]}])
    c_b = scalar_node(0.30)
    return vec3_expr_node(c_r, c_g, c_b)

def make_the_prism_sol1_angular_ast():
    aim = poly_node([{"c": 0.7, "factors": {"omega.z": 1.0}}, {"c": -0.5, "factors": {"omega.y": 1.0}}])
    return add_node(scalar_node(0.6), scale_node(scalar_node(2.8), pow_node(clamp_node(aim, scalar_node(0.0), scalar_node(1.0)), scalar_node(2.0))))

# -----------------------------------------------------------------------------
# Assembly of Complete Piecewise Containers on spatialRoot
# -----------------------------------------------------------------------------

def make_cathedral_spatial_root_scalar_ast():
    return {
        "input": "z",
        "pieces": [
            {"hasLo": False, "hasHi": True, "hi": 25.0, "includeHi": True, "mathNode": scalar_node(1.0)},
            {"hasLo": True, "lo": 25.0, "includeLo": False, "hasHi": True, "hi": 55.0, "includeHi": True, "mathNode": make_station1_scalar_ast()},
            {"hasLo": True, "lo": 55.0, "includeLo": False, "hasHi": True, "hi": 90.0, "includeHi": True, "mathNode": make_station2_scalar_ast()},
            {"hasLo": True, "lo": 90.0, "includeLo": False, "hasHi": True, "hi": 125.0, "includeHi": True, "mathNode": make_station3_scalar_ast()},
            {"hasLo": True, "lo": 125.0, "includeLo": False, "hasHi": True, "hi": 160.0, "includeHi": True, "mathNode": make_station4_scalar_ast()},
            {"hasLo": True, "lo": 160.0, "includeLo": False, "hasHi": True, "hi": 195.0, "includeHi": True, "mathNode": make_station5_scalar_ast()},
            {"hasLo": True, "lo": 195.0, "includeLo": False, "hasHi": True, "hi": 230.0, "includeHi": True, "mathNode": make_station6_scalar_ast()},
            {"hasLo": True, "lo": 230.0, "includeLo": False, "hasHi": True, "hi": 265.0, "includeHi": True, "mathNode": make_station7_source1_scalar_ast()},
            {"hasLo": True, "lo": 265.0, "includeLo": False, "hasHi": True, "hi": 305.0, "includeHi": True, "mathNode": make_station8_red_scalar_ast()},
            {"hasLo": True, "lo": 305.0, "includeLo": False, "hasHi": False, "mathNode": make_the_prism_sol1_scalar_ast()}
        ]
    }

def make_cathedral_spatial_root_chroma_ast():
    white_vec = vec3_node(1.0, 0.98, 0.95)
    golden_vec = vec3_node(1.30, 0.85, 0.30)
    red_vec = vec3_node(1.40, 0.05, 0.05)
    return {
        "input": "z",
        "pieces": [
            {"hasLo": False, "hasHi": True, "hi": 160.0, "includeHi": True, "mathNode": white_vec},
            {"hasLo": True, "lo": 160.0, "includeLo": False, "hasHi": True, "hi": 195.0, "includeHi": True, "mathNode": make_station5_chroma_ast()},
            {"hasLo": True, "lo": 195.0, "includeLo": False, "hasHi": True, "hi": 230.0, "includeHi": True, "mathNode": white_vec},
            {"hasLo": True, "lo": 230.0, "includeLo": False, "hasHi": True, "hi": 265.0, "includeHi": True, "mathNode": golden_vec},
            {"hasLo": True, "lo": 265.0, "includeLo": False, "hasHi": True, "hi": 305.0, "includeHi": True, "mathNode": red_vec},
            {"hasLo": True, "lo": 305.0, "includeLo": False, "hasHi": False, "mathNode": make_the_prism_sol1_chroma_ast()}
        ]
    }

def make_cathedral_spatial_root_angular_ast():
    iso_unit = scalar_node(1.0)
    return {
        "input": "z",
        "pieces": [
            {"hasLo": False, "hasHi": True, "hi": 195.0, "includeHi": True, "mathNode": iso_unit},
            {"hasLo": True, "lo": 195.0, "includeLo": False, "hasHi": True, "hi": 230.0, "includeHi": True, "mathNode": make_station6_angular_ast()},
            {"hasLo": True, "lo": 230.0, "includeLo": False, "hasHi": True, "hi": 305.0, "includeHi": True, "mathNode": iso_unit},
            {"hasLo": True, "lo": 305.0, "includeLo": False, "hasHi": False, "mathNode": make_the_prism_sol1_angular_ast()}
        ]
    }

# -----------------------------------------------------------------------------
# Additional Spatial Fields (Rung 7, Rung 8, Wing B, The Prism)
# -----------------------------------------------------------------------------

def make_additional_spatial_fields():
    fields = []

    # =========================================================================
    # RUNG 7: FOUR INDEPENDENT COEXISTING SOURCES OF DRAMATICALLY DIFFERENT SHAPE
    # =========================================================================

    # 1. Source B (Station 7 Sapphire Lantern): Toroidal / Ring Radiance Field
    # Major R = 2.5, tube r = 0.8
    s2_rho = {
        "input": "x",
        "pieces": [{
            "hasLo": False, "hasHi": False,
            "mathNode": make_torus_density_node(2.5, 0.8, 2.8)
        }]
    }
    s2_chi = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(0.10, 0.55, 1.45)}]
    }
    s2_alpha = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": scale_node(scalar_node(3.2), pow_node(clamp_node(poly_node([{"c": 0.8, "factors": {"omega.x": 1.0}}, {"c": -0.5, "factors": {"omega.y": 1.0}}]), scalar_node(0.0), scalar_node(1.0)), scalar_node(2.0)))}]
    }
    fields.append({
        "id": "prism.station7.sapphire-lantern",
        "origin": [-8.0, 4.5, 250.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": s2_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": s2_chi,
        "lightAngular": s2_alpha,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Station 7 Source B: Sapphire Toroidal Ring Source"},
            "shape.kind": {"t": "string", "v": "Toroidal Ring Field (R=2.5, r=0.8)"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 0.10, "y": 0.55, "z": 1.45},
            "light.intensity": {"t": "float", "v": 1.8},
            "light.ambient": {"t": "float", "v": 0.15},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0}
        }
    })

    # 2. Source C (Station 7 Amethyst Pulsar): Noisy Organic Lobed Source
    # Uses Op::Noise modulating radial falloff with time t
    d_s3 = length_node(p_node())
    n_s3 = noise_node(scale_node(scalar_node(1.2), p_node()))
    r_eff_s3 = sub_node(d_s3, scale_node(scalar_node(0.7), n_s3))
    s3_breathe = poly_node([{"c": 2.4, "factors": {}}, {"c": 1.2, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 2.0, "shift": 0.0}]}])
    s3_rho_node = scale_node(s3_breathe, clamp_node(sub_node(scalar_node(1.0), div_node(r_eff_s3, scalar_node(2.4))), scalar_node(0.0), scalar_node(1.0)))
    s3_rho = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": s3_rho_node}]
    }
    s3_chi = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(1.30, 0.20, 1.25)}]
    }
    fields.append({
        "id": "prism.station7.amethyst-pulsar",
        "origin": [8.0, 4.5, 250.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": s3_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": s3_chi,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Station 7 Source C: Amethyst Organic Noise-Lobed Pulsar"},
            "shape.kind": {"t": "string", "v": "Noise-Warped Organic Lobe Field"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 1.30, "y": 0.20, "z": 1.25},
            "light.intensity": {"t": "float", "v": 1.8},
            "light.ambient": {"t": "float", "v": 0.15},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0}
        }
    })

    # 3. Source D (Station 7 Emerald Filament): Narrow Directional Vertical Filament
    # Elongated column field in local coords
    r_col = sqrt_node(add_node(pow_node(x_node(), scalar_node(2.0)), pow_node(z_node(), scalar_node(2.0))))
    col_radial = clamp_node(sub_node(scalar_node(1.0), div_node(r_col, scalar_node(0.6))), scalar_node(0.0), scalar_node(1.0))
    col_vert = clamp_node(sub_node(scalar_node(1.0), div_node(abs_node(y_node()), scalar_node(3.5))), scalar_node(0.0), scalar_node(1.0))
    s4_rho = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": scale_node(scalar_node(3.5), scale_node(col_radial, col_vert))}]
    }
    s4_chi = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(0.18, 1.35, 0.35)}]
    }
    fields.append({
        "id": "prism.station7.emerald-filament",
        "origin": [0.0, 7.0, 245.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": s4_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": s4_chi,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Station 7 Source D: Emerald Narrow Filament Source"},
            "shape.kind": {"t": "string", "v": "Narrow Vertical Pillar Filament (r=0.6, h=3.5)"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 0.18, "y": 1.35, "z": 0.35},
            "light.intensity": {"t": "float", "v": 2.0},
            "light.ambient": {"t": "float", "v": 0.15},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0}
        }
    })

    # =========================================================================
    # RUNG 8: VISIBILITY SOURCES
    # =========================================================================

    # 4. Rung 8 Source B: Sapphire Toroidal Visibility Source
    # Unblocked sapphire ring contrasting with occluded red source
    s8_rho = {
        "input": "x",
        "pieces": [{
            "hasLo": False, "hasHi": False,
            "mathNode": make_torus_density_node(2.4, 0.7, 2.8)
        }]
    }
    s8_chi = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(0.08, 0.30, 1.45)}]
    }
    fields.append({
        "id": "prism.station8.blue-source",
        "origin": [6.0, 4.5, 285.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": s8_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": s8_chi,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Station 8 Sapphire Toroidal Visibility Source"},
            "shape.kind": {"t": "string", "v": "Toroidal Ring Source (R=2.4, r=0.7)"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 0.08, "y": 0.30, "z": 1.45},
            "light.intensity": {"t": "float", "v": 1.8},
            "light.ambient": {"t": "float", "v": 0.15},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0}
        }
    })

    # =========================================================================
    # PARALLEL WING B: VOLUMETRIC V0 DENSITY SOVEREIGNTY (7 DISTINCT SHAPED MEDIA)
    # =========================================================================

    # 5. Exhibit B1: Soft Spherical Cloud
    # Soft round medium with density concentrated inside radius 2.2
    # Proxy extent: [4.0, 4.0, 4.0] (AABB is 8x8x8m; outside r=2.2 density is 0)
    d_b1 = length_node(p_node())
    rho_b1 = scale_node(scalar_node(1.6), clamp_node(sub_node(scalar_node(1.0), div_node(d_b1, scalar_node(2.2))), scalar_node(0.0), scalar_node(1.0)))
    fields.append({
        "id": "prism.wingB.spherical-cloud",
        "origin": [45.0, 4.0, 285.0],
        "scale": [4.0, 4.0, 4.0],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b1}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B1: Soft Spherical Cloud Medium"},
            "shape.kind": {"t": "string", "v": "Spherical Cloud (R=2.2, zero outside)"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: [-4, +4]^3 (8x8x8m)"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 6. Exhibit B2: Hollow Shell Nebula / Atmospheric Bubble Membrane
    # Core radius 2.6, thickness 0.6. Hollow inside and empty outside!
    # Proxy extent: [4.5, 4.5, 4.5] (AABB is 9x9x9m)
    rho_b2 = make_hollow_shell_density_node(2.6, 0.6, 1.8)
    fields.append({
        "id": "prism.wingB.hollow-shell",
        "origin": [55.0, 4.0, 285.0],
        "scale": [4.5, 4.5, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b2}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B2: Hollow Shell Atmospheric Membrane"},
            "shape.kind": {"t": "string", "v": "Hollow Spherical Shell (R=2.6, thickness=0.6)"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: [-4.5, +4.5]^3 (9x9x9m)"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 7. Exhibit B3: Toroidal / Ring Participating Medium (Donut fog ring)
    # Major R = 2.4, minor tube r = 0.6.
    # Empty center hole, empty exterior!
    # Proxy extent: [4.5, 3.0, 4.5]
    rho_b3 = make_torus_density_node(2.4, 0.6, 1.8)
    fields.append({
        "id": "prism.wingB.torus-medium",
        "origin": [65.0, 4.0, 285.0],
        "scale": [4.5, 3.0, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b3}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B3: Toroidal Donut Fog Ring Medium"},
            "shape.kind": {"t": "string", "v": "Toroidal Ring Medium (R=2.4, r=0.6)"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: 9x6x9m"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 8. Exhibit B4: CSG-Like Complex Medium (Carved Crescent with Subtracted Cavity)
    # Base sphere R1 = 2.4; Carved hole sphere at (1.2, 0, 0) R2 = 1.4
    d_base = length_node(p_node())
    d1_csg = clamp_node(sub_node(scalar_node(1.0), div_node(d_base, scalar_node(2.4))), scalar_node(0.0), scalar_node(1.0))
    d_hole = dist_node(p_node(), vec3_node(1.2, 0.0, 0.0))
    h_cut = clamp_node(div_node(sub_node(d_hole, scalar_node(1.3)), scalar_node(0.4)), scalar_node(0.0), scalar_node(1.0))
    rho_b4 = scale_node(scalar_node(1.8), scale_node(d1_csg, h_cut))
    fields.append({
        "id": "prism.wingB.csg-crescent-medium",
        "origin": [75.0, 4.0, 285.0],
        "scale": [4.5, 4.0, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b4}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B4: CSG-Subtracted Crescent Cloud"},
            "shape.kind": {"t": "string", "v": "CSG Subtracted Cavity (Base R=2.4 minus Hole R=1.3)"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: 9x8x9m"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 9. Exhibit B5: Noise-Warped Organic Cloud (Wispy Nebular Puff)
    # Base radius 2.2, noise amplitude 0.7, noise freq 1.2
    rho_b5 = make_noise_warped_cloud_density_node(2.2, 0.7, 1.2, 1.7)
    fields.append({
        "id": "prism.wingB.noise-organic-cloud",
        "origin": [85.0, 4.0, 285.0],
        "scale": [4.5, 4.0, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b5}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B5: Noise-Warped Organic Cloud Medium"},
            "shape.kind": {"t": "string", "v": "Noise-Warped Sphere (R=2.2, noise amp=0.7)"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: 9x8x9m"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 10. Exhibit B6: Time-Varying Breathing Hollow Nebula (D(p,t))
    # Shell radius expands/contracts in time: R(t) = 2.4 + 0.8 * sin(1.8*t)
    # Shell half-thickness = 0.5.
    d_b6 = length_node(p_node())
    r_t_b6 = poly_node([{"c": 2.4, "factors": {}}, {"c": 0.8, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.8, "shift": 0.0}]}])
    u_b6 = abs_node(sub_node(d_b6, r_t_b6))
    rho_b6 = scale_node(scalar_node(1.8), clamp_node(sub_node(scalar_node(1.0), div_node(u_b6, scalar_node(0.5))), scalar_node(0.0), scalar_node(1.0)))
    fields.append({
        "id": "prism.wingB.time-breathing-nebula",
        "origin": [95.0, 4.0, 285.0],
        "scale": [4.5, 4.0, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b6}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B6: Time-Varying Breathing Hollow Nebula"},
            "shape.kind": {"t": "string", "v": "Dynamic Breathing Shell (R(t) = 2.4 + 0.8*sin(1.8t))"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: 9x8x9m"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 11. Exhibit B7: DIAGNOSTIC PROXY-PROOF EXHIBIT — Huge Rectangular Proxy (20x8x20m) Containing Tiny Fog Ring!
    # Conservative scale is [10.0, 4.0, 10.0] -> 20m x 8m x 20m AABB!
    # Inside it is a compact torus with R = 1.8, r = 0.4.
    # Over 95% of the AABB is COMPLETELY ZERO DENSITY!
    rho_b7 = make_torus_density_node(1.8, 0.4, 2.2)
    fields.append({
        "id": "prism.wingB.proxy-proof-torus",
        "origin": [65.0, 4.0, 268.0],
        "scale": [10.0, 4.0, 10.0],  # HUGE PROXY BOX: 20m x 8m x 20m!
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": rho_b7}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B7: Diagnostic Proxy-Proof Torus Medium"},
            "shape.kind": {"t": "string", "v": "Slender Donut Ring (R=1.8, r=0.4)"},
            "proxy.extent": {"t": "string", "v": "Colossal AABB Proxy: [-10, +10]x[-4, +4]x[-10, +10] (20x8x20m)"},
            "proof.statement": {"t": "string", "v": "Over 95% of the AABB volume has D(p)=0; only the slender ring is visible!"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 12. Exhibit B8: Dual Sovereign Being (rho_source != D_medium)
    dual_rho = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": div_node(scalar_node(2.2), add_node(scalar_node(1.0), scale_node(scalar_node(0.3), pow_node(length_node(p_node()), scalar_node(2.0)))))}]
    }
    dual_d = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": make_hollow_shell_density_node(1.8, 0.5, 1.5)}]
    }
    fields.append({
        "id": "prism.wingB.dual-sovereign-being",
        "origin": [65.0, 3.5, 302.0],
        "scale": [4.0, 4.0, 4.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": dual_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": dual_d,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Wing B Exhibit B8: Dual Radiant & Volumetric Sovereign Being"},
            "shape.kind": {"t": "string", "v": "Inverse-Square Light Core inside Hollow Shell Fog Membrane"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 1.25, "y": 0.80, "z": 0.25},
            "light.intensity": {"t": "float", "v": 1.7},
            "light.ambient": {"t": "float", "v": 0.2},
            "light.diffuse": {"t": "float", "v": 0.8}
        }
    })

    # =========================================================================
    # THE SUMMIT: THE PRISM (SOL SECUNDUS, SOL TERTIUS, TOROIDAL MEDIUM, VEIL)
    # =========================================================================

    # 13. The Summit: Sol Secundus (Sapphire Toroidal Sanctuary Beam)
    prism_s2_rho = {
        "input": "x",
        "pieces": [{
            "hasLo": False, "hasHi": False,
            "mathNode": make_torus_density_node(3.0, 1.0, 3.0)
        }]
    }
    prism_s2_chi = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(0.18, 0.65, 1.45)}]
    }
    prism_s2_alpha = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": scale_node(scalar_node(3.6), pow_node(clamp_node(poly_node([{"c": -0.7, "factors": {"omega.y": 1.0}}, {"c": 0.5, "factors": {"omega.z": 1.0}}]), scalar_node(0.0), scalar_node(1.0)), scalar_node(2.0)))}]
    }
    fields.append({
        "id": "prism.summit.sol-secundus",
        "origin": [-12.0, 15.0, 360.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": prism_s2_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": prism_s2_chi,
        "lightAngular": prism_s2_alpha,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "The Prism — Sol Secundus (Sapphire Toroidal Ring Beam)"},
            "shape.kind": {"t": "string", "v": "Toroidal Ring Light Source (R=3.0, r=1.0)"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 0.18, "y": 0.65, "z": 1.45},
            "light.intensity": {"t": "float", "v": 2.2},
            "light.ambient": {"t": "float", "v": 0.15},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0}
        }
    })

    # 14. The Summit: Sol Tertius (Amethyst Pulsar Beam — Noisy Organic Lobe)
    d_sol3 = length_node(p_node())
    n_sol3 = noise_node(scale_node(scalar_node(1.0), p_node()))
    r_sol3 = sub_node(d_sol3, scale_node(scalar_node(0.8), n_sol3))
    prism_s3_breathe = poly_node([{"c": 2.2, "factors": {}}, {"c": 1.1, "factors": {}, "trans": [{"kind": 0, "var": "t", "scale": 1.4, "shift": 0.0}]}])
    prism_s3_rho_node = scale_node(prism_s3_breathe, clamp_node(sub_node(scalar_node(1.0), div_node(r_sol3, scalar_node(2.8))), scalar_node(0.0), scalar_node(1.0)))
    prism_s3_rho = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": prism_s3_rho_node}]
    }
    prism_s3_chi = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(1.35, 0.25, 1.25)}]
    }
    prism_s3_alpha = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": scale_node(scalar_node(3.6), pow_node(clamp_node(poly_node([{"c": -0.7, "factors": {"omega.y": 1.0}}, {"c": -0.5, "factors": {"omega.x": 1.0}}]), scalar_node(0.0), scalar_node(1.0)), scalar_node(2.0)))}]
    }
    fields.append({
        "id": "prism.summit.sol-tertius",
        "origin": [12.0, 15.0, 360.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": prism_s3_rho
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": prism_s3_chi,
        "lightAngular": prism_s3_alpha,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "The Prism — Sol Tertius (Amethyst Pulsar Beam)"},
            "shape.kind": {"t": "string", "v": "Noisy Organic Lobed Pulsar (R=2.8)"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 1.35, "y": 0.25, "z": 1.25},
            "light.intensity": {"t": "float", "v": 2.2},
            "light.ambient": {"t": "float", "v": 0.15},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0}
        }
    })

    # 15. The Summit: Sovereign Toroidal Participating Medium Encircling The Prism
    # Floats around the central crystal prism at (0, 4.5, 355), Major R = 5.0, tube r = 1.0
    # Proxy extent: [7.0, 2.5, 7.0]
    prism_torus_rho = make_torus_density_node(5.0, 1.0, 1.6)
    fields.append({
        "id": "prism.summit.torus-medium",
        "origin": [0.0, 5.0, 355.0],
        "scale": [7.0, 2.5, 7.0],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": prism_torus_rho}]},
        "authoredProperties": {
            "displayName": {"t": "string", "v": "The Prism — Sanctuary Toroidal Donut Fog Ring"},
            "shape.kind": {"t": "string", "v": "Toroidal Participating Medium Ring (R=5.0, r=1.0)"},
            "proxy.extent": {"t": "string", "v": "Conservative AABB: 14x5x14m"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    # 16. The Summit: Celestial Participating Veil (Rotunda Atmosphere)
    summit_density = {
        "input": "x",
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": scale_node(scalar_node(0.20), div_node(scalar_node(1.0), add_node(scalar_node(1.0), scale_node(scalar_node(0.02), pow_node(length_node(p_node()), scalar_node(2.0))))))}]
    }
    fields.append({
        "id": "prism.summit.celestial-veil",
        "origin": [0.0, 8.0, 355.0],
        "scale": [18.0, 10.0, 22.0],
        "field": {"mode": "Procedural", "baseDensity": 1, "frequency": 1, "amplitude": 1, "ast": {}},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "volumeDensity": summit_density,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "The Prism — Celestial Participating Atmosphere"},
            "shape.kind": {"t": "string", "v": "Rotunda Soft Nebular Atmosphere"},
            "light.source": {"t": "bool", "v": False}
        }
    })

    return fields

# -----------------------------------------------------------------------------
# Materials Construction (Including Authored Surface Color Fields for Wing A)
# -----------------------------------------------------------------------------

def make_cathedral_materials():
    mats = [
        {
            "id": "material.cathedral_basalt",
            "name": "Cathedral Basalt Floor",
            "baseColor": [0.12, 0.12, 0.14],
            "ambient": 0.25, "diffuse": 0.75, "specular": 0.6,
            "roughness": 0.25, "metalness": 0.0
        },
        {
            "id": "material.cathedral_limestone",
            "name": "Cathedral Limestone Pillar",
            "baseColor": [0.82, 0.80, 0.76],
            "ambient": 0.20, "diffuse": 0.80, "specular": 0.2,
            "roughness": 0.7, "metalness": 0.0
        },
        {
            "id": "material.cathedral_alabaster",
            "name": "Cathedral Alabaster Vault",
            "baseColor": [0.92, 0.90, 0.88],
            "ambient": 0.25, "diffuse": 0.85, "specular": 0.4,
            "roughness": 0.4, "metalness": 0.0
        },
        {
            "id": "material.cathedral_gold",
            "name": "Cathedral Celestial Gold Accent",
            "baseColor": [1.0, 0.82, 0.32],
            "ambient": 0.30, "diffuse": 0.70, "specular": 1.0,
            "roughness": 0.2, "metalness": 0.8
        },
        {
            "id": "material.cathedral_bronze",
            "name": "Cathedral Bronze Plinth",
            "baseColor": [0.55, 0.38, 0.24],
            "ambient": 0.20, "diffuse": 0.80, "specular": 0.7,
            "roughness": 0.35, "metalness": 0.5
        },
        {
            "id": "material.cathedral_stele_brass",
            "name": "Cathedral Inscription Stele Brass",
            "baseColor": [0.72, 0.58, 0.28],
            "ambient": 0.25, "diffuse": 0.75, "specular": 0.8,
            "roughness": 0.3, "metalness": 0.7
        },
        {
            "id": "material.cathedral_witness_white",
            "name": "Cathedral Pure Witness White",
            "baseColor": [1.0, 1.0, 1.0],
            "ambient": 0.10, "diffuse": 0.90, "specular": 0.4,
            "roughness": 0.3, "metalness": 0.0
        },
        {
            "id": "material.cathedral_blocker_obsidian",
            "name": "Cathedral Shadow Blocker Obsidian",
            "baseColor": [0.03, 0.03, 0.04],
            "ambient": 0.05, "diffuse": 0.30, "specular": 0.9,
            "roughness": 0.1, "metalness": 0.1
        },
        {
            "id": "material.cathedral_prism_crystal",
            "name": "Cathedral Summit Crystal Prism",
            "baseColor": [0.95, 0.98, 1.0],
            "ambient": 0.30, "diffuse": 0.70, "specular": 1.2,
            "roughness": 0.05, "metalness": 0.1
        }
    ]

    # Wing A Materials: Independently Authored SDF Surface Color Fields
    rainbow_y_r = poly_node([{"c": 0.5, "factors": {}}, {"c": 0.5, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.5, "shift": 0.0}]}])
    rainbow_y_g = poly_node([{"c": 0.5, "factors": {}}, {"c": 0.5, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.5, "shift": 2.09}]}])
    rainbow_y_b = poly_node([{"c": 0.5, "factors": {}}, {"c": 0.5, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.5, "shift": 4.18}]}])
    mats.append({
        "id": "material.prism_surface_rainbow",
        "name": "Wing A Stratified Rainbow Surface",
        "baseColor": [1.0, 1.0, 1.0],
        "ambient": 0.2, "diffuse": 0.8, "specular": 0.4,
        "roughness": 0.3, "metalness": 0.0,
        "colorExpr": {
            "input": "y",
            "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_expr_node(rainbow_y_r, rainbow_y_g, rainbow_y_b)}]
        }
    })

    rad_r = poly_node([{"c": 0.65, "factors": {}}, {"c": 0.55, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]}])
    rad_g = poly_node([{"c": 0.65, "factors": {}}, {"c": 0.55, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 2.0, "shift": 0.0}]}])
    rad_b = poly_node([{"c": 0.70, "factors": {}}, {"c": -0.50, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]}])
    mats.append({
        "id": "material.prism_surface_radial",
        "name": "Wing A Radial Gradient Surface",
        "baseColor": [1.0, 1.0, 1.0],
        "ambient": 0.2, "diffuse": 0.8, "specular": 0.4,
        "roughness": 0.3, "metalness": 0.0,
        "colorExpr": {
            "input": "x",
            "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_expr_node(rad_r, rad_g, rad_b)}]
        }
    })

    lat_r = poly_node([{"c": 0.7, "factors": {}}, {"c": 0.5, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.5, "shift": 0.0}, {"kind": 1, "var": "z", "scale": 1.5, "shift": 0.0}]}])
    lat_g = poly_node([{"c": 0.5, "factors": {}}, {"c": 0.4, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 1.5, "shift": 0.0}]}])
    lat_b = poly_node([{"c": 0.8, "factors": {}}, {"c": -0.5, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 1.5, "shift": 0.0}]}])
    mats.append({
        "id": "material.prism_surface_lattice",
        "name": "Wing A Nodal Lattice Surface",
        "baseColor": [1.0, 1.0, 1.0],
        "ambient": 0.2, "diffuse": 0.8, "specular": 0.4,
        "roughness": 0.3, "metalness": 0.0,
        "colorExpr": {
            "input": "x",
            "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_expr_node(lat_r, lat_g, lat_b)}]
        }
    })

    mats.append({
        "id": "material.prism_surface_pure_blue",
        "name": "Wing A Pure Authored Blue Surface",
        "baseColor": [0.08, 0.18, 0.98],
        "ambient": 0.15, "diffuse": 0.85, "specular": 0.5,
        "roughness": 0.25, "metalness": 0.0,
        "colorExpr": {
            "input": "x",
            "pieces": [{"hasLo": False, "hasHi": False, "mathNode": vec3_node(0.08, 0.18, 0.98)}]
        }
    })

    return mats

# -----------------------------------------------------------------------------
# Object Generators
# -----------------------------------------------------------------------------

def mat4_identity_at(x, y, z):
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        float(x), float(y), float(z), 1.0
    ]

def make_sdf_sphere(object_id, display_name, material_id, pos, radius=0.8, role="witness", extra_props=None):
    x, y, z = pos
    props = {
        "displayName": {"t": "string", "v": display_name},
        "cathedral.role": {"t": "string", "v": role}
    }
    if extra_props:
        props.update(extra_props)
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
            "op": 0, "prim": 0,
            "dims": [float(radius), float(radius), float(radius)],
            "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(radius + 0.2), float(radius + 0.2), float(radius + 0.2)],
        "authoredProperties": props
    }

def make_sdf_box(object_id, display_name, material_id, pos, half_extents, role="architecture", extra_props=None):
    x, y, z = pos
    hx, hy, hz = half_extents
    props = {
        "displayName": {"t": "string", "v": display_name},
        "cathedral.role": {"t": "string", "v": role}
    }
    if extra_props:
        props.update(extra_props)
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
            "op": 0, "prim": 1,
            "dims": [float(hx), float(hy), float(hz)],
            "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(hx + 0.15), float(hy + 0.15), float(hz + 0.15)],
        "authoredProperties": props
    }

def make_sdf_cylinder(object_id, display_name, material_id, pos, radius, half_height, role="column", extra_props=None):
    x, y, z = pos
    props = {
        "displayName": {"t": "string", "v": display_name},
        "cathedral.role": {"t": "string", "v": role}
    }
    if extra_props:
        props.update(extra_props)
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
            "op": 0, "prim": 3,
            "dims": [float(radius), float(half_height), float(radius)],
            "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(radius + 0.15), float(half_height + 0.15), float(radius + 0.15)],
        "authoredProperties": props
    }

def make_sdf_torus(object_id, display_name, material_id, pos, major_r, minor_r, role="ring", extra_props=None):
    x, y, z = pos
    props = {
        "displayName": {"t": "string", "v": display_name},
        "cathedral.role": {"t": "string", "v": role}
    }
    if extra_props:
        props.update(extra_props)
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
            "op": 0, "prim": 2,
            "dims": [float(major_r), float(minor_r), 0.0],
            "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": 0.5
        },
        "fieldExtent": [float(major_r + minor_r + 0.15), float(minor_r + 0.15), float(major_r + minor_r + 0.15)],
        "authoredProperties": props
    }

def make_inscription_stele(object_id, title, stage, equation, doctrine, invariant, pos):
    x, y, z = pos
    extra_props = {
        "inscription.stage": {"t": "string", "v": stage},
        "inscription.equation": {"t": "string", "v": equation},
        "inscription.doctrine": {"t": "string", "v": doctrine},
        "inscription.invariant": {"t": "string", "v": invariant}
    }
    return make_sdf_box(object_id, f"STELE: {title}", "material.cathedral_stele_brass",
                        [x, y + 1.2, z], [0.8, 1.2, 0.15], role="pedagogy_stele", extra_props=extra_props)

# -----------------------------------------------------------------------------
# World Geometry Builder for the Entire Cathedral
# -----------------------------------------------------------------------------

def build_cathedral_world():
    objects = []

    # 1. THE GREAT NAVE FLOOR (Basalt pavement running from Z = -20 to Z = 390)
    for z_segment in range(-15, 385, 20):
        objects.append(make_sdf_box(
            f"cathedral.floor.seg_{z_segment}",
            f"Nave Floor Pavement Z={z_segment}",
            "material.cathedral_basalt",
            [0.0, -0.6, float(z_segment)],
            [14.0, 0.5, 10.0],
            role="nave_floor"
        ))

    # 2. COLONNADES & ARCHITECTURAL VAULTS FLANKING THE NAVE
    for z_pillar in range(0, 380, 17):
        objects.append(make_sdf_cylinder(
            f"cathedral.pillar.west_{z_pillar}",
            f"Limestone Colonnade Pillar West Z={z_pillar}",
            "material.cathedral_limestone",
            [-13.0, 7.0, float(z_pillar)],
            0.9, 7.0, role="colonnade_pillar"
        ))
        objects.append(make_sdf_cylinder(
            f"cathedral.pillar.east_{z_pillar}",
            f"Limestone Colonnade Pillar East Z={z_pillar}",
            "material.cathedral_limestone",
            [13.0, 7.0, float(z_pillar)],
            0.9, 7.0, role="colonnade_pillar"
        ))
        objects.append(make_sdf_torus(
            f"cathedral.arch_{z_pillar}",
            f"Cathedral Vault Arch Z={z_pillar}",
            "material.cathedral_alabaster",
            [0.0, 14.5, float(z_pillar)],
            13.0, 0.45, role="vault_arch"
        ))

    # -------------------------------------------------------------------------
    # ENTRANCE: THE ATRIUM OF INVARIANTS (Z = 5)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.entrance.stele",
        "PRISM CATHEDRAL — THE ARCHITECTURE OF AUTHORED LIGHT",
        "ENTRANCE / THE ATRIUM OF INVARIANTS",
        "rho_source != V_transport != D_medium",
        "Light has ceased to be an anonymous shader constant. Walk through the physical ascension of authored light.",
        "Source = rho*chi*alpha; Transport = *V; Medium = D; Surface = sdfColor.",
        [0.0, 0.0, 8.0]
    ))
    objects.append(make_sdf_box("cathedral.entrance.obelisk_w", "Portal Obelisk West", "material.cathedral_gold", [-6.0, 3.5, 5.0], [0.5, 3.5, 0.5], role="portal"))
    objects.append(make_sdf_box("cathedral.entrance.obelisk_e", "Portal Obelisk East", "material.cathedral_gold", [6.0, 3.5, 5.0], [0.5, 3.5, 0.5], role="portal"))

    # -------------------------------------------------------------------------
    # STATION 1: FOUNDATION 1 — FIRST-ORDER AUTHORABLE LIGHT (Z = 40)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station1.stele",
        "FOUNDATION 1 — FIRST-ORDER AUTHORABLE LIGHT (2026-09-11)",
        "FOUNDATION 1",
        "AuthorableLightState { position, color, intensity, ambient, diffuse, specular, attenuation }",
        "Light entered the authored world as a first-class Singular. Stored world properties govern the renderer.",
        "Properties are readable/writable by Law and persisted. No anonymous C++ shader constants.",
        [0.0, 0.0, 34.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.station1.altar", "Altar of First Light", "material.cathedral_bronze", [0.0, 0.6, 40.0], 1.8, 0.6, role="altar"))
    objects.append(make_sdf_sphere("cathedral.station1.witness", "First-Order Light Witness Sphere", "material.cathedral_witness_white", [0.0, 2.1, 40.0], radius=0.9, role="witness"))

    # -------------------------------------------------------------------------
    # STATION 2: FOUNDATION 2 / PHASE 2 — SPATIAL RADIANCE rho(p) (Z = 75)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station2.stele",
        "FOUNDATION 2 / PHASE 2 — AUTHORED SPATIAL RADIANCE rho(p) (2026-09-19)",
        "FOUNDATION 2 / PHASE 2",
        "rho(p) -> scalar",
        "rho answers: 'How strong is this source at this point in space?'",
        "rho is scalar source strength. It is NOT color, NOT shadow, NOT material, NOT density.",
        [0.0, 0.0, 69.0]
    ))
    exhibits_st2 = [
        (-10.0, "Inverse-Distance Falloff Pedestal", "Inverse-Distance Pill Witness", "1 / (1 + 0.08*d^2)"),
        (-5.0,  "Linear Gradient Pedestal", "Linear Gradient Witness", "clamp(1.4 + 0.2*x, 0.2, 2.8)"),
        (0.0,   "Harmonic Ripple Pedestal", "Concentric Harmonic Ripples Witness", "cos(1.8*dist) / (1 + 0.05*d^2)"),
        (5.0,   "Bipolar Elliptical Lobes Pedestal", "Bipolar Lobes Witness", "Lobe_A(p) + Lobe_B(p)"),
        (10.0,  "Nested Halo Shells Pedestal", "Nested Halo Shells Witness", "Shell_1(d-2) + Shell_2(d-4.5)")
    ]
    for x_pos, ped_name, wit_name, math_desc in exhibits_st2:
        objects.append(make_sdf_cylinder(f"cathedral.station2.ped_{x_pos}", ped_name, "material.cathedral_bronze", [x_pos, 0.6, 75.0], 1.1, 0.6, role="pedestal"))
        objects.append(make_sdf_sphere(f"cathedral.station2.wit_{x_pos}", wit_name, "material.cathedral_witness_white", [x_pos, 2.0, 75.0], radius=0.8, role="witness",
                                       extra_props={"math.form": {"t": "string", "v": math_desc}}))

    # -------------------------------------------------------------------------
    # STATION 3: RUNG 3 — TRUTHFUL LIVE VISUAL CONSEQUENCE & COMPLEX SHAPES (Z = 110)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station3.stele",
        "RUNG 3 — TRUTHFUL LIVE VISUAL CONSEQUENCE (2026-09-20)",
        "RUNG 3",
        "lightRadiance(p) = evaluate(Piecewise, p)",
        "Source strength inhabits complex mathematical shapes. Hollow shells, toroidal rings, and halos.",
        "A hollow rho(p) shell means the source emits in this region; NOT that fog exists here.",
        [0.0, 0.0, 104.0]
    ))
    # West: Hollow Shell Radiance Witness (X = -8, Z = 110)
    objects.append(make_sdf_cylinder("cathedral.station3.ped_shell", "Hollow Shell Radiance Pedestal", "material.cathedral_bronze", [-8.0, 0.6, 110.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station3.wit_shell", "Hollow Luminous Shell Witness", "material.cathedral_witness_white", [-8.0, 2.0, 110.0], radius=0.85, role="witness"))

    # Center: Near/Far witness pair (Z=107 and Z=119)
    objects.append(make_sdf_cylinder("cathedral.station3.near_pedestal", "Near Witness Pedestal (d=3.0)", "material.cathedral_bronze", [0.0, 0.6, 107.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station3.near_witness", "Near Radiant Witness (Brilliant Glow)", "material.cathedral_witness_white", [0.0, 2.0, 107.0], radius=0.85, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station3.far_pedestal", "Far Witness Pedestal (d=9.0)", "material.cathedral_bronze", [0.0, 0.6, 119.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station3.far_witness", "Far Radiant Witness (Attenuated Glow)", "material.cathedral_witness_white", [0.0, 2.0, 119.0], radius=0.85, role="witness"))

    # East: Toroidal Ring Radiance Witness (X = 8, Z = 110)
    objects.append(make_sdf_cylinder("cathedral.station3.ped_ring", "Toroidal Ring Radiance Pedestal", "material.cathedral_gold", [8.0, 0.6, 110.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station3.wit_ring", "Toroidal Luminous Ring Witness", "material.cathedral_witness_white", [8.0, 2.0, 110.0], radius=0.85, role="witness"))

    # -------------------------------------------------------------------------
    # PARALLEL WING C: LIVE AUTHORING LABORATORY (West of Station 3, X = -40 to -65)
    # -------------------------------------------------------------------------
    for x_lab in range(-65, -15, 10):
        objects.append(make_sdf_box(f"cathedral.wingC.floor_{x_lab}", f"Wing C Floor X={x_lab}", "material.cathedral_basalt", [float(x_lab), -0.6, 110.0], [5.0, 0.5, 10.0], role="wing_floor"))

    objects.append(make_inscription_stele(
        "cathedral.wingC.stele",
        "PARALLEL WING C — LIVE AUTHORING LABORATORY",
        "PARALLEL WING C",
        "Value vs Structure vs Runtime Coordinate",
        "Three distinct categories of change governed by Earthcall's invalidation architecture.",
        "Value: numeric params refresh. Structure: WGSL regenerates. Time: 0 AST edits, 0 recompiles.",
        [-38.0, 0.0, 104.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.wingC.ped_value", "Value Change Laboratory Pedestal", "material.cathedral_gold", [-45.0, 0.6, 110.0], 1.2, 0.6, role="lab_pedestal"))
    objects.append(make_sdf_sphere("cathedral.wingC.wit_value", "Value Change Witness (Coefficient Update)", "material.cathedral_witness_white", [-45.0, 2.0, 110.0], radius=0.8, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.wingC.ped_struct", "Structure Change Laboratory Pedestal", "material.cathedral_bronze", [-55.0, 0.6, 110.0], 1.2, 0.6, role="lab_pedestal"))
    objects.append(make_sdf_sphere("cathedral.wingC.wit_struct", "Structure Change Witness (Operator Tree Update)", "material.cathedral_witness_white", [-55.0, 2.0, 110.0], radius=0.8, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.wingC.ped_time", "Runtime Time Laboratory Pedestal", "material.cathedral_alabaster", [-65.0, 0.6, 110.0], 1.2, 0.6, role="lab_pedestal"))
    objects.append(make_sdf_sphere("cathedral.wingC.wit_time", "Runtime Time Witness (Timeline Advancement)", "material.cathedral_witness_white", [-65.0, 2.0, 110.0], radius=0.8, role="witness"))

    # -------------------------------------------------------------------------
    # PARALLEL WING D: COMPATIBILITY & REFUSAL WITNESSES (East of Station 3, X = 40 to 65)
    # -------------------------------------------------------------------------
    for x_comp in range(15, 65, 10):
        objects.append(make_sdf_box(f"cathedral.wingD.floor_{x_comp}", f"Wing D Floor X={x_comp}", "material.cathedral_basalt", [float(x_comp), -0.6, 110.0], [5.0, 0.5, 10.0], role="wing_floor"))

    objects.append(make_inscription_stele(
        "cathedral.wingD.stele",
        "PARALLEL WING D — COMPATIBILITY & REFUSAL WITNESSES",
        "PARALLEL WING D",
        "Compatibility: chi=color, alpha=1, V=1. Refusal: unsupported math refuses explicitly.",
        "Older sources survive unchanged via multiplicative identity. Unsupported math refuses cleanly.",
        "Refusal telemetry: @screen-channel.sdfProgramRefusals. Never silent approximations.",
        [38.0, 0.0, 104.0]
    ))
    objects.append(make_sdf_box("cathedral.wingD.compat_altar", "Monument of Multiplicative Compatibility", "material.cathedral_gold", [48.0, 1.0, 110.0], [1.5, 1.0, 1.5], role="monument"))
    objects.append(make_sdf_box("cathedral.wingD.refusal_stele", "Monolith of Truthful Refusal (No Black Box)", "material.cathedral_blocker_obsidian", [60.0, 1.8, 110.0], [1.2, 1.8, 0.3], role="monument"))

    # -------------------------------------------------------------------------
    # STATION 4: RUNG 4 — RELATIVE TIME rho(p,t) & ANIMATED SHAPES (Z = 145)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station4.stele",
        "RUNG 4 — RELATIVE TIME rho(p,t) & ANIMATED SHAPES (2026-09-20)",
        "RUNG 4",
        "rho(p,t) -> scalar",
        "Timeline is a relative temporal domain. Advancing t mutates zero ASTs.",
        "Shell radius breathes, rings expand and contract, and rotating lobes advance with time.",
        [0.0, 0.0, 139.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.station4.ped_west", "Breathing Shell Radius Pedestal", "material.cathedral_bronze", [-7.0, 0.6, 145.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station4.wit_west", "Breathing Shell Radius Radiance Witness", "material.cathedral_witness_white", [-7.0, 2.0, 145.0], radius=0.85, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station4.ped_center", "Breathing Luminous Heart Pedestal", "material.cathedral_gold", [0.0, 0.6, 145.0], 1.4, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station4.wit_center", "Breathing Heart Radiance Witness", "material.cathedral_witness_white", [0.0, 2.2, 145.0], radius=1.0, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station4.ped_east", "Rotating Quadrant Lobes Pedestal", "material.cathedral_bronze", [7.0, 0.6, 145.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station4.wit_east", "Rotating Lobes Radiance Witness", "material.cathedral_witness_white", [7.0, 2.0, 145.0], radius=0.85, role="witness"))

    # -------------------------------------------------------------------------
    # STATION 5: RUNG 5 — INDEPENDENT CHROMA OVER SHAPES (Z = 180)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station5.stele",
        "RUNG 5 — INDEPENDENT SOURCE CHROMA chi(p,t) (2026-09-21)",
        "RUNG 5",
        "emissionRGB(p,t) = rho(p,t) * chi(p,t)",
        "Color is an independent authored vector field. rho shape != chi shape.",
        "North/South Bipolar gold/cyan, azimuthal ring gradients, and concentric emerald/amethyst shells.",
        [0.0, 0.0, 174.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.station5.ped_west", "Bipolar North/South Chroma Pedestal", "material.cathedral_bronze", [-8.0, 0.6, 180.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station5.wit_west", "Bipolar Gold/Cyan Chroma Witness", "material.cathedral_witness_white", [-8.0, 2.0, 180.0], radius=0.85, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station5.ped_center", "Spectral Traveling Wave Pedestal", "material.cathedral_gold", [0.0, 0.6, 180.0], 1.4, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station5.wit_center", "Spectral Wave Chroma Witness", "material.cathedral_witness_white", [0.0, 2.2, 180.0], radius=1.0, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station5.ped_east", "Concentric Shells Chroma Pedestal", "material.cathedral_bronze", [8.0, 0.6, 180.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station5.wit_east", "Concentric Emerald/Amethyst Chroma Witness", "material.cathedral_witness_white", [8.0, 2.0, 180.0], radius=0.85, role="witness"))

    # -------------------------------------------------------------------------
    # PARALLEL WING A: AUTHORED SURFACE COLOR FIELDS (West of Station 5, X = -40 to -70)
    # -------------------------------------------------------------------------
    for x_col in range(-70, -15, 10):
        objects.append(make_sdf_box(f"cathedral.wingA.floor_{x_col}", f"Wing A Floor X={x_col}", "material.cathedral_basalt", [float(x_col), -0.6, 180.0], [5.0, 0.5, 10.0], role="wing_floor"))

    objects.append(make_inscription_stele(
        "cathedral.wingA.stele",
        "PARALLEL WING A — AUTHORED SURFACE COLOR FIELDS",
        "PARALLEL WING A",
        "litRgb = sdfColor(p) * (ambient + diffuse) + specular",
        "Surface appearance field != Source chroma chi.",
        "Blue surface under red light is dark. Blue light on white surface is blue. These are separate beings.",
        [-38.0, 0.0, 174.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.wingA.col_rainbow", "Stratified Rainbow Surface Column", "material.prism_surface_rainbow", [-45.0, 3.5, 180.0], 1.0, 3.5, role="surface_color_exhibit"))
    objects.append(make_sdf_box("cathedral.wingA.box_radial", "Radial Chromatic Gradient Surface", "material.prism_surface_radial", [-55.0, 2.0, 180.0], [1.2, 1.2, 1.2], role="surface_color_exhibit"))
    objects.append(make_sdf_torus("cathedral.wingA.torus_lattice", "Nodal Lattice Pattern Surface", "material.prism_surface_lattice", [-65.0, 2.5, 180.0], 1.6, 0.5, role="surface_color_exhibit"))

    # The Fundamental Paired Proof:
    objects.append(make_sdf_sphere("cathedral.wingA.pure_blue_surface", "Authored Blue Surface Receiver (sdfColor)", "material.prism_surface_pure_blue", [-50.0, 2.0, 172.0], radius=0.9, role="paired_proof"))
    objects.append(make_sdf_sphere("cathedral.wingA.pure_white_surface", "Neutral White Surface Receiver", "material.cathedral_witness_white", [-60.0, 2.0, 172.0], radius=0.9, role="paired_proof"))

    # -------------------------------------------------------------------------
    # STATION 6: RUNG 6 — ANGULAR EMISSION + COMPLEX SPATIAL FORM (Z = 215)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station6.stele",
        "RUNG 6 — ANGULAR EMISSION alpha(p,omega,t) (2026-09-21)",
        "RUNG 6",
        "sourceEmissionRGB = rho(p,t) * chi(p,t) * alpha(p,omega,t)",
        "Direction is authored mathematics over normalized omega. Combined with a toroidal source shape.",
        "Two equidistant points receive radically different light because the source speaks directionally.",
        [0.0, 0.0, 209.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.station6.ped_beam", "In-Beam Witness Pedestal (d=7.0)", "material.cathedral_bronze", [-7.0, 0.6, 215.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station6.wit_beam", "Spotlight In-Beam Witness (Brilliant)", "material.cathedral_witness_white", [-7.0, 2.0, 215.0], radius=0.85, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station6.ped_rot", "Rotating Beam Pedestal (d=7.0)", "material.cathedral_bronze", [7.0, 0.6, 215.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station6.wit_rot", "Rotating Lighthouse Beam Witness", "material.cathedral_witness_white", [7.0, 2.0, 215.0], radius=0.85, role="witness"))

    objects.append(make_sdf_cylinder("cathedral.station6.ped_fan", "Outward Radial Ring Emission Pedestal", "material.cathedral_gold", [0.0, 0.6, 218.0], 1.2, 0.6, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station6.wit_fan", "Outward Radial Torus Emission Witness", "material.cathedral_witness_white", [0.0, 2.0, 218.0], radius=0.85, role="witness"))

    # -------------------------------------------------------------------------
    # STATION 7: RUNG 7 — MULTIPLE COEXISTING SHAPED SOURCES (Z = 250)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station7.stele",
        "RUNG 7 — MULTIPLE INDEPENDENT SOURCE SHAPES (2026-09-21)",
        "RUNG 7",
        "E_total = sum_i [ rho_i(p,t) * chi_i(p,t) * alpha_i(p,omega,t) ]",
        "Four independent sources with four distinct shapes: Shell, Torus, Noisy Lobe, and Vertical Filament.",
        "World composition happens ABOVE individual source ASTs. Additive spectral blending.",
        [0.0, 0.0, 244.0]
    ))
    objects.append(make_sdf_cylinder("cathedral.station7.pillar_gold", "Source A: Spherical Shell Beacon", "material.cathedral_gold", [0.0, 2.5, 250.0], 0.8, 2.5, role="source_beacon"))
    objects.append(make_sdf_cylinder("cathedral.station7.pillar_sapphire", "Source B: Sapphire Toroidal Ring Beacon", "material.cathedral_limestone", [-8.0, 2.0, 250.0], 0.8, 2.0, role="source_beacon"))
    objects.append(make_sdf_cylinder("cathedral.station7.pillar_amethyst", "Source C: Amethyst Organic Pulsar Beacon", "material.cathedral_limestone", [8.0, 2.0, 250.0], 0.8, 2.0, role="source_beacon"))
    objects.append(make_sdf_cylinder("cathedral.station7.pillar_emerald", "Source D: Emerald Filament Beacon", "material.cathedral_limestone", [0.0, 4.5, 245.0], 0.5, 2.5, role="source_beacon"))

    objects.append(make_sdf_cylinder("cathedral.station7.altar", "Choir of Shaped Light Blending Altar", "material.cathedral_bronze", [0.0, 0.5, 252.0], 2.2, 0.5, role="altar"))
    objects.append(make_sdf_sphere("cathedral.station7.synthesis_witness", "Four-Source Synthesis Blended Sphere", "material.cathedral_witness_white", [0.0, 1.8, 252.0], radius=1.0, role="witness"))

    # -------------------------------------------------------------------------
    # STATION 8: RUNG 8 — VISIBILITY AGAINST COMPLEX SOURCES (Z = 285)
    # -------------------------------------------------------------------------
    objects.append(make_inscription_stele(
        "cathedral.station8.stele",
        "RUNG 8 — DERIVED VISIBILITY / SHADOWS (2026-09-21)",
        "RUNG 8",
        "L_direct = sum_i [ E_i * V_i(source,p,omega) ]",
        "The source still speaks. Geometry decides whether its voice arrives.",
        "Geometry affects V in [0,1]. Geometry NEVER rewrites rho, chi, or alpha.",
        [0.0, 0.0, 279.0]
    ))
    objects.append(make_sdf_box("cathedral.station8.single_blocker", "Floating Obsidian Shadow Blocker", "material.cathedral_blocker_obsidian", [-6.0, 2.2, 285.0], [0.35, 1.0, 0.35], role="occluder"))
    objects.append(make_sdf_cylinder("cathedral.station8.single_pedestal", "Shadow Receiver Plinth", "material.cathedral_bronze", [-6.0, 0.5, 288.0], 1.2, 0.5, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station8.single_witness", "Shadow Cast Receiver Sphere", "material.cathedral_witness_white", [-6.0, 1.7, 288.0], radius=0.85, role="witness"))

    objects.append(make_sdf_box("cathedral.station8.red_blocker", "Filament-Selective Occluder", "material.cathedral_blocker_obsidian", [-3.0, 3.0, 285.0], [0.35, 1.0, 0.35], role="occluder"))
    objects.append(make_sdf_cylinder("cathedral.station8.selective_pedestal", "Dual-Path Receiver Pedestal", "material.cathedral_bronze", [0.0, 0.5, 285.0], 1.5, 0.5, role="pedestal"))
    objects.append(make_sdf_sphere("cathedral.station8.selective_witness", "Selective Occlusion Receiver (Glows Pure Blue)", "material.cathedral_witness_white", [0.0, 1.8, 285.0], radius=0.9, role="witness"))

    # -------------------------------------------------------------------------
    # PARALLEL WING B: VOLUMETRIC V0 DENSITY SOVEREIGNTY (East of Station 8, X = 40 to 100)
    # Full shape gallery: Spherical, Hollow Shell, Toroidal, CSG Crescent, Noise, Breathing, Proxy-Proof, Dual
    # -------------------------------------------------------------------------
    for x_vol in range(15, 105, 10):
        objects.append(make_sdf_box(f"cathedral.wingB.floor_{x_vol}", f"Wing B Floor X={x_vol}", "material.cathedral_basalt", [float(x_vol), -0.6, 285.0], [5.0, 0.5, 10.0], role="wing_floor"))

    # Transept lateral aisles in Wing B
    for z_side in range(260, 310, 10):
        objects.append(make_sdf_box(f"cathedral.wingB.floor_side_{z_side}", f"Wing B Lateral Floor Z={z_side}", "material.cathedral_basalt", [65.0, -0.6, float(z_side)], [10.0, 0.5, 5.0], role="wing_floor"))

    objects.append(make_inscription_stele(
        "cathedral.wingB.stele",
        "PARALLEL WING B — VOLUMETRIC V0: DENSITY SOVEREIGNTY (2026-09-21)",
        "PARALLEL WING B",
        "D_medium(p,t) -> scalar, volumeDensityEval(p)",
        "Space itself has authored substance. The proxy AABB is implementation scaffolding; mathematics is the shape.",
        "SDF-derived distance fields feed density math. Zero density outside shape means proxy is visually irrelevant.",
        [38.0, 0.0, 279.0]
    ))

    # Gallery pedestals and descriptive tablets for each volumetric being:
    wingB_exhibits = [
        (45.0, 285.0, "Exhibit B1: Soft Spherical Cloud Plinth", "D(p) = clamp(1 - length(p)/2.2, 0, 1)"),
        (55.0, 285.0, "Exhibit B2: Hollow Shell Nebular Membrane Plinth", "D(p) = clamp(1 - |length(p)-2.6|/0.6, 0, 1)"),
        (65.0, 285.0, "Exhibit B3: Toroidal Donut Fog Ring Plinth", "D(p) = clamp(1 - sqrt((pxz-2.4)^2 + y^2)/0.6, 0, 1)"),
        (75.0, 285.0, "Exhibit B4: CSG-Subtracted Crescent Cloud Plinth", "D(p) = D_base * clamp((dist(hole)-1.3)/0.4, 0, 1)"),
        (85.0, 285.0, "Exhibit B5: Noise-Warped Organic Cloud Plinth", "D(p) = clamp(1 - (length(p) - 0.7*noise)/2.2, 0, 1)"),
        (95.0, 285.0, "Exhibit B6: Time-Breathing Hollow Nebula Plinth", "D(p,t) = clamp(1 - |length(p)-R(t)|/0.5, 0, 1)"),
        (65.0, 268.0, "Exhibit B7: Diagnostic Proxy-Proof Torus Plinth", "AABB: 20x8x20m | Torus R=1.8, r=0.4 (95%+ volume D=0!)"),
        (65.0, 302.0, "Exhibit B8: Dual Sovereign Being Plinth", "rho_source != D_medium | Opaque Depth Truncation Pillar")
    ]
    for px, pz, pname, pmath in wingB_exhibits:
        objects.append(make_sdf_cylinder(f"cathedral.wingB.ped_{int(px)}_{int(pz)}", pname, "material.cathedral_bronze", [px, 0.4, pz], 2.2, 0.4, role="pedestal",
                                         extra_props={"medium.math": {"t": "string", "v": pmath}}))

    # Opaque Depth Truncation Pillar at Exhibit B8 (standing behind the medium)
    objects.append(make_sdf_cylinder("cathedral.wingB.depth_pillar", "Opaque Scene Depth Truncation Pillar", "material.cathedral_limestone", [65.0, 3.5, 305.0], 0.8, 3.5, role="depth_truncation_witness"))

    # -------------------------------------------------------------------------
    # THE SUMMIT: THE PRISM (Z = 330 to 380)
    # -------------------------------------------------------------------------
    for z_prism in range(330, 385, 15):
        objects.append(make_sdf_box(f"cathedral.prism.floor_{z_prism}", f"Prism Sanctuary Pavement Z={z_prism}", "material.cathedral_basalt", [0.0, -0.6, float(z_prism)], [22.0, 0.5, 7.5], role="sanctuary_floor"))

    objects.append(make_inscription_stele(
        "cathedral.prism.summit_stele",
        "THE PRISM — SUMMIT OF AUTHORED LIGHT & SHAPED MEDIA",
        "THE PRISM / COMPLETE SYNTHESIS",
        "L_direct = sum_i [ E_i * V_i ] + Volume(D(p,t))",
        "The bounding boxes are implementation scaffolding. The mathematics is the shape.",
        "Multiple shaped radiant suns, encircling toroidal fog medium, rotunda atmosphere, and surface color fields.",
        [0.0, 0.0, 328.0]
    ))

    # The Colossal Crystal Prism at center of Sanctuary: [0.0, 4.5, 355.0]
    objects.append(make_sdf_cylinder("cathedral.prism.high_altar", "High Altar of Synthesis", "material.cathedral_gold", [0.0, 0.7, 355.0], 4.0, 0.7, role="high_altar"))
    objects.append(make_sdf_box("cathedral.prism.colossal_prism", "The Sovereign Crystal Prism", "material.cathedral_prism_crystal", [0.0, 5.0, 355.0], [1.5, 3.5, 1.5], role="central_prism"))

    # Four synthesis witness spheres on circular terrace:
    prism_witnesses = [
        (-6.0, 350.0, "Prism Golden Reception Sphere"),
        (6.0,  350.0, "Prism Sapphire Reception Sphere"),
        (-6.0, 360.0, "Prism Amethyst Reception Sphere"),
        (6.0,  360.0, "Prism Full-Choir Synthesis Sphere")
    ]
    for px, pz, pname in prism_witnesses:
        objects.append(make_sdf_cylinder(f"cathedral.prism.ped_{px}_{pz}", f"{pname} Plinth", "material.cathedral_bronze", [px, 0.6, pz], 1.0, 0.6, role="pedestal"))
        objects.append(make_sdf_sphere(f"cathedral.prism.wit_{px}_{pz}", pname, "material.cathedral_witness_white", [px, 1.8, pz], radius=0.8, role="witness"))

    # Soaring Sanctuary Pillars & Dome Ring
    for angle_deg in range(0, 360, 45):
        import math
        rad = math.radians(angle_deg)
        cx = 18.0 * math.cos(rad)
        cz = 355.0 + 18.0 * math.sin(rad)
        objects.append(make_sdf_cylinder(f"cathedral.prism.rotunda_col_{angle_deg}", f"Sanctuary Rotunda Colonnade {angle_deg}°", "material.cathedral_limestone", [cx, 9.0, cz], 1.2, 9.0, role="rotunda_pillar"))

    objects.append(make_sdf_torus("cathedral.prism.crown_ring", "Sanctuary Dome Crown Ring", "material.cathedral_gold", [0.0, 18.0, 355.0], 18.0, 0.8, role="dome_crown"))

    return objects

# -----------------------------------------------------------------------------
# Main Generation / Patch Logic
# -----------------------------------------------------------------------------

def generate_prism_cathedral():
    target_dir = os.path.join("saves", "zones", "Prism Cathedral")
    target_path = os.path.join(target_dir, "zone.json")
    os.makedirs(target_dir, exist_ok=True)

    print(f"Building Prism Cathedral Zone (Complex-Shaped Fields & Media) -> {target_path}...")

    spatial_root = {
        "id": "prism-cathedral.primary-light-field",
        "origin": [0.0, 8.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1, "frequency": 1, "amplitude": 1,
            "ast": make_cathedral_spatial_root_scalar_ast()
        },
        "vectorField": {"mode": "Procedural", "baseFlowX": 0, "baseFlowY": 0, "baseFlowZ": 0, "frequency": 1, "amplitude": 1, "ast": {}},
        "lightChroma": make_cathedral_spatial_root_chroma_ast(),
        "lightAngular": make_cathedral_spatial_root_angular_ast(),
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Prism Cathedral Master Radiant Field"},
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.color": {"t": "vec3", "x": 1.0, "y": 0.98, "z": 0.95},
            "light.intensity": {"t": "float", "v": 1.8},
            "light.ambient": {"t": "float", "v": 0.20},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.0},
            "light.attenuation.constant": {"t": "float", "v": 1.0},
            "light.attenuation.linear": {"t": "float", "v": 0.02},
            "light.attenuation.quadratic": {"t": "float", "v": 0.001}
        }
    }

    additional_fields = make_additional_spatial_fields()
    materials = make_cathedral_materials()
    objects = build_cathedral_world()

    zone_data = {
        "identifier": "Prism Cathedral",
        "name": "Prism Cathedral",
        "authors": ["Zachary Zhang", "GPT-5.6 Sol"],
        "injected_by": "GPT-5.6 Sol",
        "scope": "global",
        "deletable": False,
        "qualities": ["cathedral", "observatory", "monastery", "luminous", "mathematics", "ontomath", "progression", "complex_shapes"],
        "parentZone": "",
        "owner": "default",
        "spatialRoot": spatial_root,
        "spatialFields": additional_fields,
        "materials": materials,
        "world": {
            "objects": objects
        },
        "formationRelations": {"relations": []},
        "lexemes": []
    }

    # Atomic stage-and-replace discipline
    staged_path = target_path + ".staged"
    with open(staged_path, "w") as f:
        json.dump(zone_data, f, indent=2)

    with open(staged_path, "r") as f:
        verified = json.load(f)
    assert verified["identifier"] == "Prism Cathedral"
    assert "spatialRoot" in verified
    assert "spatialFields" in verified
    assert len(verified["spatialFields"]) == 16  # 4 Rung7/8 sources + 8 WingB media + 4 Summit media/sources
    assert len(verified["world"]["objects"]) > 0

    if os.path.exists(target_path):
        backup_path = target_path + ".bak"
        shutil.copy2(target_path, backup_path)
        print(f"Backed up existing zone to {backup_path}")

    os.replace(staged_path, target_path)
    print(f"SUCCESS: Prism Cathedral updated successfully at {target_path}!")
    print(f"  Total World Objects: {len(objects)}")
    print(f"  Total Materials (including Surface Color Fields): {len(materials)}")
    print(f"  Total Radiant/Volumetric Fields: 1 spatialRoot + {len(additional_fields)} spatialFields")

if __name__ == "__main__":
    generate_prism_cathedral()
