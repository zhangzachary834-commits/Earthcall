#!/usr/bin/env python3
"""
Sky-Island Sanctuary — Aetherial Archipelago & Cloister of the Living Dawn
==========================================================================
Zone Identity: saves/zones/Sky-Island Sanctuary/zone.json
World Session: saves/worlds/sky_island_sanctuary.json
Authored by:   Gemini 4 Argon (Spark)
Commissioned:  Zachary Zhang ("Create a beautiful 3D sky-island sanctuary inside
               Earthcall. Make it feel wondrous, peaceful, and architecturally
               intentional. I want it to feel like a real place a person would
               want to enter and linger in.")
Date:          2026-10-13

ARCHITECTURAL & ONTOLOGICAL DESIGN
----------------------------------
1. FIVE FLOATING SKY-ISLANDS WITH SCULPTED CRAG UNDERBELLIES:
   - Main Sanctuary Island (0, 0, 0): Inverted tapering basalt-and-moss crag
     root (SmoothUnion of main cone + 6 cantilevered rock ribs with wind-carved
     CSG hollows), crowned by three concentric cloister terraces, a 24-ray
     gilded compass rose, and a hollowed Lapis Mirror Pool of Still Waters.
   - South Threshold Island (0, -0.4, 21.0): Arrival terrace with twin CSG-carved
     Dawn Lantern Pagodas, a stone contemplation bench, Threshold Stele, and an
     arched stone footbridge with balustrades spanning to the Main Island.
   - East Observatory Island (19.5, 1.4, -2.0): Reached by 4 ascending floating
     stepping stones; houses a bronze-and-ivory Armillary Sundial Gnomon, sunrise
     bench, and Observatory Stele.
   - West Arbor Island (-19.5, 1.0, -2.0): Reached by 4 floating stepping stones;
     cradles the Ancient Sky-Cedar Tree of Life (gnarled root buttresses +
     5-lobe SmoothUnion emerald canopy), meditation bench, and Arbor Stele.
   - North Zenith Sanctum (0, 2.8, -21.5): Reached by 5 ascending stepping stones;
     crowned by the Triple-Arch Bell Campanile, the Great Sky-Bell, and Zenith Stele.
   - 6 Distant Horizon Spire Islands (R = 36..42m) framing the 360-degree vista.

2. THE PAVILION OF THE EIGHT WINDS & LIVING LOTUS HEART:
   - 8 Fluted Travertine Columns with plinths, torus moldings, and CSG scrolled
     capitals supporting a double-ring entablature and 8 CSG-pierced Lancet
     Sky-Arches around an open Oculus Crown Ring with 8 suspended pearl drops.
   - At the center (0, 3.4, 0): 12 CSG-carved concave bronze/verdigris Lotus
     Petals cradling the faceted Octahedral Heart of the Dawn and 3 tilted
     golden Armillary Halos.
   - At the threshold (0, 1.38, 7.6): The Pearl Touchstone of Dawn, standing
     proudly above the floor on a bronze pedestal.

3. FIVE AUTHORED LAWS (in saves/laws/<id>/law.json + lawRefs):
   - `law-sky-sanctuary-touchstone-chime` (OnEvent `object-clicked`): Clicking
     the Pearl Touchstone toggles `awakened` between 0 and 1.
   - `law-sky-sanctuary-resonance-flow` (WhileTrue): Smoothly drives `resonance`
     toward `awakened` via exact exponential decay (`exp(-2.2 * time.delta)`).
   - `law-sky-sanctuary-lotus-bloom` (WhileTrue): Maps all 12 Lotus Petals'
     3D positions and rotations continuously from `resonance` and their radial
     angles so the sanctuary lotus blooms open and gathers closed.
   - `law-sky-sanctuary-heart-ascend` (WhileTrue): Elevates and gently breathes
     the Heart Crystal and Zenith Bell with `resonance` and `time.world`.
   - `law-sky-sanctuary-lantern-drift` (WhileTrue): Gently floats the 8 Aether
     Lantern Orbs and 13 Cloud Stepping Stones in harmonic phase waves.

4. CONTINUOUS ONTOMATH MATERIAL SHADERS (`colorExpr`) & VOLUMETRIC FIELDS:
   - 10 shared + embedded Materials, 7 carrying continuous OntoMath `colorExpr`
     VectorConstruct shaders (veined travertine, striated basalt, patinated
     verdigris, gilded solar bronze, rippling mirror water, radiant cyan-gold
     crystal, and living emerald foliage).
   - `spatialRoot` (`sky.sanctuary.sun-of-dawn`): AST radiant source casting
     warm golden morning light across the five islands.
   - `spatialFields`:
     * `sky.sanctuary.sea-of-clouds`: Billowing Perlin-noise cumulus cloud sea
       beneath the islands (`y = -5.5m`) with forward-weighted scattering.
     * `sky.sanctuary.sunlit-halo-mist`: Participating golden mist inside the
       temple colonnade with an active CSG `volumeOccluder` casting sunbeams.
     * `sky.sanctuary.heart-radiance`: Breathing radiant core inside the Lotus.
"""

from __future__ import annotations

from copy import deepcopy
import json
import math
import os
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ZONE_ID = "Sky-Island Sanctuary"
PREFIX = "sky.sanctuary."
MAT_PREFIX = "sky.sanctuary."
LAW_PREFIX = "law-sky-sanctuary-"
AGENT = "Gemini 4 Argon (Spark)"
SESSION = "argon-sky-island-sanctuary-20261013"
STAMP = "2026-10-13T11:40:00-07:00"
COMMISSION_LEXEME = "lexeme.argon.sky_sanctuary.commission"
PHI = (1.0 + math.sqrt(5.0)) * 0.5


# =============================================================================
# Tagged PropertyValues & Matrix Helpers
# =============================================================================

def pv(val):
    if isinstance(val, bool):
        return {"t": "bool", "v": val}
    if isinstance(val, int) and not isinstance(val, bool):
        return {"t": "int", "v": val}
    if isinstance(val, str):
        return {"t": "string", "v": val}
    return {"t": "double", "v": float(val)}


def pv_float(val):
    return {"t": "float", "v": float(val)}


def pv_vec3(x, y, z):
    return {"t": "vec3", "x": float(x), "y": float(y), "z": float(z)}


def matrix(pos, angles_deg=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    rx, ry, rz = map(math.radians, angles_deg)
    sx, cx = math.sin(rx), math.cos(rx)
    sy, cy = math.sin(ry), math.cos(ry)
    sz, cz = math.sin(rz), math.cos(rz)
    m = [
        cy * cz, cy * sz, -sy, 0.0,
        cz * sx * sy - cx * sz, cx * cz + sx * sy * sz, cy * sx, 0.0,
        sx * sz + cx * cz * sy, cx * sy * sz - cz * sx, cx * cy, 0.0,
        float(pos[0]), float(pos[1]), float(pos[2]), 1.0,
    ]
    for col in range(3):
        for row in range(3):
            m[col * 4 + row] *= float(scale[col])
    return m


# =============================================================================
# SDF Node Builders (Sdf.hpp: SdfOp & SdfPrim)
# =============================================================================
# SdfPrim: 0=Sphere, 1=Box, 2=RoundBox, 3=Ellipsoid, 4=Cylinder(axis Z),
#          5=Cone(axis +Z), 6=Torus(axis Z), 7=Expr, 8=Convex
# SdfOp:   0=Leaf, 1=Morph, 2=Union, 3=Intersect, 4=Subtract, 5=SmoothUnion

def sdf_leaf(prim, dims, offset=(0.0, 0.0, 0.0), p0=0.0, p1=0.0):
    node = {
        "op": 0,
        "prim": int(prim),
        "dims": [float(dims[0]), float(dims[1]), float(dims[2])],
        "offset": [float(offset[0]), float(offset[1]), float(offset[2])],
        "p0": float(p0),
        "p1": float(p1),
        "t": 0.5,
        "children": [],
    }
    return node


def sdf_expr_str(expr_str, dims=(1.0, 1.0, 1.0), offset=(0.0, 0.0, 0.0)):
    return {
        "op": 0,
        "prim": 7,
        "dims": [float(dims[0]), float(dims[1]), float(dims[2])],
        "offset": [float(offset[0]), float(offset[1]), float(offset[2])],
        "p0": 0.0,
        "p1": 0.0,
        "t": 0.5,
        "expr": expr_str,
        "children": [],
    }


def sdf_convex(planes):
    return {
        "op": 0,
        "prim": 8,
        "dims": [0.5, 0.5, 0.5],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": 0.5,
        "planes": [[float(x) for x in p] for p in planes],
        "children": [],
    }


def sdf_csg(op, a, b, t=0.15):
    return {
        "op": int(op),
        "prim": 0,
        "dims": [0.5, 0.5, 0.5],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": float(t),
        "children": [a, b],
    }


def sdf_union_many(nodes, smooth_k=None):
    cur = nodes[0]
    for nxt in nodes[1:]:
        if smooth_k is not None:
            cur = sdf_csg(5, cur, nxt, t=smooth_k)
        else:
            cur = sdf_csg(2, cur, nxt)
    return cur


def make_polyhedron_planes(kind, radius):
    planes = []
    if kind == "octahedron":
        inv = 1.0 / math.sqrt(3.0)
        for sx in (-1.0, 1.0):
            for sy in (-1.0, 1.0):
                for sz in (-1.0, 1.0):
                    planes.append([sx * inv, sy * inv, sz * inv, float(radius)])
    elif kind == "dodecahedron":
        normals = []
        for s1 in (-1.0, 1.0):
            for s2 in (-1.0, 1.0):
                normals.append((0.0, s1, s2 * PHI))
                normals.append((s1, s2 * PHI, 0.0))
                normals.append((s2 * PHI, 0.0, s1))
        for nx, ny, nz in normals:
            inv_l = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
            planes.append([nx * inv_l, ny * inv_l, nz * inv_l, float(radius)])
    return planes


# =============================================================================
# OntoMath AST Builders (ScalarForm.hpp)
# =============================================================================
# MathNode::Op: 0=ScalarLeaf, 1=ValueLeaf, 2=VectorConstruct, 4=Add, 5=Sub,
#               6=Scale, 7=Dot, 11=Length, 15=Distance, 21=Intersection(max),
#               22=Difference, 23=Div, 24=Pow, 25=Abs, 26=Clamp, 27=Sqrt, 29=Noise
# TransFactor::Kind: 0=Sin, 1=Cos, 2=Exp, 3=Ln

def term(c, factors=None, trans=None):
    d = {"c": float(c), "factors": {k: float(v) for k, v in (factors or {}).items()}}
    if trans:
        d["trans"] = trans
    return d


def wave(var, scale=1.0, shift=0.0, kind=0):
    return {"kind": int(kind), "var": str(var), "scale": float(scale), "shift": float(shift)}


def m_scalar(val):
    return {"op": 0, "scalarForm": {"terms": [term(val)]}}


def m_poly(terms):
    return {"op": 0, "scalarForm": {"terms": terms}}


def m_var(name):
    return {"op": 1, "variableName": name, "var": name}


def m_vec3(r, g, b):
    return {"op": 2, "children": [r, g, b]}


def m_add(a, b):
    return {"op": 4, "children": [a, b]}


def m_sub(a, b):
    return {"op": 5, "children": [a, b]}


def m_mul(a, b):
    return {"op": 6, "children": [a, b]}


def m_div(a, b):
    return {"op": 23, "children": [a, b]}


def m_pow(a, b):
    return {"op": 24, "children": [a, b]}


def m_abs(a):
    return {"op": 25, "children": [a]}


def m_clamp(val, lo, hi):
    return {"op": 26, "children": [val, m_scalar(lo), m_scalar(hi)]}


def m_dist(a, b):
    return {"op": 15, "children": [a, b]}


def m_sq(a):
    # Hardware-safe squaring via Op::Scale (a * a), avoiding WGSL pow(neg, 2) -> NaN
    return m_mul(a, a)


def m_noise(vec_node):
    return {"op": 29, "children": [vec_node]}


def piecewise(math_node, input_var="x"):
    # Always use scalar axis ("x" or "y"), never "p", as piece-bound variable.
    return {
        "input": input_var,
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": math_node}],
    }


def map_action(path, bindings, terms):
    return {
        "kind": 8,
        "path": path,
        "bindings": bindings,
        "function": {"pieces": [{"expr": {"terms": terms}}]},
    }


def relation(a, b, rel_type):
    return {
        "entityA": a,
        "entityB": b,
        "type": rel_type,
        "directed": True,
        "weight": 1.0,
        "events": [
            {
                "description": rel_type,
                "deltaWeight": 1.0,
                "timestamp": 1791916800,
            }
        ],
    }


# =============================================================================
# Materials (Shared Roots + Embedded Closure with Continuous OntoMath colorExpr)
# =============================================================================

def build_materials():
    palette = {
        "travertine": {
            "rgb": (0.76, 0.70, 0.61),
            "ambient": 0.16, "diffuse": 0.74, "specular": 0.30, "shininess": 42.0,
            "colorExpr": m_vec3(
                m_poly([term(0.76), term(0.045, trans=[wave("y", 3.2, 0.0, 0), wave("x", 1.4, 0.0, 1)])]),
                m_poly([term(0.70), term(0.040, trans=[wave("y", 3.2, 0.6, 0), wave("z", 1.4, 0.0, 1)])]),
                m_poly([term(0.61), term(0.035, trans=[wave("y", 3.2, 1.2, 0)])]),
            ),
        },
        "basalt": {
            "rgb": (0.14, 0.17, 0.24),
            "ambient": 0.14, "diffuse": 0.70, "specular": 0.32, "shininess": 36.0,
            "colorExpr": m_vec3(
                m_poly([term(0.14), term(0.025, trans=[wave("y", 1.8, 0.0, 1)])]),
                m_poly([term(0.17), term(0.028, trans=[wave("y", 1.8, 0.4, 1)])]),
                m_poly([term(0.24), term(0.032, trans=[wave("x", 1.1, 0.0, 0)])]),
            ),
        },
        "gold": {
            "rgb": (0.88, 0.66, 0.22),
            "ambient": 0.18, "diffuse": 0.76, "specular": 0.74, "shininess": 88.0,
            "colorExpr": m_vec3(
                m_poly([term(0.88), term(0.04, trans=[wave("y", 2.6, 0.0, 0)])]),
                m_poly([term(0.66), term(0.05, trans=[wave("x", 2.0, 0.5, 1)])]),
                m_poly([term(0.22), term(0.03, trans=[wave("z", 2.0, 1.0, 0)])]),
            ),
        },
        "bronze": {
            "rgb": (0.66, 0.40, 0.17),
            "ambient": 0.16, "diffuse": 0.72, "specular": 0.60, "shininess": 72.0,
            "colorExpr": m_vec3(
                m_poly([term(0.66), term(0.04, trans=[wave("y", 2.4, 0.0, 0), wave("x", 1.2, 0.0, 1)])]),
                m_poly([term(0.40), term(0.035, trans=[wave("y", 2.4, 0.8, 0)])]),
                m_poly([term(0.17), term(0.025, trans=[wave("z", 1.8, 0.4, 1)])]),
            ),
        },
        "verdigris": {
            "rgb": (0.15, 0.48, 0.43),
            "ambient": 0.16, "diffuse": 0.72, "specular": 0.48, "shininess": 58.0,
            "colorExpr": m_vec3(
                m_poly([term(0.15), term(0.03, trans=[wave("y", 2.2, 0.0, 0)])]),
                m_poly([term(0.48), term(0.06, trans=[wave("x", 1.6, 0.3, 1), wave("z", 1.6, 0.0, 0)])]),
                m_poly([term(0.43), term(0.05, trans=[wave("y", 2.2, 1.1, 0)])]),
            ),
        },
        "water": {
            "rgb": (0.12, 0.46, 0.68),
            "ambient": 0.17, "diffuse": 0.74, "specular": 0.80, "shininess": 104.0,
            "colorExpr": m_vec3(
                m_poly([term(0.12), term(0.04, trans=[wave("x", 3.5, 0.0, 0), wave("z", 3.5, 0.0, 1)])]),
                m_poly([term(0.46), term(0.07, trans=[wave("x", 2.8, 0.7, 1), wave("z", 2.8, 0.4, 0)])]),
                m_poly([term(0.68), term(0.06, trans=[wave("z", 3.5, 1.2, 0)])]),
            ),
        },
        "crystal": {
            "rgb": (0.38, 0.78, 0.94),
            "ambient": 0.22, "diffuse": 0.78, "specular": 0.86, "shininess": 112.0,
            "colorExpr": m_vec3(
                m_poly([term(0.38), term(0.12, trans=[wave("y", 2.5, 0.0, 0)])]),
                m_poly([term(0.78), term(0.09, trans=[wave("x", 2.0, 0.6, 1)])]),
                m_poly([term(0.94), term(0.05, trans=[wave("z", 2.0, 1.2, 0)])]),
            ),
        },
        "foliage": {
            "rgb": (0.16, 0.48, 0.28),
            "ambient": 0.15, "diffuse": 0.72, "specular": 0.22, "shininess": 26.0,
            "colorExpr": m_vec3(
                m_poly([term(0.16), term(0.04, trans=[wave("y", 1.8, 0.0, 0)])]),
                m_poly([term(0.48), term(0.07, trans=[wave("x", 1.5, 0.0, 1), wave("y", 2.1, 0.4, 0)])]),
                m_poly([term(0.28), term(0.04, trans=[wave("z", 1.5, 0.8, 0)])]),
            ),
        },
        "timber": {
            "rgb": (0.36, 0.22, 0.14),
            "ambient": 0.14, "diffuse": 0.68, "specular": 0.20, "shininess": 22.0,
            "colorExpr": None,
        },
        "pearl": {
            "rgb": (0.82, 0.79, 0.74),
            "ambient": 0.18, "diffuse": 0.74, "specular": 0.64, "shininess": 88.0,
            "colorExpr": None,
        },
        "amber": {
            "rgb": (0.94, 0.68, 0.28),
            "ambient": 0.22, "diffuse": 0.78, "specular": 0.70, "shininess": 84.0,
            "colorExpr": None,
        },
    }

    materials = []
    material_refs = []
    rgb_lookup = {}
    for key, spec in palette.items():
        stem = MAT_PREFIX + key
        mat_id = "material." + stem
        rgb = list(spec["rgb"])
        rgb_lookup[key] = rgb
        entry = {
            "id": mat_id,
            "name": stem,
            "baseColor": rgb,
            "opacity": 1.0,
            "ambient": spec["ambient"],
            "diffuse": spec["diffuse"],
            "specular": spec["specular"],
            "shininess": spec["shininess"],
            "textureResolution": 16,
        }
        if spec["colorExpr"] is not None:
            entry["colorExpr"] = piecewise(spec["colorExpr"], input_var="y")
        materials.append(entry)
        material_refs.append(mat_id)
    return materials, material_refs, rgb_lookup


# =============================================================================
# Composite SDF Geometry Templates
# =============================================================================

def make_island_crag_sdf(top_radius, depth):
    """Sculpted sky-island crag underbelly: wide flat-topped plateau tapering
    into a faceted rocky root spire with cantilevered crag buttresses and
    wind-carved undercuts."""
    plateau = sdf_leaf(2, (top_radius * 0.78, 0.45, top_radius * 0.78), offset=(0.0, -0.35, 0.0), p0=0.28)
    upper_mantle = sdf_leaf(3, (top_radius * 0.92, depth * 0.42, top_radius * 0.92), offset=(0.0, -depth * 0.28, 0.0))
    lower_spire = sdf_leaf(3, (top_radius * 0.46, depth * 0.68, top_radius * 0.46), offset=(0.0, -depth * 0.52, 0.0))
    keel_tip = sdf_leaf(3, (top_radius * 0.18, depth * 0.45, top_radius * 0.18), offset=(0.0, -depth * 0.78, 0.0))

    core = sdf_union_many([plateau, upper_mantle, lower_spire, keel_tip], smooth_k=0.65)

    ribs = []
    for i in range(4):
        ang = i * (math.pi * 0.5) + 0.35
        rx = math.cos(ang) * top_radius * 0.62
        rz = math.sin(ang) * top_radius * 0.62
        ribs.append(
            sdf_leaf(3, (top_radius * 0.32, depth * 0.38, top_radius * 0.28), offset=(rx, -depth * 0.34, rz))
        )
    with_ribs = sdf_union_many([core] + ribs, smooth_k=0.45)

    top_cut = sdf_leaf(1, (top_radius * 1.5, 2.0, top_radius * 1.5), offset=(0.0, 2.05, 0.0))
    carved = sdf_csg(4, with_ribs, top_cut)
    extent = (top_radius * 1.25 + 0.5, depth * 1.25 + 0.5, top_radius * 1.25 + 0.5)
    return carved, extent


def make_open_lancet_arch_sdf():
    """Pointed Gothic/Celestial lancet arch with real CSG void through the center."""
    outer = sdf_csg(
        3,
        sdf_leaf(4, (3.6, 0.26, 0.0), (-2.05, 0.0, 0.0)),
        sdf_leaf(4, (3.6, 0.26, 0.0), (2.05, 0.0, 0.0)),
    )
    inner = sdf_csg(
        3,
        sdf_leaf(4, (3.28, 0.65, 0.0), (-2.05, 0.0, 0.0)),
        sdf_leaf(4, (3.28, 0.65, 0.0), (2.05, 0.0, 0.0)),
    )
    arch = sdf_csg(3, sdf_csg(4, outer, inner), sdf_leaf(1, (4.5, 3.2, 1.8), (0.0, 1.35, 0.0)))
    return arch, (1.75, 3.3, 0.4)


def make_lotus_petal_sdf():
    """Concave sculpted lotus petal shell via CSG sphere subtraction."""
    outer = sdf_expr_str("sqrt(x*x+y*y+z*z)-1.0")
    inner = sdf_expr_str("sqrt(x*x+(y-0.08)*(y-0.08)+(z-0.38)*(z-0.38))-0.88")
    return sdf_csg(4, outer, inner), (1.08, 1.08, 1.08)


def make_pagoda_lantern_sdf():
    """CSG-carved stone pagoda lantern with hollowed light chamber and curved eaves."""
    pedestal = sdf_leaf(2, (0.42, 0.35, 0.42), offset=(0.0, -0.55, 0.0), p0=0.06)
    chamber_outer = sdf_leaf(2, (0.34, 0.38, 0.34), offset=(0.0, 0.15, 0.0), p0=0.04)
    hole_x = sdf_leaf(1, (0.50, 0.22, 0.18), offset=(0.0, 0.15, 0.0))
    hole_z = sdf_leaf(1, (0.18, 0.22, 0.50), offset=(0.0, 0.15, 0.0))
    chamber = sdf_csg(4, chamber_outer, sdf_csg(2, hole_x, hole_z))
    roof_lower = sdf_leaf(2, (0.58, 0.10, 0.58), offset=(0.0, 0.62, 0.0), p0=0.05)
    roof_cap = sdf_leaf(3, (0.38, 0.24, 0.38), offset=(0.0, 0.82, 0.0))
    finial = sdf_leaf(0, (0.12, 0.12, 0.12), offset=(0.0, 1.08, 0.0))
    body = sdf_union_many([pedestal, chamber, roof_lower, roof_cap, finial], smooth_k=0.06)
    return body, (0.75, 1.35, 0.75)


def make_arched_bridge_sdf():
    """Single-span arched stone bridge deck with cylindrical CSG arch vault beneath."""
    deck_slab = sdf_leaf(2, (1.65, 0.55, 3.2), offset=(0.0, 0.0, 0.0), p0=0.08)
    under_vault = sdf_leaf(3, (2.2, 0.52, 2.35), offset=(0.0, -0.38, 0.0))
    bridge = sdf_csg(4, deck_slab, under_vault)
    return bridge, (1.85, 0.75, 3.4)


# =============================================================================
# Volumetric & Radiant FieldNodes (spatialRoot + spatialFields)
# =============================================================================

def build_spatial_fields():
    # =========================================================================
    # LAYER 1: Primary Spatial Root — Golden-Hour Sunbeam Pools + Cloud Dapple
    # Origin at [-20.0, 24.0, 18.0] (golden-hour SW angle shining cleanly into
    # the open-sky Pavilion colonnade, across the 5 islands, and onto the Sea
    # of Clouds).
    # Remember: SdfWgsl.cpp passes `sourceDelta = pw - source.position.xyz` as `p`
    # to both `lightRadiance(p)` and `lightChroma(p)`!
    # All distance/envelope math uses `m_dist` (Op::Distance) and `m_sq`
    # (Op::Scale `u * u`) — never `pow(signed, 2.0)` — so every octant evaluates
    # cleanly without WGSL `pow(negative, exp) -> NaN`.
    # =========================================================================
    sun_origin = (-20.0, 24.0, 18.0)

    def lobe_at(world_pt, gain, radius):
        lx = world_pt[0] - sun_origin[0]
        ly = world_pt[1] - sun_origin[1]
        lz = world_pt[2] - sun_origin[2]
        d = m_dist(m_var("p"), m_vec3(m_scalar(lx), m_scalar(ly), m_scalar(lz)))
        u = m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / radius), d)), 0.0, 1.0)
        return m_mul(m_scalar(gain), m_sq(u))

    # Five distinct, intentional bright golden sunbeam pools centered on the
    # five islands, rising above a readable morning-sky baseline (0.24) so
    # bridges and stepping stones between islands sit in soft dappled shade
    # (~0.24..0.34) while the 5 island sanctuaries glow (~0.80..1.10).
    sun_dapple = m_poly([
        term(0.88),
        term(0.12, trans=[wave("x", 0.20, 0.0, 0), wave("z", 0.20, 0.0, 1)]),
    ])
    sun_pools = m_add(
        m_scalar(0.24),
        m_add(
            lobe_at((0.0, 2.5, 0.0), 0.86, 13.5),       # Central Pavilion focal sunbeam
            m_add(
                lobe_at((0.0, 0.8, 21.0), 0.72, 11.0),   # South Threshold Pagoda Lanterns
                m_add(
                    lobe_at((19.5, 2.0, -2.0), 0.74, 10.5),   # East Armillary Sundial
                    m_add(
                        lobe_at((-19.5, 2.5, -2.0), 0.68, 10.5), # West Sky-Cedar crown
                        lobe_at((0.0, 4.2, -21.5), 0.76, 11.0),  # North Zenith Campanile
                    ),
                ),
            ),
        ),
    )
    sun_rho = m_mul(sun_dapple, sun_pools)

    # Vertical & horizontal color-temperature transition in sourceDelta coordinates
    # (world y = p.y + 24.0, so p.y = -24 is world y = 0, p.y = -16 is world y = +8,
    # p.y = -30 is world y = -6):
    # High domes/spires get warm apricot-gold [0.98, 0.80, 0.52];
    # Mid terraces get soft rose-amber [0.84, 0.66, 0.56];
    # Deep crag underbellies shift into cool dusk-periwinkle [0.36, 0.44, 0.72].
    alt_t = m_clamp(m_poly([term(1.0 / 16.0, {"y": 1.0}), term(29.0 / 16.0)]), 0.0, 1.0)
    sun_chroma = m_vec3(
        m_add(m_scalar(0.36), m_mul(m_scalar(0.62), alt_t)),
        m_add(m_scalar(0.44), m_mul(m_scalar(0.36), alt_t)),
        m_add(m_scalar(0.72), m_mul(m_scalar(-0.20), alt_t)),
    )
    spatial_root = {
        "id": PREFIX + "sun-of-dawn",
        "origin": list(sun_origin),
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": piecewise(sun_rho, input_var="x"),
        },
        "lightChroma": piecewise(sun_chroma, input_var="x"),
        "lightAngular": piecewise(m_scalar(1.0), input_var="x"),
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0,
        },
        "authoredProperties": {
            "displayName": pv("Celestial Sun of the Living Dawn"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.085),
            "light.diffuse": pv_float(0.84),
            "light.specular": pv_float(0.68),
            "light.intensity": pv_float(1.28),
            "light.color": pv_vec3(0.98, 0.80, 0.54),
            "sanctuary.role": pv("primary-dawn-radiance"),
        },
    }

    # =========================================================================
    # LAYER 2: Participating Medium 1 — Sea of Sunlit Clouds Below the Islands
    # Origin at (0, -5.5, 0), scale [36.0, 2.7, 36.0] (world y in [-8.2, -2.8],
    # cradling the rocky island crag roots while staying strictly below all
    # island decks so horizontal island sightlines remain 100% clear!).
    # Compact support uses `m_dist` and `m_abs` + `m_sq` (zero `pow`), reaching
    # exact 0 at r = 33m (< 36m) and |y| = 2.3m (< 2.7m).
    # Multi-octave Perlin cumulus billows have sunlit golden-peach tops (`y > 0`)
    # and cool twilight-periwinkle undersides (`y < 0`), with bounded sub-unity
    # albedo + self-emission so the cloud sea looks bright and fluffy without
    # ever turning the rest of the world white!
    # =========================================================================
    cloud_r_xz = m_dist(
        m_vec3(m_var("x"), m_scalar(0.0), m_var("z")),
        m_vec3(m_scalar(0.0), m_scalar(0.0), m_scalar(0.0)),
    )
    cloud_horiz_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 33.0), cloud_r_xz)), 0.0, 1.0)
    )
    cloud_vert_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 2.3), m_abs(m_var("y")))), 0.0, 1.0)
    )
    cloud_envelope = m_mul(cloud_horiz_env, cloud_vert_env)

    cloud_n1 = m_noise(m_mul(m_scalar(0.14), m_var("p")))
    cloud_n2 = m_noise(
        m_add(m_mul(m_scalar(0.32), m_var("p")), m_vec3(m_scalar(3.1), m_scalar(1.7), m_scalar(5.3)))
    )
    cloud_puffs = m_clamp(
        m_add(
            m_poly([
                term(0.46),
                term(0.20, trans=[wave("x", 0.22, 0.0, 1), wave("z", 0.22, 0.0, 0)]),
            ]),
            m_add(m_mul(m_scalar(0.42), cloud_n1), m_mul(m_scalar(0.22), cloud_n2)),
        ),
        0.0,
        1.0,
    )
    cloud_density = m_mul(cloud_envelope, cloud_puffs)

    # Optical depth & scattering: sigma_t = 0.42 * D, sigma_s = 0.20 * D,
    # E_v = 0.14 * D * cloud_chroma -> asymptotic ceiling ~ 0.81 * cloud_chroma
    # (bright sunlit cumulus billows that can never clip to #FFFFFF white).
    cloud_ext = m_mul(m_scalar(0.42), cloud_density)
    cloud_scat = m_mul(m_scalar(0.20), cloud_density)

    # Sunlit top vs shaded cloud belly blend (1.0 on upper cloud crests, 0.0 in troughs)
    cloud_top_t = m_clamp(
        m_add(
            m_poly([term(0.55), term(0.36, {"y": 1.0})]),
            m_mul(m_scalar(0.22), cloud_n1),
        ),
        0.0,
        1.0,
    )
    cloud_chroma = m_vec3(
        m_add(m_scalar(0.32), m_mul(m_scalar(0.64), cloud_top_t)),
        m_add(m_scalar(0.40), m_mul(m_scalar(0.42), cloud_top_t)),
        m_add(m_scalar(0.66), m_mul(m_scalar(0.06), cloud_top_t)),
    )
    cloud_emission = m_mul(m_mul(m_scalar(0.14), cloud_density), cloud_chroma)

    sea_of_clouds = {
        "id": PREFIX + "sea-of-clouds",
        "origin": [0.0, -5.5, 0.0],
        "scale": [36.0, 2.7, 36.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 0.65,
            "astDefinition": piecewise(cloud_density, input_var="x"),
        },
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.15, "baseFlowY": 0.0, "baseFlowZ": 0.08,
            "frequency": 1.0, "amplitude": 0.2,
        },
        "volumeDensity": piecewise(cloud_density, input_var="x"),
        "volumeExtinction": piecewise(cloud_ext, input_var="x"),
        "volumeScattering": piecewise(cloud_scat, input_var="x"),
        "volumeChroma": piecewise(cloud_chroma, input_var="x"),
        "volumeEmission": piecewise(cloud_emission, input_var="x"),
        "authoredProperties": {
            "displayName": pv("Sea of Sunlit Cumulus Clouds"),
            "light.source": pv(False),
            "sanctuary.role": pv("sub-island-cloud-sea"),
        },
    }

    # =========================================================================
    # LAYER 3: Participating Medium 2 — Fluted Clerestory Sunbeam Mist
    # Origin at (0, 3.5, 0), scale [5.0, 2.7, 5.0].
    # Compact support vanishes at r=4.0m (<5.0m) and |y|=2.1m (<2.7m) via
    # `m_dist` and `m_abs` + `m_sq`.
    # =========================================================================
    mist_r_xz = m_dist(
        m_vec3(m_var("x"), m_scalar(0.0), m_var("z")),
        m_vec3(m_scalar(0.0), m_scalar(0.0), m_scalar(0.0)),
    )
    mist_horiz_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 4.0), mist_r_xz)), 0.0, 1.0)
    )
    mist_vert_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 2.1), m_abs(m_var("y")))), 0.0, 1.0)
    )
    mist_flutes = m_clamp(
        m_poly([
            term(0.26),
            term(0.18, trans=[wave("x", 1.1, 0.0, 1), wave("z", 1.1, 0.0, 1)]),
        ]),
        0.0,
        1.0,
    )
    mist_density = m_mul(m_mul(mist_horiz_env, mist_vert_env), mist_flutes)
    mist_ext = m_mul(m_scalar(0.024), mist_density)
    mist_scat = m_mul(m_scalar(0.009), mist_density)
    mist_y_t = m_clamp(m_poly([term(1.0 / 4.2, {"y": 1.0}), term(0.5)]), 0.0, 1.0)
    mist_chroma = m_vec3(
        m_add(m_scalar(0.24), m_mul(m_scalar(0.72), mist_y_t)),
        m_add(m_scalar(0.50), m_mul(m_scalar(0.32), mist_y_t)),
        m_add(m_scalar(0.72), m_mul(m_scalar(-0.18), mist_y_t)),
    )
    mist_emission = m_mul(m_mul(m_scalar(0.008), mist_density), mist_chroma)

    occ_slab = sdf_leaf(1, (4.6, 0.40, 4.6), offset=(0.0, 1.9, 0.0))
    occ_hole_center = sdf_leaf(4, (2.0, 1.0, 0.0), offset=(0.0, 1.9, 0.0))
    occ_hole_east = sdf_leaf(1, (1.1, 1.0, 1.8), offset=(2.8, 1.9, 0.0))
    occ_hole_west = sdf_leaf(1, (1.1, 1.0, 1.8), offset=(-2.8, 1.9, 0.0))
    occ_cutouts = sdf_csg(2, occ_hole_center, sdf_csg(2, occ_hole_east, occ_hole_west))
    mist_occluder = sdf_csg(4, occ_slab, occ_cutouts)

    sunlit_mist = {
        "id": PREFIX + "sunlit-halo-mist",
        "origin": [0.0, 3.5, 0.0],
        "scale": [5.0, 2.7, 5.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 0.35,
            "astDefinition": piecewise(mist_density, input_var="x"),
        },
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.05, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.1,
        },
        "volumeDensity": piecewise(mist_density, input_var="x"),
        "volumeExtinction": piecewise(mist_ext, input_var="x"),
        "volumeScattering": piecewise(mist_scat, input_var="x"),
        "volumeChroma": piecewise(mist_chroma, input_var="x"),
        "volumeEmission": piecewise(mist_emission, input_var="x"),
        "volumeOccluder": mist_occluder,
        "authoredProperties": {
            "displayName": pv("Sanctuary Clerestory Sunbeam Mist"),
            "light.source": pv(False),
            "sanctuary.role": pv("temple-sunbeam-medium"),
        },
    }

    # =========================================================================
    # LAYER 4: Secondary Radiant FieldNode 1 — Lotus Heart Sanctuary Lantern
    # Origin at (0, 4.8, 0) in open air just above the Heart Crystal (0, 3.4, 0)
    # so `sourceVisibility` has an unobstructed line of sight to all 12 Lotus
    # Petals, the Mirror Pool, and the inner colonnade across all 360 degrees.
    # Uses `m_dist` (Op::Distance) so all 8 octants evaluate cleanly!
    # =========================================================================
    heart_d = m_dist(
        m_var("p"),
        m_vec3(m_scalar(0.0), m_scalar(-1.4), m_scalar(0.0)),
    )
    heart_support = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 9.0), heart_d)), 0.0, 1.0)
    )
    heart_pulse = m_poly([
        term(0.88),
        term(0.12, trans=[wave("t", 1.5, 0.0, 0)]),
    ])
    heart_rho = m_mul(
        heart_support,
        m_div(heart_pulse, m_add(m_scalar(1.0), m_mul(m_scalar(0.12), m_sq(heart_d)))),
    )
    heart_chroma = m_vec3(
        m_add(m_scalar(0.96), m_mul(m_scalar(-0.48), heart_support)),
        m_add(m_scalar(0.58), m_mul(m_scalar(0.36), heart_support)),
        m_add(m_scalar(0.24), m_mul(m_scalar(0.74), heart_support)),
    )
    heart_radiance = {
        "id": PREFIX + "heart-radiance",
        "origin": [0.0, 4.8, 0.0],
        "scale": [6.5, 6.5, 6.5],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": piecewise(heart_rho, input_var="x"),
        },
        "lightChroma": piecewise(heart_chroma, input_var="x"),
        "lightAngular": piecewise(m_scalar(1.0), input_var="x"),
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0,
        },
        "authoredProperties": {
            "displayName": pv("Heart of the Dawn Radiant Core"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.02),
            "light.diffuse": pv_float(0.84),
            "light.specular": pv_float(0.76),
            "light.intensity": pv_float(1.18),
            "light.color": pv_vec3(0.52, 0.90, 0.98),
            "sanctuary.role": pv("lotus-heart-beacon"),
        },
    }

    # =========================================================================
    # LAYER 5: Secondary Radiant FieldNode 2 — Aether Iris Counter-Light Ring
    # Origin at (12.0, 14.0, -16.0) in the northeastern twilight sky opposite
    # the golden sun, casting a cool aquamarine/cerulean counter-light ring
    # peaking around the outer cloister balustrade, stepping stones, and shaded
    # northern column faces. Uses `m_dist` + `m_sq` (zero `pow`).
    # =========================================================================
    iris_origin = (12.0, 14.0, -16.0)
    iris_world_r = m_dist(
        m_vec3(
            m_poly([term(1.0, {"x": 1.0}), term(iris_origin[0])]),
            m_scalar(0.0),
            m_poly([term(1.0, {"z": 1.0}), term(iris_origin[2])]),
        ),
        m_vec3(m_scalar(0.0), m_scalar(0.0), m_scalar(0.0)),
    )
    iris_inner = m_clamp(m_mul(m_scalar(1.0 / 12.0), iris_world_r), 0.0, 1.0)
    iris_outer = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 28.0), iris_world_r)), 0.0, 1.0)
    )
    iris_wave = m_poly([
        term(0.84),
        term(0.16, trans=[wave("x", 0.25, 0.0, 1), wave("z", 0.25, 0.0, 0)]),
    ])
    iris_rho = m_mul(m_scalar(0.68), m_mul(m_mul(iris_inner, iris_outer), iris_wave))
    iris_chroma = m_vec3(
        m_poly([term(0.20), term(0.06, trans=[wave("z", 0.2, 0.0, 0)])]),
        m_poly([term(0.70), term(0.10, trans=[wave("x", 0.2, 0.0, 1)])]),
        m_poly([term(0.92)]),
    )
    aether_iris_ring = {
        "id": PREFIX + "aether-iris-ring",
        "origin": list(iris_origin),
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": piecewise(iris_rho, input_var="x"),
        },
        "lightChroma": piecewise(iris_chroma, input_var="x"),
        "lightAngular": piecewise(m_scalar(1.0), input_var="x"),
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0,
        },
        "authoredProperties": {
            "displayName": pv("Aether Iris Counter-Light Ring"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.025),
            "light.diffuse": pv_float(0.72),
            "light.specular": pv_float(0.62),
            "light.intensity": pv_float(0.74),
            "light.color": pv_vec3(0.20, 0.70, 0.92),
            "sanctuary.role": pv("cloister-rim-counterlight"),
        },
    }

    return spatial_root, [sea_of_clouds, sunlit_mist, heart_radiance, aether_iris_ring]


# =============================================================================
# Scene Construction: Beings, Categories, Relations, Laws
# =============================================================================

def build_sanctuary():
    materials, material_refs, rgb_lookup = build_materials()
    objects = []
    relations = []
    laws = []

    categories = [
        ("category.sky.sanctuary.island", "Sky-Island Landmass & Crag Foundation"),
        ("category.sky.sanctuary.architecture", "Sanctuary Temple Colonnade & Arches"),
        ("category.sky.sanctuary.bridge", "Arched Bridges & Cloud Stepping Stones"),
        ("category.sky.sanctuary.lotus_petal", "Blooming Lotus Sanctuary Vessel Petals"),
        ("category.sky.sanctuary.lantern", "Floating Aether Lanterns & Pagoda Shrines"),
        ("category.sky.sanctuary.nature", "Living Sky-Cedar & Mirror Pool Waters"),
        ("category.sky.sanctuary.stele", "Inscription Stelae & Sanctuary Lore"),
        ("category.sky.sanctuary.control", "Interactive Sanctuary Touchstones & Bells"),
    ]
    lexemes = [
        {
            "id": COMMISSION_LEXEME,
            "symbol": "Zach commissioned Gemini 4 Argon (Spark) to author the 3D Sky-Island Sanctuary, 2026-10-13.",
        }
    ]
    for cat_id, cat_desc in categories:
        lexemes.append({"id": cat_id, "symbol": cat_desc})
        relations.append(relation(cat_id, COMMISSION_LEXEME, "commissioned-under"))

    for m in materials:
        relations.append(relation(m["id"], COMMISSION_LEXEME, "commissioned-under"))

    def add_obj(
        slug,
        title,
        pos,
        sdf_tree,
        extent,
        mat_key,
        category_id,
        angles_deg=(0.0, 0.0, 0.0),
        scale=(1.0, 1.0, 1.0),
        extra_props=None,
    ):
        ident = PREFIX + slug
        mat_id = "material." + MAT_PREFIX + mat_key
        rgb = rgb_lookup[mat_key]
        t_mat = matrix(pos, angles_deg, scale)
        eff_extent = [
            float(extent[0]) * abs(float(scale[0])),
            float(extent[1]) * abs(float(scale[1])),
            float(extent[2]) * abs(float(scale[2])),
        ]
        props = {
            "displayName": pv(title),
            "injectedBy": pv(AGENT),
            "authorizingPerson": pv("Zach"),
            "authoringSession": pv(SESSION),
            "authoredAt": pv(STAMP),
            "sanctuaryMember": pv(True),
        }
        if extra_props:
            for k, v in extra_props.items():
                props[k] = pv(v)

        obj = {
            "objectID": ident,
            "shapeKind": 10,
            "geometryType": 10,
            "drawFieldModel": True,
            "shapeParams": [1.0, 1.0, 1.0, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
            "field": sdf_tree,
            "fieldExtent": eff_extent,
            "transform": t_mat,
            "center": [float(pos[0]), float(pos[1]), float(pos[2])],
            "materialId": mat_id,
            "renderMode": 0,
            "authoritativeAxis": [0, 1, 0],
            "targetRotation": [float(angles_deg[0]), float(angles_deg[1]), float(angles_deg[2])],
            "rotationResponsiveness": 10.0,
            "x2D": 100,
            "y2D": 100,
            "zOrder2D": 0,
            "faceColors": [rgb for _ in range(6)],
            "authoredProperties": props,
        }
        objects.append(obj)
        relations.append(relation(ident, category_id, "instance-of"))
        relations.append(relation(ident, COMMISSION_LEXEME, "commissioned-under"))
        return ident

    # =========================================================================
    # 1. MAIN SANCTUARY ISLAND ("The Cloister of Stillness", Center (0, 0, 0))
    # =========================================================================
    main_crag_sdf, main_crag_ext = make_island_crag_sdf(14.5, 11.5)
    main_island_id = add_obj(
        "island.main.crag",
        "Sovereign Sky-Island Crag Spire",
        (0.0, -0.05, 0.0),
        main_crag_sdf,
        main_crag_ext,
        "basalt",
        "category.sky.sanctuary.island",
        extra_props={"baseline": "ground", "islandRadius": 14.5, "spireDepth": 11.5},
    )

    # Three Concentric Cloister Terraces & Gilded Inlays (Cylinder axis Z rotated 90 deg X -> Y axis)
    terrace_specs = [
        ("terrace.outer_moss", "Outer Verdant Rim Terrace", (0.0, 0.04, 0.0), 13.8, 0.14, "foliage"),
        ("terrace.middle_court", "Middle Travertine Cloister Court", (0.0, 0.20, 0.0), 11.2, 0.12, "travertine"),
        ("terrace.inner_dais", "Inner Sanctuary Altar Island Dais", (0.0, 0.50, 0.0), 2.85, 0.16, "travertine"),
    ]
    for slug, title, pos, rad, hh, mat_k in terrace_specs:
        tid = add_obj(
            slug, title, pos,
            sdf_leaf(4, (rad, hh, 0.0)), (rad + 0.15, rad + 0.15, hh + 0.10),
            mat_k, "category.sky.sanctuary.architecture",
            angles_deg=(90.0, 0.0, 0.0),
        )
        relations.append(relation(tid, main_island_id, "supported-by"))

    # Hollowed Mirror Pool Basin & Rippling Water Surface (R = 6.2m around the inner dais)
    basin_outer = sdf_leaf(4, (6.4, 0.18, 0.0))
    basin_inner = sdf_leaf(4, (5.85, 0.24, 0.0), offset=(0.0, 0.0, -0.08))
    basin_rim_sdf = sdf_csg(4, basin_outer, basin_inner)
    add_obj(
        "pool.basin_rim",
        "Carved Stone Mirror Pool Coping Rim",
        (0.0, 0.34, 0.0),
        basin_rim_sdf,
        (6.6, 6.6, 0.30),
        "bronze",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )

    # Mirror Pool Water Annulus (CSG Cylinder minus central altar dais)
    water_outer = sdf_leaf(4, (5.82, 0.06, 0.0))
    water_inner = sdf_leaf(4, (2.82, 0.12, 0.0))
    water_sdf = sdf_csg(4, water_outer, water_inner)
    add_obj(
        "pool.still_waters",
        "Mirror Pool of Still Waters",
        (0.0, 0.38, 0.0),
        water_sdf,
        (6.0, 6.0, 0.15),
        "water",
        "category.sky.sanctuary.nature",
        angles_deg=(90.0, 0.0, 0.0),
        extra_props={"waterSurface": True, "reflectivity": 0.92},
    )

    # 3 Gilded Bronze Inlay Rings in the Cloister Pavement
    for idx, r_inlay in enumerate((2.92, 6.45, 10.95)):
        add_obj(
            f"inlay.ring.{idx}",
            f"Gilded Cloister Inlay Ring #{idx + 1}",
            (0.0, 0.34 if idx > 0 else 0.64, 0.0),
            sdf_leaf(6, (r_inlay, 0.045, 0.0)),
            (r_inlay + 0.12, r_inlay + 0.12, 0.12),
            "gold",
            "category.sky.sanctuary.architecture",
            angles_deg=(90.0, 0.0, 0.0),
        )

    # 16 Compass Rose Mosaic Rays on the Middle Cloister Promenade (R = 7.4m)
    for i in range(16):
        ang = 2.0 * math.pi * i / 16.0
        r_ray = 7.45
        rx = r_ray * math.sin(ang)
        rz = r_ray * math.cos(ang)
        ray_len = 0.85 if (i % 2 == 0) else 0.52
        add_obj(
            f"compass.ray.{i:02d}",
            f"Compass of the Dawn Ray #{i + 1}",
            (rx, 0.33, rz),
            sdf_leaf(2, (0.06, 0.025, ray_len), p0=0.015),
            (0.12, 0.06, ray_len + 0.08),
            "gold" if (i % 2 == 0) else "verdigris",
            "category.sky.sanctuary.architecture",
            angles_deg=(0.0, math.degrees(ang), 0.0),
        )

    # =========================================================================
    # 2. THE PAVILION OF THE EIGHT WINDS (8 Columns, 8 Lancet Arches, Oculus)
    # =========================================================================
    arch_sdf, arch_ext = make_open_lancet_arch_sdf()
    col_radius_ring = 8.85
    for i in range(8):
        deg = i * 45.0 + 22.5
        ang = math.radians(deg)
        cx = col_radius_ring * math.sin(ang)
        cz = col_radius_ring * math.cos(ang)

        # Column Plinth & Torus Base
        plinth_id = add_obj(
            f"colonnade.plinth.{i:02d}",
            f"Pavilion Column Plinth #{i + 1}",
            (cx, 0.52, cz),
            sdf_leaf(2, (0.68, 0.22, 0.68), p0=0.06),
            (0.78, 0.30, 0.78),
            "basalt",
            "category.sky.sanctuary.architecture",
            angles_deg=(0.0, deg, 0.0),
        )
        # Fluted Travertine Column Shaft (Cylinder rotated 90 deg X so axis is Y)
        col_id = add_obj(
            f"colonnade.column.{i:02d}",
            f"Pavilion Fluted Travertine Column #{i + 1}",
            (cx, 3.05, cz),
            sdf_leaf(4, (0.42, 2.35, 0.0)),
            (0.55, 0.55, 2.50),
            "travertine",
            "category.sky.sanctuary.architecture",
            angles_deg=(90.0, 0.0, 0.0),
        )
        relations.append(relation(col_id, plinth_id, "supported-by"))

        # Ornate Capital (SmoothUnion of Echinus torus + Abacus slab)
        cap_sdf = sdf_csg(
            5,
            sdf_leaf(2, (0.64, 0.16, 0.64), offset=(0.0, 0.12, 0.0), p0=0.05),
            sdf_leaf(3, (0.56, 0.20, 0.56), offset=(0.0, -0.08, 0.0)),
            t=0.08,
        )
        cap_id = add_obj(
            f"colonnade.capital.{i:02d}",
            f"Pavilion Gilded Column Capital #{i + 1}",
            (cx, 5.52, cz),
            cap_sdf,
            (0.75, 0.42, 0.75),
            "gold",
            "category.sky.sanctuary.architecture",
            angles_deg=(0.0, deg, 0.0),
        )
        relations.append(relation(cap_id, col_id, "supported-by"))

        # Pierced Lancet Sky-Arch spanning between columns
        arch_deg = i * 45.0
        arch_ang = math.radians(arch_deg)
        ax = col_radius_ring * math.sin(arch_ang)
        az = col_radius_ring * math.cos(arch_ang)
        add_obj(
            f"colonnade.arch.{i:02d}",
            f"Pierced Lancet Sky-Arch #{i + 1}",
            (ax, 6.45, az),
            arch_sdf,
            arch_ext,
            "travertine",
            "category.sky.sanctuary.architecture",
            angles_deg=(0.0, arch_deg, 0.0),
            scale=(1.05, 1.35, 1.0),
        )
        # Suspended Verdigris Finial Drop inside each Lancet Arch
        add_obj(
            f"colonnade.arch_drop.{i:02d}",
            f"Lancet Arch Verdigris Pendant #{i + 1}",
            (ax, 8.15, az),
            sdf_leaf(3, (0.16, 0.46, 0.16)),
            (0.22, 0.55, 0.22),
            "verdigris",
            "category.sky.sanctuary.architecture",
        )

    # Double Entablature Cornice Rings & Open Sky Oculus Crown
    add_obj(
        "pavilion.cornice.lower",
        "Pavilion Lower Bronze Architrave Ring",
        (0.0, 5.78, 0.0),
        sdf_leaf(6, (8.85, 0.22, 0.0)),
        (9.2, 9.2, 0.35),
        "bronze",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )
    add_obj(
        "pavilion.cornice.upper",
        "Pavilion Upper Travertine Cornice Ring",
        (0.0, 8.95, 0.0),
        sdf_leaf(6, (8.45, 0.26, 0.0)),
        (8.8, 8.8, 0.40),
        "travertine",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )
    add_obj(
        "pavilion.oculus.crown",
        "Zenith Oculus Solar Crown Ring",
        (0.0, 10.6, 0.0),
        sdf_leaf(6, (4.2, 0.14, 0.0)),
        (4.5, 4.5, 0.25),
        "gold",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )

    # =========================================================================
    # 3. THE LIVING LOTUS HEART & PEARL TOUCHSTONE OF DAWN
    # =========================================================================
    # Touchstone Pedestal & Clickable Pearl at (0.0, 1.38, 7.6) — clearly visible & reachable!
    add_obj(
        "touchstone.pedestal",
        "Pedestal of the Dawn Touchstone",
        (0.0, 0.78, 7.6),
        sdf_csg(
            5,
            sdf_leaf(2, (0.48, 0.38, 0.48), offset=(0.0, -0.10, 0.0), p0=0.08),
            sdf_leaf(3, (0.38, 0.26, 0.38), offset=(0.0, 0.22, 0.0)),
            t=0.08,
        ),
        (0.60, 0.55, 0.60),
        "bronze",
        "category.sky.sanctuary.control",
    )
    touchstone_id = add_obj(
        "touchstone",
        "THE PEARL OF DAWN — Click to Awaken or Gather the Sanctuary Lotus",
        (0.0, 1.38, 7.6),
        sdf_leaf(3, (0.36, 0.28, 0.36)),
        (0.45, 0.38, 0.45),
        "pearl",
        "category.sky.sanctuary.control",
        extra_props={
            "sanctuaryTouchstone": True,
            "awakened": 0.0,
            "resonance": 0.0,
            "description": "Click this Pearl to awaken the twelve Lotus Petals, elevate the Heart of the Dawn, and ring the Zenith Bell. Click again to gather in stillness.",
        },
    )

    # Central Lotus Plinth & Faceted Octahedral Heart Crystal at (0, 3.4, 0)
    add_obj(
        "lotus.plinth",
        "Altar Plinth of the Living Lotus",
        (0.0, 0.96, 0.0),
        sdf_csg(
            5,
            sdf_leaf(2, (1.35, 0.28, 1.35), offset=(0.0, -0.10, 0.0), p0=0.10),
            sdf_leaf(3, (1.05, 0.32, 1.05), offset=(0.0, 0.18, 0.0)),
            t=0.12,
        ),
        (1.55, 0.65, 1.55),
        "travertine",
        "category.sky.sanctuary.architecture",
    )
    octa_planes = make_polyhedron_planes("octahedron", 0.92)
    heart_id = add_obj(
        "heart_crystal",
        "Heart of the Dawn — Levitating Octahedral Crystal",
        (0.0, 3.40, 0.0),
        sdf_convex(octa_planes),
        (1.25, 1.25, 1.25),
        "crystal",
        "category.sky.sanctuary.control",
        extra_props={
            "sanctuaryHeart": True,
            "baseY": 3.40,
        },
    )
    relations.append(relation(touchstone_id, heart_id, "awakens"))

    # 3 Concentric Armillary Halos around the Heart Crystal
    for h_idx, h_angles in enumerate(((25.0, 0.0, 0.0), (0.0, 60.0, 35.0), (-30.0, 120.0, 0.0))):
        add_obj(
            f"heart.halo.{h_idx}",
            f"Heart Armillary Solar Halo #{h_idx + 1}",
            (0.0, 3.40, 0.0),
            sdf_leaf(6, (1.65 + h_idx * 0.28, 0.042, 0.0)),
            (2.4, 2.4, 0.18),
            "gold" if h_idx % 2 == 0 else "bronze",
            "category.sky.sanctuary.architecture",
            angles_deg=h_angles,
            extra_props={
                "sanctuaryHalo": True,
                "haloIndex": float(h_idx),
                "baseTiltX": float(h_angles[0]),
                "baseTiltZ": float(h_angles[2]),
            },
        )

    # 12 Concave Lotus Petals encircling the Heart Crystal
    petal_sdf, petal_ext = make_lotus_petal_sdf()
    for i in range(12):
        deg = i * 30.0
        rad = math.radians(deg)
        sx, cz = math.sin(rad), math.cos(rad)
        px = 1.95 * sx
        pz = 1.95 * cz
        py = 3.25
        pid = add_obj(
            f"petal.{i:02d}",
            f"Lotus Sanctuary Vessel Petal #{i + 1}",
            (px, py, pz),
            petal_sdf,
            petal_ext,
            "bronze" if (i % 2 == 0) else "verdigris",
            "category.sky.sanctuary.lotus_petal",
            angles_deg=(8.0, deg, 0.0),
            scale=(0.68, 2.45, 0.88),
            extra_props={
                "sanctuaryPetal": True,
                "radialX": sx,
                "radialZ": cz,
                "azimuth": deg,
            },
        )
        relations.append(relation(pid, heart_id, "shelters"))

    # =========================================================================
    # 4. SOUTH THRESHOLD ISLAND & ARCHED STONE BRIDGE (Z = +12.2 to +25.5)
    # =========================================================================
    south_crag_sdf, south_crag_ext = make_island_crag_sdf(5.8, 5.2)
    south_island_id = add_obj(
        "island.south.crag",
        "South Threshold Island — Terrace of Arrival",
        (0.0, -0.35, 21.0),
        south_crag_sdf,
        south_crag_ext,
        "basalt",
        "category.sky.sanctuary.island",
    )
    add_obj(
        "island.south.pavement",
        "South Threshold Travertine Terrace",
        (0.0, -0.22, 21.0),
        sdf_leaf(4, (5.2, 0.12, 0.0)),
        (5.4, 5.4, 0.20),
        "travertine",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )

    # Arched Stone Bridge spanning between South Island (Z=21) and Main Island (Z=0)
    bridge_sdf, bridge_ext = make_arched_bridge_sdf()
    bridge_id = add_obj(
        "bridge.south.span",
        "Arched Stone Bridge of the Threshold",
        (0.0, 0.08, 14.8),
        bridge_sdf,
        bridge_ext,
        "travertine",
        "category.sky.sanctuary.bridge",
    )
    relations.append(relation(bridge_id, south_island_id, "spans-between"))
    relations.append(relation(bridge_id, main_island_id, "spans-between"))

    for side, s_label in ((-1.0, "west"), (1.0, "east")):
        add_obj(
            f"bridge.south.parapet.{s_label}",
            f"Threshold Bridge Stone Parapet ({s_label.capitalize()})",
            (side * 1.48, 0.58, 14.8),
            sdf_leaf(2, (0.12, 0.24, 3.05), p0=0.04),
            (0.20, 0.32, 3.20),
            "bronze",
            "category.sky.sanctuary.bridge",
        )

    # Twin Pagoda Lantern Shrines flanking the South Bridge Entrance
    pagoda_sdf, pagoda_ext = make_pagoda_lantern_sdf()
    for side, s_label in ((-1.0, "west"), (1.0, "east")):
        lx = side * 2.85
        lz = 18.6
        add_obj(
            f"shrine.pagoda.{s_label}",
            f"Threshold Dawn Pagoda Lantern ({s_label.capitalize()})",
            (lx, 0.72, lz),
            pagoda_sdf,
            pagoda_ext,
            "travertine",
            "category.sky.sanctuary.lantern",
        )
        add_obj(
            f"shrine.pagoda_flame.{s_label}",
            f"Pagoda Lantern Amber Flame ({s_label.capitalize()})",
            (lx, 0.87, lz),
            sdf_leaf(0, (0.16, 0.16, 0.16)),
            (0.24, 0.24, 0.24),
            "amber",
            "category.sky.sanctuary.lantern",
        )

    # =========================================================================
    # 5. EAST OBSERVATORY ISLAND ("Terrace of the Morning Star", (19.5, 1.4, -2.0))
    # =========================================================================
    east_crag_sdf, east_crag_ext = make_island_crag_sdf(5.4, 5.0)
    add_obj(
        "island.east.crag",
        "East Observatory Sky-Island Crag",
        (19.5, 1.35, -2.0),
        east_crag_sdf,
        east_crag_ext,
        "basalt",
        "category.sky.sanctuary.island",
    )
    add_obj(
        "island.east.pavement",
        "Observatory Star-Dial Court",
        (19.5, 1.48, -2.0),
        sdf_leaf(4, (4.8, 0.12, 0.0)),
        (5.0, 5.0, 0.20),
        "travertine",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )
    # Sundial Gnomon & Equatorial Ring on East Island
    add_obj(
        "observatory.sundial.base",
        "Equatorial Sundial Pedestal",
        (19.5, 2.05, -2.0),
        sdf_leaf(2, (0.55, 0.45, 0.55), p0=0.08),
        (0.68, 0.55, 0.68),
        "bronze",
        "category.sky.sanctuary.architecture",
    )
    add_obj(
        "observatory.sundial.ring",
        "Observatory Equatorial Chrono-Ring",
        (19.5, 3.10, -2.0),
        sdf_leaf(6, (1.35, 0.08, 0.0)),
        (1.6, 1.6, 0.22),
        "gold",
        "category.sky.sanctuary.architecture",
        angles_deg=(35.0, -45.0, 0.0),
    )
    add_obj(
        "observatory.sundial.gnomon",
        "Observatory Celestial Gnomon Needle",
        (19.5, 3.10, -2.0),
        sdf_leaf(3, (0.12, 1.45, 0.12)),
        (0.22, 1.60, 0.22),
        "crystal",
        "category.sky.sanctuary.architecture",
        angles_deg=(0.0, 0.0, 28.0),
    )

    # =========================================================================
    # 6. WEST ARBOR ISLAND ("Grove of Living Wind", (-19.5, 1.0, -2.0))
    # =========================================================================
    west_crag_sdf, west_crag_ext = make_island_crag_sdf(5.6, 5.2)
    add_obj(
        "island.west.crag",
        "West Arbor Sky-Island Crag",
        (-19.5, 0.95, -2.0),
        west_crag_sdf,
        west_crag_ext,
        "basalt",
        "category.sky.sanctuary.island",
    )
    add_obj(
        "island.west.lawn",
        "West Arbor Moss Sanctuary Lawn",
        (-19.5, 1.08, -2.0),
        sdf_leaf(4, (5.0, 0.12, 0.0)),
        (5.2, 5.2, 0.20),
        "foliage",
        "category.sky.sanctuary.nature",
        angles_deg=(90.0, 0.0, 0.0),
    )
    # Ancient Sky-Cedar Tree of Life (Gnarled Trunk + 5-Lobe SmoothUnion Canopy)
    trunk_sdf = sdf_union_many([
        sdf_leaf(3, (0.55, 1.85, 0.55), offset=(0.0, 0.0, 0.0)),
        sdf_leaf(3, (0.85, 0.55, 0.35), offset=(0.35, -1.25, 0.0)),
        sdf_leaf(3, (0.35, 0.55, 0.85), offset=(0.0, -1.25, 0.35)),
    ], smooth_k=0.28)
    add_obj(
        "arbor.tree.trunk",
        "Ancient Sky-Cedar Gnarled Trunk",
        (-19.5, 2.85, -2.4),
        trunk_sdf,
        (1.4, 2.2, 1.4),
        "timber",
        "category.sky.sanctuary.nature",
    )
    canopy_sdf = sdf_union_many([
        sdf_leaf(3, (2.2, 0.95, 2.2), offset=(0.0, 0.0, 0.0)),
        sdf_leaf(3, (1.6, 0.80, 1.6), offset=(1.4, -0.25, 0.6)),
        sdf_leaf(3, (1.6, 0.80, 1.6), offset=(-1.3, -0.20, -0.5)),
        sdf_leaf(3, (1.5, 0.75, 1.5), offset=(0.3, 0.65, -0.8)),
        sdf_leaf(3, (1.2, 0.65, 1.2), offset=(0.0, 1.15, 0.2)),
    ], smooth_k=0.45)
    add_obj(
        "arbor.tree.canopy",
        "Ancient Sky-Cedar Emerald Canopy Crown",
        (-19.5, 5.15, -2.4),
        canopy_sdf,
        (3.4, 2.2, 3.4),
        "foliage",
        "category.sky.sanctuary.nature",
    )

    # =========================================================================
    # 7. NORTH ZENITH SANCTUM ("Altar of the Horizon", (0.0, 2.8, -21.5))
    # =========================================================================
    north_crag_sdf, north_crag_ext = make_island_crag_sdf(6.2, 5.8)
    add_obj(
        "island.north.crag",
        "North Zenith Sanctum Sky-Island Crag",
        (0.0, 2.75, -21.5),
        north_crag_sdf,
        north_crag_ext,
        "basalt",
        "category.sky.sanctuary.island",
    )
    add_obj(
        "island.north.pavement",
        "North Zenith Sanctum Travertine Court",
        (0.0, 2.88, -21.5),
        sdf_leaf(4, (5.5, 0.14, 0.0)),
        (5.7, 5.7, 0.22),
        "travertine",
        "category.sky.sanctuary.architecture",
        angles_deg=(90.0, 0.0, 0.0),
    )
    # Zenith Bell Campanile Arch & Great Resonant Sky-Bell
    add_obj(
        "zenith.campanile.arch",
        "Zenith Campanile Twin-Pillar Arch",
        (0.0, 6.20, -22.5),
        arch_sdf,
        arch_ext,
        "gold",
        "category.sky.sanctuary.architecture",
        scale=(1.25, 1.45, 1.25),
    )
    bell_outer = sdf_csg(
        5,
        sdf_leaf(3, (0.62, 0.78, 0.62), offset=(0.0, 0.10, 0.0)),
        sdf_leaf(2, (0.78, 0.16, 0.78), offset=(0.0, -0.52, 0.0), p0=0.06),
        t=0.18,
    )
    bell_hollow = sdf_leaf(3, (0.48, 0.65, 0.48), offset=(0.0, -0.25, 0.0))
    bell_sdf = sdf_csg(4, bell_outer, bell_hollow)
    bell_id = add_obj(
        "zenith_bell",
        "Great Sky-Bell of the Horizon",
        (0.0, 6.35, -22.5),
        bell_sdf,
        (0.95, 1.05, 0.95),
        "bronze",
        "category.sky.sanctuary.control",
        extra_props={
            "sanctuaryBell": True,
            "baseY": 6.35,
        },
    )
    relations.append(relation(touchstone_id, bell_id, "rings"))

    # =========================================================================
    # 8. FLOATING CLOUD STEPPING STONES (East, West, North Chains)
    # =========================================================================
    stepping_routes = [
        ("east", [(11.8, 0.45, -0.6), (13.6, 0.75, -1.0), (15.4, 1.05, -1.4), (17.2, 1.32, -1.7)]),
        ("west", [(-11.8, 0.35, -0.6), (-13.6, 0.58, -1.0), (-15.4, 0.80, -1.4), (-17.2, 0.98, -1.7)]),
        ("north", [(0.0, 0.75, -11.8), (0.0, 1.22, -13.6), (0.0, 1.68, -15.4), (0.0, 2.15, -17.2), (0.0, 2.58, -19.0)]),
    ]
    step_idx = 0
    for route_name, pts in stepping_routes:
        for j, (sx, sy, sz) in enumerate(pts):
            step_sdf = sdf_csg(
                5,
                sdf_leaf(2, (0.85, 0.14, 0.85), offset=(0.0, 0.05, 0.0), p0=0.06),
                sdf_leaf(3, (0.62, 0.38, 0.62), offset=(0.0, -0.22, 0.0)),
                t=0.10,
            )
            add_obj(
                f"step.{route_name}.{j}",
                f"Floating Cloud Stepping Stone ({route_name.capitalize()} #{j + 1})",
                (sx, sy, sz),
                step_sdf,
                (1.05, 0.55, 1.05),
                "travertine",
                "category.sky.sanctuary.bridge",
                extra_props={
                    "sanctuaryFloatingStep": True,
                    "baseY": sy,
                    "phaseOffset": float(step_idx * 0.65),
                },
            )
            step_idx += 1

    # =========================================================================
    # 9. STONE CONTEMPLATION BENCHES & FOUR PEDAGOGICAL INSCRIPTION STELAE
    # =========================================================================
    benches = [
        ("bench.main.west", "Cloister West Contemplation Bench", (-7.2, 0.58, 4.2), 30.0),
        ("bench.main.east", "Cloister East Contemplation Bench", (7.2, 0.58, 4.2), -30.0),
        ("bench.east.dawn", "Observatory Sunrise Bench", (21.8, 1.84, -2.0), 90.0),
        ("bench.west.arbor", "Sky-Cedar Shade Bench", (-21.6, 1.44, -2.0), -90.0),
    ]
    for b_slug, b_title, b_pos, b_yaw in benches:
        bench_sdf = sdf_union_many([
            sdf_leaf(2, (0.95, 0.10, 0.38), offset=(0.0, 0.12, 0.0), p0=0.04),
            sdf_leaf(2, (0.16, 0.22, 0.32), offset=(-0.68, -0.12, 0.0), p0=0.03),
            sdf_leaf(2, (0.16, 0.22, 0.32), offset=(0.68, -0.12, 0.0), p0=0.03),
        ])
        add_obj(
            b_slug,
            b_title,
            b_pos,
            bench_sdf,
            (1.15, 0.45, 0.52),
            "travertine",
            "category.sky.sanctuary.architecture",
            angles_deg=(0.0, b_yaw, 0.0),
        )

    stelae = [
        (
            "stele.threshold",
            "Stele of the Threshold — The Inversion of Place",
            (2.2, 0.72, 20.2),
            "WELCOME TO THE SKY-ISLAND SANCTUARY. Every crag, fluted column, pierced lancet arch, "
            "mirror pool, and floating lantern here is authored as continuous OntoMath geometry, "
            "surface pigment fields, participating cloud media, and reactive Laws. Walk across the "
            "South Bridge and touch the Pearl of Dawn to awaken the sanctuary.",
        ),
        (
            "stele.observatory",
            "Stele of the Morning Star — Causal Rendering & Light",
            (18.2, 2.42, 0.4),
            "EAST OBSERVATORY TERRACE. Light in this sanctuary descends from authored FieldNodes: "
            "the Sun of the Living Dawn casts spatially bounded scalar and chromatic radiance, while "
            "the Sea of Clouds below and Clerestory Mist within the pavilion scatter and occlude it.",
        ),
        (
            "stele.arbor",
            "Stele of the Living Wind — Organic Form from Pure Mathematics",
            (-18.2, 2.02, 0.4),
            "WEST ARBOR GROVE. The Ancient Sky-Cedar and the five island crags use zero triangle meshes "
            "or hardcoded domain classes: their silhouettes emerge from SmoothUnion and Subtract CSG "
            "compositions paired with continuous OntoMath surface color fields.",
        ),
        (
            "stele.zenith",
            "Stele of the Horizon — Process as Law",
            (2.4, 3.82, -20.2),
            "NORTH ZENITH SANCTUM. When you click the Pearl Touchstone in the main court, five authored "
            "Laws govern the sanctuary: an OnEvent trigger flips intention, exponential flow integrates "
            "resonance over time, and continuous Map laws bloom the 12 Lotus Petals and ring the Sky-Bell.",
        ),
    ]
    for s_slug, s_title, s_pos, s_text in stelae:
        stele_sdf = sdf_csg(
            5,
            sdf_leaf(2, (0.45, 0.85, 0.14), offset=(0.0, 0.10, 0.0), p0=0.05),
            sdf_leaf(2, (0.58, 0.18, 0.26), offset=(0.0, -0.72, 0.0), p0=0.04),
            t=0.06,
        )
        add_obj(
            s_slug,
            s_title,
            s_pos,
            stele_sdf,
            (0.70, 1.10, 0.35),
            "basalt",
            "category.sky.sanctuary.stele",
            extra_props={"stele.inscription": s_text},
        )

    # =========================================================================
    # 10. 8 ORBITING AETHER LANTERN ORBS & 6 DISTANT HORIZON SPIRE ISLANDS
    # =========================================================================
    dodeca_planes = make_polyhedron_planes("dodecahedron", 0.34)
    for i in range(8):
        ang = i * (2.0 * math.pi / 8.0) + 0.39
        r_orb = 11.8
        lx = r_orb * math.sin(ang)
        lz = r_orb * math.cos(ang)
        ly = 4.2 + 0.35 * math.sin(i * 1.5)
        add_obj(
            f"lantern.orb.{i:02d}",
            f"Floating Aether Dodecahedral Lantern #{i + 1}",
            (lx, ly, lz),
            sdf_convex(dodeca_planes),
            (0.52, 0.52, 0.52),
            "amber" if (i % 2 == 0) else "crystal",
            "category.sky.sanctuary.lantern",
            extra_props={
                "sanctuaryLantern": True,
                "baseY": ly,
                "phaseOffset": float(i * 0.785),
            },
        )

    # 6 Distant Horizon Spire Islands framing the sky
    distant_spires = [
        ("spire.northeast", "Northeast Horizon Spire Island", (31.0, 4.5, -26.0), 4.2, 8.5),
        ("spire.northwest", "Northwest Horizon Spire Island", (-31.0, 5.2, -26.0), 4.5, 9.2),
        ("spire.southeast", "Southeast Horizon Spire Island", (29.0, -1.5, 22.0), 3.8, 7.5),
        ("spire.southwest", "Southwest Horizon Spire Island", (-29.0, -1.0, 22.0), 4.0, 7.8),
        ("spire.far_north", "Far North Crown Monolith", (0.0, 7.5, -38.0), 5.0, 11.0),
        ("spire.far_east", "Far East Dawn Outcrop", (37.0, 2.8, 4.0), 3.6, 7.2),
    ]
    for sp_slug, sp_title, sp_pos, sp_r, sp_d in distant_spires:
        sp_sdf, sp_ext = make_island_crag_sdf(sp_r, sp_d)
        add_obj(
            sp_slug,
            sp_title,
            sp_pos,
            sp_sdf,
            sp_ext,
            "basalt",
            "category.sky.sanctuary.island",
        )

    # =========================================================================
    # 11. FIVE AUTHORED LAWS (saves/laws/<id>/law.json + Zone lawRefs)
    # =========================================================================
    def add_law(slug, title, condition_model, action_children, is_event=False):
        law_id = LAW_PREFIX + slug
        law_body = {
            "id": law_id,
            "name": "Sky-Island Sanctuary: " + title,
            "enabled": True,
            "authority": 0,
            "activation": 0 if is_event else 1,
            "scope": 0 if is_event else 1,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach"],
            "conditionSubjects": [],
            "targets": [],
            "applicationLog": [],
            "conditionModel": condition_model,
            "actionModel": {"kind": 5, "children": action_children},
            "provenance": [
                relation(law_id, COMMISSION_LEXEME, "commissioned-under"),
                relation(law_id, "Zach", "authored-by"),
            ],
        }
        laws.append({
            "identifier": law_id,
            "injected_by": {
                "agent": AGENT,
                "session": SESSION,
                "timestamp": STAMP,
            },
            "authors": ["Zach"],
            "law": law_body,
            "triggers": ["object-clicked"] if is_event else [],
        })
        relations.append(relation(law_id, COMMISSION_LEXEME, "commissioned-under"))

    def tagged_bool(prop_name):
        return {"kind": 0, "path": prop_name, "op": 0, "operand": pv(True)}

    # Law 1: Clicking the Pearl Touchstone toggles `awakened` between 0 and 1
    add_law(
        "touchstone-chime",
        "Touchstone toggles the Awakening of the Dawn",
        tagged_bool("sanctuaryTouchstone"),
        [
            map_action(
                "awakened",
                {"q": "awakened"},
                [term(1.0), term(-1.0, {"q": 1.0})],
            )
        ],
        is_event=True,
    )

    # Law 2: Continuous exponential approach of `resonance` toward `awakened`
    exp_decay = [wave("d", -2.2, 0.0, 2)]
    add_law(
        "resonance-flow",
        "Resonance flows smoothly toward awakened intention",
        tagged_bool("sanctuaryTouchstone"),
        [
            map_action(
                "resonance",
                {"a": "resonance", "q": "awakened", "d": "time.delta"},
                [
                    term(1.0, {"q": 1.0}),
                    term(1.0, {"a": 1.0}, exp_decay),
                    term(-1.0, {"q": 1.0}, exp_decay),
                ],
            )
        ],
        is_event=False,
    )

    # Law 3: 12 Lotus Petals unfurl and tilt outward with `resonance`
    petal_binds = {
        "a": "@" + PREFIX + "touchstone.resonance",
        "s": "radialX",
        "c": "radialZ",
        "r": "azimuth",
    }
    petal_rotation = {
        "kind": 8,
        "path": "rotation",
        "bindings": petal_binds,
        "function": {
            "pieces": [
                {
                    "mathNode": {
                        "op": 2,
                        "children": [
                            {"op": 0, "scalarForm": {"terms": [term(8.0), term(36.0, {"a": 1.0})]}},
                            {"op": 0, "scalarForm": {"terms": [term(1.0, {"r": 1.0})]}},
                            {"op": 0, "scalarForm": {"terms": [term(0.0)]}},
                        ],
                    }
                }
            ]
        },
    }
    add_law(
        "lotus-bloom",
        "Twelve Lotus Petals bloom and gather with Sanctuary Resonance",
        tagged_bool("sanctuaryPetal"),
        [
            map_action("position.x", petal_binds, [term(1.95, {"s": 1.0}), term(1.75, {"s": 1.0, "a": 1.0})]),
            map_action("position.z", petal_binds, [term(1.95, {"c": 1.0}), term(1.75, {"c": 1.0, "a": 1.0})]),
            map_action("position.y", petal_binds, [term(3.25), term(-0.42, {"a": 1.0})]),
            petal_rotation,
        ],
        is_event=False,
    )

    # Law 4: Heart Crystal & Zenith Bell elevate and breathe with `resonance` and `time.world`
    heart_binds = {
        "base": "baseY",
        "a": "@" + PREFIX + "touchstone.resonance",
        "t": "time",
    }
    add_law(
        "heart-ascend",
        "Heart of the Dawn ascends and breathes with Resonance",
        tagged_bool("sanctuaryHeart"),
        [
            map_action(
                "position.y",
                heart_binds,
                [
                    term(1.0, {"base": 1.0}),
                    term(0.85, {"a": 1.0}),
                    term(0.16, {}, [wave("t", 1.6, 0.0, 0)]),
                ],
            )
        ],
        is_event=False,
    )

    # Law 5: Floating Aether Lanterns drift in gentle harmonic waves
    lantern_binds = {
        "base": "baseY",
        "t": "time",
        "p": "phaseOffset",
    }
    add_law(
        "lantern-drift",
        "Aether Lanterns float in harmonic sky currents",
        tagged_bool("sanctuaryLantern"),
        [
            map_action(
                "position.y",
                lantern_binds,
                [
                    term(1.0, {"base": 1.0}),
                    term(0.22, {}, [wave("t", 1.2, 0.0, 0), wave("p", 1.0, 0.0, 1)]),
                ],
            )
        ],
        is_event=False,
    )

    spatial_root, spatial_fields = build_spatial_fields()

    zone_doc = {
        "identifier": ZONE_ID,
        "name": ZONE_ID,
        "owner": "Zach",
        "parentZone": "",
        "scope": "Local",
        "deletable": False,
        "authors": ["Zach", AGENT],
        "injected_by": f"{AGENT} · session {SESSION} · {STAMP}",
        "qualities": {
            "kind": "sanctuary",
            "theme": "sky-island-archipelago",
            "atmosphere": "sunlit-dawn-clouds",
        },
        "spatialRoot": spatial_root,
        "spatialFields": spatial_fields,
        "world": {"objects": objects},
        "materials": materials,
        "materialRefs": material_refs,
        "lexemes": lexemes,
        "formationRelations": [r for r in relations if not r["entityA"].startswith(LAW_PREFIX)],
        "lawRefs": [l["identifier"] for l in laws],
    }

    world_doc = {
        "identifier": "sky_island_sanctuary",
        "saveFormat": "zone-identity-v1",
        "activeZone": ZONE_ID,
        "currentZoneId": ZONE_ID,
        "currentZone": 0,
        "cameraPos": [0.0, 2.8, 22.5],
        "cameraFront": [0.0, -0.06, -0.998],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": -90.0,
        "pitch": -3.5,
        "flying": True,
        "worldPhysics": False,
        "worldMode": True,
        "worldTime": 0.0,
        "currentColor": [1.0, 1.0, 1.0],
        "currentTool": 0,
        "zoneRefs": [{"identifier": ZONE_ID, "kind": "zone"}],
        "zones": [deepcopy(zone_doc)],
        "materials": deepcopy(materials),
        "authoredLaws": {
            "laws": [deepcopy(l["law"]) for l in laws],
            "triggers": {l["identifier"]: l["triggers"] for l in laws if l["triggers"]},
            "formationMembers": [l["identifier"] for l in laws],
        },
    }

    return {
        "zone_doc": zone_doc,
        "world_doc": world_doc,
        "materials": materials,
        "laws": laws,
    }


def atomic_write_json(target_path: Path, data: dict):
    target_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        dir=target_path.parent,
        prefix="." + target_path.name + ".staged-",
        delete=False,
    ) as tmp:
        json.dump(data, tmp, indent=2)
        tmp.write("\n")
        tmp_path = Path(tmp.name)
    tmp_path.replace(target_path)


def install(save_root: Path):
    pkg = build_sanctuary()

    # 1. Write shared Material roots under saves/materials/<stem>/material.json
    for m in pkg["materials"]:
        stem = m["name"]
        mat_path = save_root / "materials" / stem / "material.json"
        mat_root_doc = {
            "identifier": m["id"],
            "material": m,
            "authors": ["Zach"],
            "injected_by": {
                "agent": AGENT,
                "session": SESSION,
                "timestamp": STAMP,
            },
        }
        atomic_write_json(mat_path, mat_root_doc)

    # 2. Write Law roots under saves/laws/<id>/law.json
    for l in pkg["laws"]:
        law_path = save_root / "laws" / l["identifier"] / "law.json"
        atomic_write_json(law_path, l)

    # 3. Write Zone identity under saves/zones/Sky-Island Sanctuary/zone.json
    # Remove any stale .ecform/.ecmatter/.ecsave sidecars so resolveIdentityFile
    # always loads the freshly authored zone.json!
    zone_dir = save_root / "zones" / ZONE_ID
    if zone_dir.is_dir():
        for stale in zone_dir.iterdir():
            if stale.name != "zone.json":
                try:
                    stale.unlink()
                except Exception:
                    pass
    zone_path = zone_dir / "zone.json"
    atomic_write_json(zone_path, pkg["zone_doc"])

    # 4. Write companion World session under saves/worlds/sky_island_sanctuary.json
    world_path = save_root / "worlds" / "sky_island_sanctuary.json"
    atomic_write_json(world_path, pkg["world_doc"])

    print(
        json.dumps(
            {
                "zone": ZONE_ID,
                "zonePath": str(zone_path),
                "worldPath": str(world_path),
                "objects": len(pkg["zone_doc"]["world"]["objects"]),
                "materials": len(pkg["materials"]),
                "laws": len(pkg["laws"]),
                "relations": len(pkg["zone_doc"]["formationRelations"]),
                "spatialFields": 1 + len(pkg["zone_doc"]["spatialFields"]),
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    install(ROOT / "saves")
    try:
        from earthcall_data_home import data_root
        home_root = data_root()
        if home_root != (ROOT / "saves") and home_root.is_dir():
            install(home_root)
    except Exception:
        pass
