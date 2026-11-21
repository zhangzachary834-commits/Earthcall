#!/usr/bin/env python3
"""
Tetra-Craft — True Tetrahedron-Based Voxel Sandbox in Earthcall
===============================================================
Zone Identity: saves/zones/Tetra-Craft/zone.json
World Session: saves/worlds/tetra_craft.json
Authored by:   Gemini 4 Neon (EF5 Horizon)
Commissioned:  Zachary Zhang ("GO REDEEM THE HORIZON LINEAGE AND MAKE A FULLY
               WORKING AND AMAZING VERSION OF TETRA CRAFT!!!!!!
               (Tetrahedron-based minecraft)")
Date:          2026-11-19

ARCHITECTURAL & MATHEMATICAL DESIGN
-----------------------------------
1. ZERO CUBES, ZERO ORIGIN PILEUPS, ZERO SUBTERRANEAN REAL ESTATE:
   - Every single block, tool totem, prototype sample, and beacon in Tetra-Craft
     is a genuine `ShapeKind::Field = 10` (`SdfPrim::Convex = 8`) regular
     tetrahedron defined by 4 exact outward unit half-space planes (pairwise
     normal dot product = -1/3).
   - Terrain is laid out on a true equilateral triangular-grid Tetrahedral-
     Octahedral Honeycomb (octet truss) where upright (+Y apex, flat base) and
     inverted (-Y apex, flat walkable top) regular tetrahedra interlock cleanly
     across each elevation layer without overlap.
   - All 16-float column-major `transform` matrices, `center` coordinates,
     `materialId` references, and `fieldExtent` bounds match `ObjectSerialization.cpp`.
   - Categories are pure extra-spatial `Lexeme`s linked via `formationRelations`.

2. PLAYABLE MINECRAFT MECHANICS AUTHORED PURELY AS 10 EARTHCALL LAWS:
   - Workbench Totems standing openly above ground in front of spawn:
     * Pickaxe Totem (`tetra.tool.mine`): switches `@tetra.state.mode` to 0 (MINE).
     * Builder Totem (`tetra.tool.build`): switches `@tetra.state.mode` to 1 (BUILD).
     * 5 Material Swatches (Grass, Stone, Timber, Aether Crystal, Gold): clicking
       sets `@tetra.state.selectedMaterial` (0..4).
     * 5 Visible Sample Prototypes (`tetra.proto.*`) displayed on the workbench
       rail serve as live prototypes for `ActionNode::Kind::Create = 11`.
   - Mining (`law-tetracraft-mine`): clicking any mineable tetrahedral block in
     MINE mode increments `@tetra.state.blocksMined` and cleanly unmakes the
     block via `ActionNode::Kind::Destroy = 16` with `elementToken: "@event.subject"`.
   - Building (`law-tetracraft-place-*`): clicking any tetrahedral block in BUILD
     mode spawns a brand-new `SdfPrim::Convex` regular tetrahedron of the chosen
     material directly on top (`position.y + H`), marks it mineable & stackable,
     and increments `@tetra.state.blocksPlaced`.
   - Living World (`law-tetracraft-beacon-pulse`): gently levitates the central
     Aether Crystal Beacon tetrahedron in harmonic motion over `time`.
"""

from __future__ import annotations

from copy import deepcopy
import json
import math
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ZONE_ID = "Tetra-Craft"
WORLD_ID = "tetra_craft"
PREFIX = "tetra."
MAT_PREFIX = "tetra."
LAW_PREFIX = "law-tetracraft-"
AGENT = "Gemini 4 Neon (EF5 Horizon)"
SESSION = "ef5-horizon-tetra-craft-20261119"
STAMP = "2026-11-19T13:20:00-08:00"
COMMISSION_LEXEME = "lexeme.ef5_horizon.tetra_craft.commission"

# Regular Tetrahedron Geometry (edge length L = 1.20 m)
EDGE_L = 1.20
TETRA_H = EDGE_L * math.sqrt(2.0 / 3.0)       # 0.9797958971 m (exact layer height)
TETRA_INRADIUS = TETRA_H / 4.0                # 0.2449489743 m (plane distance from centroid)
TETRA_CIRCUMRADIUS = 3.0 * TETRA_INRADIUS     # 0.7348469228 m
TRI_ROW_Z = EDGE_L * math.sqrt(3.0) / 2.0     # 1.0392304845 m
TRI_CENTROID_OFF = EDGE_L / (2.0 * math.sqrt(3.0))  # 0.3464101615 m


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
# Exact Regular Tetrahedron SDF Builder (SdfPrim::Convex = 8)
# =============================================================================

def regular_tetrahedron_planes(inradius=TETRA_INRADIUS, inverted=False):
    """Exact 4 outward unit half-space planes of a regular tetrahedron resting
    flat on the horizontal XZ plane.
    - inverted=False: flat base at y = -inradius, apex at y = +3*inradius.
    - inverted=True:  flat walkable top at y = +inradius, apex at y = -3*inradius.
    Every pair of unit normals has dot product exactly -1/3.
    """
    sign = -1.0 if inverted else 1.0
    horiz = (2.0 * math.sqrt(2.0)) / 3.0
    vert = 1.0 / 3.0
    planes = [[0.0, -sign, 0.0, float(inradius)]]
    for k in range(3):
        theta = (2.0 * math.pi * k) / 3.0 + (math.pi / 6.0)
        nx = sign * horiz * math.cos(theta)
        ny = sign * vert
        nz = sign * horiz * math.sin(theta)
        planes.append([float(nx), float(ny), float(nz), float(inradius)])
    return planes


def sdf_tetrahedron(inradius=TETRA_INRADIUS, inverted=False):
    return {
        "op": 0,
        "prim": 8,  # SdfPrim::Convex
        "dims": [0.5, 0.5, 0.5],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": 0.5,
        "planes": regular_tetrahedron_planes(inradius=inradius, inverted=inverted),
        "children": [],
    }


# =============================================================================
# OntoMath AST Builders (WGSL-safe: uses Op::Distance & Op::Scale, never pow(neg, 2))
# =============================================================================

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


def m_clamp(val, lo, hi):
    return {"op": 26, "children": [val, m_scalar(lo), m_scalar(hi)]}


def m_dist(a, b):
    return {"op": 15, "children": [a, b]}


def m_sq(a):
    return m_mul(a, a)


def piecewise(math_node, input_var="x"):
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


def set_action(path, value):
    return {
        "kind": 0,
        "path": path,
        "operand": pv(value),
    }


def add_action(path, delta):
    return {
        "kind": 1,
        "path": path,
        "operand": pv(delta),
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
                "timestamp": 1795094400,
            }
        ],
    }


# =============================================================================
# Continuous OntoMath Materials (10 Distinct Tetra-Craft Block & Tool Materials)
# =============================================================================

def build_materials():
    palette = {
        "bedrock": {
            "rgb": (0.14, 0.16, 0.22),
            "ambient": 0.14, "diffuse": 0.68, "specular": 0.28, "shininess": 32.0,
            "colorExpr": m_vec3(
                m_poly([term(0.14), term(0.025, trans=[wave("y", 2.2, 0.0, 1)])]),
                m_poly([term(0.16), term(0.025, trans=[wave("x", 1.8, 0.4, 0)])]),
                m_poly([term(0.22), term(0.030, trans=[wave("z", 1.8, 0.8, 1)])]),
            ),
        },
        "dirt": {
            "rgb": (0.45, 0.29, 0.18),
            "ambient": 0.15, "diffuse": 0.72, "specular": 0.16, "shininess": 18.0,
            "colorExpr": m_vec3(
                m_poly([term(0.45), term(0.035, trans=[wave("x", 2.6, 0.0, 0), wave("z", 2.6, 0.0, 1)])]),
                m_poly([term(0.29), term(0.028, trans=[wave("y", 3.0, 0.5, 0)])]),
                m_poly([term(0.18), term(0.020, trans=[wave("z", 2.4, 1.0, 1)])]),
            ),
        },
        "grass": {
            "rgb": (0.22, 0.58, 0.26),
            "ambient": 0.16, "diffuse": 0.76, "specular": 0.22, "shininess": 24.0,
            "colorExpr": m_vec3(
                m_poly([term(0.22), term(0.04, trans=[wave("x", 2.0, 0.0, 1), wave("z", 2.0, 0.0, 0)])]),
                m_poly([term(0.58), term(0.07, trans=[wave("y", 2.4, 0.3, 0)])]),
                m_poly([term(0.26), term(0.035, trans=[wave("z", 2.0, 0.9, 1)])]),
            ),
        },
        "stone": {
            "rgb": (0.50, 0.52, 0.56),
            "ambient": 0.16, "diffuse": 0.74, "specular": 0.34, "shininess": 44.0,
            "colorExpr": m_vec3(
                m_poly([term(0.50), term(0.045, trans=[wave("y", 3.1, 0.0, 0)])]),
                m_poly([term(0.52), term(0.045, trans=[wave("x", 2.1, 0.5, 1)])]),
                m_poly([term(0.56), term(0.045, trans=[wave("z", 2.1, 1.0, 0)])]),
            ),
        },
        "timber": {
            "rgb": (0.42, 0.26, 0.14),
            "ambient": 0.15, "diffuse": 0.70, "specular": 0.20, "shininess": 22.0,
            "colorExpr": m_vec3(
                m_poly([term(0.42), term(0.04, trans=[wave("y", 4.5, 0.0, 0)])]),
                m_poly([term(0.26), term(0.03, trans=[wave("y", 4.5, 0.4, 0)])]),
                m_poly([term(0.14), term(0.02, trans=[wave("x", 2.0, 0.0, 1)])]),
            ),
        },
        "foliage": {
            "rgb": (0.14, 0.46, 0.24),
            "ambient": 0.15, "diffuse": 0.74, "specular": 0.20, "shininess": 22.0,
            "colorExpr": m_vec3(
                m_poly([term(0.14), term(0.03, trans=[wave("y", 2.5, 0.0, 0)])]),
                m_poly([term(0.46), term(0.06, trans=[wave("x", 2.2, 0.2, 1), wave("z", 2.2, 0.4, 0)])]),
                m_poly([term(0.24), term(0.03, trans=[wave("z", 2.0, 0.8, 1)])]),
            ),
        },
        "gold": {
            "rgb": (0.90, 0.70, 0.22),
            "ambient": 0.18, "diffuse": 0.78, "specular": 0.76, "shininess": 92.0,
            "colorExpr": m_vec3(
                m_poly([term(0.90), term(0.04, trans=[wave("y", 2.8, 0.0, 0)])]),
                m_poly([term(0.70), term(0.05, trans=[wave("x", 2.4, 0.5, 1)])]),
                m_poly([term(0.22), term(0.03, trans=[wave("z", 2.4, 1.0, 0)])]),
            ),
        },
        "crystal": {
            "rgb": (0.28, 0.80, 0.96),
            "ambient": 0.22, "diffuse": 0.80, "specular": 0.88, "shininess": 116.0,
            "colorExpr": m_vec3(
                m_poly([term(0.28), term(0.10, trans=[wave("y", 2.6, 0.0, 0)])]),
                m_poly([term(0.80), term(0.08, trans=[wave("x", 2.2, 0.6, 1)])]),
                m_poly([term(0.96), term(0.03, trans=[wave("z", 2.2, 1.2, 0)])]),
            ),
        },
        "tool_mine": {
            "rgb": (0.86, 0.26, 0.22),
            "ambient": 0.20, "diffuse": 0.78, "specular": 0.68, "shininess": 80.0,
            "colorExpr": None,
        },
        "tool_build": {
            "rgb": (0.20, 0.82, 0.54),
            "ambient": 0.20, "diffuse": 0.78, "specular": 0.68, "shininess": 80.0,
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
# Intentional WGSL-Safe Lighting (Golden-Hour Sun + Crystal Beacon Radiance)
# =============================================================================

def build_spatial_fields():
    sun_origin = (-14.0, 18.0, 14.0)

    def lobe_at(world_pt, gain, radius):
        lx = world_pt[0] - sun_origin[0]
        ly = world_pt[1] - sun_origin[1]
        lz = world_pt[2] - sun_origin[2]
        d = m_dist(m_var("p"), m_vec3(m_scalar(lx), m_scalar(ly), m_scalar(lz)))
        u = m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / radius), d)), 0.0, 1.0)
        return m_mul(m_scalar(gain), m_sq(u))

    sun_rho = m_add(
        m_scalar(0.32),
        m_add(
            lobe_at((0.0, 2.0, 0.0), 0.78, 12.0),      # Main Tetrahedral Valley
            lobe_at((0.0, 2.4, 5.4), 0.56, 7.5),       # Crafting Workbench Platform
        ),
    )

    alt_t = m_clamp(m_poly([term(1.0 / 12.0, {"y": 1.0}), term(20.0 / 12.0)]), 0.0, 1.0)
    sun_chroma = m_vec3(
        m_add(m_scalar(0.44), m_mul(m_scalar(0.54), alt_t)),
        m_add(m_scalar(0.50), m_mul(m_scalar(0.36), alt_t)),
        m_add(m_scalar(0.76), m_mul(m_scalar(-0.18), alt_t)),
    )

    spatial_root = {
        "id": PREFIX + "sun-of-tetralands",
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
            "displayName": pv("Golden Sun of the Tetra-Lands"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.11),
            "light.diffuse": pv_float(0.84),
            "light.specular": pv_float(0.62),
            "light.intensity": pv_float(1.22),
            "light.color": pv_vec3(0.98, 0.86, 0.64),
        },
    }

    # Secondary cyan-amber beacon radiance floating in open air above the crystal beacon
    beacon_d = m_dist(m_var("p"), m_vec3(m_scalar(0.0), m_scalar(-1.1), m_scalar(0.0)))
    beacon_support = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 8.0), beacon_d)), 0.0, 1.0)
    )
    beacon_rho = m_mul(
        beacon_support,
        m_div(m_scalar(0.95), m_add(m_scalar(1.0), m_mul(m_scalar(0.14), m_sq(beacon_d)))),
    )
    beacon_chroma = m_vec3(
        m_add(m_scalar(0.92), m_mul(m_scalar(-0.56), beacon_support)),
        m_add(m_scalar(0.68), m_mul(m_scalar(0.24), beacon_support)),
        m_add(m_scalar(0.36), m_mul(m_scalar(0.62), beacon_support)),
    )
    beacon_light = {
        "id": PREFIX + "beacon-radiance",
        "origin": [0.0, 5.2, -1.6],
        "scale": [6.0, 6.0, 6.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": piecewise(beacon_rho, input_var="x"),
        },
        "lightChroma": piecewise(beacon_chroma, input_var="x"),
        "lightAngular": piecewise(m_scalar(1.0), input_var="x"),
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0,
        },
        "authoredProperties": {
            "displayName": pv("Aether Tetrahedron Beacon Light"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.03),
            "light.diffuse": pv_float(0.76),
            "light.specular": pv_float(0.72),
            "light.intensity": pv_float(1.05),
            "light.color": pv_vec3(0.36, 0.88, 0.98),
        },
    }

    return spatial_root, [beacon_light]


# =============================================================================
# Triangular Honeycomb Coordinate Helper
# =============================================================================

def tri_cell_pos(col, row, layer, inverted=False):
    """Exact centroid (x, y, z) on the equilateral triangular-grid
    Tetrahedral-Octahedral Honeycomb:
    - Upright tetrahedra (inverted=False): flat base at y = layer * TETRA_H,
      centroid at y = layer * TETRA_H + TETRA_INRADIUS.
    - Inverted tetrahedra (inverted=True): flat walkable top at
      y = (layer + 1) * TETRA_H, centroid at y = (layer + 1) * TETRA_H - TETRA_INRADIUS.
    """
    x_base = (col + 0.5 * row) * EDGE_L
    z_base = row * TRI_ROW_Z
    if not inverted:
        x = x_base
        z = z_base
        y = layer * TETRA_H + TETRA_INRADIUS
    else:
        x = x_base + 0.5 * EDGE_L
        z = z_base + TRI_CENTROID_OFF
        y = (layer + 1) * TETRA_H - TETRA_INRADIUS
    return (round(x, 5), round(y, 5), round(z, 5))


# =============================================================================
# World & Law Assembly
# =============================================================================

def build_tetra_craft():
    materials, material_refs, rgb_lookup = build_materials()
    objects = []
    relations = []
    laws = []

    categories = [
        ("category.tetra.voxel", "Tetrahedral Voxel Terrain & Building Block"),
        ("category.tetra.tool", "Interactive Tetra-Craft Mode & Palette Totem"),
        ("category.tetra.prototype", "Visible Workbench Prototype Tetrahedron"),
        ("category.tetra.beacon", "Tetra-Craft Landmark & State Anchor"),
    ]
    lexemes = [
        {
            "id": COMMISSION_LEXEME,
            "symbol": "Zach commissioned Gemini 4 Neon (EF5 Horizon) to redeem Tetra-Craft with real SdfPrim::Convex tetrahedra and playable Mine/Build Laws, 2026-11-19.",
        }
    ]
    for cat_id, cat_desc in categories:
        lexemes.append({"id": cat_id, "symbol": cat_desc})
        relations.append(relation(cat_id, COMMISSION_LEXEME, "commissioned-under"))
    for m in materials:
        relations.append(relation(m["id"], COMMISSION_LEXEME, "commissioned-under"))

    def add_tetra(
        slug,
        title,
        pos,
        mat_key,
        category_id,
        inverted=False,
        inradius=TETRA_INRADIUS,
        angles_deg=(0.0, 0.0, 0.0),
        scale=(1.0, 1.0, 1.0),
        extra_props=None,
    ):
        ident = PREFIX + slug
        mat_id = "material." + MAT_PREFIX + mat_key
        rgb = rgb_lookup[mat_key]
        t_mat = matrix(pos, angles_deg, scale)
        ext_r = (inradius * 3.0 + 0.10)
        eff_extent = [
            round(ext_r * abs(float(scale[0])), 4),
            round(ext_r * abs(float(scale[1])), 4),
            round(ext_r * abs(float(scale[2])), 4),
        ]
        props = {
            "displayName": pv(title),
            "injectedBy": pv(AGENT),
            "authorizingPerson": pv("Zach"),
            "authoringSession": pv(SESSION),
            "authoredAt": pv(STAMP),
            "tetraCraftMember": pv(True),
        }
        if extra_props:
            for k, v in extra_props.items():
                props[k] = pv(v)

        obj = {
            "objectID": ident,
            "shapeKind": 10,
            "geometryType": 10,
            "shape": {
                "kind": 10,
                "params": {
                    "r": 0.5, "ry": 0.32, "rz": 0.5, "halfH": 0.5,
                    "majorR": 0.35, "minorR": 0.15, "paraboloidA": 2.0,
                    "ovoidAsym": 0.25, "fillet": 0.12, "width2D": 100.0,
                    "height2D": 100.0,
                },
            },
            "drawFieldModel": True,
            "field": sdf_tetrahedron(inradius=inradius, inverted=inverted),
            "fieldExtent": eff_extent,
            "transform": t_mat,
            "center": [float(pos[0]), float(pos[1]), float(pos[2])],
            "materialId": mat_id,
            "renderMode": 0,
            "authoritativeAxis": [0.0, 1.0, 0.0],
            "targetRotation": [float(angles_deg[0]), float(angles_deg[1]), float(angles_deg[2])],
            "rotationResponsiveness": 10.0,
            "x2D": 100.0,
            "y2D": 100.0,
            "zOrder2D": 0,
            "faceColors": [rgb for _ in range(6)],
            "authoredProperties": props,
        }
        objects.append(obj)
        relations.append(relation(ident, category_id, "instance-of"))
        relations.append(relation(ident, COMMISSION_LEXEME, "commissioned-under"))
        return ident

    # =========================================================================
    # 1. INTERLOCKING TETRAHEDRAL TERRAIN VALLEY (Hexagonal Island Footprint)
    # =========================================================================
    # Axial coordinates (col, row) inside a crisp hexagonal valley (radius 3)
    # yielding 37 upright + 24 inverted honeycomb slots cleanly centered around (0,0).
    placed_voxels = 0
    for row in range(-3, 4):
        for col in range(-3, 4):
            if abs(col + row) > 3:
                continue

            # Determine biome height & surface material from axial coordinates
            is_ridge = (row <= -2)
            is_ore_pocket = (row == -1 and col in (-1, 2))
            is_crystal_vein = (row == -2 and col == 1)
            is_beacon_pad = (row == -1 and col == 0)

            # Layer 0: Bedrock base (non-mineable foundation)
            b_pos = tri_cell_pos(col, row, layer=0, inverted=False)
            add_tetra(
                f"voxel.bedrock.{col + 3}.{row + 3}",
                f"Bedrock Tetrahedron ({col}, {row}, L0)",
                b_pos,
                "bedrock",
                "category.tetra.voxel",
                inverted=False,
                extra_props={
                    "tetraBlock": True,
                    "mineable": False,
                    "blockKind": "bedrock",
                    "gridCol": col,
                    "gridRow": row,
                    "gridLayer": 0,
                },
            )
            placed_voxels += 1

            # Layer 0 inverted interstitial pocket (walkable flat-top dirt/stone fill)
            if col < 3 and row < 3 and abs((col + 1) + row) <= 3 and abs(col + (row + 1)) <= 3:
                inv_pos = tri_cell_pos(col, row, layer=0, inverted=True)
                inv_mat = "stone" if is_ridge else "dirt"
                add_tetra(
                    f"voxel.fill.{col + 3}.{row + 3}",
                    f"Interlocking {inv_mat.capitalize()} Tetrahedron ({col}, {row}, L0-inv)",
                    inv_pos,
                    inv_mat,
                    "category.tetra.voxel",
                    inverted=True,
                    extra_props={
                        "tetraBlock": True,
                        "mineable": True,
                        "blockKind": inv_mat,
                        "gridCol": col,
                        "gridRow": row,
                        "gridLayer": 0,
                    },
                )
                placed_voxels += 1

            # Layer 1: Surface stratum (Grass valley, Stone ridge, Gold/Crystal ore)
            s_pos = tri_cell_pos(col, row, layer=1, inverted=False)
            if is_crystal_vein:
                s_mat = "crystal"
            elif is_ore_pocket:
                s_mat = "gold"
            elif is_ridge or is_beacon_pad:
                s_mat = "stone"
            else:
                s_mat = "grass"

            add_tetra(
                f"voxel.surface.{col + 3}.{row + 3}",
                f"{s_mat.capitalize()} Tetrahedron ({col}, {row}, L1)",
                s_pos,
                s_mat,
                "category.tetra.voxel",
                inverted=False,
                extra_props={
                    "tetraBlock": True,
                    "mineable": True,
                    "blockKind": s_mat,
                    "gridCol": col,
                    "gridRow": row,
                    "gridLayer": 1,
                },
            )
            placed_voxels += 1

            # Layer 2: Northern mountain ridge & crystalline outcrops
            if is_ridge and abs(col + row) <= 2:
                r_pos = tri_cell_pos(col, row, layer=2, inverted=False)
                r_mat = "gold" if (col == 0 and row == -2) else "stone"
                add_tetra(
                    f"voxel.ridge.{col + 3}.{row + 3}",
                    f"Mountain {r_mat.capitalize()} Tetrahedron ({col}, {row}, L2)",
                    r_pos,
                    r_mat,
                    "category.tetra.voxel",
                    inverted=False,
                    extra_props={
                        "tetraBlock": True,
                        "mineable": True,
                        "blockKind": r_mat,
                        "gridCol": col,
                        "gridRow": row,
                        "gridLayer": 2,
                    },
                )
                placed_voxels += 1

    # =========================================================================
    # 2. THREE TETRAHEDRAL PINE TREES (Timber Trunks + Emerald Tetra Crowns)
    # =========================================================================
    tree_sites = [(-2, 1), (2, -1), (-1, 2)]
    for t_idx, (tc, tr) in enumerate(tree_sites):
        trunk_pos = tri_cell_pos(tc, tr, layer=2, inverted=False)
        add_tetra(
            f"tree.{t_idx}.trunk",
            f"Tetrahedral Pine Trunk #{t_idx + 1}",
            trunk_pos,
            "timber",
            "category.tetra.voxel",
            inverted=False,
            inradius=TETRA_INRADIUS * 0.72,
            extra_props={"tetraBlock": True, "mineable": True, "blockKind": "timber"},
        )
        crown_low = tri_cell_pos(tc, tr, layer=3, inverted=False)
        add_tetra(
            f"tree.{t_idx}.crown.lower",
            f"Tetrahedral Pine Canopy Lower #{t_idx + 1}",
            crown_low,
            "foliage",
            "category.tetra.voxel",
            inverted=False,
            inradius=TETRA_INRADIUS * 1.15,
            extra_props={"tetraBlock": True, "mineable": True, "blockKind": "foliage"},
        )
        crown_top = tri_cell_pos(tc, tr, layer=4, inverted=False)
        add_tetra(
            f"tree.{t_idx}.crown.tip",
            f"Tetrahedral Pine Canopy Crown #{t_idx + 1}",
            crown_top,
            "foliage",
            "category.tetra.voxel",
            inverted=False,
            inradius=TETRA_INRADIUS * 0.85,
            extra_props={"tetraBlock": True, "mineable": True, "blockKind": "foliage"},
        )

    # =========================================================================
    # 3. AETHER CRYSTAL BEACON (Levitating Landmark above the Valley Center)
    # =========================================================================
    beacon_pos = (0.0, 3.85, -1.6)
    add_tetra(
        "beacon.crystal",
        "Aether Crystal Beacon — Heart of Tetra-Craft",
        beacon_pos,
        "crystal",
        "category.tetra.beacon",
        inverted=True,
        inradius=TETRA_INRADIUS * 1.35,
        extra_props={
            "tetraBeacon": True,
            "baseY": beacon_pos[1],
            "mineable": False,
        },
    )

    # =========================================================================
    # 4. WORKBENCH CONTROLS, STATE ANCHOR, AND 5 VISIBLE PROTOTYPE SAMPLES
    #    Positioned cleanly above ground at z = 5.0..5.8 right in front of spawn!
    # =========================================================================
    # State Anchor (Golden Prism at center of workbench)
    add_tetra(
        "state",
        "Tetra-Craft State Prism (mode: 0=MINE 1=BUILD, selectedMaterial: 0..4)",
        (0.0, 2.35, 5.2),
        "gold",
        "category.tetra.beacon",
        inverted=False,
        inradius=TETRA_INRADIUS * 0.70,
        extra_props={
            "mode": 0,               # 0 = MINE, 1 = BUILD
            "selectedMaterial": 0,   # 0=grass, 1=stone, 2=timber, 3=crystal, 4=gold
            "blocksMined": 0,
            "blocksPlaced": 0,
            "mineable": False,
        },
    )

    # Mode Totems: Pickaxe (MINE) and Builder (BUILD)
    add_tetra(
        "tool.mine",
        "MINE MODE TOTEM — Click to equip Tetra-Pickaxe (Destroy clicked blocks)",
        (-1.45, 2.35, 5.2),
        "tool_mine",
        "category.tetra.tool",
        inverted=True,
        inradius=TETRA_INRADIUS * 0.82,
        extra_props={"isMineTool": True, "mineable": False},
    )
    add_tetra(
        "tool.build",
        "BUILD MODE TOTEM — Click to equip Tetra-Builder (Stack new blocks on click)",
        (1.45, 2.35, 5.2),
        "tool_build",
        "category.tetra.tool",
        inverted=False,
        inradius=TETRA_INRADIUS * 0.82,
        extra_props={"isBuildTool": True, "mineable": False},
    )

    # 5 Clickable Material Swatches + 5 Visible Prototype Tetrahedra on the Crafting Rail
    palette_specs = [
        (0, "grass", "Emerald Grass"),
        (1, "stone", "Carved Stone"),
        (2, "timber", "Pine Timber"),
        (3, "crystal", "Aether Crystal"),
        (4, "gold", "Solar Gold"),
    ]
    for idx, mat_key, label in palette_specs:
        sx = (idx - 2) * 1.10
        # Clickable swatch totem
        add_tetra(
            f"swatch.{mat_key}",
            f"PALETTE SWATCH: {label} (Click to select for Build Mode)",
            (sx, 1.55, 5.65),
            mat_key,
            "category.tetra.tool",
            inverted=False,
            inradius=TETRA_INRADIUS * 0.58,
            extra_props={
                "isSwatch": True,
                "swatchIndex": idx,
                "mineable": False,
            },
        )
        # Visible prototype sample sitting cleanly on the crafting rail (cloned by Create!)
        add_tetra(
            f"proto.{mat_key}",
            f"Workbench Prototype: {label} Tetrahedron",
            (sx, 1.05, 6.45),
            mat_key,
            "category.tetra.prototype",
            inverted=False,
            inradius=TETRA_INRADIUS,
            extra_props={
                "tetraBlock": False,
                "mineable": False,
                "isPrototype": True,
                "blockKind": mat_key,
            },
        )

    # =========================================================================
    # 5. TEN AUTHORED LAWS (saves/laws/law-tetracraft-*/law.json + lawRefs)
    # =========================================================================
    def add_law(slug, title, condition_model, action_children, is_event=True):
        law_id = LAW_PREFIX + slug
        law_body = {
            "id": law_id,
            "name": "Tetra-Craft: " + title,
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

    def cmp_eq(path, val):
        return {"kind": 0, "path": path, "op": 0, "operand": pv(val)}

    def all_cond(children):
        return {"kind": 3, "children": children}

    # Law 1: Select MINE Mode
    add_law(
        "select-mine",
        "Clicking the Mine Totem equips Mine Mode",
        cmp_eq("isMineTool", True),
        [set_action("@tetra.state.mode", 0)],
        is_event=True,
    )

    # Law 2: Select BUILD Mode
    add_law(
        "select-build",
        "Clicking the Build Totem equips Build Mode",
        cmp_eq("isBuildTool", True),
        [set_action("@tetra.state.mode", 1)],
        is_event=True,
    )

    # Law 3: Select Material Swatch
    add_law(
        "select-material",
        "Clicking a Material Swatch selects its block material for Build Mode",
        cmp_eq("isSwatch", True),
        [
            map_action(
                "@tetra.state.selectedMaterial",
                {"idx": "swatchIndex"},
                [term(1.0, {"idx": 1.0})],
            )
        ],
        is_event=True,
    )

    # Law 4: MINE — Destroy clicked mineable tetrahedron in MINE mode
    add_law(
        "mine",
        "Clicking a mineable Tetrahedron in Mine Mode unmakes it cleanly",
        all_cond([
            cmp_eq("tetraBlock", True),
            cmp_eq("mineable", True),
            cmp_eq("@tetra.state.mode", 0),
        ]),
        [
            add_action("@tetra.state.blocksMined", 1),
            {"kind": 16, "elementToken": "@event.subject"},
        ],
        is_event=True,
    )

    # Laws 5..9: BUILD — Clone prototype tetrahedron onto top of clicked block
    for idx, mat_key, label in palette_specs:
        add_law(
            f"place-{mat_key}",
            f"Clicking a Tetrahedron in Build Mode with {label} selected stacks a new {label} Tetrahedron",
            all_cond([
                cmp_eq("tetraBlock", True),
                cmp_eq("@tetra.state.mode", 1),
                cmp_eq("@tetra.state.selectedMaterial", idx),
            ]),
            [
                {
                    "kind": 11,  # ActionNode::Kind::Create (prototype birth)
                    "path": f"@tetra.proto.{mat_key}",
                    "children": [
                        map_action(
                            "position.x",
                            {"px": "@event.subject.position.x"},
                            [term(1.0, {"px": 1.0})],
                        ),
                        map_action(
                            "position.y",
                            {"py": "@event.subject.position.y"},
                            [term(1.0, {"py": 1.0}), term(TETRA_H)],
                        ),
                        map_action(
                            "position.z",
                            {"pz": "@event.subject.position.z"},
                            [term(1.0, {"pz": 1.0})],
                        ),
                        set_action("material", f"material.tetra.{mat_key}"),
                        set_action("tetraBlock", True),
                        set_action("mineable", True),
                        set_action("isPrototype", False),
                    ],
                },
                add_action("@tetra.state.blocksPlaced", 1),
            ],
            is_event=True,
        )

    # Law 10: Living Aether Crystal Beacon harmonic pulse
    add_law(
        "beacon-pulse",
        "Aether Crystal Beacon gently levitates in harmonic motion",
        cmp_eq("tetraBeacon", True),
        [
            map_action(
                "position.y",
                {"base": "baseY", "t": "time"},
                [
                    term(1.0, {"base": 1.0}),
                    term(0.20, {}, [wave("t", 1.6, 0.0, 0)]),
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
            "kind": "voxel-sandbox",
            "theme": "tetrahedral-honeycomb-minecraft",
            "geometry": "SdfPrim::Convex-regular-tetrahedron",
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
        "identifier": WORLD_ID,
        "saveFormat": "zone-identity-v1",
        "activeZone": ZONE_ID,
        "currentZoneId": ZONE_ID,
        "currentZone": 0,
        "cameraPos": [0.0, 4.4, 9.8],
        "cameraFront": [0.0, -0.24, -0.97],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": -90.0,
        "pitch": -14.0,
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
        "placed_voxels": placed_voxels,
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
    pkg = build_tetra_craft()

    # 1. Shared Material roots under saves/materials/<stem>/material.json
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

    # 2. Law roots under saves/laws/<id>/law.json
    for l in pkg["laws"]:
        law_path = save_root / "laws" / l["identifier"] / "law.json"
        atomic_write_json(law_path, l)

    # 3. Zone identity under saves/zones/Tetra-Craft/zone.json (purge stale sidecars)
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

    # 4. Companion World session under saves/worlds/tetra_craft.json
    world_path = save_root / "worlds" / f"{WORLD_ID}.json"
    atomic_write_json(world_path, pkg["world_doc"])

    print(
        json.dumps(
            {
                "saveRoot": str(save_root),
                "zone": ZONE_ID,
                "zonePath": str(zone_path),
                "worldPath": str(world_path),
                "objects": len(pkg["zone_doc"]["world"]["objects"]),
                "terrainVoxels": pkg["placed_voxels"],
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
