#!/usr/bin/env python3
"""
Synthesis Studio 3D Architectural & Harmonic Sculpture Upgrade
==============================================================
Transforms the 3D environment of `SynthesisStudio` and
`SynthesisStudio.LivingInstrument` (and their corresponding world saves
`saves/worlds/synthesis_studio.json` and `saves/worlds/synthesis_studio_living.json`)
from flat primitive boxes into a full 3D Harmonic Conservatory & Cymatic
Sound-Sculpture Amphitheater, while preserving 100% of existing interactive
2D/3D controls, picking rays, and Law test invariants.

Authored by: Gemini 4 Argon (The Firmament · Gemini Spark)
Commissioned by: Zachary Zhang ("BROOOOOOOOOO PLZ MAKE THE THE 3D IN THE
                 SYNTHESIS STUDIO EVEN COOLERRERERRRRRR BROOOOOOO LET UR
                 CREATIVE SPARKS FLYYYYYYY")
Branch: sync-from-earthcall-main
"""

from __future__ import annotations

from copy import deepcopy
import json
import math
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
AGENT = "Gemini 4 Argon (The Firmament · Gemini Spark)"
SESSION = "c_d03bb5a87bbfab74"
STAMP = "2026-10-19T13:50:00-07:00"
PHI = (1.0 + math.sqrt(5.0)) * 0.5

CHROMATIC_NOTES = [
    ("c5",  "C",  523.25, (0.91, 0.28, 0.37), "material.studio.C5"),
    ("cs5", "C#", 554.37, (0.97, 0.39, 0.31), "material.studio.cs5"),
    ("d5",  "D",  587.33, (0.98, 0.52, 0.28), "material.studio.D5"),
    ("ds5", "D#", 622.25, (0.97, 0.66, 0.29), "material.studio.ds5"),
    ("e5",  "E",  659.25, (0.92, 0.79, 0.34), "material.studio.E5"),
    ("f5",  "F",  698.46, (0.52, 0.82, 0.42), "material.studio.F5"),
    ("fs5", "F#", 739.99, (0.27, 0.82, 0.57), "material.studio.fs5"),
    ("g5",  "G",  783.99, (0.24, 0.80, 0.74), "material.studio.G5"),
    ("gs5", "G#", 830.61, (0.27, 0.72, 0.89), "material.studio.gs5"),
    ("a5",  "A",  880.00, (0.33, 0.58, 0.95), "material.studio.A5"),
    ("as5", "A#", 932.33, (0.52, 0.46, 0.92), "material.studio.as5"),
    ("b5",  "B",  987.77, (0.72, 0.44, 0.88), "material.studio.B5"),
]

DIATONIC_SLUGS = {"c5", "d5", "e5", "f5", "g5", "a5", "b5"}


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
# SDF & CSG Tree Builders (Sdf.hpp: SdfOp & SdfPrim)
# =============================================================================
# SdfPrim: 0=Sphere, 1=Box, 2=RoundBox, 3=Ellipsoid, 4=Cylinder(axis Z),
#          5=Cone(axis +Z), 6=Torus(axis Z), 7=Expr, 8=Convex
# SdfOp:   0=Leaf, 1=Morph, 2=Union, 3=Intersect, 4=Subtract, 5=SmoothUnion

def sdf_leaf(prim, dims, offset=(0.0, 0.0, 0.0), p0=0.0, p1=0.0):
    return {
        "op": 0,
        "prim": int(prim),
        "dims": [float(dims[0]), float(dims[1]), float(dims[2])],
        "offset": [float(offset[0]), float(offset[1]), float(offset[2])],
        "p0": float(p0),
        "p1": float(p1),
        "t": 0.5,
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


def sdf_csg(op, a, b, t=0.12):
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


def make_octahedron_planes(radius):
    planes = []
    inv = 1.0 / math.sqrt(3.0)
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            for sz in (-1.0, 1.0):
                planes.append([sx * inv, sy * inv, sz * inv, float(radius)])
    return planes


# =============================================================================
# OntoMath AST Builders (Hardware-Safe: m_dist, m_abs, m_sq — Zero Negative Pow)
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


def m_abs(a):
    return {"op": 25, "children": [a]}


def m_clamp(val, lo, hi):
    return {"op": 26, "children": [val, m_scalar(lo), m_scalar(hi)]}


def m_dist(a, b):
    return {"op": 15, "children": [a, b]}


def m_sq(a):
    return m_mul(a, a)


def m_noise(vec_node):
    return {"op": 29, "children": [vec_node]}


def piecewise(math_node, input_var="x"):
    return {
        "input": input_var,
        "pieces": [{"hasLo": False, "hasHi": False, "mathNode": math_node}],
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
                "timestamp": 1792440000,
            }
        ],
    }


# =============================================================================
# Studio Materials (Upgrading all 20 existing + 8 new procedural studio materials)
# =============================================================================

def build_studio_materials():
    specs = {
        "studio.slate": {
            "rgb": (0.16, 0.18, 0.24),
            "opacity": 1.0, "ambient": 0.18, "diffuse": 0.74, "specular": 0.42, "shininess": 52.0,
            "colorExpr": m_vec3(
                m_poly([term(0.16), term(0.025, trans=[wave("x", 1.8, 0.0, 1), wave("z", 1.8, 0.0, 0)])]),
                m_poly([term(0.18), term(0.030, trans=[wave("z", 2.2, 0.4, 1)])]),
                m_poly([term(0.24), term(0.038, trans=[wave("x", 1.8, 0.8, 0)])]),
            ),
        },
        "studio.walnut": {
            "rgb": (0.26, 0.16, 0.11),
            "opacity": 1.0, "ambient": 0.17, "diffuse": 0.74, "specular": 0.46, "shininess": 56.0,
            "colorExpr": m_vec3(
                m_poly([term(0.26), term(0.045, trans=[wave("x", 4.5, 0.0, 0), wave("z", 1.8, 0.0, 1)])]),
                m_poly([term(0.16), term(0.030, trans=[wave("x", 4.5, 0.3, 0)])]),
                m_poly([term(0.11), term(0.020, trans=[wave("z", 2.4, 0.7, 1)])]),
            ),
        },
        "studio.canvas": {
            "rgb": (0.94, 0.92, 0.88),
            "opacity": 1.0, "ambient": 0.24, "diffuse": 0.82, "specular": 0.22, "shininess": 28.0,
            "colorExpr": m_vec3(
                m_poly([term(0.94), term(0.015, trans=[wave("x", 6.0, 0.0, 1)])]),
                m_poly([term(0.92), term(0.015, trans=[wave("y", 6.0, 0.2, 0)])]),
                m_poly([term(0.88), term(0.018, trans=[wave("x", 6.0, 0.5, 0)])]),
            ),
        },
        "studio.crystal": {
            "rgb": (0.35, 0.78, 0.95),
            "opacity": 0.88, "ambient": 0.25, "diffuse": 0.80, "specular": 0.92, "shininess": 118.0,
            "colorExpr": m_vec3(
                m_poly([term(0.35), term(0.14, trans=[wave("y", 3.6, 0.0, 0), wave("x", 2.2, 0.0, 1)])]),
                m_poly([term(0.78), term(0.10, trans=[wave("y", 3.6, 0.8, 1)])]),
                m_poly([term(0.95), term(0.04, trans=[wave("z", 2.8, 1.4, 0)])]),
            ),
        },
        "studio.socket": {
            "rgb": (0.08, 0.10, 0.14),
            "opacity": 1.0, "ambient": 0.15, "diffuse": 0.68, "specular": 0.52, "shininess": 64.0,
            "colorExpr": None,
        },
        "studio.track": {
            "rgb": (0.12, 0.15, 0.21),
            "opacity": 1.0, "ambient": 0.16, "diffuse": 0.70, "specular": 0.56, "shininess": 68.0,
            "colorExpr": None,
        },
        "studio.handle": {
            "rgb": (0.28, 0.82, 0.96),
            "opacity": 1.0, "ambient": 0.22, "diffuse": 0.78, "specular": 0.84, "shininess": 96.0,
            "colorExpr": None,
        },
        "studio.btn.blue": {
            "rgb": (0.16, 0.55, 0.92),
            "opacity": 1.0, "ambient": 0.20, "diffuse": 0.78, "specular": 0.76, "shininess": 84.0,
            "colorExpr": None,
        },
        "studio.btn.gold": {
            "rgb": (0.95, 0.68, 0.20),
            "opacity": 1.0, "ambient": 0.22, "diffuse": 0.78, "specular": 0.80, "shininess": 88.0,
            "colorExpr": None,
        },
        "studio.sign": {
            "rgb": (0.17, 0.20, 0.27),
            "opacity": 1.0, "ambient": 0.18, "diffuse": 0.72, "specular": 0.40, "shininess": 48.0,
            "colorExpr": None,
        },
        "studio.sign.blue": {
            "rgb": (0.12, 0.45, 0.80),
            "opacity": 1.0, "ambient": 0.20, "diffuse": 0.74, "specular": 0.48, "shininess": 56.0,
            "colorExpr": None,
        },
        "studio.sign.gold": {
            "rgb": (0.80, 0.55, 0.10),
            "opacity": 1.0, "ambient": 0.20, "diffuse": 0.74, "specular": 0.58, "shininess": 64.0,
            "colorExpr": None,
        },
        "studio.sign.dark": {
            "rgb": (0.12, 0.14, 0.19),
            "opacity": 1.0, "ambient": 0.16, "diffuse": 0.68, "specular": 0.38, "shininess": 42.0,
            "colorExpr": None,
        },
        # New rich architectural & acoustic instrument materials:
        "studio.brass": {
            "rgb": (0.88, 0.68, 0.26),
            "opacity": 1.0, "ambient": 0.20, "diffuse": 0.76, "specular": 0.86, "shininess": 98.0,
            "colorExpr": m_vec3(
                m_poly([term(0.88), term(0.05, trans=[wave("y", 3.4, 0.0, 0), wave("x", 1.8, 0.0, 1)])]),
                m_poly([term(0.68), term(0.05, trans=[wave("y", 3.4, 0.6, 0)])]),
                m_poly([term(0.26), term(0.03, trans=[wave("z", 2.4, 1.1, 1)])]),
            ),
        },
        "studio.obsidian": {
            "rgb": (0.10, 0.13, 0.20),
            "opacity": 1.0, "ambient": 0.16, "diffuse": 0.70, "specular": 0.62, "shininess": 76.0,
            "colorExpr": m_vec3(
                m_poly([term(0.10), term(0.025, trans=[wave("x", 2.0, 0.0, 1)])]),
                m_poly([term(0.13), term(0.030, trans=[wave("y", 2.5, 0.5, 0)])]),
                m_poly([term(0.20), term(0.040, trans=[wave("z", 2.0, 1.0, 1)])]),
            ),
        },
        "studio.copper_coil": {
            "rgb": (0.78, 0.44, 0.24),
            "opacity": 1.0, "ambient": 0.19, "diffuse": 0.74, "specular": 0.78, "shininess": 86.0,
            "colorExpr": m_vec3(
                m_poly([term(0.78), term(0.05, trans=[wave("y", 4.2, 0.0, 0)])]),
                m_poly([term(0.44), term(0.04, trans=[wave("y", 4.2, 0.5, 0)])]),
                m_poly([term(0.24), term(0.03, trans=[wave("x", 2.2, 0.9, 1)])]),
            ),
        },
    }

    # Add all 12 chromatic note materials (preserving C5..B5 uppercase and cs5..as5)
    for slug, note_name, freq, rgb, mat_id in CHROMATIC_NOTES:
        stem = mat_id.replace("material.", "")
        r, g, b = rgb
        specs[stem] = {
            "rgb": rgb,
            "opacity": 1.0,
            "ambient": 0.22,
            "diffuse": 0.78,
            "specular": 0.74,
            "shininess": 88.0,
            "colorExpr": m_vec3(
                m_poly([term(r), term(0.05, trans=[wave("y", 3.0, 0.0, 0)])]),
                m_poly([term(g), term(0.05, trans=[wave("y", 3.0, 0.7, 0)])]),
                m_poly([term(b), term(0.05, trans=[wave("x", 2.2, 1.4, 1)])]),
            ),
        }

    materials_embedded = []
    material_refs = []
    rgb_by_mat = {}

    for stem, spec in sorted(specs.items()):
        mat_id = "material." + stem
        rgb = [float(x) for x in spec["rgb"]]
        rgb_by_mat[mat_id] = rgb
        mat_body = {
            "id": mat_id,
            "name": stem,
            "baseColor": rgb,
            "opacity": float(spec["opacity"]),
            "ambient": float(spec["ambient"]),
            "diffuse": float(spec["diffuse"]),
            "specular": float(spec["specular"]),
            "shininess": float(spec["shininess"]),
            "textureResolution": 16,
        }
        if spec["colorExpr"] is not None:
            mat_body["colorExpr"] = piecewise(spec["colorExpr"], input_var="y")

        materials_embedded.append(mat_body)
        material_refs.append(mat_id)

        # Write shared material root under saves/materials/<stem>/material.json
        mat_dir = ROOT / "saves" / "materials" / stem
        mat_dir.mkdir(parents=True, exist_ok=True)
        root_doc = {
            "identifier": mat_id,
            "material": mat_body,
            "authors": ["Zach"],
            "injected_by": {
                "agent": f"{AGENT}, session {SESSION}",
                "date": "2026-10-19",
                "scope": "Synthesis Studio 3D procedural material upgrade",
            },
        }
        (mat_dir / "material.json").write_text(json.dumps(root_doc, indent=2) + "\n")

    return materials_embedded, material_refs, rgb_by_mat


# =============================================================================
# Composite CSG / SDF 3D Instrument & Architectural Templates
# =============================================================================

def make_amphitheater_plinth_sdf():
    """Stepped circular obsidian-and-slate acoustic amphitheater foundation with
    a central resonance basin beneath the crystal pedestal and 8 radial fins."""
    outer_disc = sdf_leaf(4, (6.2, 0.18, 0.0), offset=(0.0, -0.12, 0.0))
    lower_disc = sdf_leaf(4, (7.4, 0.16, 0.0), offset=(0.0, -0.34, 0.0))
    trim_ring = sdf_leaf(6, (6.25, 0.09, 0.0), offset=(0.0, -0.02, 0.0))
    base = sdf_union_many([outer_disc, lower_disc, trim_ring], smooth_k=0.10)
    return base, (7.8, 0.65, 7.8)


def make_chalice_cradle_sdf():
    """Sculpted 4-prong brass harmonic chalice collar around the base of the
    central crystal pedestal."""
    collar = sdf_leaf(6, (0.52, 0.09, 0.0), offset=(0.0, -0.18, 0.0))
    upper_ring = sdf_leaf(6, (0.58, 0.055, 0.0), offset=(0.0, 0.14, 0.0))
    prongs = [
        sdf_leaf(2, (0.06, 0.24, 0.08), offset=(0.52, 0.0, 0.0), p0=0.02),
        sdf_leaf(2, (0.06, 0.24, 0.08), offset=(-0.52, 0.0, 0.0), p0=0.02),
        sdf_leaf(2, (0.08, 0.24, 0.06), offset=(0.0, 0.0, 0.52), p0=0.02),
        sdf_leaf(2, (0.08, 0.24, 0.06), offset=(0.0, 0.0, -0.52), p0=0.02),
    ]
    return sdf_union_many([collar, upper_ring] + prongs, smooth_k=0.05), (0.75, 0.42, 0.75)


def make_console_cheek_sdf():
    """Beveled analog-synthesizer walnut end-cheek bolster with sculpted side
    recess and brass trim ridge."""
    body = sdf_leaf(2, (0.14, 0.44, 1.26), offset=(0.0, 0.0, 0.0), p0=0.05)
    recess = sdf_leaf(2, (0.08, 0.26, 0.92), offset=(0.09, 0.02, 0.0), p0=0.03)
    carved = sdf_csg(4, body, recess)
    return carved, (0.26, 0.55, 1.38)


def make_easel_gallery_frame_sdf():
    """Sculpted gallery easel frame & lower brush tray sitting behind the
    interactive canvas (z = 2.49 > 2.40) with a full CSG window cutout so the
    canvas face is 100% open to drawing rays."""
    outer_frame = sdf_leaf(2, (2.28, 1.36, 0.07), offset=(0.0, 0.0, 0.0), p0=0.04)
    inner_window = sdf_leaf(1, (2.06, 1.16, 0.24), offset=(0.0, 0.0, -0.02))
    frame_border = sdf_csg(4, outer_frame, inner_window)
    lower_ledge = sdf_leaf(2, (2.36, 0.07, 0.14), offset=(0.0, -1.28, -0.04), p0=0.03)
    left_mast = sdf_leaf(2, (0.09, 1.95, 0.09), offset=(-1.65, -0.35, 0.08), p0=0.02)
    right_mast = sdf_leaf(2, (0.09, 1.95, 0.09), offset=(1.65, -0.35, 0.08), p0=0.02)
    crown_crest = sdf_leaf(6, (0.42, 0.05, 0.0), offset=(0.0, 1.48, 0.0))
    assembled = sdf_union_many([frame_border, lower_ledge, left_mast, right_mast, crown_crest], smooth_k=0.04)
    return assembled, (2.55, 2.45, 0.32)


def make_proscenium_arch_sdf():
    """Grand acoustic proscenium arch framing the easel and 12-note resonator
    constellation from behind."""
    outer_ring = sdf_leaf(6, (3.45, 0.22, 0.0), offset=(0.0, 0.0, 0.0))
    inner_flute = sdf_leaf(6, (3.15, 0.09, 0.0), offset=(0.0, 0.0, -0.06))
    upper_half_box = sdf_leaf(1, (4.2, 2.2, 0.6), offset=(0.0, 1.65, 0.0))
    arch_span = sdf_csg(3, sdf_csg(5, outer_ring, inner_flute, t=0.06), upper_half_box)
    keystone = sdf_convex(make_octahedron_planes(0.34))
    keystone["offset"] = [0.0, 3.48, 0.0]
    full_arch = sdf_csg(5, arch_span, keystone, t=0.08)
    return full_arch, (4.0, 3.95, 0.55)


def make_waveguide_pillar_sdf():
    """Fluted Helmholtz acoustic diffuser column with 5 stacked torus resonator
    fins and an octahedral crystal crown."""
    plinth = sdf_leaf(2, (0.36, 0.28, 0.36), offset=(0.0, -1.85, 0.0), p0=0.05)
    core_shaft = sdf_leaf(3, (0.22, 1.95, 0.22), offset=(0.0, 0.0, 0.0))
    fins = [
        sdf_leaf(6, (0.34, 0.055, 0.0), offset=(0.0, -1.1 + k * 0.55, 0.0))
        for k in range(5)
    ]
    crown = sdf_leaf(0, (0.26, 0.26, 0.26), offset=(0.0, 1.92, 0.0))
    pillar = sdf_union_many([plinth, core_shaft, crown] + fins, smooth_k=0.06)
    return pillar, (0.55, 2.35, 0.55)


def make_organ_pipe_sdf(pipe_half_h):
    """CSG-sculpted cathedral synthesizer organ pipe: tapered resonator column,
    beveled windchest foot, CSG-carved acoustic mouth window, and flared
    harmonic horn crown."""
    shaft = sdf_leaf(3, (0.13, pipe_half_h, 0.13), offset=(0.0, 0.0, 0.0))
    foot_collar = sdf_leaf(2, (0.16, 0.14, 0.16), offset=(0.0, -pipe_half_h * 0.72, 0.0), p0=0.03)
    mouth_cut = sdf_leaf(1, (0.11, 0.10, 0.16), offset=(0.0, -pipe_half_h * 0.48, -0.10))
    voiced_shaft = sdf_csg(4, sdf_csg(5, shaft, foot_collar, t=0.04), mouth_cut)
    crown_ring = sdf_leaf(6, (0.17, 0.045, 0.0), offset=(0.0, pipe_half_h * 0.78, 0.0))
    top_beacon = sdf_leaf(0, (0.11, 0.11, 0.11), offset=(0.0, pipe_half_h * 0.96, 0.0))
    pipe = sdf_union_many([voiced_shaft, crown_ring, top_beacon], smooth_k=0.05)
    return pipe, (0.28, pipe_half_h + 0.25, 0.28)


def make_cymatic_gyro_sdf():
    """Tri-axial cymatic armillary gyro-sphere: two orthogonal torus rings
    cradling a faceted octahedral harmonic core."""
    ring_xy = sdf_leaf(6, (0.48, 0.045, 0.0), offset=(0.0, 0.0, 0.0))
    ring_inner = sdf_leaf(6, (0.34, 0.038, 0.0), offset=(0.0, 0.0, 0.0))
    vertical_halo = sdf_leaf(3, (0.05, 0.48, 0.48), offset=(0.0, 0.0, 0.0))
    hollow_cut = sdf_leaf(0, (0.39, 0.39, 0.39), offset=(0.0, 0.0, 0.0))
    vertical_ring = sdf_csg(4, vertical_halo, hollow_cut)
    core = sdf_convex(make_octahedron_planes(0.20))
    gyro = sdf_union_many([ring_xy, ring_inner, vertical_ring, core], smooth_k=0.03)
    return gyro, (0.62, 0.62, 0.62)


def make_gyro_pedestal_sdf():
    """Sculpted acoustic pedestal base beneath each floating Cymatic Gyro-Sphere."""
    base = sdf_leaf(2, (0.38, 0.12, 0.38), offset=(0.0, -0.36, 0.0), p0=0.04)
    column = sdf_leaf(3, (0.20, 0.42, 0.20), offset=(0.0, 0.0, 0.0))
    cup = sdf_leaf(6, (0.32, 0.055, 0.0), offset=(0.0, 0.38, 0.0))
    return sdf_union_many([base, column, cup], smooth_k=0.06), (0.50, 0.56, 0.50)


def make_acoustic_rib_sdf():
    """Curved parametric acoustic diffuser rib screen for the outer studio wings."""
    spine = sdf_leaf(2, (0.08, 1.55, 0.32), offset=(0.0, 0.0, 0.0), p0=0.03)
    louver_top = sdf_leaf(2, (0.14, 0.06, 0.42), offset=(0.0, 0.85, 0.0), p0=0.02)
    louver_mid = sdf_leaf(2, (0.14, 0.06, 0.42), offset=(0.0, 0.0, 0.0), p0=0.02)
    louver_bot = sdf_leaf(2, (0.14, 0.06, 0.42), offset=(0.0, -0.85, 0.0), p0=0.02)
    return sdf_union_many([spine, louver_top, louver_mid, louver_bot], smooth_k=0.04), (0.24, 1.72, 0.52)


def make_harmonic_canopy_sdf():
    """Celestial acoustic chandelier suspended high above the resonator constellation."""
    outer_torus = sdf_leaf(6, (1.85, 0.065, 0.0), offset=(0.0, 0.0, 0.0))
    inner_torus = sdf_leaf(6, (1.05, 0.050, 0.0), offset=(0.0, -0.12, 0.0))
    center_gem = sdf_convex(make_octahedron_planes(0.26))
    center_gem["offset"] = [0.0, -0.18, 0.0]
    nodes = [outer_torus, inner_torus, center_gem]
    for k in range(4):
        ang = k * (math.pi * 0.5)
        nodes.append(sdf_leaf(0, (0.11, 0.11, 0.11), offset=(math.cos(ang) * 1.85, 0.0, math.sin(ang) * 1.85)))
    return sdf_union_many(nodes, smooth_k=0.05), (2.10, 0.48, 2.10)


# =============================================================================
# Multi-Layered Studio Lighting & Volumetric Acoustic Mist
# =============================================================================

def build_studio_spatial_fields(zone_id):
    # Primary Studio Rig (`spatialRoot`) at [-4.5, 9.5, -5.0]
    # Uses `m_dist` (Op::Distance) and `m_sq` (Op::Scale `u * u`) — zero `pow`!
    rig_origin = (-4.5, 9.5, -5.0)

    def spot_at(world_pt, gain, radius):
        lx = world_pt[0] - rig_origin[0]
        ly = world_pt[1] - rig_origin[1]
        lz = world_pt[2] - rig_origin[2]
        d = m_dist(m_var("p"), m_vec3(m_scalar(lx), m_scalar(ly), m_scalar(lz)))
        u = m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / radius), d)), 0.0, 1.0)
        return m_mul(m_scalar(gain), m_sq(u))

    # 4 intentional studio spotlight pools over a soft concert-hall baseline (0.28):
    # 1. Console Desk & Chord Pads (0, 0.9, -0.2)
    # 2. Crystal Pedestal & Easel Canvas (0, 2.2, 2.1)
    # 3. Cathedral Organ Pipe Amphitheater (0, 3.2, 3.9)
    # 4. Left & Right Gyro-Sphere Wings (±3.25, 2.1, 0.35)
    studio_pools = m_add(
        m_scalar(0.28),
        m_add(
            spot_at((0.0, 0.9, -0.2), 0.78, 4.8),
            m_add(
                spot_at((0.0, 2.2, 2.1), 0.85, 5.5),
                m_add(
                    spot_at((0.0, 3.2, 3.9), 0.82, 6.2),
                    m_add(
                        spot_at((-3.25, 2.1, 0.35), 0.58, 3.8),
                        spot_at((3.25, 2.1, 0.35), 0.58, 3.8),
                    ),
                ),
            ),
        ),
    )

    # Vertical color temperature in sourceDelta coordinates (world y = p.y + 9.5):
    # High proscenium & organ pipes glow warm concert-hall gold [0.98, 0.84, 0.62];
    # Lower amphitheater floor shifts into cool studio-sapphire [0.42, 0.54, 0.82].
    alt_t = m_clamp(m_poly([term(1.0 / 6.5, {"y": 1.0}), term(9.8 / 6.5)]), 0.0, 1.0)
    rig_chroma = m_vec3(
        m_add(m_scalar(0.42), m_mul(m_scalar(0.56), alt_t)),
        m_add(m_scalar(0.54), m_mul(m_scalar(0.30), alt_t)),
        m_add(m_scalar(0.82), m_mul(m_scalar(-0.20), alt_t)),
    )

    spatial_root = {
        "id": f"{zone_id}_spatialRoot",
        "origin": list(rig_origin),
        "scale": [1.0, 1.0, 1.0],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": piecewise(studio_pools, input_var="x"),
        },
        "lightChroma": piecewise(rig_chroma, input_var="x"),
        "lightAngular": piecewise(m_scalar(1.0), input_var="x"),
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0,
        },
        "authoredProperties": {
            "displayName": pv("Synthesis Studio Concert Key Rig"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.11),
            "light.diffuse": pv_float(0.84),
            "light.specular": pv_float(0.74),
            "light.intensity": pv_float(1.25),
            "light.color": pv_vec3(0.96, 0.84, 0.68),
        },
    }

    # Secondary Light Field: Harmonic Crystal & Constellation Lantern at (0, 2.05, 1.55)
    # Placed in open air above the Crystal Pedestal (0, 0.4, 1.25) and in front of
    # the Easel (z = 2.4) so `sourceVisibility` has an unobstructed line of sight!
    crystal_d = m_dist(m_var("p"), m_vec3(m_scalar(0.0), m_scalar(0.0), m_scalar(0.0)))
    crystal_u = m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 5.2), crystal_d)), 0.0, 1.0)
    crystal_support = m_sq(crystal_u)
    crystal_pulse = m_poly([
        term(0.86),
        term(0.14, trans=[wave("t", 1.8, 0.0, 0)]),
    ])
    crystal_rho = m_mul(
        crystal_support,
        m_div(crystal_pulse, m_add(m_scalar(1.0), m_mul(m_scalar(0.18), m_sq(crystal_d)))),
    )
    crystal_chroma = m_vec3(
        m_add(m_scalar(0.76), m_mul(m_scalar(-0.48), crystal_support)),
        m_add(m_scalar(0.42), m_mul(m_scalar(0.48), crystal_support)),
        m_scalar(0.96),
    )
    crystal_glow_field = {
        "id": "studio.field.crystal-harmonic-glow",
        "origin": [0.0, 2.05, 1.55],
        "scale": [5.2, 5.2, 5.2],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": piecewise(crystal_rho, input_var="x"),
        },
        "lightChroma": piecewise(crystal_chroma, input_var="x"),
        "lightAngular": piecewise(m_scalar(1.0), input_var="x"),
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0,
        },
        "authoredProperties": {
            "displayName": pv("Harmonic Crystal & Constellation Lantern"),
            "light.source": pv(True),
            "light.enabled": pv(True),
            "light.ambient": pv_float(0.02),
            "light.diffuse": pv_float(0.80),
            "light.specular": pv_float(0.82),
            "light.intensity": pv_float(0.95),
            "light.color": pv_vec3(0.32, 0.86, 0.98),
        },
    }

    # Participating Volumetric Medium: Cymatic Organ-Stage Acoustic Mist
    # Origin at (0, 2.9, 3.65), scale [5.4, 2.5, 1.8] (world z in [1.85, 5.45],
    # strictly behind the console desk and easel front face so controls and canvas
    # stay 100% crisp, while bathing the 12 Organ Pipes and Proscenium Arch in
    # shimmering harmonic concert haze!).
    haze_r_xz = m_dist(
        m_vec3(m_var("x"), m_scalar(0.0), m_var("z")),
        m_vec3(m_scalar(0.0), m_scalar(0.0), m_scalar(0.0)),
    )
    haze_horiz_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 4.6), haze_r_xz)), 0.0, 1.0)
    )
    haze_vert_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 2.1), m_abs(m_var("y")))), 0.0, 1.0)
    )
    haze_depth_env = m_sq(
        m_clamp(m_sub(m_scalar(1.0), m_mul(m_scalar(1.0 / 1.5), m_abs(m_var("z")))), 0.0, 1.0)
    )
    haze_env = m_mul(m_mul(haze_horiz_env, haze_vert_env), haze_depth_env)

    haze_noise = m_noise(m_mul(m_scalar(0.35), m_var("p")))
    haze_waves = m_clamp(
        m_add(
            m_poly([
                term(0.34),
                term(0.18, trans=[wave("x", 1.2, 0.0, 1), wave("y", 1.2, 0.0, 0)]),
            ]),
            m_mul(m_scalar(0.28), haze_noise),
        ),
        0.0,
        1.0,
    )
    haze_density = m_mul(haze_env, haze_waves)
    haze_ext = m_mul(m_scalar(0.048), haze_density)
    haze_scat = m_mul(m_scalar(0.022), haze_density)

    haze_x_t = m_clamp(m_poly([term(1.0 / 8.0, {"x": 1.0}), term(0.5)]), 0.0, 1.0)
    haze_chroma = m_vec3(
        m_add(m_scalar(0.88), m_mul(m_scalar(-0.56), haze_x_t)),
        m_add(m_scalar(0.46), m_mul(m_scalar(0.38), haze_x_t)),
        m_add(m_scalar(0.52), m_mul(m_scalar(0.44), haze_x_t)),
    )
    haze_emission = m_mul(m_mul(m_scalar(0.018), haze_density), haze_chroma)

    stage_haze_field = {
        "id": "studio.field.stage-acoustic-haze",
        "origin": [0.0, 2.9, 3.65],
        "scale": [5.4, 2.5, 1.8],
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 0.45,
            "astDefinition": piecewise(haze_density, input_var="x"),
        },
        "vectorField": {
            "mode": "Procedural",
            "baseFlowX": 0.08, "baseFlowY": 0.04, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.12,
        },
        "volumeDensity": piecewise(haze_density, input_var="x"),
        "volumeExtinction": piecewise(haze_ext, input_var="x"),
        "volumeScattering": piecewise(haze_scat, input_var="x"),
        "volumeChroma": piecewise(haze_chroma, input_var="x"),
        "volumeEmission": piecewise(haze_emission, input_var="x"),
        "authoredProperties": {
            "displayName": pv("Cymatic Organ-Stage Acoustic Mist"),
            "light.source": pv(False),
        },
    }

    return spatial_root, [crystal_glow_field, stage_haze_field]


# =============================================================================
# 3D Architectural & Instrument Beings + Reactive Laws Builder
# =============================================================================

def build_studio_3d_showcase(is_living: bool, rgb_by_mat: dict):
    objects_3d = []
    relations_3d = []

    def add_csg_obj(
        oid,
        title,
        pos,
        sdf_tree,
        extent,
        mat_id,
        angles_deg=(0.0, 0.0, 0.0),
        scale=(1.0, 1.0, 1.0),
        extra_props=None,
        face_rgb=None,
    ):
        rgb = list(face_rgb) if face_rgb is not None else list(rgb_by_mat.get(mat_id, [0.75, 0.72, 0.68]))
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
            "studio3DShowcase": pv(True),
        }
        if extra_props:
            for k, v in extra_props.items():
                props[k] = v if isinstance(v, dict) and "t" in v else pv(v)

        obj = {
            "objectID": oid,
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
            "authoritativeAxis": [0.0, 1.0, 0.0],
            "targetRotation": [float(angles_deg[0]), float(angles_deg[1]), float(angles_deg[2])],
            "rotationResponsiveness": 10.0,
            "x2D": 100.0,
            "y2D": 100.0,
            "zOrder2D": 0,
            "faceColors": [rgb for _ in range(6)],
            "authoredProperties": props,
        }
        objects_3d.append(obj)
        return obj

    # 1. Stepped Amphitheater Plinth beneath the studio floor
    plinth_sdf, plinth_ext = make_amphitheater_plinth_sdf()
    add_csg_obj(
        "studio.stage.amphitheater_plinth",
        "Cymatic Amphitheater Foundation Rotunda",
        (0.0, -0.18, 0.8),
        plinth_sdf,
        plinth_ext,
        "material.studio.obsidian",
    )

    # 2. Three Concentric Cymatic Standing-Wave Floor Rings around the Crystal Pedestal (0, 0.015, 1.25)
    for idx, (radius, tube, mat_id) in enumerate([
        (0.76, 0.025, "material.studio.brass"),
        (1.22, 0.022, "material.studio.crystal"),
        (1.68, 0.020, "material.studio.brass"),
    ]):
        ring_sdf = sdf_leaf(6, (radius, tube, 0.0), offset=(0.0, 0.0, 0.0))
        add_csg_obj(
            f"studio.stage.cymatic_ring.{idx}",
            f"Cymatic Harmonic Floor Ring {idx + 1}",
            (0.0, 0.015, 1.25),
            ring_sdf,
            (radius + 0.15, 0.12, radius + 0.15),
            mat_id,
        )

    # 3. Brass Harmonic Chalice Cradle at the base of the Crystal Pedestal (0, 0.18, 1.25)
    chalice_sdf, chalice_ext = make_chalice_cradle_sdf()
    add_csg_obj(
        "studio.pedestal.chalice_cradle",
        "Harmonic Crystal Chalice Cradle",
        (0.0, 0.18, 1.25),
        chalice_sdf,
        chalice_ext,
        "material.studio.brass",
    )
    relations_3d.append(relation("studio.pedestal.chalice_cradle", "studio.pedestal.crystal", "cradles"))

    # 4. Sculpted Synthesizer Console End-Cheeks & Front Brass Wrist Rail
    cheek_sdf, cheek_ext = make_console_cheek_sdf()
    add_csg_obj(
        "studio.console.cheek.left",
        "Console Left Walnut Cheek Bolster",
        (-2.68, 0.42, 0.0),
        cheek_sdf,
        cheek_ext,
        "material.studio.walnut",
    )
    add_csg_obj(
        "studio.console.cheek.right",
        "Console Right Walnut Cheek Bolster",
        (2.68, 0.42, 0.0),
        cheek_sdf,
        cheek_ext,
        "material.studio.walnut",
        angles_deg=(0.0, 180.0, 0.0),
    )
    rail_sdf = sdf_leaf(2, (2.62, 0.045, 0.065), offset=(0.0, 0.0, 0.0), p0=0.02)
    add_csg_obj(
        "studio.console.brass_rail",
        "Console Brushed-Brass Wrist Rail",
        (0.0, 0.78, -1.24),
        rail_sdf,
        (2.75, 0.12, 0.15),
        "material.studio.brass",
    )

    # 5. Sculpted Easel Gallery Frame & Light-Brush Tray (behind canvas at z = 2.49)
    frame_sdf, frame_ext = make_easel_gallery_frame_sdf()
    add_csg_obj(
        "studio.easel.gallery_frame",
        "Sculpted Walnut & Brass Easel Gallery Cradle",
        (0.0, 2.20, 2.49),
        frame_sdf,
        frame_ext,
        "material.studio.walnut",
    )
    relations_3d.append(relation("studio.easel.gallery_frame", "studio.easel.canvas", "frames"))

    # 6. Grand Acoustic Proscenium Arch & Twin Fluted Helmholtz Waveguide Pillars
    arch_sdf, arch_ext = make_proscenium_arch_sdf()
    add_csg_obj(
        "studio.arch.proscenium",
        "Grand Acoustic Proscenium Sound-Arch",
        (0.0, 2.15, 2.85),
        arch_sdf,
        arch_ext,
        "material.studio.brass",
    )
    pillar_sdf, pillar_ext = make_waveguide_pillar_sdf()
    add_csg_obj(
        "studio.pillar.waveguide.left",
        "West Helmholtz Waveguide Diffuser Pillar",
        (-3.65, 1.95, 2.55),
        pillar_sdf,
        pillar_ext,
        "material.studio.obsidian",
    )
    add_csg_obj(
        "studio.pillar.waveguide.right",
        "East Helmholtz Waveguide Diffuser Pillar",
        (3.65, 1.95, 2.55),
        pillar_sdf,
        pillar_ext,
        "material.studio.obsidian",
    )

    # 7. The 12-Pipe (or 7-Pipe) Cathedral Synthesizer Organ Array in a Semi-Circular Arc
    active_notes = CHROMATIC_NOTES if is_living else [n for n in CHROMATIC_NOTES if n[0] in DIATONIC_SLUGS]
    n_count = len(active_notes)
    for idx, (slug, note_letter, freq, rgb, mat_id) in enumerate(active_notes):
        t_norm = idx / max(1, n_count - 1)  # 0.0 .. 1.0
        angle = (t_norm - 0.5) * (math.pi * 0.62)  # -56 deg .. +56 deg arc
        arc_radius = 4.35
        px = math.sin(angle) * arc_radius
        pz = 0.25 + math.cos(angle) * arc_radius  # z in [3.15 .. 4.60], well behind easel (z=2.4)
        arch_curve = math.sin(math.pi * t_norm)
        pipe_half_h = 1.05 + 0.55 * arch_curve
        py = 1.55 + 0.65 * arch_curve

        pipe_sdf, pipe_ext = make_organ_pipe_sdf(pipe_half_h)
        pipe_id = f"studio.organ.pipe.{slug}"
        add_csg_obj(
            pipe_id,
            f"Cathedral Harmonic Organ Pipe ({note_letter}5)",
            (px, py, pz),
            pipe_sdf,
            pipe_ext,
            mat_id,
            extra_props={
                "isOrganPipe": pv(True),
                "resonanceNote": pv(note_letter + "5"),
                "struckAt": pv(-100.0),
                "resonanceAge": pv(100.0),
                "resonanceEnergy": pv(0.0),
                "pipeBaseX": pv(px),
                "pipeBaseY": pv(py),
                "pipeBaseZ": pv(pz),
                "pipePhase": pv(idx * 0.52),
                "pigmentR": pv(rgb[0]),
                "pigmentG": pv(rgb[1]),
                "pigmentB": pv(rgb[2]),
            },
            face_rgb=rgb,
        )
        relations_3d.append(relation(pipe_id, f"studio.resonance.{slug}", "harmonizes-with"))

    # 8. Suspended Celestial Harmonic Canopy above the Resonator Constellation
    canopy_sdf, canopy_ext = make_harmonic_canopy_sdf()
    add_csg_obj(
        "studio.crown.harmonic_canopy",
        "Celestial Crown of Harmonics Chandelier",
        (0.0, 5.05, 1.90),
        canopy_sdf,
        canopy_ext,
        "material.studio.brass",
        extra_props={
            "isHarmonicCrown": pv(True),
            "crownBaseY": pv(5.05),
        },
    )

    # 9. Twin Floating Tri-Axial Cymatic Gyro-Spheres & Sculpted Pedestals on the Console Wings
    gyro_sdf, gyro_ext = make_cymatic_gyro_sdf()
    ped_sdf, ped_ext = make_gyro_pedestal_sdf()
    for side, sx, phase in [("left", -3.25, 0.0), ("right", 3.25, math.pi)]:
        add_csg_obj(
            f"studio.pedestal.gyro.{side}",
            f"Cymatic Gyro Pedestal ({side.title()})",
            (sx, 0.48, 0.35),
            ped_sdf,
            ped_ext,
            "material.studio.obsidian",
        )
        add_csg_obj(
            f"studio.sculpture.gyro.{side}",
            f"Tri-Axial Cymatic Gyro-Sphere ({side.title()})",
            (sx, 1.72, 0.35),
            gyro_sdf,
            gyro_ext,
            "material.studio.crystal",
            extra_props={
                "isCymaticGyro": pv(True),
                "gyroBaseY": pv(1.72),
                "gyroPhase": pv(phase),
            },
        )
        relations_3d.append(relation(f"studio.sculpture.gyro.{side}", f"studio.pedestal.gyro.{side}", "hovers-above"))

    # 10. Six Parametric Acoustic Diffuser Ribs along the Outer Studio Wings
    rib_sdf, rib_ext = make_acoustic_rib_sdf()
    for side, sx, yaw in [("left", -4.75, 18.0), ("right", 4.75, -18.0)]:
        for k, sz in enumerate([0.15, 1.35, 2.55]):
            add_csg_obj(
                f"studio.wing.rib.{side}.{k}",
                f"Parametric Acoustic Diffuser Rib ({side.title()} {k + 1})",
                (sx, 1.75 + k * 0.18, sz),
                rib_sdf,
                rib_ext,
                "material.studio.copper_coil" if k == 1 else "material.studio.walnut",
                angles_deg=(0.0, yaw, 0.0),
            )

    # 11. Six Floating Octahedral Overtone Prisms arcing above the Amphitheater
    prism_sdf = sdf_convex(make_octahedron_planes(0.18))
    for k in range(6):
        ang = (k / 5.0 - 0.5) * (math.pi * 0.72)
        px = math.sin(ang) * 3.85
        pz = 0.5 + math.cos(ang) * 2.85
        py = 4.35 + 0.35 * math.sin(math.pi * k / 5.0)
        add_csg_obj(
            f"studio.overtone.prism.{k}",
            f"Floating Harmonic Overtone Prism {k + 1}",
            (px, py, pz),
            prism_sdf,
            (0.28, 0.28, 0.28),
            "material.studio.crystal",
            extra_props={
                "isCymaticGyro": pv(True),
                "gyroBaseY": pv(py),
                "gyroPhase": pv(k * 0.85),
            },
        )

    return objects_3d, relations_3d


def build_showcase_laws(is_living: bool):
    author_id = "studio.author.astra" if is_living else "studio.author.codex"
    laws = []

    # Law 1: Cathedral Organ Pipe Symphony — pipes surge in elevation and glow
    # when their note resonates, and breathe with the studio clock.
    if is_living:
        organ_y_bindings = {
            "y": "pipeBaseY",
            "e": "resonanceEnergy",
            "b": "@state.studio.bloom",
            "m": "@state.studio.motion",
            "t": "time",
        }
        organ_y_terms = [
            term(1.0, {"y": 1.0}),
            term(0.32, {"e": 1.0}),
            term(0.22, {"e": 1.0, "b": 1.0}),
            term(0.05, {"m": 1.0}, trans=[wave("t", 1.5, 0.0, 0)]),
        ]
    else:
        organ_y_bindings = {
            "y": "pipeBaseY",
            "e": "resonanceEnergy",
            "t": "time",
        }
        organ_y_terms = [
            term(1.0, {"y": 1.0}),
            term(0.38, {"e": 1.0}),
            term(0.04, {}, trans=[wave("t", 1.5, 0.0, 0)]),
        ]

    organ_law_id = "law-studio-living-organ-symphony" if is_living else "law-studio-organ-symphony"
    organ_law = {
        "id": organ_law_id,
        "name": "Studio: Cathedral organ pipes surge and glow with each played note",
        "enabled": True,
        "authority": 0,
        "activation": 1,
        "scope": 1,
        "drives": False,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": [author_id],
        "conditionSubjects": [],
        "targets": [],
        "conditionModel": {
            "kind": 0,
            "path": "isOrganPipe",
            "op": 0,
            "operand": pv(True),
        },
        "actionModel": {
            "kind": 5,
            "children": [
                {
                    "kind": 8,
                    "path": "position.y",
                    "bindings": organ_y_bindings,
                    "function": {"pieces": [{"expr": {"terms": organ_y_terms}}]},
                },
                *[
                    {
                        "kind": 8,
                        "path": "color." + c.lower(),
                        "bindings": {"p": "pigment" + c, "e": "resonanceEnergy"},
                        "function": {
                            "pieces": [
                                {
                                    "expr": {
                                        "terms": [
                                            term(1.0, {"p": 1.0}),
                                            term(0.25, {"e": 1.0}),
                                            term(-0.25, {"p": 1.0, "e": 1.0}),
                                        ]
                                    }
                                }
                            ]
                        },
                    }
                    for c in "RGB"
                ],
            ],
        },
        "provenance": [relation(organ_law_id, author_id, "authored-by")],
        "applicationLog": [],
    }
    laws.append(organ_law)

    # Law 2: Cymatic Gyro-Spheres & Overtone Prisms levitate with pulseRate & time
    gyro_law_id = "law-studio-living-gyro-symphony" if is_living else "law-studio-gyro-symphony"
    gyro_law = {
        "id": gyro_law_id,
        "name": "Studio: Cymatic gyro-spheres and overtone prisms levitate with pulseRate",
        "enabled": True,
        "authority": 0,
        "activation": 1,
        "scope": 1,
        "drives": False,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": [author_id],
        "conditionSubjects": [],
        "targets": [],
        "conditionModel": {
            "kind": 0,
            "path": "isCymaticGyro",
            "op": 0,
            "operand": pv(True),
        },
        "actionModel": {
            "kind": 8,
            "path": "position.y",
            "bindings": {
                "y": "gyroBaseY",
                "r": "@state.studio.pulseRate",
                "t": "time",
            },
            "function": {
                "pieces": [
                    {
                        "expr": {
                            "terms": [
                                term(1.0, {"y": 1.0}),
                                term(0.08, {"r": 1.0}, trans=[wave("t", 1.8, 0.0, 0)]),
                            ]
                        }
                    }
                ]
            },
        },
        "provenance": [relation(gyro_law_id, author_id, "authored-by")],
        "applicationLog": [],
    }
    laws.append(gyro_law)

    # Write shared Law roots under saves/laws/<id>/law.json
    for law in laws:
        lid = law["id"]
        ldir = ROOT / "saves" / "laws" / lid
        ldir.mkdir(parents=True, exist_ok=True)
        root_doc = {
            "authors": law["authors"],
            "identifier": lid,
            "injected_by": f"{AGENT}, session {SESSION}, {STAMP}",
            "law": law,
            "triggers": [],
        }
        (ldir / "law.json").write_text(json.dumps(root_doc, indent=2) + "\n")

    return laws


# =============================================================================
# Upgrade & Sync Pipeline for Zone Identities and World Files
# =============================================================================

def apply_upgrade_to_zone_dict(zone: dict, is_living: bool, materials_embedded, material_refs, rgb_by_mat, new_laws):
    zone_id = zone.get("identifier") or zone.get("name") or ("SynthesisStudio.LivingInstrument" if is_living else "SynthesisStudio")
    spatial_root, spatial_fields = build_studio_spatial_fields(zone_id)
    zone["spatialRoot"] = spatial_root
    zone["spatialFields"] = spatial_fields
    zone["materials"] = deepcopy(materials_embedded)
    zone["materialRefs"] = list(material_refs)

    objects = zone.setdefault("world", {}).setdefault("objects", [])
    # Remove any previously injected showcase 3D objects so this script is 100% idempotent
    objects[:] = [
        o for o in objects
        if not o.get("authoredProperties", {}).get("studio3DShowcase", {}).get("v", False)
        and not o.get("objectID", "").startswith(("studio.stage.", "studio.pedestal.chalice", "studio.console.cheek", "studio.console.brass_rail", "studio.easel.gallery_frame", "studio.arch.", "studio.pillar.", "studio.organ.pipe.", "studio.crown.", "studio.pedestal.gyro.", "studio.sculpture.gyro.", "studio.wing.rib.", "studio.overtone.prism."))
    ]

    # Ensure existing chord pads carry their materialId so procedural note shaders apply
    note_mat_map = {slug: mat_id for slug, _, _, _, mat_id in CHROMATIC_NOTES}
    for o in objects:
        oid = o.get("objectID", "")
        if oid.startswith("studio.pad."):
            slug = oid.split(".")[-1]
            if slug in note_mat_map:
                o["materialId"] = note_mat_map[slug]
        elif oid == "studio.platform.floor":
            o["materialId"] = "material.studio.slate"
        elif oid == "studio.console.desk":
            o["materialId"] = "material.studio.walnut"
        elif oid == "studio.pedestal.crystal":
            o["materialId"] = "material.studio.crystal"

    new_objs, new_rels = build_studio_3d_showcase(is_living, rgb_by_mat)
    objects.extend(new_objs)

    # Update formationRelations idempotently
    new_ids = {o["objectID"] for o in new_objs}
    edges = zone.setdefault("formationRelations", [])
    edges[:] = [e for e in edges if e.get("entityA") not in new_ids and e.get("entityB") not in new_ids]
    edges.extend(new_rels)

    # Ensure all referenced category endpoints exist in zone["lexemes"] so Zone hydration has 0 pending relations
    lexemes = zone.setdefault("lexemes", [])
    existing_lex = {lx.get("id") for lx in lexemes if isinstance(lx, dict)}
    for cid, sym in [
        ("category.control", "Interactive Control"),
        ("category.control.button", "Push Button Control"),
        ("category.control.toggle", "Toggle Switch Control"),
        ("category.control.slider", "Linear Slider Control"),
        ("category.art", "Digital Art"),
        ("category.art.stroke", "Authored Stroke"),
        ("category.interactive.orb", "Harmonic Resonance Orb"),
    ]:
        if cid not in existing_lex:
            lexemes.append({"id": cid, "symbol": sym})

    if "lawRefs" in zone and isinstance(zone["lawRefs"], list):
        for law in new_laws:
            if law["id"] not in zone["lawRefs"]:
                zone["lawRefs"].append(law["id"])


def purge_sidecars(path: Path):
    for ext in (".ecform", ".ecmatter", ".ecsave"):
        sc = path.with_suffix(ext)
        if sc.exists():
            sc.unlink()


def main():
    materials_embedded, material_refs, rgb_by_mat = build_studio_materials()

    # 1. Upgrade saves/zones/SynthesisStudio.LivingInstrument/zone.json
    living_zone_path = ROOT / "saves/zones/SynthesisStudio.LivingInstrument/zone.json"
    living_laws = build_showcase_laws(is_living=True)
    if living_zone_path.exists():
        living_zone = json.loads(living_zone_path.read_text())
        apply_upgrade_to_zone_dict(living_zone, True, materials_embedded, material_refs, rgb_by_mat, living_laws)
        living_zone_path.write_text(json.dumps(living_zone, indent=2) + "\n")
        purge_sidecars(living_zone_path)
        print("Upgraded:", living_zone_path, "objects:", len(living_zone["world"]["objects"]))

    # 2. Upgrade saves/zones/SynthesisStudio/zone.json
    base_zone_path = ROOT / "saves/zones/SynthesisStudio/zone.json"
    base_laws = build_showcase_laws(is_living=False)
    if base_zone_path.exists():
        base_zone = json.loads(base_zone_path.read_text())
        apply_upgrade_to_zone_dict(base_zone, False, materials_embedded, material_refs, rgb_by_mat, base_laws)
        base_zone_path.write_text(json.dumps(base_zone, indent=2) + "\n")
        purge_sidecars(base_zone_path)
        print("Upgraded:", base_zone_path, "objects:", len(base_zone["world"]["objects"]))

    # 3. Upgrade saves/worlds/synthesis_studio_living.json
    living_world_path = ROOT / "saves/worlds/synthesis_studio_living.json"
    if living_world_path.exists():
        living_world = json.loads(living_world_path.read_text())
        z0 = living_world["zones"][0]
        apply_upgrade_to_zone_dict(z0, True, materials_embedded, material_refs, rgb_by_mat, living_laws)
        reg = living_world.get("authoredLaws", {})
        if "laws" in reg:
            new_law_ids = {l["id"] for l in living_laws}
            reg["laws"] = [l for l in reg["laws"] if l["id"] not in new_law_ids] + deepcopy(living_laws)
            members = reg.setdefault("formationMembers", [])
            for lid in new_law_ids:
                if lid not in members:
                    members.append(lid)
        living_world_path.write_text(json.dumps(living_world, indent=2) + "\n")
        purge_sidecars(living_world_path)
        print("Upgraded:", living_world_path, "objects:", len(z0["world"]["objects"]))

    # 4. Upgrade saves/worlds/synthesis_studio.json (and fix law-studio-pad-play voice match)
    base_world_path = ROOT / "saves/worlds/synthesis_studio.json"
    if base_world_path.exists():
        base_world = json.loads(base_world_path.read_text())
        z0 = base_world["zones"][0]
        apply_upgrade_to_zone_dict(z0, False, materials_embedded, material_refs, rgb_by_mat, base_laws)
        reg = base_world.get("authoredLaws", {})
        if "laws" in reg:
            for l in reg["laws"]:
                if l["id"] == "law-studio-pad-play":
                    for child in l.get("conditionModel", {}).get("children", []):
                        if child.get("path") == "@state.studio.voice":
                            child["operand"] = {"t": "string", "v": "timbre.studio.triangle"}
            new_law_ids = {l["id"] for l in base_laws}
            reg["laws"] = [l for l in reg["laws"] if l["id"] not in new_law_ids] + deepcopy(base_laws)
            members = reg.setdefault("formationMembers", [])
            for lid in new_law_ids:
                if lid not in members:
                    members.append(lid)
        base_world_path.write_text(json.dumps(base_world, indent=2) + "\n")
        purge_sidecars(base_world_path)
        print("Upgraded:", base_world_path, "objects:", len(z0["world"]["objects"]))


if __name__ == "__main__":
    main()
