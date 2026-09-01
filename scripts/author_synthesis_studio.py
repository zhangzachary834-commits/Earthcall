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

    # 2. Zone Objects (The Studio Environment, 3D Widgets, Signage, and 2D Screen-space HUD)
    zone_objects = []

    def make_face_colors(rgb):
        r, g, b = rgb
        return [[r, g, b] for _ in range(6)]

    # -----------------------------------------------------------------------
    # Environment & Studio Furniture
    # -----------------------------------------------------------------------
    # Studio Floor
    zone_objects.append({
        "objectID": "studio.platform.floor",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -0.1, 0.0, (14.0, 0.2, 14.0)),
        "center": [0.0, -0.1, 0.0],
        "materialId": "material.studio.slate",
        "faceColors": make_face_colors((0.16, 0.18, 0.22)),
        "authoredProperties": {
            "displayName": pv("string", "Studio Platform Floor"),
            "isStudioSurface": pv("bool", True),
        },
    })

    # Studio Console Table (Modern Dark Walnut / Brushed Metal)
    zone_objects.append({
        "objectID": "studio.console.desk",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.4, 0.0, (5.2, 0.8, 2.4)),
        "center": [0.0, 0.4, 0.0],
        "materialId": "material.studio.walnut",
        "faceColors": make_face_colors((0.11, 0.12, 0.15)),
        "authoredProperties": {
            "displayName": pv("string", "Studio Control Console"),
        },
    })

    # Central Easel Canvas (Drawing Board)
    zone_objects.append({
        "objectID": "studio.easel.canvas",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 2.2, 2.4, (4.2, 2.4, 0.1)),
        "center": [0.0, 2.2, 2.4],
        "materialId": "material.studio.canvas",
        "faceColors": make_face_colors((0.94, 0.92, 0.88)),
        "authoredProperties": {
            "displayName": pv("string", "Interactive Art Canvas"),
            "isCanvas": pv("bool", True),
        },
    })

    # Canvas Title Plaque
    zone_objects.append({
        "objectID": "studio.sign.canvas",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 3.55, 2.4, (3.2, 0.22, 0.06)),
        "center": [0.0, 3.55, 2.4],
        "materialId": "material.studio.sign",
        "faceColors": make_face_colors((0.2, 0.24, 0.32)),
        "authoredProperties": {
            "displayName": pv("string", "✎ ART DRAWING CANVAS (Drag to draw light strokes)"),
        },
    })

    # Central Crystal Pedestal
    zone_objects.append({
        "objectID": "studio.pedestal.crystal",
        "shapeKind": 2, # Cylinder
        "geometryType": 2,
        "shapeParams": [0.45, 0.45, 0.45, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.4, 1.25, (0.9, 0.8, 0.9)),
        "center": [0.0, 0.4, 1.25],
        "materialId": "material.studio.crystal",
        "faceColors": make_face_colors((0.35, 0.78, 0.95)),
        "authoredProperties": {
            "displayName": pv("string", "Harmonic Crystal Pedestal"),
        },
    })

    # -----------------------------------------------------------------------
    # 3D In-World Widgets with Beveled Cradles & Labeled Signs
    # -----------------------------------------------------------------------

    # --- Widget 1: Spawn Orb Button ---
    # Base Socket / Cradle
    zone_objects.append({
        "objectID": "studio.btn.spawn-orb.base",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-1.5, 0.82, -0.35, (0.85, 0.08, 0.7)),
        "center": [-1.5, 0.82, -0.35],
        "materialId": "material.studio.socket",
        "faceColors": make_face_colors((0.08, 0.09, 0.12)),
        "authoredProperties": {"displayName": pv("string", "Spawn Button Frame")},
    })
    # Raised Push Button
    btn_orb_id = "studio.btn.spawn-orb"
    zone_objects.append({
        "objectID": btn_orb_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-1.5, 0.89, -0.35, (0.7, 0.16, 0.55)),
        "center": [-1.5, 0.89, -0.35],
        "materialId": "material.studio.btn.blue",
        "faceColors": make_face_colors((0.15, 0.58, 0.98)),
        "authoredProperties": {
            "displayName": pv("string", "✦ Spawn Harmonic Orb"),
            "controlLabel": pv("string", "Spawn Harmonic Orb"),
            "buttonRole": pv("string", "spawn-orb"),
            "restY": pv("double", 0.89),
        },
    })
    relations.append(instance_rel(btn_orb_id, "category.control.button"))

    # Labeled Plaque over Spawn Button
    zone_objects.append({
        "objectID": "studio.sign.spawn-orb",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-1.5, 1.12, -0.35, (0.75, 0.12, 0.04)),
        "center": [-1.5, 1.12, -0.35],
        "materialId": "material.studio.sign.blue",
        "faceColors": make_face_colors((0.12, 0.45, 0.8)),
        "authoredProperties": {"displayName": pv("string", "[ ✦ SPAWN ORB ]")},
    })

    # --- Widget 2: Ambient Theme Toggle ---
    # Base Socket
    zone_objects.append({
        "objectID": "studio.btn.toggle-theme.base",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-0.55, 0.82, -0.35, (0.85, 0.08, 0.7)),
        "center": [-0.55, 0.82, -0.35],
        "materialId": "material.studio.socket",
        "faceColors": make_face_colors((0.08, 0.09, 0.12)),
        "authoredProperties": {"displayName": pv("string", "Theme Toggle Frame")},
    })
    # Raised Toggle Switch
    btn_toggle_id = "studio.btn.toggle-theme"
    zone_objects.append({
        "objectID": btn_toggle_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-0.55, 0.89, -0.35, (0.7, 0.16, 0.55)),
        "center": [-0.55, 0.89, -0.35],
        "materialId": "material.studio.btn.gold",
        "faceColors": make_face_colors((1.0, 0.7, 0.15)),
        "authoredProperties": {
            "displayName": pv("string", "☼ Toggle Night/Day Theme"),
            "controlLabel": pv("string", "Toggle Night/Day Ambient"),
            "buttonRole": pv("string", "toggle-theme"),
            "controlOn": pv("bool", False),
            "restY": pv("double", 0.89),
        },
    })
    relations.append(instance_rel(btn_toggle_id, "category.control.toggle"))

    # Labeled Plaque over Theme Toggle
    zone_objects.append({
        "objectID": "studio.sign.toggle-theme",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(-0.55, 1.12, -0.35, (0.75, 0.12, 0.04)),
        "center": [-0.55, 1.12, -0.35],
        "materialId": "material.studio.sign.gold",
        "faceColors": make_face_colors((0.8, 0.55, 0.1)),
        "authoredProperties": {"displayName": pv("string", "[ ☼ DAY / NIGHT ]")},
    })

    # --- Widget 3: 4 Synthesizer Chord Pads (Piano Keyboard Row) ---
    # Common Socket Frame
    zone_objects.append({
        "objectID": "studio.pads.frame",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(1.28, 0.82, -0.35, (2.35, 0.08, 0.7)),
        "center": [1.28, 0.82, -0.35],
        "materialId": "material.studio.socket",
        "faceColors": make_face_colors((0.08, 0.09, 0.12)),
        "authoredProperties": {"displayName": pv("string", "Chord Pad Frame")},
    })

    chord_pads = [
        ("studio.pad.c5", "♫ Note C5 (Do)", 0.42, (0.95, 0.22, 0.32), 523.25, "C5"),
        ("studio.pad.e5", "♫ Note E5 (Mi)", 0.98, (1.0, 0.72, 0.12), 659.25, "E5"),
        ("studio.pad.g5", "♫ Note G5 (Sol)", 1.54, (0.18, 0.88, 0.45), 783.99, "G5"),
        ("studio.pad.b5", "♫ Note B5 (Ti)", 2.10, (0.28, 0.48, 1.0), 987.77, "B5"),
    ]
    for pad_id, pad_name, pos_x, rgb, freq, note in chord_pads:
        zone_objects.append({
            "objectID": pad_id,
            "shapeKind": 0,
            "geometryType": 0,
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(pos_x, 0.88, -0.35, (0.46, 0.14, 0.54)),
            "center": [pos_x, 0.88, -0.35],
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
                "restY": pv("double", 0.88),
            },
        })
        relations.append(instance_rel(pad_id, "category.control.button"))

    # Labeled Plaque over Chord Pads
    zone_objects.append({
        "objectID": "studio.sign.music",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(1.28, 1.12, -0.35, (2.2, 0.12, 0.04)),
        "center": [1.28, 1.12, -0.35],
        "materialId": "material.studio.sign.dark",
        "faceColors": make_face_colors((0.22, 0.25, 0.32)),
        "authoredProperties": {"displayName": pv("string", "[ ♫ CHORD KEYS: C5 · E5 · G5 · B5 ]")},
    })

    # --- Widget 4: Pulse Resonance Slider ---
    slider_track_id = "studio.slider.track"
    zone_objects.append({
        "objectID": slider_track_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.82, 0.55, (2.8, 0.04, 0.2)),
        "center": [0.0, 0.82, 0.55],
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
        "transform": mat4_translate(0.0, 0.9, 0.55, (0.38, 0.14, 0.34)),
        "center": [0.0, 0.9, 0.55],
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

    # Labeled Plaque over Slider
    zone_objects.append({
        "objectID": "studio.sign.slider",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 1.05, 0.55, (1.8, 0.1, 0.04)),
        "center": [0.0, 1.05, 0.55],
        "materialId": "material.studio.sign.dark",
        "faceColors": make_face_colors((0.25, 0.28, 0.35)),
        "authoredProperties": {"displayName": pv("string", "[ ⟷ RESONANCE SLIDER ]")},
    })

    # -----------------------------------------------------------------------
    # 2D Screen-Space HUD Action Dock (ShapeKind::Shape2D = 12)
    # Direct clickable screen controls overlaying the viewport
    # -----------------------------------------------------------------------
    # 1. Top Screen Banner Card
    zone_objects.append({
        "objectID": "hud.banner.title",
        "shapeKind": 12, # Shape2D
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "x2D": 320.0,
        "y2D": 18.0,
        "zOrder2D": 10,
        "materialId": "",
        "faceColors": make_face_colors((0.10, 0.12, 0.16)),
        "authoredProperties": {
            "displayName": pv("string", "✦ SYNTHESIS STUDIO ✦ Click HUD or 3D Widgets to Interact"),
            "shape.width2D": pv("double", 640.0),
            "shape.height2D": pv("double", 38.0),
        },
    })

    # 2. Bottom Screen Dock Background
    zone_objects.append({
        "objectID": "hud.dock.bg",
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "x2D": 240.0,
        "y2D": 645.0,
        "zOrder2D": 10,
        "materialId": "",
        "faceColors": make_face_colors((0.08, 0.10, 0.14)),
        "authoredProperties": {
            "displayName": pv("string", "HUD Control Dock Background"),
            "shape.width2D": pv("double", 800.0),
            "shape.height2D": pv("double", 55.0),
        },
    })

    # 3. 2D HUD Button: Spawn Orb
    hud_btn_spawn = "hud.btn.spawn-orb"
    zone_objects.append({
        "objectID": hud_btn_spawn,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "x2D": 255.0,
        "y2D": 653.0,
        "zOrder2D": 20,
        "materialId": "",
        "faceColors": make_face_colors((0.15, 0.58, 0.98)),
        "authoredProperties": {
            "displayName": pv("string", "HUD: Spawn Orb"),
            "controlLabel": pv("string", "SPAWN ORB"),
            "buttonRole": pv("string", "spawn-orb"),
            "shape.width2D": pv("double", 130.0),
            "shape.height2D": pv("double", 38.0),
        },
    })
    relations.append(instance_rel(hud_btn_spawn, "category.control.button"))

    # 4. 2D HUD Button: Theme Toggle
    hud_btn_theme = "hud.btn.toggle-theme"
    zone_objects.append({
        "objectID": hud_btn_theme,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "x2D": 395.0,
        "y2D": 653.0,
        "zOrder2D": 20,
        "materialId": "",
        "faceColors": make_face_colors((1.0, 0.70, 0.15)),
        "authoredProperties": {
            "displayName": pv("string", "HUD: Day/Night"),
            "controlLabel": pv("string", "DAY / NIGHT"),
            "buttonRole": pv("string", "toggle-theme"),
            "shape.width2D": pv("double", 120.0),
            "shape.height2D": pv("double", 38.0),
        },
    })
    relations.append(instance_rel(hud_btn_theme, "category.control.toggle"))

    # 5. 2D HUD Chord Keys (C5, E5, G5, B5)
    hud_pads = [
        ("hud.pad.c5", "HUD: Note C5", 525.0, (0.95, 0.22, 0.32), 523.25, "C5"),
        ("hud.pad.e5", "HUD: Note E5", 585.0, (1.0, 0.72, 0.12), 659.25, "E5"),
        ("hud.pad.g5", "HUD: Note G5", 645.0, (0.18, 0.88, 0.45), 783.99, "G5"),
        ("hud.pad.b5", "HUD: Note B5", 705.0, (0.28, 0.48, 1.0), 987.77, "B5"),
    ]
    for hpad_id, hpad_name, x_pos, rgb, freq, note in hud_pads:
        zone_objects.append({
            "objectID": hpad_id,
            "shapeKind": 12,
            "geometryType": 12,
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(0.0, 0.0, 0.0),
            "center": [0.0, 0.0, 0.0],
            "x2D": x_pos,
            "y2D": 653.0,
            "zOrder2D": 20,
            "materialId": "",
            "faceColors": make_face_colors(rgb),
            "authoredProperties": {
                "displayName": pv("string", hpad_name),
                "controlLabel": pv("string", note),
                "noteName": pv("string", note),
                "acoustic.frequency": pv("double", freq),
                "acoustic.amplitude": pv("double", 0.85),
                "acoustic.waveType": pv("string", "crystal"),
                "isChordPad": pv("bool", True),
                "shape.width2D": pv("double", 52.0),
                "shape.height2D": pv("double", 38.0),
            },
        })
        relations.append(instance_rel(hpad_id, "category.control.button"))

    # 6. 2D HUD Button: Draw Stroke Mode Indicator
    hud_btn_draw = "hud.btn.draw-stroke"
    zone_objects.append({
        "objectID": hud_btn_draw,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, 0.0, 0.0),
        "center": [0.0, 0.0, 0.0],
        "x2D": 768.0,
        "y2D": 653.0,
        "zOrder2D": 20,
        "materialId": "",
        "faceColors": make_face_colors((0.72, 0.32, 0.95)),
        "authoredProperties": {
            "displayName": pv("string", "HUD: Draw Canvas Mode"),
            "controlLabel": pv("string", "DRAW STROKES"),
            "buttonRole": pv("string", "draw-mode"),
            "shape.width2D": pv("double", 155.0),
            "shape.height2D": pv("double", 38.0),
        },
    })
    relations.append(instance_rel(hud_btn_draw, "category.control.button"))

    # Studio State Being
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
            map_path("controlOn", {"o": "controlOn"}, offset_terms("o", 1.0)),
            publish("control-activated"),
        ),
        scope=0,
    )

    # 3. Domain Law: Spawn Harmonic Orb on Button Click
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
                    set_path("scale", pv("vec3", [0.45, 0.45, 0.45])),
                    set_path("color", pv("vec3", [1.0, 0.85, 0.22])),
                    set_path("position", pv("vec3", [0.0, 1.6, 1.25])),
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

    # 4. Domain Law: Switch Ambient Theme on Toggle Click
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

    # 5. Domain Law: Synthesizer Chord Pad Activation
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

    # 6. Tactile Button Release Spring
    add_law(
        "law-studio-button-spring",
        "Studio: Restore Button Elevation",
        0,
        ["object-released"],
        related("instance-of", "category.control.button"),
        map_path("position.y", {"ry": "restY"}, copy_terms("ry")),
        scope=0,
    )

    # 7. Art Tool: Draw Stroke Singulars along Pointer Trajectory
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

    # 8. Stroke Reactive Acoustic Law: Hovering over stroke plays chime
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
        # Camera placed at comfortable eye level, looking forward and slightly down onto the console and canvas
        "cameraPos": [0.0, 1.35, -2.6],
        "cameraFront": [0.0, -0.22, 0.97],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": 90.0,
        "pitch": -13.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": [
            {"name": "material.studio.slate", "baseColor": [0.16, 0.18, 0.22], "opacity": 1.0},
            {"name": "material.studio.walnut", "baseColor": [0.11, 0.12, 0.15], "opacity": 1.0},
            {"name": "material.studio.canvas", "baseColor": [0.94, 0.92, 0.88], "opacity": 1.0},
            {"name": "material.studio.crystal", "baseColor": [0.35, 0.78, 0.95], "opacity": 0.85},
            {"name": "material.studio.btn.blue", "baseColor": [0.15, 0.58, 0.98], "opacity": 1.0},
            {"name": "material.studio.btn.gold", "baseColor": [1.0, 0.70, 0.15], "opacity": 1.0},
            {"name": "material.studio.sign", "baseColor": [0.2, 0.24, 0.32], "opacity": 1.0},
            {"name": "material.studio.socket", "baseColor": [0.08, 0.09, 0.12], "opacity": 1.0},
            {"name": "material.studio.track", "baseColor": [0.25, 0.27, 0.32], "opacity": 1.0},
            {"name": "material.studio.handle", "baseColor": [0.92, 0.94, 0.98], "opacity": 1.0},
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
