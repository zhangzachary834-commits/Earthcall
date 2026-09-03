#!/usr/bin/env python3
"""First-Mover authoring of the Earthcall Far Lands generative app.

This creates a Zone representing the Far Lands, a Minecraft-style concept built entirely
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


def compare(path, op, operand=None, operand_path=None):
    node = {"kind": 0, "path": path, "op": op}
    if operand is not None:
        node["operand"] = operand
    if operand_path is not None:
        node["operandPath"] = operand_path
    return node


def related(rel_type, other):
    return {"kind": 2, "relationType": rel_type, "otherId": other}


def all_of(*children):
    return {"kind": 3, "children": list(children)}


def any_of(*children):
    return {"kind": 4, "children": list(children)}


def not_of(child):
    return {"kind": 5, "children": [child]}


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


def flow_path(path, bindings, terms=None, pieces=None, input_var=None):
    fn = {"pieces": pieces if pieces is not None else [{"expr": {"terms": terms or []}}]}
    if input_var:
        fn["input"] = input_var
    return {"kind": 9, "path": path, "bindings": bindings, "function": fn}


def piece(terms, lo=None, hi=None):
    p = {"expr": {"terms": terms}}
    if lo is not None:
        p["lo"] = float(lo)
        p["includeLo"] = True
    if hi is not None:
        p["hi"] = float(hi)
        p["includeHi"] = True
    return p


def clamp_pieces(var, lo, hi, scale=1.0, offset=0.0):
    return [
        piece([{"c": scale * lo + offset, "factors": {}}], hi=lo),
        piece([{"c": scale, "factors": {var: 1.0}}, {"c": offset, "factors": {}}], lo=lo, hi=hi),
        piece([{"c": scale * hi + offset, "factors": {}}], lo=hi),
    ]


def sin_factor(var, scale=1.0, shift=0.0):
    return {"kind": 0, "var": var, "scale": float(scale), "shift": float(shift)}


def cos_factor(var, scale=1.0, shift=0.0):
    return {"kind": 1, "var": var, "scale": float(scale), "shift": float(shift)}


def wave_term(c, factors, trans):
    return {"c": float(c), "factors": factors, "trans": trans}


def flip_terms(var):
    return [{"c": 1.0, "factors": {}}, {"c": -1.0, "factors": {var: 1.0}}]


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


def add_relation(source_token, target_token, rel_type):
    return {
        "kind": 20,
        "sourceToken": source_token,
        "targetToken": target_token,
        "relationType": rel_type,
    }


def create_object(shape_kind, create_type, placement_path="", children=None):
    node = {
        "kind": 11,
        "shapeKind": shape_kind,
        "createType": create_type,
    }
    if placement_path:
        node["spawnPlacementPath"] = placement_path
    if children:
        node["children"] = list(children)
    return node


def copy_terms(var):
    return [{"c": 1.0, "factors": {var: 1.0}}]


def offset_terms(var, offset):
    return [
        {"c": 1.0, "factors": {var: 1.0}},
        {"c": float(offset), "factors": {}},
    ]


def const_terms(value):
    return [{"c": float(value), "factors": {}}]


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
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
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
        category_being("category.farlands.control", "Far Lands Control"),
        category_being("category.control", "UI Controls"),
        category_being("category.control.slider", "Continuous Slider"),
        category_being("category.control.button", "Action Button"),
    ]

    relations = [
        subcategory_rel("category.farlands.field", "category.farlands"),
        subcategory_rel("category.farlands.control", "category.farlands"),
        subcategory_rel("category.control.slider", "category.control"),
        subcategory_rel("category.control.button", "category.control"),
        authored_by_rel("category.farlands", AUTHOR),
    ]

    zone_objects = []

    def make_face_colors(rgb):
        r, g, b = rgb
        return [[r, g, b] for _ in range(6)]

    # 1. The Core Far Lands Math Field! (ShapeKind 10: Field)
    far_lands_field_id = "object.farlands.core_field"
    zone_objects.append({
        "objectID": far_lands_field_id,
        "shapeKind": 10,
        "geometryType": 10,
        "shapeParams": [1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0, (1000.0, 1000.0, 1000.0)),
        "center": [0.0, 0.0, 0.0],
        "materialId": "material.farlands.terrain",
        "faceColors": make_face_colors((0.1, 0.8, 0.2)),
        "authoredProperties": {
            "displayName": pv("string", "The Far Lands Spatial Math Field"),
            "field.frequency": pv("double", 1.0),
            "field.amplitude": pv("double", 1.0),
            "farlands.active": pv("bool", True),
            # Explicit SDF node mapping an OntoMath Noise (Perlin-style) as our base
            # which will be modulated by Laws to create the Far Lands effect.
            "astDefinition": pv("string", """
            {
                "kind": "sdf",
                "op": "noise",
                "scale": 1.0,
                "children": [
                    {"kind": "var", "name": "position"}
                ]
            }
            """)
        },
    })
    relations.append(instance_rel(far_lands_field_id, "category.farlands.field"))

    # 2. State Container for distance tracking and dynamic modifiers
    state_id = "state.farlands"
    zone_objects.append({
        "objectID": state_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -10.0, 0.0, (0.01, 0.01, 0.01)),
        "center": [0.0, -10.0, 0.0],
        "materialId": "material.farlands.dark",
        "faceColors": make_face_colors((0.1, 0.1, 0.1)),
        "authoredProperties": {
            "displayName": pv("string", "Far Lands State"),
            "distanceFromOrigin": pv("double", 0.0),
            "corruptionLevel": pv("double", 0.0),
            "musicIntensity": pv("double", 0.0),
        },
    })

    # 3. Base Platform so we don't fall infinitely right away
    zone_objects.append({
        "objectID": "object.farlands.spawn_platform",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -2.0, 0.0, (20.0, 1.0, 20.0)),
        "center": [0.0, -2.0, 0.0],
        "materialId": "material.farlands.safezone",
        "faceColors": make_face_colors((0.4, 0.4, 0.5)),
        "authoredProperties": {
            "displayName": pv("string", "Spawn Safezone"),
        },
    })

    # 4. HUD / Text Elements for flavor
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

    zone_objects.append(text2d("hud.title", "EARTHCALL FAR LANDS", 30, 30, 28, (0.8, 0.2, 0.8)))
    zone_objects.append(text2d("hud.subtitle", "Walk outward to witness math corrupt geometry.", 30, 60, 14, (0.6, 0.6, 0.6)))
    zone_objects.append(text2d("hud.dist", "Distance: [calculated dynamically]", 30, 90, 14, (0.9, 0.9, 0.2)))


    # --- LAWS ---

    # Law 1: Calculate Distance from Origin using a Law Map (approximate length via max of absolute x/z for grid-like expansion, or simple x+z if we want linear drift).
    # Since we don't have a direct sqrt in our basic nodes, we'll use a fast Manhattan-ish distance or just rely on Z-axis movement for demo.
    # Let's say Far Lands is reached by moving along the Z axis (forward/back) primarily, or X. We'll sum abs(X) and abs(Z).
    add_law(
        "law-farlands-calc-dist",
        "Calculate Player Distance from Center",
        1, # WhileTrue
        [],
        # Target the state object
        compare("displayName", 0, pv("string", "Far Lands State")),
        seq(
            # Using Map to grab locomotion channel's world X and Z, approximate distance
            map_path("distanceFromOrigin", {"x": "@locomotion-channel.playerX", "z": "@locomotion-channel.playerZ"},
                     pieces=[piece([
                         # naive dist = x*x + z*z mapped directly, or just x+z.
                         # Actually we can just do Math: |x| + |z|. Wait, OntoMath doesn't have an easy abs here unless we do piecewise.
                         # Let's just do distance = (x*x + z*z) * 0.01 for simplicity!
                         {"c": 0.01, "factors": {"x": 2.0}},
                         {"c": 0.01, "factors": {"z": 2.0}},
                     ])])
        ),
        scope=1
    )

    # Law 2: Modulate Field Frequency based on Distance
    # This is Path A Procedural math integration from FAR_LANDS_FRAMEWORK.md!
    # "flow @field.frequency += 0.01 * dt"
    # Wait, if we use distance to directly drive frequency: frequency = 1.0 + dist * 0.005
    add_law(
        "law-farlands-field-corruption",
        "Modulate Terrain Math Field Based on Distance",
        1,
        [],
        related("instance-of", "category.farlands.field"),
        seq(
            # As distance grows, field frequency increases, causing the geometry to become wilder and more corrugated
            map_path("field.frequency", {"d": "@state.farlands.distanceFromOrigin"},
                     terms=[
                         {"c": 1.0, "factors": {}},
                         {"c": 0.005, "factors": {"d": 1.0}}
                     ]),
            # Amplitude also increases slightly to make the terrain "fold" higher
            map_path("field.amplitude", {"d": "@state.farlands.distanceFromOrigin"},
                     terms=[
                         {"c": 1.0, "factors": {}},
                         {"c": 0.002, "factors": {"d": 1.0}}
                     ])
        ),
        scope=1
    )

    # Law 3: Update HUD distance
    add_law(
        "law-farlands-hud-sync",
        "Update HUD with current corruption level",
        1,
        [],
        compare("displayName", 0, pv("string", "Distance: [calculated dynamically]")),
        # We can't directly string-concat in our basic Actions easily, so we will just change color as it gets further!
        seq(
            map_path("color.r", {"d": "@state.farlands.distanceFromOrigin"},
                     pieces=clamp_pieces("d", 0.0, 1000.0, scale=0.001, offset=0.2), input_var="d"),
            map_path("color.g", {"d": "@state.farlands.distanceFromOrigin"},
                     pieces=clamp_pieces("d", 0.0, 1000.0, scale=-0.001, offset=0.8), input_var="d"),
            map_path("color.b", {"d": "@state.farlands.distanceFromOrigin"},
                     pieces=clamp_pieces("d", 0.0, 1000.0, scale=0.001, offset=0.2), input_var="d"),
        ),
        scope=1
    )


    # Law 4: The Sound of the Far Lands
    # From the docs: "The recursive farLayer function, evaluated with time as a variable instead of spatial coordinates, produces a pressure wave."
    # We will dynamically play a sound whose frequency and amplitude shift with distance.
    add_law(
        "law-farlands-ambient-sound",
        "Ambient Harmonic Resonance of the Far Lands",
        1, # WhileTrue
        [],
        # Target the core field, so it emits the sound
        related("instance-of", "category.farlands.field"),
        seq(
            map_path("acoustic.frequency", {"d": "@state.farlands.distanceFromOrigin"},
                     # Base low drone (40Hz) that goes down to infrasound floor as distance increases!
                     # 40 - dist * 0.05
                     terms=[
                         {"c": 40.0, "factors": {}},
                         {"c": -0.05, "factors": {"d": 1.0}}
                     ]),
            map_path("acoustic.amplitude", {"d": "@state.farlands.distanceFromOrigin"},
                     pieces=clamp_pieces("d", 0.0, 5000.0, scale=0.0001, offset=0.1), input_var="d"),
            set_path("acoustic.waveType", pv("string", "sine")),
            set_path("acoustic.isSoundEmitter", pv("bool", True))
        ),
        scope=1
    )


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
        "cameraPos": [0.0, 2.0, 5.0],
        "cameraFront": [0.0, 0.0, -1.0],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": -90.0,
        "pitch": 0.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": [
            {"name": "material.farlands.safezone", "baseColor": [0.4, 0.4, 0.5], "opacity": 1.0},
            {"name": "material.farlands.terrain", "baseColor": [0.1, 0.8, 0.2], "opacity": 1.0},
            {"name": "material.farlands.dark", "baseColor": [0.1, 0.1, 0.1], "opacity": 1.0},
        ],
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
