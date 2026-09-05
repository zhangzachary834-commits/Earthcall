#!/usr/bin/env python3
"""Authoring of the Earthcall Far Lands generative zone and world.

Creates a Zone representing the Far Lands, an iconic concept built entirely
out of Earthcall's generative Laws and OntoMath.
"""

from __future__ import annotations

import argparse
import json
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
        category_being("category.control", "UI Controls"),
        category_being("category.control.button", "Action Button"),
    ]

    relations = [
        subcategory_rel("category.farlands.field", "category.farlands"),
        subcategory_rel("category.farlands.monolith", "category.farlands"),
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

    # Octave 0: Macro-continental rolling terrain
    oct0 = make_octave(0.003, 0.003, 20.0)

    # Octave 1: Rolling foothills and landscape variance
    oct1 = make_octave(0.009, 0.009, 12.0)

    # Octave 2 & 3: Edge Far Lands - Z Axis Vertical Wall Curtains
    # Stretched along X, tightly oscillating along Z -> colossal parallel walls
    oct2 = make_octave(0.004, 0.035, 45.0)
    oct3 = make_octave(0.002, 0.070, 22.0)

    # Octave 4 & 5: Edge Far Lands - X Axis Vertical Wall Curtains
    # Stretched along Z, tightly oscillating along X -> orthogonal cross-walls
    oct4 = make_octave(0.035, 0.004, 45.0)
    oct5 = make_octave(0.070, 0.002, 22.0)

    # Octave 6: Geological striations & micro-wall fluting
    oct6 = make_octave(0.060, 0.060, 8.0)

    # Sum of all octaves: forms Edge Walls, Corner Monolithic Towers, and Canyons
    h_walls = math_add(
        math_add(oct0, oct1),
        math_add(
            math_add(oct2, oct3),
            math_add(math_add(oct4, oct5), oct6)
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
        "fieldExtent": [1500.0, 180.0, 1500.0],
        "field": field_node,
    })
    relations.append(instance_rel(far_lands_field_id, "category.farlands.field"))

    # -------------------------------------------------------------------------
    # 2. Spawn Observation Dais & Horizon Beacon
    # -------------------------------------------------------------------------
    # Instead of an empty, flat rectangular slab in a void, the spawn area
    # features an elegant observation dais nestled into the terrain at origin,
    # accompanied by a monolith beacon and an interactive resonance crystal.
    dais_id = "object.farlands.dais"
    zone_objects.append({
        "objectID": dais_id,
        "shapeKind": 3,  # Cylinder plinth
        "geometryType": 3,
        "shapeParams": [6.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -0.1, 0.0, (12.0, 0.4, 12.0)),
        "center": [0.0, -0.1, 0.0],
        "materialId": "material.farlands.safezone",
        "faceColors": make_face_colors((0.18, 0.18, 0.22)),
        "authoredProperties": {
            "displayName": pv("string", "Spawn Observation Dais"),
        },
    })

    # Slender observation beacon pylon
    beacon_id = "object.farlands.beacon"
    zone_objects.append({
        "objectID": beacon_id,
        "shapeKind": 9,  # RoundedBox
        "geometryType": 9,
        "shapeParams": [0.4, 0.4, 0.4, 1.2, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0],
        "transform": mat4_translate(0.0, 1.2, -4.0, (0.8, 2.4, 0.8)),
        "center": [0.0, 1.2, -4.0],
        "materialId": "material.farlands.rock",
        "faceColors": make_face_colors((0.25, 0.28, 0.32)),
        "authoredProperties": {
            "displayName": pv("string", "Far Lands Horizon Beacon"),
        },
    })
    relations.append(instance_rel(beacon_id, "category.farlands.monolith"))

    # Interactive floating crystal core
    crystal_id = "object.farlands.resonance_crystal"
    zone_objects.append({
        "objectID": crystal_id,
        "shapeKind": 2,  # Sphere
        "geometryType": 2,
        "shapeParams": [0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 2.8, -4.0, (0.8, 0.8, 0.8)),
        "center": [0.0, 2.8, -4.0],
        "materialId": "material.farlands.beacon",
        "faceColors": make_face_colors((0.3, 0.85, 0.95)),
        "authoredProperties": {
            "displayName": pv("string", "Harmonic Resonance Core"),
            "buttonRole": pv("string", "farlands-pulse"),
            "acoustic.frequency": pv("double", 528.0),
            "acoustic.amplitude": pv("double", 0.6),
            "acoustic.waveType": pv("string", "crystal"),
        },
    })
    relations.append(instance_rel(crystal_id, "category.control.button"))

    # -------------------------------------------------------------------------
    # 3. State Container & Dynamic Tracking
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
        },
    })

    # -------------------------------------------------------------------------
    # 4. HUD / Text Elements
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

    zone_objects.append(text2d("hud.title", "EARTHCALL FAR LANDS", 30, 30, 26, (0.4, 0.9, 0.6)))
    zone_objects.append(text2d("hud.subtitle", "Generative OntoMath Landscape — Traverse the mathematical horizon.", 30, 60, 13, (0.7, 0.7, 0.75)))
    zone_objects.append(text2d("hud.hint", "Click the Resonance Core to sound the harmonic pulse.", 30, 82, 12, (0.3, 0.85, 0.95)))

    # -------------------------------------------------------------------------
    # 5. Laws
    # -------------------------------------------------------------------------
    # Law 1: Resonance Crystal Pulse Sound & Event
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

    # -------------------------------------------------------------------------
    # 6. Materials & Zone Assembly
    # -------------------------------------------------------------------------
    materials = [
        {
            "name": "material.farlands.terrain",
            "baseColor": [0.15, 0.65, 0.35],
            "ambient": 0.25,
            "diffuse": 0.85,
            "specular": 0.15,
            "shininess": 12.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.rock",
            "baseColor": [0.25, 0.28, 0.32],
            "ambient": 0.20,
            "diffuse": 0.80,
            "specular": 0.20,
            "shininess": 16.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.beacon",
            "baseColor": [0.30, 0.85, 0.95],
            "ambient": 0.50,
            "diffuse": 0.90,
            "specular": 0.90,
            "shininess": 64.0,
            "opacity": 1.0,
        },
        {
            "name": "material.farlands.safezone",
            "baseColor": [0.18, 0.18, 0.22],
            "ambient": 0.30,
            "diffuse": 0.70,
            "specular": 0.30,
            "shininess": 24.0,
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
        "cameraPos": [0.0, 3.0, 6.0],
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
