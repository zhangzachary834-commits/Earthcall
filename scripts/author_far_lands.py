#!/usr/bin/env python3
"""Authoring of the Earthcall Far Lands generative zone and world.

Creates a Zone representing the Far Lands, an iconic concept built entirely
out of Earthcall's generative Laws, OntoMath, and first-order ontological beings.

Originally conceptualized by Zach ("Like Minecraft's Far Lands in concept but its
Earthcall's generative Laws and OntoMath generating that cosmically enormous feeling").
Synthesized & deepened by Gemini Spark.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

AUTHOR = "author.gemini-spark"
ZONE_ID = "FarLands"


def pv(t, v):
    if t == "vec3":
        x, y, z = v
        return {"t": "vec3", "x": float(x), "y": float(y), "z": float(z)}
    return {"t": t, "v": v}


# --- OntoMath MathNode AST Constructors ---

def math_scalar(v):
    return {
        "op": 0,
        "scalarForm": {
            "terms": [
                {"c": float(v), "factors": {}}
            ]
        }
    }


def math_var(name):
    return {
        "op": 1,
        "var": name
    }


def math_vec3(x, y, z):
    return {
        "op": 2,
        "children": [x, y, z]
    }


def math_comp(vec, axis):
    return {
        "op": 3,
        "arg": axis,
        "children": [vec]
    }


def math_add(a, b):
    return {
        "op": 4,
        "children": [a, b]
    }


def math_sub(a, b):
    return {
        "op": 5,
        "children": [a, b]
    }


def math_scale(a, b):
    return {
        "op": 6,
        "children": [a, b]
    }


def math_noise(vec):
    return {
        "op": 29,
        "children": [vec]
    }


# --- Law Condition & Action Constructors ---

def compare(path, op, operand=None, operand_path=None):
    node = {"kind": 0, "path": path, "op": op}
    if operand is not None:
        node["operand"] = operand
    if operand_path is not None:
        node["operandPath"] = operand_path
    return node


def related(rel_type, other):
    return {"kind": 2, "relationType": rel_type, "otherId": other}


def seq(*children):
    return {"kind": 5, "children": list(children)}


def set_path(path, operand):
    return {"kind": 0, "path": path, "operand": operand}


def add_property(name, operand, owner=""):
    node = {"kind": 12, "propertyName": name, "operand": operand}
    if owner:
        node["path"] = owner
    return node


def map_path(path, bindings, terms=None, pieces=None, input_var=None):
    fn = {"pieces": pieces if pieces is not None else [{"expr": {"terms": terms or []}}]}
    if input_var:
        fn["input"] = input_var
    return {"kind": 8, "path": path, "bindings": bindings, "function": fn}


def offset_terms(var, offset):
    return [
        {"c": 1.0, "factors": {var: 1.0}},
        {"c": float(offset), "factors": {}},
    ]


def publish(event, subject=""):
    return {
        "kind": 10,
        "eventType": event,
        "publishSubject": subject,
        "publishObject": "",
    }


def play_audio(freq_path, amp_path, wave_type="crystal"):
    return {
        "kind": 18,
        "path": freq_path,
        "input": amp_path,
        "propertyName": wave_type,
    }


def provenance(law_id):
    return [{
        "type": "authored-by",
        "entityA": law_id,
        "entityB": AUTHOR,
        "directed": True,
        "weight": 1.0,
        "events": [{"description": "authored-by", "deltaWeight": 1.0, "timestamp": 1787395000}],
    }]


def mat4_translate(x, y, z, scale=(1.0, 1.0, 1.0)):
    sx, sy, sz = scale
    return [
        sx, 0.0, 0.0, 0.0,
        0.0, sy, 0.0, 0.0,
        0.0, 0.0, sz, 0.0,
        x, y, z, 1.0,
    ]


def category_being(object_id, display_name):
    return {
        "objectID": object_id,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0] * 11,
        "transform": mat4_translate(0.0, 0.0, 0.0, (0.01, 0.01, 0.01)),
        "center": [0.0, 0.0, 0.0],
        "materialId": "",
        "authoredProperties": {"displayName": pv("string", display_name)},
    }


def instance_rel(a, b):
    return {
        "type": "instance-of",
        "entityA": a,
        "entityB": b,
        "directed": True,
        "weight": 1.0,
        "events": [{"description": "instance-of", "deltaWeight": 1.0, "timestamp": 1787395000}],
    }


def authored_by_rel(a, b):
    return {
        "type": "authored-by",
        "entityA": a,
        "entityB": b,
        "directed": True,
        "weight": 1.0,
        "events": [{"description": "authored-by", "deltaWeight": 1.0, "timestamp": 1787395000}],
    }


def subcategory_rel(a, b):
    return {
        "type": "subcategory-of",
        "entityA": a,
        "entityB": b,
        "directed": True,
        "weight": 1.0,
        "events": [{"description": "subcategory-of", "deltaWeight": 1.0, "timestamp": 1787395000}],
    }


LAWS = []
TRIGGERS = {}
FORMATION = []


def add_law(law_id, name, activation, triggers, cond, action, scope=1):
    law = {
        "id": law_id,
        "name": name,
        "enabled": True,
        "authority": 0,
        "activation": activation,
        "scope": scope,
        "drives": False,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": [AUTHOR],
        "conditionSubjects": [],
        "targets": [],
        "conditionModel": cond,
        "actionModel": action,
        "provenance": provenance(law_id),
        "applicationLog": [],
    }
    LAWS.append(law)
    FORMATION.append(law_id)
    if triggers:
        TRIGGERS[law_id] = triggers


def build_world():
    categories = [
        category_being(AUTHOR, "Gemini Spark"),
        category_being("Zach", "Zachary Zhang"),
        category_being("category.farlands", "Far Lands"),
        category_being("category.farlands.field", "Far Lands Mathematical Field"),
        category_being("category.farlands.monolith", "Far Lands Horizon Beacon"),
        category_being("category.farlands.sanctuary", "Far Lands Sanctuary Architecture"),
        category_being("category.farlands.overlook", "Far Lands Vista Overlook"),
        category_being("category.control", "UI Controls"),
        category_being("category.control.button", "Action Button"),
    ]

    relations = [
        subcategory_rel("category.farlands.field", "category.farlands"),
        subcategory_rel("category.farlands.monolith", "category.farlands"),
        subcategory_rel("category.farlands.sanctuary", "category.farlands"),
        subcategory_rel("category.farlands.overlook", "category.farlands"),
        subcategory_rel("category.control.button", "category.control"),
        authored_by_rel("category.farlands", AUTHOR),
    ]

    zone_objects = []

    def make_face_colors(rgb):
        r, g, b = rgb
        return [[r, g, b] for _ in range(6)]

    # -------------------------------------------------------------------------
    # 1. The Core Far Lands Math Field (ShapeKind 10: Field, Prim 7: Expr)
    # -------------------------------------------------------------------------
    # Constructs the legendary Far Lands landscape via pure OntoMath:
    # 2D component-extracted coordinates (Component(p, "x") and Component(p, "z"))
    # ensure exact heightfield classification (isHeightfieldExpr) and tight
    # Lipschitz bounds (estimateLipschitz), giving both fast GPU DDA raymarching
    # and solid marching-tetrahedra physical collision.
    px = math_comp(math_var("p"), "x")
    pz = math_comp(math_var("p"), "z")

    def make_octave(freq_x, freq_z, amp):
        nx = math_scale(math_scalar(freq_x), px)
        nz = math_scale(math_scalar(freq_z), pz)
        arg = math_vec3(nx, math_scalar(0.0), nz)
        return math_scale(math_scalar(amp), math_noise(arg))

    # Octave 0: Macro-continental base relief (gentle sweeping elevation)
    oct0 = make_octave(0.0018, 0.0018, 18.0)

    # Octave 1: Rolling foothills and landscape variance
    oct1 = make_octave(0.0055, 0.0055, 14.0)

    # Octave 2: Edge Far Lands - Z Axis Colossal Curtain Walls (65m sheer vertical cliffs)
    oct2 = make_octave(0.0025, 0.038, 65.0)

    # Octave 3: High-frequency Z Axis Corrugation & Vertical Striations
    oct3 = make_octave(0.0012, 0.076, 30.0)

    # Octave 4: Edge Far Lands - X Axis Colossal Curtain Walls (perpendicular cross-walls)
    oct4 = make_octave(0.038, 0.0025, 65.0)

    # Octave 5: High-frequency X Axis Corrugation
    oct5 = make_octave(0.076, 0.0012, 30.0)

    # Octave 6: Corner Interference Spire Lattice (where X & Z wall harmonics collide)
    oct6 = make_octave(0.045, 0.045, 24.0)

    # Octave 7: Geological micro-strata & razor fluting
    oct7 = make_octave(0.12, 0.12, 7.0)

    # Sum of all octaves: forms Edge Walls, Corner Monolithic Towers, and Abyssal Chasms
    h_walls = math_add(
        math_add(oct0, oct1),
        math_add(
            math_add(oct2, oct3),
            math_add(
                math_add(oct4, oct5),
                math_add(oct6, oct7)
            )
        )
    )

    # Ground surface SDF: y - h(x, z) = 0
    sdf_math = math_sub(math_var("y"), h_walls)

    field_node = {
        "op": 0,      # Leaf
        "prim": 7,    # Expr
        "dims": [0.5, 0.5, 0.5],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": 0.5,
        "mathNode": sdf_math,
    }

    far_lands_field_id = "object.farlands.terrain"
    zone_objects.append({
        "objectID": far_lands_field_id,
        "shapeKind": 10,
        "geometryType": 10,
        "shapeParams": [0.0] * 11,
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "materialId": "material.farlands.terrain",
        "baseline": "ground",
        "attributes": {"baseline": "ground"},
        "authoredProperties": {
            "displayName": pv("string", "The Far Lands Mathematical Field"),
        },
        "fieldExtent": [2000.0, 240.0, 2000.0],
        "field": field_node,
    })
    relations.append(instance_rel(far_lands_field_id, "category.farlands.field"))

    # -------------------------------------------------------------------------
    # 2. Grand Celestial Sanctuary Architecture at Origin
    # -------------------------------------------------------------------------
    # Multi-tiered concentric obsidian & crystalline amphitheater nestled at origin:
    
    # Outer Grand Plinth (Radius 16m, thickness 0.5m)
    dais_outer_id = "object.farlands.dais_outer"
    zone_objects.append({
        "objectID": dais_outer_id,
        "shapeKind": 3,  # Cylinder plinth
        "geometryType": 3,
        "shapeParams": [16.0, 0.0, 0.0, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -0.3, 0.0, (32.0, 0.5, 32.0)),
        "center": [0.0, -0.3, 0.0],
        "materialId": "material.farlands.obsidian",
        "faceColors": make_face_colors((0.08, 0.09, 0.12)),
        "authoredProperties": {
            "displayName": pv("string", "Outer Sanctuary Plinth"),
        },
    })
    relations.append(instance_rel(dais_outer_id, "category.farlands.sanctuary"))

    # Inner Concentric Sanctuary Arena (Radius 9.5m, elevated +0.2m)
    dais_inner_id = "object.farlands.dais_inner"
    zone_objects.append({
        "objectID": dais_inner_id,
        "shapeKind": 3,  # Cylinder plinth
        "geometryType": 3,
        "shapeParams": [9.5, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.1, 0.0, (19.0, 0.4, 19.0)),
        "center": [0.0, 0.1, 0.0],
        "materialId": "material.farlands.platform",
        "faceColors": make_face_colors((0.14, 0.16, 0.22)),
        "authoredProperties": {
            "displayName": pv("string", "Inner Sanctuary Arena"),
        },
    })
    relations.append(instance_rel(dais_inner_id, "category.farlands.sanctuary"))

    # Luminescent Energy Torus Ring framing inner sanctuary arena
    sanctuary_ring_id = "object.farlands.sanctuary_ring"
    zone_objects.append({
        "objectID": sanctuary_ring_id,
        "shapeKind": 8,  # Torus
        "geometryType": 8,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 9.0, 0.16, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.32, 0.0, (1.0, 1.0, 1.0)),
        "center": [0.0, 0.32, 0.0],
        "materialId": "material.farlands.beacon",
        "faceColors": make_face_colors((0.25, 0.90, 1.00)),
        "authoredProperties": {
            "displayName": pv("string", "Sanctuary Runic Torus"),
        },
    })
    relations.append(instance_rel(sanctuary_ring_id, "category.farlands.sanctuary"))

    # -------------------------------------------------------------------------
    # 3. Four Cardinal Monolith Pylons (North, South, East, West)
    # -------------------------------------------------------------------------
    # Each cardinal direction holds an imposing architectural monolith with a
    # floating harmonic crystal that can be sounded by the user.

    monoliths_config = [
        {
            "slug": "north",
            "name": "North Chrono-Curtain Obelisk",
            "pos": (0.0, 4.5, -14.0),
            "size": (1.2, 9.0, 1.2),
            "crystal_pos": (0.0, 9.8, -14.0),
            "material": "material.farlands.amethyst",
            "color": (0.80, 0.35, 1.00),
            "freq": 528.0,
            "amp": 0.65,
            "wave": "crystal",
            "role": "farlands-pillar-north",
            "title": "Chrono-Curtain Harmonic (528 Hz)"
        },
        {
            "slug": "south",
            "name": "South Abyssal Rift Obelisk",
            "pos": (0.0, 4.5, 14.0),
            "size": (1.2, 9.0, 1.2),
            "crystal_pos": (0.0, 9.8, 14.0),
            "material": "material.farlands.beacon",
            "color": (0.25, 0.75, 1.00),
            "freq": 396.0,
            "amp": 0.70,
            "wave": "sine",
            "role": "farlands-pillar-south",
            "title": "Abyssal Grounding Harmonic (396 Hz)"
        },
        {
            "slug": "east",
            "name": "East Lattice Frontier Obelisk",
            "pos": (14.0, 4.5, 0.0),
            "size": (1.2, 9.0, 1.2),
            "crystal_pos": (14.0, 9.8, 0.0),
            "material": "material.farlands.emerald",
            "color": (0.20, 0.95, 0.55),
            "freq": 639.0,
            "amp": 0.60,
            "wave": "triangle",
            "role": "farlands-pillar-east",
            "title": "Lattice Connection Harmonic (639 Hz)"
        },
        {
            "slug": "west",
            "name": "West Solar Horizon Obelisk",
            "pos": (-14.0, 4.5, 0.0),
            "size": (1.2, 9.0, 1.2),
            "crystal_pos": (-14.0, 9.8, 0.0),
            "material": "material.farlands.amber",
            "color": (1.00, 0.65, 0.20),
            "freq": 741.0,
            "amp": 0.60,
            "wave": "saw",
            "role": "farlands-pillar-west",
            "title": "Solar Horizon Awakening (741 Hz)"
        },
    ]

    for m in monoliths_config:
        pylon_id = f"object.farlands.monolith.{m['slug']}"
        zone_objects.append({
            "objectID": pylon_id,
            "shapeKind": 9,  # RoundedBox
            "geometryType": 9,
            "shapeParams": [0.6, 0.6, 0.6, 4.5, 0.0, 0.0, 0.0, 0.0, 0.15, 0.0, 0.0],
            "transform": mat4_translate(*m["pos"], scale=m["size"]),
            "center": list(m["pos"]),
            "materialId": "material.farlands.obsidian",
            "faceColors": make_face_colors((0.10, 0.11, 0.15)),
            "authoredProperties": {
                "displayName": pv("string", m["name"]),
            },
        })
        relations.append(instance_rel(pylon_id, "category.farlands.monolith"))

        # Floating Apex Pulsar Crystal
        crystal_id = f"object.farlands.crystal.{m['slug']}"
        zone_objects.append({
            "objectID": crystal_id,
            "shapeKind": 2,  # Sphere
            "geometryType": 2,
            "shapeParams": [0.55, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(*m["crystal_pos"], scale=(1.1, 1.1, 1.1)),
            "center": list(m["crystal_pos"]),
            "materialId": m["material"],
            "faceColors": make_face_colors(m["color"]),
            "authoredProperties": {
                "displayName": pv("string", m["title"]),
                "buttonRole": pv("string", m["role"]),
                "acoustic.frequency": pv("double", m["freq"]),
                "acoustic.amplitude": pv("double", m["amp"]),
                "acoustic.waveType": pv("string", m["wave"]),
            },
        })
        relations.append(instance_rel(crystal_id, "category.control.button"))

    # -------------------------------------------------------------------------
    # 4. Central Singularity Core & Orbital Gyroscope
    # -------------------------------------------------------------------------
    # Altar Pedestal Base
    altar_pedestal_id = "object.farlands.altar_pedestal"
    zone_objects.append({
        "objectID": altar_pedestal_id,
        "shapeKind": 3,  # Cylinder plinth
        "geometryType": 3,
        "shapeParams": [1.8, 0.0, 0.0, 0.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.6, 0.0, (3.6, 1.2, 3.6)),
        "center": [0.0, 0.6, 0.0],
        "materialId": "material.farlands.gold",
        "faceColors": make_face_colors((0.95, 0.78, 0.32)),
        "authoredProperties": {
            "displayName": pv("string", "Altar of the Singularity"),
        },
    })
    relations.append(instance_rel(altar_pedestal_id, "category.farlands.sanctuary"))

    # Floating Singularity Core
    singularity_core_id = "object.farlands.singularity_core"
    zone_objects.append({
        "objectID": singularity_core_id,
        "shapeKind": 2,  # Sphere
        "geometryType": 2,
        "shapeParams": [0.75, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 3.8, 0.0, (1.5, 1.5, 1.5)),
        "center": [0.0, 3.8, 0.0],
        "materialId": "material.farlands.beacon",
        "faceColors": make_face_colors((0.35, 0.95, 1.00)),
        "authoredProperties": {
            "displayName": pv("string", "Hyper-Resonance Singularity Core"),
            "buttonRole": pv("string", "farlands-pulse"),
            "acoustic.frequency": pv("double", 432.0),
            "acoustic.amplitude": pv("double", 0.75),
            "acoustic.waveType": pv("string", "crystal"),
        },
    })
    relations.append(instance_rel(singularity_core_id, "category.control.button"))

    # Orbital Gyroscope Ring encircling the Singularity Core
    singularity_ring_id = "object.farlands.singularity_ring"
    zone_objects.append({
        "objectID": singularity_ring_id,
        "shapeKind": 8,  # Torus
        "geometryType": 8,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 1.7, 0.09, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 3.8, 0.0, (1.0, 1.0, 1.0)),
        "center": [0.0, 3.8, 0.0],
        "materialId": "material.farlands.gold",
        "faceColors": make_face_colors((0.95, 0.82, 0.38)),
        "authoredProperties": {
            "displayName": pv("string", "Singularity Orbital Gyroscope"),
        },
    })

    # 4 Orbiting Harmonic Satellite Motes
    satellites = [
        ("mote_n", (0.0, 3.8, -2.4), (0.75, 0.35, 1.0)),
        ("mote_s", (0.0, 3.8, 2.4), (0.25, 0.85, 1.0)),
        ("mote_e", (2.4, 3.8, 0.0), (0.20, 0.95, 0.55)),
        ("mote_w", (-2.4, 3.8, 0.0), (1.0, 0.70, 0.25)),
    ]
    for sat_slug, sat_pos, sat_color in satellites:
        sat_id = f"object.farlands.{sat_slug}"
        zone_objects.append({
            "objectID": sat_id,
            "shapeKind": 2,
            "geometryType": 2,
            "shapeParams": [0.18, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(*sat_pos, scale=(0.36, 0.36, 0.36)),
            "center": list(sat_pos),
            "materialId": "material.farlands.beacon",
            "faceColors": make_face_colors(sat_color),
            "authoredProperties": {
                "displayName": pv("string", f"Harmonic Satellite ({sat_slug})"),
            },
        })

    # -------------------------------------------------------------------------
    # 5. Cantilevered Vista Overlook Piers
    # -------------------------------------------------------------------------
    # North Overlook Pier: projecting toward the Edge Wall Curtains
    pier_n_walk_id = "object.farlands.overlook_n.walkway"
    zone_objects.append({
        "objectID": pier_n_walk_id,
        "shapeKind": 0,  # Box
        "geometryType": 0,
        "shapeParams": [1.6, 0.2, 7.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, -21.0, (3.2, 0.4, 14.0)),
        "center": [0.0, 0.0, -21.0],
        "materialId": "material.farlands.obsidian",
        "faceColors": make_face_colors((0.10, 0.11, 0.15)),
        "authoredProperties": {
            "displayName": pv("string", "North Edge Vista Catwalk"),
        },
    })
    relations.append(instance_rel(pier_n_walk_id, "category.farlands.overlook"))

    pier_n_deck_id = "object.farlands.overlook_n.deck"
    zone_objects.append({
        "objectID": pier_n_deck_id,
        "shapeKind": 3,  # Cylinder
        "geometryType": 3,
        "shapeParams": [3.6, 0.0, 0.0, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, -28.0, (7.2, 0.5, 7.2)),
        "center": [0.0, 0.0, -28.0],
        "materialId": "material.farlands.platform",
        "faceColors": make_face_colors((0.16, 0.18, 0.24)),
        "authoredProperties": {
            "displayName": pv("string", "North Edge Vista Overlook Deck"),
        },
    })
    relations.append(instance_rel(pier_n_deck_id, "category.farlands.overlook"))

    # Edge Overlook Lantern Pillars (Left & Right)
    for side, lx in (("left", -3.2), ("right", 3.2)):
        lantern_id = f"object.farlands.overlook_n.lantern_{side}"
        zone_objects.append({
            "objectID": lantern_id,
            "shapeKind": 9,  # RoundedBox
            "geometryType": 9,
            "shapeParams": [0.25, 0.25, 0.25, 1.2, 0.0, 0.0, 0.0, 0.0, 0.08, 0.0, 0.0],
            "transform": mat4_translate(lx, 1.2, -28.0, (0.5, 2.4, 0.5)),
            "center": [lx, 1.2, -28.0],
            "materialId": "material.farlands.beacon",
            "faceColors": make_face_colors((0.30, 0.90, 1.0)),
            "authoredProperties": {
                "displayName": pv("string", f"Vista Beacon Lantern ({side})"),
            },
        })

    # East Overlook Pier: projecting toward the Corner Spires
    pier_e_walk_id = "object.farlands.overlook_e.walkway"
    zone_objects.append({
        "objectID": pier_e_walk_id,
        "shapeKind": 0,  # Box
        "geometryType": 0,
        "shapeParams": [6.0, 0.2, 1.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(20.0, 0.0, 0.0, (12.0, 0.4, 3.2)),
        "center": [20.0, 0.0, 0.0],
        "materialId": "material.farlands.obsidian",
        "faceColors": make_face_colors((0.10, 0.11, 0.15)),
        "authoredProperties": {
            "displayName": pv("string", "East Lattice Vista Catwalk"),
        },
    })
    relations.append(instance_rel(pier_e_walk_id, "category.farlands.overlook"))

    pier_e_deck_id = "object.farlands.overlook_e.deck"
    zone_objects.append({
        "objectID": pier_e_deck_id,
        "shapeKind": 3,
        "geometryType": 3,
        "shapeParams": [3.6, 0.0, 0.0, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(26.0, 0.0, 0.0, (7.2, 0.5, 7.2)),
        "center": [26.0, 0.0, 0.0],
        "materialId": "material.farlands.platform",
        "faceColors": make_face_colors((0.16, 0.18, 0.24)),
        "authoredProperties": {
            "displayName": pv("string", "East Corner Vista Deck"),
        },
    })
    relations.append(instance_rel(pier_e_deck_id, "category.farlands.overlook"))

    # -------------------------------------------------------------------------
    # 6. Floating Sky Megaliths (Atmospheric Scale)
    # -------------------------------------------------------------------------
    # Colossal suspended geometric fragments floating above the canyons
    sky_megaliths = [
        ("sky_alpha", (0.0, 52.0, -75.0), (12.0, 26.0, 12.0), (0.12, 0.14, 0.18)),
        ("sky_beta", (75.0, 60.0, 65.0), (18.0, 18.0, 30.0), (0.14, 0.15, 0.20)),
        ("sky_gamma", (-70.0, 48.0, -55.0), (12.0, 24.0, 12.0), (0.10, 0.12, 0.16)),
    ]
    for sky_slug, sky_pos, sky_scale, sky_color in sky_megaliths:
        sky_id = f"object.farlands.{sky_slug}"
        zone_objects.append({
            "objectID": sky_id,
            "shapeKind": 0,  # Box
            "geometryType": 0,
            "shapeParams": [sky_scale[0] * 0.5, sky_scale[1] * 0.5, sky_scale[2] * 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(*sky_pos, scale=sky_scale),
            "center": list(sky_pos),
            "materialId": "material.farlands.obsidian",
            "faceColors": make_face_colors(sky_color),
            "authoredProperties": {
                "displayName": pv("string", f"Floating Sky Megalith ({sky_slug})"),
            },
        })

    # -------------------------------------------------------------------------
    # 7. State Container & Dynamic Telemetry
    # -------------------------------------------------------------------------
    state_id = "state.farlands"
    zone_objects.append({
        "objectID": state_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -20.0, 0.0, (0.01, 0.01, 0.01)),
        "center": [0.0, -20.0, 0.0],
        "materialId": "material.farlands.safezone",
        "faceColors": make_face_colors((0.1, 0.1, 0.1)),
        "authoredProperties": {
            "displayName": pv("string", "Far Lands State"),
            "pulseCount": pv("double", 0.0),
            "themeMode": pv("double", 0.0),
            "activePillar": pv("string", "SANCTUARY"),
        },
    })

    # -------------------------------------------------------------------------
    # 8. 2D Sci-Fi / Celestial HUD & Exploration Deck
    # -------------------------------------------------------------------------
    def text2d(object_id, text, x, y, size, rgb, z=30):
        return {
            "objectID": object_id,
            "shapeKind": 13,
            "geometryType": 13,
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, float(size)],
            "transform": mat4_translate(0.0, 0.0, 0.0),
            "center": [0.0, 0.0, 0.0],
            "x2D": float(x),
            "y2D": float(y),
            "zOrder2D": z,
            "materialId": "",
            "faceColors": make_face_colors(rgb),
            "authoredProperties": {
                "displayName": pv("string", text),
                "label2D": pv("string", text),
                "shape.width2D": pv("double", 0.0),
                "shape.height2D": pv("double", float(size)),
                "pickPriority": pv("double", -1.0),
            },
        }

    def button2d(object_id, text, x, y, w, h, rgb, role, z=35, text_size=13):
        return {
            "objectID": object_id,
            "shapeKind": 12,  # Shape2D
            "geometryType": 12,
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, float(w), float(h)],
            "transform": mat4_translate(0.0, 0.0, 0.0),
            "center": [0.0, 0.0, 0.0],
            "x2D": float(x),
            "y2D": float(y),
            "zOrder2D": z,
            "materialId": "",
            "faceColors": make_face_colors(rgb),
            "authoredProperties": {
                "displayName": pv("string", text),
                "label2D": pv("string", text),
                "shape.width2D": pv("double", float(w)),
                "shape.height2D": pv("double", float(h)),
                "buttonRole": pv("string", role),
                "label.size2D": pv("double", float(text_size)),
            },
        }

    # Top Header & Metadata
    zone_objects.append(text2d("hud.title", "EARTHCALL // THE FAR LANDS FRACTAL FRONTIER", 32, 28, 24, (0.35, 0.95, 0.75)))
    zone_objects.append(text2d("hud.subtitle", "OntoMath Generative Horizon — Exact Mathematical Degeneration at Scale", 34, 58, 13, (0.92, 0.78, 0.45)))
    zone_objects.append(text2d("hud.author", "Authored by Gemini Spark // Commissioned by Zach", 34, 78, 11, (0.55, 0.65, 0.75)))

    # Telemetry Panel
    zone_objects.append(text2d("hud.telemetry.sector", "SECTOR: ORIGIN OBSERVATORY SANCTUARY [0.0, 0.0]", 34, 106, 12, (0.80, 0.85, 0.92)))
    zone_objects.append(text2d("hud.telemetry.field", "FIELD: 8-OCTAVE ANISOTROPIC LATTICE // GPU DDA ACCELERATED", 34, 124, 12, (0.45, 0.75, 0.85)))
    zone_objects.append(text2d("hud.telemetry.resonance", "RESONANCE: HARMONIC ACTIVE // 528 Hz SOLFEGGIO", 34, 142, 12, (0.30, 0.90, 0.95)))
    zone_objects.append(text2d("hud.telemetry.pulses", "COSMIC PULSES SOUNDED: 0", 34, 160, 12, (0.95, 0.75, 0.35)))

    # Interactive HUD Bottom Dock
    dock_bg = {
        "objectID": "hud.dock.bg",
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1220.0, 56.0],
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "x2D": 30.0,
        "y2D": 640.0,
        "zOrder2D": 20,
        "materialId": "",
        "faceColors": make_face_colors((0.05, 0.07, 0.10)),
        "authoredProperties": {
            "displayName": pv("string", "HUD Dock Background"),
            "shape.width2D": pv("double", 1220.0),
            "shape.height2D": pv("double", 56.0),
            "pickPriority": pv("double", -1.0),
        },
    }
    zone_objects.append(dock_bg)

    hud_buttons = [
        ("hud.btn.pulse", "[ PULSE CORE ]", 45, 650, 150, 36, (0.16, 0.48, 0.55), "farlands-pulse", 13),
        ("hud.btn.chord", "[ HARMONIC CHORD ]", 205, 650, 165, 36, (0.42, 0.24, 0.56), "farlands-chord", 13),
        ("hud.btn.north", "[ NORTH: 528Hz ]", 380, 650, 140, 36, (0.35, 0.20, 0.50), "farlands-pillar-north", 12),
        ("hud.btn.south", "[ SOUTH: 396Hz ]", 530, 650, 140, 36, (0.18, 0.32, 0.52), "farlands-pillar-south", 12),
        ("hud.btn.east", "[ EAST: 639Hz ]", 680, 650, 140, 36, (0.18, 0.45, 0.32), "farlands-pillar-east", 12),
        ("hud.btn.west", "[ WEST: 741Hz ]", 830, 650, 140, 36, (0.52, 0.35, 0.18), "farlands-pillar-west", 12),
        ("hud.btn.theme", "[ ATMOSPHERE: AURORA / VOID ]", 980, 650, 240, 36, (0.45, 0.38, 0.20), "farlands-theme-toggle", 12),
    ]

    for btn_id, label, bx, by, bw, bh, rgb, role, tsize in hud_buttons:
        btn_obj = button2d(btn_id, label, bx, by, bw, bh, rgb, role, z=35, text_size=tsize)
        zone_objects.append(btn_obj)
        relations.append(instance_rel(btn_id, "category.control.button"))

    # -------------------------------------------------------------------------
    # 9. Autonomic Laws
    # -------------------------------------------------------------------------
    # Law 1: Central Singularity Pulse Law
    add_law(
        "law-farlands-pulse",
        "Far Lands: Sound Resonance Pulse on Core Activation",
        0,  # OnDemand / Action
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "farlands-pulse")),
        seq(
            play_audio("acoustic.frequency", "acoustic.amplitude", "crystal"),
            map_path("@state.farlands.pulseCount", {"c": "@state.farlands.pulseCount"}, offset_terms("c", 1.0)),
            publish("farlands-pulsed", "state.farlands"),
        ),
        scope=1,
    )

    # Law 2: Full Cosmic Polyphonic Chord Law
    add_law(
        "law-farlands-chord",
        "Far Lands: Sound Polyphonic Celestial Chord",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "farlands-chord")),
        seq(
            play_audio("acoustic.frequency", "acoustic.amplitude", "crystal"),
            play_audio("acoustic.frequency", "acoustic.amplitude", "sine"),
            map_path("@state.farlands.pulseCount", {"c": "@state.farlands.pulseCount"}, offset_terms("c", 1.0)),
            set_path("@hud.telemetry.resonance.label2D", pv("string", "RESONANCE: CELESTIAL POLYPHONIC CHORD")),
            publish("farlands-chord-sounded", "state.farlands"),
        ),
        scope=1,
    )

    # Laws 3-6: Cardinal Pylon Tones & Resonance Routing
    pylon_laws = [
        ("north", "North Chrono-Curtain Harmonic", "farlands-pillar-north", "RESONANCE: NORTH PILLAR (528 Hz - CHRONO CURTAIN)"),
        ("south", "South Abyssal Rift Harmonic", "farlands-pillar-south", "RESONANCE: SOUTH PILLAR (396 Hz - ABYSSAL RIFT)"),
        ("east", "East Lattice Frontier Harmonic", "farlands-pillar-east", "RESONANCE: EAST PILLAR (639 Hz - LATTICE FRONTIER)"),
        ("west", "West Solar Horizon Harmonic", "farlands-pillar-west", "RESONANCE: WEST PILLAR (741 Hz - SOLAR HORIZON)"),
    ]

    for slug, law_title, role_val, readout_text in pylon_laws:
        add_law(
            f"law-farlands-{slug}",
            f"Far Lands: Sound {law_title}",
            0,
            ["control-activated"],
            compare("buttonRole", 0, pv("string", role_val)),
            seq(
                play_audio("acoustic.frequency", "acoustic.amplitude", "crystal"),
                set_path("@hud.telemetry.resonance.label2D", pv("string", readout_text)),
                publish(f"farlands-{slug}-activated", f"object.farlands.crystal.{slug}"),
            ),
            scope=1,
        )

    # Law 7: Atmosphere Theme Toggle
    add_law(
        "law-farlands-theme-toggle",
        "Far Lands: Toggle Atmosphere Between Aurora & Void",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "farlands-theme-toggle")),
        seq(
            play_audio("acoustic.frequency", "acoustic.amplitude", "triangle"),
            map_path("@state.farlands.themeMode", {"m": "@state.farlands.themeMode"}, [
                {"c": 1.0, "factors": {}},
                {"c": -1.0, "factors": {"m": 1.0}}
            ]),
            set_path("@hud.telemetry.field.label2D", pv("string", "FIELD: ATMOSPHERIC POLARITY INVERTED")),
            publish("farlands-theme-toggled", "state.farlands"),
        ),
        scope=1,
    )

    # -------------------------------------------------------------------------
    # 10. Materials & Zone Assembly
    # -------------------------------------------------------------------------
    materials = [
        {
            "name": "material.farlands.terrain",
            "baseColor": [0.10, 0.55, 0.32],
            "ambient": 0.30,
            "diffuse": 0.85,
            "specular": 0.35,
            "shininess": 28.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.rock",
            "baseColor": [0.18, 0.20, 0.24],
            "ambient": 0.22,
            "diffuse": 0.78,
            "specular": 0.25,
            "shininess": 20.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.obsidian",
            "baseColor": [0.06, 0.07, 0.10],
            "ambient": 0.20,
            "diffuse": 0.70,
            "specular": 0.85,
            "shininess": 96.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.beacon",
            "baseColor": [0.25, 0.95, 1.00],
            "ambient": 0.75,
            "diffuse": 0.95,
            "specular": 0.95,
            "shininess": 128.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.gold",
            "baseColor": [0.95, 0.78, 0.32],
            "ambient": 0.38,
            "diffuse": 0.85,
            "specular": 0.80,
            "shininess": 64.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.amethyst",
            "baseColor": [0.75, 0.30, 0.95],
            "ambient": 0.70,
            "diffuse": 0.90,
            "specular": 0.90,
            "shininess": 96.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.emerald",
            "baseColor": [0.20, 0.92, 0.55],
            "ambient": 0.70,
            "diffuse": 0.90,
            "specular": 0.85,
            "shininess": 80.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.amber",
            "baseColor": [1.00, 0.65, 0.20],
            "ambient": 0.70,
            "diffuse": 0.90,
            "specular": 0.85,
            "shininess": 80.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.platform",
            "baseColor": [0.14, 0.16, 0.20],
            "ambient": 0.28,
            "diffuse": 0.72,
            "specular": 0.35,
            "shininess": 24.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.safezone",
            "baseColor": [0.12, 0.13, 0.16],
            "ambient": 0.30,
            "diffuse": 0.75,
            "specular": 0.40,
            "shininess": 32.0,
            "opacity": 1.0,
        },
    ]

    zone = {
        "injected_by": "Gemini Spark (authored)",
        "authors": ["Zach"],
        "name": ZONE_ID,
        "identifier": ZONE_ID,
        "owner": "Player",
        "parentZone": "",
        "scope": "Local",
        "qualities": {"kind": "farlands"},
        "world": {"objects": zone_objects},
        "formationRelations": relations,
    }

    session = {
        "saveFormat": "zone-identity-v1",
        "injected_by": "Gemini Spark (authored)",
        "authors": ["Zach"],
        "currentZone": 0,
        "currentZoneId": ZONE_ID,
        "flying": True,
        "cameraPos": [0.0, 3.5, 6.0],
        "cameraFront": [0.0, -0.05, -1.0],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": -90.0,
        "pitch": -3.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": materials,
        "categories": categories,
        "zoneRefs": [{"identifier": ZONE_ID, "kind": "farlands"}],
        "zones": [zone],
        "authoredLaws": {
            "firstMoverEnabled": {
                "physics-gravity": True,
                "physics-kinematics": True,
                "physics-acoustics": True,
                "interaction-channel": True,
                "locomotion-channel": True,
                "creation-channel": True,
                "shape-generator-3d-law": False,
                "tool-create-3d-law": False,
            },
            "formationMembers": FORMATION,
            "laws": LAWS,
            "triggers": TRIGGERS,
        },
    }

    return session, zone


def main():
    parser = argparse.ArgumentParser(description="Author the Far Lands save files.")
    parser.add_argument("--force", action="store_true",
                        help="overwrite an existing Zone identity file (a .bak is kept)")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    session, zone = build_world()
    world_path = root / "saves" / "worlds" / "far_lands.json"
    zone_path = root / "saves" / "zones" / ZONE_ID / "zone.json"

    if zone_path.exists() and not args.force:
        print(f"REFUSED: {zone_path} already exists.")
        print("  Re-run with --force to replace it.")
        return 1

    world_path.parent.mkdir(parents=True, exist_ok=True)
    zone_path.parent.mkdir(parents=True, exist_ok=True)

    backups = []
    for existing in (world_path, zone_path):
        if existing.exists():
            backup = existing.with_suffix(existing.suffix + ".bak")
            backup.write_text(existing.read_text())
            backups.append(backup)

    shadowed = []
    for ext in (".ecform", ".ecmatter"):
        stale = world_path.with_suffix(ext)
        if stale.exists():
            stale.unlink()
            shadowed.append(stale)

    world_path.write_text(json.dumps(session, indent=2) + "\n")
    zone_path.write_text(json.dumps(zone, indent=2) + "\n")

    print(f"Authored {world_path}")
    print(f"Authored {zone_path}")
    for b in backups:
        print(f"  Backed up      {b}")
    for sh in shadowed:
        print(f"  Removed stale  {sh}")
    print(f"  Zone Objects: {len(zone['world']['objects'])}")
    print(f"  Relations: {len(zone['formationRelations'])}")
    print(f"  Laws: {len(LAWS)}")
    print(f"  Author: {AUTHOR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
