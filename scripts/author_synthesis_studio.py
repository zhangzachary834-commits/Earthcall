#!/usr/bin/env python3
"""First-Mover authoring of the Synthesis Studio interactive widget app.

Demonstrates 2D/3D visual block widgets (Buttons, Toggles, Sliders, Chord Pads)
and interactive art strokes authored completely as Laws, Singulars, Relations,
and Formations in strict accordance with AGENTS.md and the 7 Refusals.

Author: Gemini Spark
Injected by: Gemini Spark
Stakeholder: Zach
"""

from __future__ import annotations

import base64
import json
from pathlib import Path

AUTHOR = "author.gemini-spark"
ZONE_ID = "SynthesisStudio"


def pv(t, v):
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


def map_path(path, bindings, terms):
    return {
        "kind": 8,
        "path": path,
        "bindings": bindings,
        "function": {"pieces": [{"expr": {"terms": terms}}]},
    }


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
    # 1. Categories
    categories = [
        category_being(AUTHOR, "Gemini Spark"),
        category_being("grok-4.6", "Grok 4.6"),
        category_being("Zach", "Zachary Zhang"),
        category_being("category.control", "UI Controls"),
        category_being("category.control.button", "Action Button"),
        category_being("category.control.toggle", "Toggle Switch"),
        category_being("category.control.slider", "Continuous Slider"),
        category_being("category.art", "Digital Art"),
        category_being("category.art.stroke", "Authored Stroke"),
        category_being("category.interactive.orb", "Harmonic Resonance Orb"),
    ]

    relations = [
        subcategory_rel("category.control.button", "category.control"),
        subcategory_rel("category.control.toggle", "category.control"),
        subcategory_rel("category.control.slider", "category.control"),
        subcategory_rel("category.art.stroke", "category.art"),
        instance_rel("category.control", AUTHOR),
        instance_rel("category.art", AUTHOR),
    ]

    # 2. Zone Objects (The Studio Environment and Widgets)
    zone_objects = []

    def make_face_colors(rgb):
        r, g, b = rgb
        return [[r, g, b] for _ in range(6)]

    # Studio Floor
    zone_objects.append({
        "objectID": "studio.platform.floor",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -0.1, 0.0, (12.0, 0.2, 12.0)),
        "center": [0.0, -0.1, 0.0],
        "materialId": "material.studio.slate",
        "faceColors": make_face_colors((0.18, 0.20, 0.24)),
        "authoredProperties": {
            "displayName": pv("string", "Studio Platform Floor"),
            "isStudioSurface": pv("bool", True),
        },
    })

    # Studio Central Console Table
    zone_objects.append({
        "objectID": "studio.console.desk",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.4, 0.0, (4.8, 0.8, 2.2)),
        "center": [0.0, 0.4, 0.0],
        "materialId": "material.studio.walnut",
        "faceColors": make_face_colors((0.12, 0.13, 0.16)),
        "authoredProperties": {
            "displayName": pv("string", "Studio Control Console"),
        },
    })

    # Drawing Easel / Canvas Board
    zone_objects.append({
        "objectID": "studio.easel.canvas",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 2.1, 2.2, (3.8, 2.4, 0.1)),
        "center": [0.0, 2.1, 2.2],
        "materialId": "material.studio.canvas",
        "faceColors": make_face_colors((0.92, 0.90, 0.86)),
        "authoredProperties": {
            "displayName": pv("string", "Interactive Art Canvas"),
            "isCanvas": pv("bool", True),
        },
    })

    # Harmonic Pedestal
    zone_objects.append({
        "objectID": "studio.pedestal.crystal",
        "shapeKind": 2, # Cylinder
        "geometryType": 2,
        "shapeParams": [0.4, 0.4, 0.4, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.4, 1.2, (0.8, 0.8, 0.8)),
        "center": [0.0, 0.4, 1.2],
        "materialId": "material.studio.crystal",
        "faceColors": make_face_colors((0.3, 0.75, 0.9)),
        "authoredProperties": {
            "displayName": pv("string", "Harmonic Crystal Pedestal"),
        },
    })

    # -----------------------------------------------------------------------
    # Widget 1: Spawn Harmonic Orb Button
    # -----------------------------------------------------------------------
    btn_orb_id = "studio.btn.spawn-orb"
    zone_objects.append({
        "objectID": btn_orb_id,
        "shapeKind": 0, # Flat block button
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-1.4, 0.88, -0.35, (0.7, 0.16, 0.55)),
        "center": [-1.4, 0.88, -0.35],
        "materialId": "material.studio.btn.blue",
        "faceColors": make_face_colors((0.15, 0.55, 0.95)),
        "authoredProperties": {
            "displayName": pv("string", "Button: Spawn Orb"),
            "controlLabel": pv("string", "Spawn Harmonic Orb"),
            "buttonRole": pv("string", "spawn-orb"),
            "restY": pv("double", 0.88),
        },
    })
    relations.append(instance_rel(btn_orb_id, "category.control.button"))

    # -----------------------------------------------------------------------
    # Widget 2: Ambient Theme Toggle Switch
    # -----------------------------------------------------------------------
    btn_toggle_id = "studio.btn.toggle-theme"
    zone_objects.append({
        "objectID": btn_toggle_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-0.5, 0.88, -0.35, (0.7, 0.16, 0.55)),
        "center": [-0.5, 0.88, -0.35],
        "materialId": "material.studio.btn.gold",
        "faceColors": make_face_colors((1.0, 0.65, 0.1)),
        "authoredProperties": {
            "displayName": pv("string", "Toggle: Ambient Theme"),
            "controlLabel": pv("string", "Toggle Night/Day Ambient"),
            "buttonRole": pv("string", "toggle-theme"),
            "controlOn": pv("bool", False),
            "restY": pv("double", 0.88),
        },
    })
    relations.append(instance_rel(btn_toggle_id, "category.control.toggle"))

    # -----------------------------------------------------------------------
    # Widget 3: Synthesizer Chord Pads (4 Musical Blocks)
    # -----------------------------------------------------------------------
    chord_pads = [
        ("studio.pad.c5", "Chord Pad: C5", 0.4, (0.95, 0.22, 0.32), 523.25, "C5"),
        ("studio.pad.e5", "Chord Pad: E5", 0.95, (1.0, 0.72, 0.12), 659.25, "E5"),
        ("studio.pad.g5", "Chord Pad: G5", 1.5, (0.18, 0.85, 0.42), 783.99, "G5"),
        ("studio.pad.b5", "Chord Pad: B5", 2.05, (0.28, 0.45, 1.0), 987.77, "B5"),
    ]
    for pad_id, pad_name, pos_x, rgb, freq, note in chord_pads:
        zone_objects.append({
            "objectID": pad_id,
            "shapeKind": 0,
            "geometryType": 0,
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(pos_x, 0.87, -0.35, (0.45, 0.14, 0.5)),
            "center": [pos_x, 0.87, -0.35],
            "materialId": f"material.studio.{note}",
            "faceColors": make_face_colors(rgb),
            "authoredProperties": {
                "displayName": pv("string", pad_name),
                "controlLabel": pv("string", f"Play Note {note}"),
                "noteName": pv("string", note),
                "acoustic.frequency": pv("double", freq),
                "acoustic.amplitude": pv("double", 0.85),
                "acoustic.waveType": pv("string", "crystal"),
                "isChordPad": pv("bool", True),
                "restY": pv("double", 0.87),
            },
        })
        relations.append(instance_rel(pad_id, "category.control.button"))

    # -----------------------------------------------------------------------
    # Widget 4: Continuous Pulse Slider
    # -----------------------------------------------------------------------
    slider_track_id = "studio.slider.track"
    zone_objects.append({
        "objectID": slider_track_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.83, 0.5, (2.6, 0.04, 0.18)),
        "center": [0.0, 0.83, 0.5],
        "materialId": "material.studio.track",
        "faceColors": make_face_colors((0.25, 0.27, 0.32)),
        "authoredProperties": {
            "displayName": pv("string", "Pulse Rate Slider Track"),
        },
    })

    slider_handle_id = "studio.slider.handle"
    zone_objects.append({
        "objectID": slider_handle_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.9, 0.5, (0.35, 0.14, 0.32)),
        "center": [0.0, 0.9, 0.5],
        "materialId": "material.studio.handle",
        "faceColors": make_face_colors((0.92, 0.94, 0.98)),
        "authoredProperties": {
            "displayName": pv("string", "Pulse Rate Handle"),
            "controlLabel": pv("string", "Pulse Rate"),
            "controlValue": pv("double", 1.0),
            "controlMin": pv("double", 0.2),
            "controlMax": pv("double", 3.0),
            "controlStep": pv("double", 0.1),
            "restY": pv("double", 0.9),
        },
    })
    relations.append(instance_rel(slider_handle_id, "category.control.slider"))

    # State Being
    state_id = "state.studio"
    zone_objects.append({
        "objectID": state_id,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0, (0.01, 0.01, 0.01)),
        "center": [0.0, 0.0, 0.0],
        "materialId": "",
        "authoredProperties": {
            "displayName": pv("string", "Studio Ambient State"),
            "themeNight": pv("bool", False),
            "pulseRate": pv("double", 1.0),
            "spawnCount": pv("int", 0),
            "soundFreq": pv("double", 440.0),
            "soundAmp": pv("double", 0.8),
        },
    })

    # -----------------------------------------------------------------------
    # Author Laws
    # -----------------------------------------------------------------------

    # 1. Archetype Button Law (INTERACTION_AS_LAW.md §6a)
    add_law(
        "law-control-button-archetype",
        "Control Archetype: Button Activation",
        0, # Activation::OnEvent
        ["object-clicked"],
        related("instance-of", "category.control.button"),
        publish("control-activated"),
        scope=0, # Scope::Subject
    )

    # 2. Archetype Toggle Law (INTERACTION_AS_LAW.md §6b)
    add_law(
        "law-control-toggle-archetype",
        "Control Archetype: Toggle Flip",
        0,
        ["object-clicked"],
        related("instance-of", "category.control.toggle"),
        seq(
            map_path("controlOn", {"o": "controlOn"}, offset_terms("o", 1.0)), # arithmetic flip mapped in math
            publish("control-activated"),
        ),
        scope=0,
    )

    # 3. Domain Law: Spawn Harmonic Orb when Spawn Button is Activated
    add_law(
        "law-studio-spawn-orb",
        "Studio: Spawn Harmonic Orb on Button Click",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "spawn-orb")),
        seq(
            create_object(
                1, # Sphere
                "interactive.harmonic.orb",
                children=[
                    set_path("scale", pv("vec3", [0.4, 0.4, 0.4])),
                    set_path("color", pv("vec3", [1.0, 0.84, 0.22])),
                    set_path("position", pv("vec3", [0.0, 1.6, 1.2])),
                    set_path("acoustic.frequency", pv("double", 880.0)),
                    set_path("acoustic.amplitude", pv("double", 0.9)),
                    add_relation("", "category.art.stroke", "instance-of"),
                    add_relation("", "category.interactive.orb", "instance-of"),
                ],
            ),
            play_audio("@state.studio.soundFreq", "@state.studio.soundAmp", "crystal"),
            map_path("position.y", {"ry": "restY"}, offset_terms("ry", -0.04)),
            publish("orb-spawned", "state.studio"),
        ),
        scope=0,
    )

    # 4. Domain Law: Toggle Theme Reaction (Switches Platform Visual Styling)
    add_law(
        "law-studio-theme-toggle",
        "Studio: Switch Ambient Theme on Toggle Click",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "toggle-theme")),
        seq(
            play_audio("@state.studio.soundFreq", "@state.studio.soundAmp", "bell"),
            publish("theme-toggled", "state.studio"),
        ),
        scope=0,
    )

    # 5. Domain Law: Synthesizer Chord Pad Activation (Plays Musical Notes)
    add_law(
        "law-studio-pad-play",
        "Studio: Play Musical Note on Chord Pad Click",
        0,
        ["control-activated"],
        compare("isChordPad", 0, pv("bool", True)),
        seq(
            play_audio("acoustic.frequency", "acoustic.amplitude", "crystal"),
            map_path("position.y", {"ry": "restY"}, offset_terms("ry", -0.04)),
            publish("note-played"),
        ),
        scope=0,
    )

    # 6. Tactile Button Release Spring (Restores Y after click)
    add_law(
        "law-studio-button-spring",
        "Studio: Restore Button Elevation",
        0,
        ["object-released"],
        related("instance-of", "category.control.button"),
        map_path("position.y", {"ry": "restY"}, copy_terms("ry")),
        scope=0,
    )

    # 7. Art Tool: Draw Stroke Singulars along cursor trajectory in Draw Mode
    add_law(
        "law-art-stroke-draw",
        "Art Tool: Draw Stroke Singulars along Pointer",
        1, # Activation::WhileTrue
        [],
        all_of(
            compare("@interaction-channel.leftDown", 0, pv("bool", True)),
            compare("@creation-channel.active3DMode", 0, pv("string", "Draw")),
            compare("@interaction-channel.dragging", 0, pv("bool", True)),
        ),
        create_object(
            1, # Sphere
            "art.stroke.segment",
            placement_path="@interaction-channel.pointerWorld",
            children=[
                set_path("scale", pv("vec3", [0.08, 0.08, 0.08])),
                set_path("color", pv("vec3", [1.0, 0.85, 0.15])),
                add_relation("", "category.art.stroke", "instance-of"),
            ],
        ),
        scope=1, # Scope::Everyone
    )

    # 8. Stroke Reactive Acoustic Law: Hovering over drawn stroke plays chime
    add_law(
        "law-stroke-hover-sound",
        "Art Stroke: Sound Chime on Hover",
        0,
        ["object-hover-entered"],
        related("instance-of", "category.art.stroke"),
        play_audio("acoustic.frequency", "acoustic.amplitude", "crystal"),
        scope=0,
    )

    # 9. Stroke Reactive Glow Law: Hovering over stroke illuminates it
    add_law(
        "law-stroke-hover-glow",
        "Art Stroke: Illuminate on Pointer Hover",
        1, # Activation::WhileTrue
        [],
        all_of(
            related("instance-of", "category.art.stroke"),
            compare("@world.pointerOver", 0, pv("bool", True)),
        ),
        set_path("color", pv("vec3", [1.0, 1.0, 1.0])),
        scope=1,
    )

    # Assemble Zone and Session
    zone = {
        "name": ZONE_ID,
        "identifier": ZONE_ID,
        "owner": "Player",
        "parentZone": "",
        "scope": "Local",
        "qualities": {"kind": "studio"},
        "world": {"objects": zone_objects},
        "formationRelations": relations,
    }

    session = {
        "saveFormat": "zone-identity-v1",
        "currentZone": 0,
        "currentZoneId": ZONE_ID,
        "flying": True,
        "cameraPos": [0.0, 2.2, -4.2],
        "cameraFront": [0.0, -0.22, 0.97],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": 90.0,
        "pitch": -12.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": [
            {"name": "material.studio.slate", "baseColor": [0.18, 0.20, 0.24], "opacity": 1.0},
            {"name": "material.studio.walnut", "baseColor": [0.12, 0.13, 0.16], "opacity": 1.0},
            {"name": "material.studio.canvas", "baseColor": [0.92, 0.90, 0.86], "opacity": 1.0},
            {"name": "material.studio.crystal", "baseColor": [0.3, 0.75, 0.9], "opacity": 0.85},
            {"name": "material.studio.btn.blue", "baseColor": [0.15, 0.55, 0.95], "opacity": 1.0},
            {"name": "material.studio.btn.gold", "baseColor": [1.0, 0.65, 0.1], "opacity": 1.0},
        ],
        "categories": categories,
        "zoneRefs": [{"identifier": ZONE_ID, "kind": "studio"}],
        "zones": [zone],
        "authoredLaws": {
            "firstMoverEnabled": {
                "physics-gravity": False,
                "physics-kinematics": False,
                "physics-acoustics": False,
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
    root = Path(__file__).resolve().parents[1]
    session, zone = build_world()
    world_path = root / "saves" / "worlds" / "synthesis_studio.json"
    zone_path = root / "saves" / "zones" / ZONE_ID / "zone.json"
    world_path.parent.mkdir(parents=True, exist_ok=True)
    zone_path.parent.mkdir(parents=True, exist_ok=True)
    world_path.write_text(json.dumps(session, indent=2) + "\n")
    zone_path.write_text(json.dumps(zone, indent=2) + "\n")
    print(f"Authored {world_path}")
    print(f"Authored {zone_path}")
    print(f"  Zone Objects: {len(zone['world']['objects'])}")
    print(f"  Relations: {len(zone['formationRelations'])}")
    print(f"  Laws: {len(LAWS)}")
    print(f"  Author: {AUTHOR}")


if __name__ == "__main__":
    main()
