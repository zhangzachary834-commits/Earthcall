#!/usr/bin/env python3
"""
Radiance Gallery — 5-Chamber OntoMath Radiance Rung 3 Showcase Generator

This script authors a new demo Zone ("Radiance Gallery") whose purpose is to visibly
demonstrate what authored scalar radiance fields rho(p) can do under Rung 3.

Architectural Invariants Respected:
1. rho(p) is strictly a scalar source-strength / emission-envelope field:
   rho(p) -> scalar >= 0
2. No reinterpretation of rho as chroma, directionality, visibility/occlusion,
   material response, GI, or time variation.
3. No new Light classes, hardcoded shaders, or fake tricks.
4. All lighting differences come directly from the authored OntoMath piecewise AST
   evaluated in WebGPU through the production SdfWgsl compiler (drawFieldModel / drawImplicit).
5. All 5 rooms feature identical-geometry, identical-material SDF witness objects so that
   all visual contrast is 100% truthful to the authored scalar light field.
"""

import json
import os
import math

# -----------------------------------------------------------------------------
# 1. OntoMath AST Authoring Helpers
# -----------------------------------------------------------------------------

def scalar_node(val):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(val), "factors": {}}]}}

def scalar_poly_node(terms):
    return {"op": 0, "scalarForm": {"terms": terms}}

def vec3_node(x, y, z):
    return {
        "op": 2,
        "children": [scalar_node(x), scalar_node(y), scalar_node(z)]
    }

def p_node():
    return {"op": 1, "var": "p"}

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

def noise_node(vec_expr):
    return {"op": 29, "children": [vec_expr]}

# -----------------------------------------------------------------------------
# 2. Five Chamber Scalar Radiance Fields
# All coordinates p = pw - u.lightPos. With u.lightPos = (0, 12, 0):
# p.x = pw.x, p.z = pw.z, p.y = pw.y - 12.0
# Room centers in world space are at y = 2.5, so p.y = -9.5.
# -----------------------------------------------------------------------------

# Room 1: Uniform / Baseline Room (Center: x = -40, z = 0)
# Goal: establish restrained, smooth baseline normal behavior (~1.0)
def make_room1_ast():
    terms = [
        {"c": 1.05, "factors": {}},
        {"c": 0.12, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.3, "shift": 0.0}]},
        {"c": 0.08, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.3, "shift": 12.0}]}
    ]
    return scalar_poly_node(terms)

# Room 2: Strong Falloff Chamber (Center: x = -20, z = 0)
# Goal: extreme spatial attenuation between center focal source and periphery (>15x ratio)
# rho_2(p) = 4.2 / (1.0 + 0.45 * [(p.x + 20)^2 + (p.y + 9.5)^2 + p.z^2])
# Expanded polynomial denominator:
# 1.0 + 0.45*(400 + 40*x + x^2 + 90.25 + 19*y + y^2 + z^2)
# = 221.6125 + 18.0*x + 8.55*y + 0.45*x^2 + 0.45*y^2 + 0.45*z^2
def make_room2_ast():
    terms = [
        {"c": 221.6125, "factors": {}},
        {"c": 18.0, "factors": {"x": 1.0}},
        {"c": 8.55, "factors": {"y": 1.0}},
        {"c": 0.45, "factors": {"x": 2.0}},
        {"c": 0.45, "factors": {"y": 2.0}},
        {"c": 0.45, "factors": {"z": 2.0}}
    ]
    denom = scalar_poly_node(terms)
    num = scalar_node(4.2)
    return div_node(num, denom)

# Room 3: Halo / Shell Chamber (Center: x = 0, z = 0)
# Goal: sculpt radiance into concentric bright shells separated by dark valleys
# Shell 1 (inner ring, r=2.5m, peak 2.4): 2.4 / (1.0 + 3.2 * (dist - 2.5)^2)
# Shell 2 (outer ring, r=5.5m, peak 2.0): 2.0 / (1.0 + 2.2 * (dist - 5.5)^2)
# Plus baseline ambient: 0.15
def make_room3_ast():
    c = vec3_node(0.0, -9.5, 0.0)
    d = dist_node(p_node(), c)
    
    # Inner Shell (r = 2.5)
    delta1 = sub_node(d, scalar_node(2.5))
    delta1_sq = pow_node(delta1, scalar_node(2.0))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(3.2), delta1_sq))
    shell1 = div_node(scalar_node(2.4), denom1)
    
    # Outer Shell (r = 5.5)
    delta2 = sub_node(d, scalar_node(5.5))
    delta2_sq = pow_node(delta2, scalar_node(2.0))
    denom2 = add_node(scalar_node(1.0), scale_node(scalar_node(2.2), delta2_sq))
    shell2 = div_node(scalar_node(2.0), denom2)
    
    shells = add_node(shell1, shell2)
    return add_node(shells, scalar_node(0.15))

# Room 4: Multi-Lobe Chamber (Center: x = 20, z = 0)
# Goal: three distinct focal lobes shaping the space with bright focal clusters
# Lobe 1 (North): (20.0, -9.5, 4.0), peak 2.6
# Lobe 2 (Southwest): (16.5, -9.5, -3.0), peak 2.4
# Lobe 3 (Southeast): (23.5, -9.5, -3.0), peak 2.4
def make_room4_ast():
    c1 = vec3_node(20.0, -9.5, 4.0)
    d1_sq = pow_node(dist_node(p_node(), c1), scalar_node(2.0))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(0.8), d1_sq))
    lobe1 = div_node(scalar_node(2.6), denom1)
    
    c2 = vec3_node(16.5, -9.5, -3.0)
    d2_sq = pow_node(dist_node(p_node(), c2), scalar_node(2.0))
    denom2 = add_node(scalar_node(1.0), scale_node(scalar_node(0.8), d2_sq))
    lobe2 = div_node(scalar_node(2.4), denom2)
    
    c3 = vec3_node(23.5, -9.5, -3.0)
    d3_sq = pow_node(dist_node(p_node(), c3), scalar_node(2.0))
    denom3 = add_node(scalar_node(1.0), scale_node(scalar_node(0.8), d3_sq))
    lobe3 = div_node(scalar_node(2.4), denom3)
    
    lobes12 = add_node(lobe1, lobe2)
    lobes123 = add_node(lobes12, lobe3)
    return add_node(lobes123, scalar_node(0.12))

# Room 5: Procedural / Cathedral Chamber (Center: x = 40, z = 0)
# Goal: rich procedural spatial variation combining dome envelope, 2D harmonic lattice, and 3D Perlin noise
def make_room5_ast():
    c5 = vec3_node(40.0, -9.5, 0.0)
    d5_sq = pow_node(dist_node(p_node(), c5), scalar_node(2.0))
    denom5 = add_node(scalar_node(1.0), scale_node(scalar_node(0.035), d5_sq))
    base_dome = div_node(scalar_node(2.5), denom5)
    
    # 0.8*(x-40) = 0.8*x - 32.0; 1.6*(x-40) = 1.6*x - 64.0
    lattice_terms = [
        {"c": 0.60, "factors": {}},
        {"c": 0.25, "factors": {}, "trans": [
            {"kind": 1, "var": "x", "scale": 0.8, "shift": -32.0},
            {"kind": 1, "var": "z", "scale": 0.8, "shift": 0.0}
        ]},
        {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.6, "shift": -64.0}]},
        {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 1.6, "shift": 0.0}]}
    ]
    lattice = scalar_poly_node(lattice_terms)
    
    scaled_p = scale_node(scalar_node(0.18), p_node())
    noise = noise_node(scaled_p)
    scaled_noise = scale_node(scalar_node(0.28), noise)
    
    modulation = add_node(lattice, scaled_noise)
    return scale_node(base_dome, modulation)

def make_gallery_ast_definition():
    """
    Constructs the 5-chamber piecewise OntoMath scalar field partitioned along x:
      Room 1: x in [-inf, -30.0]
      Room 2: x in [-30.0, -10.0]
      Room 3: x in [-10.0,  10.0]
      Room 4: x in [ 10.0,  30.0]
      Room 5: x in [ 30.0,  +inf]
    """
    return {
        "input": "x",
        "pieces": [
            {
                "hasLo": False,
                "hasHi": True,
                "hi": -30.0,
                "includeHi": True,
                "mathNode": make_room1_ast()
            },
            {
                "hasLo": True,
                "lo": -30.0,
                "includeLo": False,
                "hasHi": True,
                "hi": -10.0,
                "includeHi": True,
                "mathNode": make_room2_ast()
            },
            {
                "hasLo": True,
                "lo": -10.0,
                "includeLo": False,
                "hasHi": True,
                "hi": 10.0,
                "includeHi": True,
                "mathNode": make_room3_ast()
            },
            {
                "hasLo": True,
                "lo": 10.0,
                "includeLo": False,
                "hasHi": True,
                "hi": 30.0,
                "includeHi": True,
                "mathNode": make_room4_ast()
            },
            {
                "hasLo": True,
                "lo": 30.0,
                "includeLo": False,
                "hasHi": False,
                "mathNode": make_room5_ast()
            }
        ]
    }

# -----------------------------------------------------------------------------
# 3. Object & Geometry Generators
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
            "prim": 0, # SdfPrim::Sphere
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
            "prim": 1, # SdfPrim::Box
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
            "prim": 4, # SdfPrim::Cylinder
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
            "prim": 6, # SdfPrim::Torus
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
# 4. Build Full Radiance Gallery Zone Document
# -----------------------------------------------------------------------------

def build_radiance_gallery_zone():
    objects = []
    
    # -------------------------------------------------------------------------
    # A. Gallery Promenade Floor & Architectural Dividing Portal Portals
    # -------------------------------------------------------------------------
    # Main continuous gallery floor extending through all 5 rooms: X in [-52, 52], Z in [-10, 10]
    objects.append(make_sdf_box_object(
        "gallery-promenade-floor", "Gallery Promenade Floor", "gallery.pedestal.stone",
        pos=[0.0, -0.2, 0.0], half_extents=[52.0, 0.2, 10.0], role="architecture"
    ))
    
    # Dividing portal arches / columns between chambers:
    # Portal 1-2 at X = -30
    objects.append(make_sdf_box_object("portal-col-1-2-north", "Portal 1-2 Arch North", "gallery.arch.gold", [-30.0, 3.0, 5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-col-1-2-south", "Portal 1-2 Arch South", "gallery.arch.gold", [-30.0, 3.0, -5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-lintel-1-2", "Portal 1-2 Header Lintel", "gallery.arch.gold", [-30.0, 5.8, 0.0], [0.5, 0.3, 5.5]))

    # Portal 2-3 at X = -10
    objects.append(make_sdf_box_object("portal-col-2-3-north", "Portal 2-3 Arch North", "gallery.arch.gold", [-10.0, 3.0, 5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-col-2-3-south", "Portal 2-3 Arch South", "gallery.arch.gold", [-10.0, 3.0, -5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-lintel-2-3", "Portal 2-3 Header Lintel", "gallery.arch.gold", [-10.0, 5.8, 0.0], [0.5, 0.3, 5.5]))

    # Portal 3-4 at X = 10
    objects.append(make_sdf_box_object("portal-col-3-4-north", "Portal 3-4 Arch North", "gallery.arch.gold", [10.0, 3.0, 5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-col-3-4-south", "Portal 3-4 Arch South", "gallery.arch.gold", [10.0, 3.0, -5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-lintel-3-4", "Portal 3-4 Header Lintel", "gallery.arch.gold", [10.0, 5.8, 0.0], [0.5, 0.3, 5.5]))

    # Portal 4-5 at X = 30
    objects.append(make_sdf_box_object("portal-col-4-5-north", "Portal 4-5 Arch North", "gallery.arch.gold", [30.0, 3.0, 5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-col-4-5-south", "Portal 4-5 Arch South", "gallery.arch.gold", [30.0, 3.0, -5.0], [0.5, 3.0, 0.5]))
    objects.append(make_sdf_box_object("portal-lintel-4-5", "Portal 4-5 Header Lintel", "gallery.arch.gold", [30.0, 5.8, 0.0], [0.5, 0.3, 5.5]))

    # -------------------------------------------------------------------------
    # B. Chamber 1 — Uniform / Baseline Room (Center: X = -40, Z = 0)
    # -------------------------------------------------------------------------
    # Central Pedestal and Standard Witness Sphere
    objects.append(make_sdf_cylinder_object("r1-pedestal-center", "Chamber 1 Baseline Pedestal", "gallery.pedestal.stone", [-40.0, 0.8, 0.0], 1.1, 0.8))
    objects.append(make_sdf_sphere_object("r1-witness-center", "Chamber 1 Baseline Center Witness", "gallery.marble.white", [-40.0, 2.5, 0.0], 0.85, role="baseline_center"))
    
    # Flanking comparison witnesses across the baseline room
    objects.append(make_sdf_cylinder_object("r1-pedestal-north", "Chamber 1 North Pedestal", "gallery.pedestal.stone", [-40.0, 0.6, 4.5], 0.8, 0.6))
    objects.append(make_sdf_sphere_object("r1-witness-north", "Chamber 1 North Witness", "gallery.marble.white", [-40.0, 2.0, 4.5], 0.75, role="baseline_flank"))

    objects.append(make_sdf_cylinder_object("r1-pedestal-south", "Chamber 1 South Pedestal", "gallery.pedestal.stone", [-40.0, 0.6, -4.5], 0.8, 0.6))
    objects.append(make_sdf_sphere_object("r1-witness-south", "Chamber 1 South Witness", "gallery.marble.white", [-40.0, 2.0, -4.5], 0.75, role="baseline_flank"))

    # Backdrop inspection wall panel
    objects.append(make_sdf_box_object("r1-backdrop-west", "Chamber 1 West Gallery Wall", "gallery.marble.white", [-51.5, 2.5, 0.0], [0.4, 2.5, 7.5], role="backdrop"))

    # -------------------------------------------------------------------------
    # C. Chamber 2 — Strong Falloff Chamber (Center: X = -20, Z = 0)
    # -------------------------------------------------------------------------
    # Central High Altar / Pedestal with blazing focal witness
    objects.append(make_sdf_cylinder_object("r2-altar-pedestal", "Chamber 2 Focal Altar Pedestal", "gallery.pedestal.stone", [-20.0, 1.0, 0.0], 1.2, 1.0))
    objects.append(make_sdf_sphere_object("r2-witness-focal-center", "Chamber 2 Blazing Focal Witness", "gallery.marble.white", [-20.0, 2.7, 0.0], 0.85, role="falloff_focal_peak"))

    # Near witnesses (r = 1.6m) — clearly illuminated
    objects.append(make_sdf_cylinder_object("r2-pedestal-near-e", "Chamber 2 Near East Pedestal", "gallery.pedestal.stone", [-18.4, 0.7, 0.0], 0.6, 0.7))
    objects.append(make_sdf_sphere_object("r2-witness-near-e", "Chamber 2 Near East Witness", "gallery.marble.white", [-18.4, 2.0, 0.0], 0.65, role="falloff_near"))

    objects.append(make_sdf_cylinder_object("r2-pedestal-near-w", "Chamber 2 Near West Pedestal", "gallery.pedestal.stone", [-21.6, 0.7, 0.0], 0.6, 0.7))
    objects.append(make_sdf_sphere_object("r2-witness-near-w", "Chamber 2 Near West Witness", "gallery.marble.white", [-21.6, 2.0, 0.0], 0.65, role="falloff_near"))

    # Mid-range witnesses (r = 3.5m) — visibly dimmed
    objects.append(make_sdf_cylinder_object("r2-pedestal-mid-n", "Chamber 2 Mid North Pedestal", "gallery.pedestal.stone", [-20.0, 0.7, 3.5], 0.6, 0.7))
    objects.append(make_sdf_sphere_object("r2-witness-mid-n", "Chamber 2 Mid North Witness", "gallery.marble.white", [-20.0, 2.0, 3.5], 0.65, role="falloff_mid"))

    objects.append(make_sdf_cylinder_object("r2-pedestal-mid-s", "Chamber 2 Mid South Pedestal", "gallery.pedestal.stone", [-20.0, 0.7, -3.5], 0.6, 0.7))
    objects.append(make_sdf_sphere_object("r2-witness-mid-s", "Chamber 2 Mid South Witness", "gallery.marble.white", [-20.0, 2.0, -3.5], 0.65, role="falloff_mid"))

    # Far periphery witnesses (r = 6.0m) — deep dramatic falloff
    objects.append(make_sdf_cylinder_object("r2-pedestal-far-n", "Chamber 2 Far North Pedestal", "gallery.pedestal.stone", [-20.0, 0.7, 6.2], 0.6, 0.7))
    objects.append(make_sdf_sphere_object("r2-witness-far-n", "Chamber 2 Far North Witness", "gallery.marble.white", [-20.0, 2.0, 6.2], 0.65, role="falloff_far"))

    objects.append(make_sdf_cylinder_object("r2-pedestal-far-s", "Chamber 2 Far South Pedestal", "gallery.pedestal.stone", [-20.0, 0.7, -6.2], 0.6, 0.7))
    objects.append(make_sdf_sphere_object("r2-witness-far-s", "Chamber 2 Far South Witness", "gallery.marble.white", [-20.0, 2.0, -6.2], 0.65, role="falloff_far"))

    # -------------------------------------------------------------------------
    # D. Chamber 3 — Halo / Shell Chamber (Center: X = 0, Z = 0)
    # -------------------------------------------------------------------------
    # Dark core central column (proves the center is a hollow, dim dip)
    objects.append(make_sdf_cylinder_object("r3-core-pedestal", "Chamber 3 Dark Core Pedestal", "gallery.pedestal.stone", [0.0, 0.8, 0.0], 0.8, 0.8))
    objects.append(make_sdf_sphere_object("r3-witness-dark-core", "Chamber 3 Hollow Dark Core Witness", "gallery.marble.white", [0.0, 2.4, 0.0], 0.75, role="halo_dark_core"))

    # Inner Luminous Ring (r = 2.5m) — glowing halo band
    objects.append(make_sdf_torus_object("r3-halo-inner-torus", "Chamber 3 Inner Halo Ring", "gallery.marble.white", [0.0, 1.2, 0.0], major_r=2.5, minor_r=0.25, role="halo_inner_ring"))
    objects.append(make_sdf_sphere_object("r3-witness-inner-n", "Chamber 3 Inner Halo Witness North", "gallery.marble.white", [0.0, 2.5, 2.5], 0.75, role="halo_inner_peak"))
    objects.append(make_sdf_sphere_object("r3-witness-inner-s", "Chamber 3 Inner Halo Witness South", "gallery.marble.white", [0.0, 2.5, -2.5], 0.75, role="halo_inner_peak"))

    # Dark Valley between shells (r = 4.0m) — brightness dips
    objects.append(make_sdf_cylinder_object("r3-valley-pedestal-n", "Chamber 3 Inter-Shell Valley Pedestal", "gallery.pedestal.stone", [0.0, 0.6, 4.0], 0.6, 0.6))
    objects.append(make_sdf_sphere_object("r3-witness-valley-n", "Chamber 3 Valley Witness North", "gallery.marble.white", [0.0, 1.9, 4.0], 0.7, role="halo_valley_dip"))

    # Outer Luminous Ring (r = 5.5m) — second glowing halo band
    objects.append(make_sdf_torus_object("r3-halo-outer-torus", "Chamber 3 Outer Halo Ring", "gallery.marble.white", [0.0, 1.0, 0.0], major_r=5.5, minor_r=0.25, role="halo_outer_ring"))
    objects.append(make_sdf_sphere_object("r3-witness-outer-n", "Chamber 3 Outer Halo Witness North", "gallery.marble.white", [0.0, 2.5, 5.5], 0.75, role="halo_outer_peak"))
    objects.append(make_sdf_sphere_object("r3-witness-outer-s", "Chamber 3 Outer Halo Witness South", "gallery.marble.white", [0.0, 2.5, -5.5], 0.75, role="halo_outer_peak"))

    # -------------------------------------------------------------------------
    # E. Chamber 4 — Multi-Lobe Chamber (Center: X = 20, Z = 0)
    # -------------------------------------------------------------------------
    # Triad of radiant altar nodes:
    # Lobe 1 (North): (20.0, 2.5, 4.0)
    objects.append(make_sdf_cylinder_object("r4-pedestal-lobe1", "Chamber 4 Lobe 1 North Altar", "gallery.pedestal.stone", [20.0, 0.9, 4.0], 1.0, 0.9))
    objects.append(make_sdf_sphere_object("r4-witness-lobe1", "Chamber 4 Lobe 1 Focal Witness", "gallery.marble.white", [20.0, 2.6, 4.0], 0.85, role="lobe_peak_alpha"))

    # Lobe 2 (Southwest): (16.5, 2.5, -3.0)
    objects.append(make_sdf_cylinder_object("r4-pedestal-lobe2", "Chamber 4 Lobe 2 SW Altar", "gallery.pedestal.stone", [16.5, 0.9, -3.0], 1.0, 0.9))
    objects.append(make_sdf_sphere_object("r4-witness-lobe2", "Chamber 4 Lobe 2 Focal Witness", "gallery.marble.white", [16.5, 2.6, -3.0], 0.85, role="lobe_peak_beta"))

    # Lobe 3 (Southeast): (23.5, 2.5, -3.0)
    objects.append(make_sdf_cylinder_object("r4-pedestal-lobe3", "Chamber 4 Lobe 3 SE Altar", "gallery.pedestal.stone", [23.5, 0.9, -3.0], 1.0, 0.9))
    objects.append(make_sdf_sphere_object("r4-witness-lobe3", "Chamber 4 Lobe 3 Focal Witness", "gallery.marble.white", [23.5, 2.6, -3.0], 0.85, role="lobe_peak_gamma"))

    # Central Triad Saddle Witness at (20.0, 2.0, 0.0) — shows valley between lobes
    objects.append(make_sdf_cylinder_object("r4-pedestal-saddle", "Chamber 4 Central Saddle Pedestal", "gallery.pedestal.stone", [20.0, 0.6, 0.0], 0.8, 0.6))
    objects.append(make_sdf_sphere_object("r4-witness-saddle", "Chamber 4 Inter-Lobe Saddle Witness", "gallery.marble.white", [20.0, 2.0, 0.0], 0.75, role="lobe_saddle_valley"))

    # -------------------------------------------------------------------------
    # F. Chamber 5 — Procedural / Cathedral Chamber (Center: X = 40, Z = 0)
    # -------------------------------------------------------------------------
    # Monumental central cathedral pillar & sanctuary witness
    objects.append(make_sdf_cylinder_object("r5-cathedral-monument-pedestal", "Chamber 5 Cathedral Rotunda Base", "gallery.pedestal.stone", [40.0, 1.1, 0.0], 1.6, 1.1))
    objects.append(make_sdf_sphere_object("r5-cathedral-monument-witness", "Chamber 5 Procedural Apex Witness", "gallery.marble.white", [40.0, 2.9, 0.0], 0.95, role="procedural_apex"))

    # Surrounding cathedral colonnade array displaying procedural caustic light waves
    objects.append(make_sdf_cylinder_object("r5-col-ne", "Chamber 5 Colonnade NE", "gallery.marble.white", [43.5, 2.2, 3.5], 0.45, 2.2, role="procedural_colonnade"))
    objects.append(make_sdf_cylinder_object("r5-col-nw", "Chamber 5 Colonnade NW", "gallery.marble.white", [36.5, 2.2, 3.5], 0.45, 2.2, role="procedural_colonnade"))
    objects.append(make_sdf_cylinder_object("r5-col-se", "Chamber 5 Colonnade SE", "gallery.marble.white", [43.5, 2.2, -3.5], 0.45, 2.2, role="procedural_colonnade"))
    objects.append(make_sdf_cylinder_object("r5-col-sw", "Chamber 5 Colonnade SW", "gallery.marble.white", [36.5, 2.2, -3.5], 0.45, 2.2, role="procedural_colonnade"))

    # East Terminal Apse Wall Panel
    objects.append(make_sdf_box_object("r5-apse-wall", "Chamber 5 Terminal Apse Wall", "gallery.marble.white", [51.5, 2.5, 0.0], [0.4, 2.5, 7.5], role="backdrop"))

    # -------------------------------------------------------------------------
    # G. Materials and Spatial Root Document Assembly
    # -------------------------------------------------------------------------
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

    ast_def = make_gallery_ast_definition()

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
                "displayName": {"t": "string", "v": "Radiance Gallery Unified Light Field"},
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
                "astDefinition": ast_def
            },
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
        "injected_by": "Gemini Spark — OntoMath Radiance Rung 3 Showcase Zone (Radiance Gallery)"
    }

    world_doc = {
        "identifier": "radiance_gallery",
        "name": "radiance_gallery",
        "owner": "Zach",
        "description": "OntoMath Radiance Rung 3 Demonstration Gallery — 5 distinct authored scalar light fields",
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
        "injected_by": "Gemini Spark — OntoMath Radiance Rung 3 Showcase Zone"
    }

    return zone_doc, world_doc

def main():
    repo_root = "/Users/zacharyzhang/Documents/GitHub/Earthcall"
    zone_doc, world_doc = build_radiance_gallery_zone()

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
    num_pieces = len(zone_doc["spatialRoot"]["field"]["astDefinition"]["pieces"])
    print(f"[Radiance Gallery] Successfully generated 5-room exhibition with {num_objects} objects across {num_pieces} authored scalar field pieces.")

if __name__ == "__main__":
    main()
