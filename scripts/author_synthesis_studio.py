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

import argparse
import json
from pathlib import Path

AUTHOR = "author.gemini-spark"
ZONE_ID = "SynthesisStudio"


def pv(t, v):
    """A PropertyValue, in the shape propertyValueFromJson actually reads.

    A vec3 does NOT serialize as {"t":"vec3","v":[x,y,z]} — it serializes as
    {"t":"vec3","x":..,"y":..,"z":..} (Serialization's propertyValueToJson /
    propertyValueFromJson). Every vec3 this file authored used the "v" form, so
    every one of them deserialized to the ZERO VECTOR and nothing ever noticed,
    because nothing read one back: the orbs' authored position (0, 1.6, 1.25)
    was really (0,0,0) — they spawned inside the floor at the world origin —
    the stroke colour was black, and the hover-glow's white was black too.
    """
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
    """ActionNode::AddProperty — grant a being a property it did not have.

    Set WRITES an existing property and answers NoSuchProperty otherwise; it
    does not mint one. So every `set("acoustic.frequency", …)` aimed at a
    NEWBORN failed silently against a being that had no such property yet —
    which is why the spawned orbs and stroke segments came out with none of
    the acoustics the hover-sound law then tried to read. Registered names
    (position, color, shape.r) still take Set; authored ones take this.
    """
    node = {"kind": 12, "propertyName": name, "operand": operand}
    if owner:
        node["path"] = owner
    return node


def map_path(path, bindings, terms=None, pieces=None, input_var=None):
    """ActionNode::Map — path := f(bindings).

    `terms` is the one-piece shorthand. `pieces` + `input_var` is the real
    thing: a Piecewise whose bounds are cut on `input_var`, which is how a
    clamp is written in this substrate. Outside every piece the function is
    UNDEFINED and the law does not fire, so a clamp must cover the whole line
    with a constant piece on each side rather than leaving the ends open.
    """
    fn = {"pieces": pieces if pieces is not None else [{"expr": {"terms": terms or []}}]}
    if input_var:
        fn["input"] = input_var
    return {"kind": 8, "path": path, "bindings": bindings, "function": fn}


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
    """scale·clamp(var, lo, hi) + offset, as three exact pieces.

    Below `lo` and above `hi` the value is the constant the bound maps to —
    which is what makes it a clamp rather than a hole in the domain. A slider
    whose model went undefined past its authored max would freeze instead of
    stopping, and a frozen control reads as a broken one.
    """
    return [
        piece(const_terms(scale * lo + offset), hi=lo),
        piece([{"c": scale, "factors": {var: 1.0}}, {"c": offset, "factors": {}}], lo=lo, hi=hi),
        piece(const_terms(scale * hi + offset), lo=hi),
    ]


# A transcendental factor: kind(scale·var + shift). Kind 0 = Sin, 1 = Cos.
def sin_factor(var, scale=1.0, shift=0.0):
    return {"kind": 0, "var": var, "scale": float(scale), "shift": float(shift)}


def cos_factor(var, scale=1.0, shift=0.0):
    return {"kind": 1, "var": var, "scale": float(scale), "shift": float(shift)}


def wave_term(c, factors, trans):
    """One product term carrying a sinusoid: c · Π var^exp · Π trans(...).

    OntoMath's `trans` factors are what make an oscillation exact law text
    rather than a curve approximation — see ScalarForm.hpp. `scale` inside the
    factor is a constant, so a sinusoid's FREQUENCY is authored and its
    AMPLITUDE is what a bound variable can move.
    """
    return {"c": float(c), "factors": factors, "trans": trans}


def flip_terms(var):
    """The toggle, as mathematics: v := 1 - v.

    Authored as `v + 1` originally, which on a bool latches ON and never
    returns — every click past the first coerced 2, 3, 4… back to true
    (audit §B1). ControlPatterns.cpp's createToggleLaw carries the long-form
    account of why the flip belongs in the mathematics and not in a pair of
    laws; this is that same form, written as save data.
    """
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
    """The edge a category carries back to whoever authored it.

    Was written as `instance-of` (audit §C1), which says the taxonomy is an
    instance of Gemini Spark — not what anybody meant. `authored-by` is the type
    ControlPatterns.cpp's seedArtCategories already uses for exactly this.
    """
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
        authored_by_rel("category.control", AUTHOR),
        authored_by_rel("category.art", AUTHOR),
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
        "faceColors": make_face_colors((0.13, 0.145, 0.185)),
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
        "faceColors": make_face_colors((0.085, 0.095, 0.125)),
        "authoredProperties": {
            "displayName": pv("string", "Studio Control Console"),
            "isStudioSurface": pv("bool", True),
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
        "faceColors": make_face_colors((0.17, 0.20, 0.27)),
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
            "isPulseCrystal": pv("bool", True),
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
        "faceColors": make_face_colors((0.055, 0.065, 0.09)),
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
        "faceColors": make_face_colors((0.16, 0.55, 0.92)),
        "authoredProperties": {
            "displayName": pv("string", "✦ Spawn Harmonic Orb"),
            "controlLabel": pv("string", "SPAWN ORB"),
            "buttonRole": pv("string", "spawn-orb"),
            "acoustic.frequency": pv("double", 587.33),   # D5
            "acoustic.amplitude": pv("double", 0.55),
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
        "faceColors": make_face_colors((0.055, 0.065, 0.09)),
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
        "faceColors": make_face_colors((0.95, 0.68, 0.20)),
        "authoredProperties": {
            "displayName": pv("string", "☼ Toggle Night/Day Theme"),
            "controlLabel": pv("string", "DAY / NIGHT"),
            "buttonRole": pv("string", "toggle-theme"),
            "acoustic.frequency": pv("double", 392.0),    # G4
            "acoustic.amplitude": pv("double", 0.55),
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
        "faceColors": make_face_colors((0.055, 0.065, 0.09)),
        "authoredProperties": {"displayName": pv("string", "Chord Pad Frame")},
    })

    chord_pads = [
        # All seven notes of the C major scale across the visible spectrum:
        # C5 (Do) - Crimson Red
        # D5 (Re) - Amber Orange
        # E5 (Mi) - Warm Gold
        # F5 (Fa) - Fresh Green
        # G5 (Sol) - Emerald Teal
        # A5 (La) - Azure Blue
        # B5 (Ti) - Royal Indigo
        ("studio.pad.c5", "♫ Note C5 (Do)", 0.29, (0.90, 0.28, 0.36), 523.25, "C5"),
        ("studio.pad.d5", "♫ Note D5 (Re)", 0.62, (0.95, 0.50, 0.20), 587.33, "D5"),
        ("studio.pad.e5", "♫ Note E5 (Mi)", 0.95, (0.96, 0.70, 0.18), 659.25, "E5"),
        ("studio.pad.f5", "♫ Note F5 (Fa)", 1.28, (0.38, 0.78, 0.26), 698.46, "F5"),
        ("studio.pad.g5", "♫ Note G5 (Sol)", 1.61, (0.24, 0.82, 0.48), 783.99, "G5"),
        ("studio.pad.a5", "♫ Note A5 (La)", 1.94, (0.20, 0.62, 0.94), 880.00, "A5"),
        ("studio.pad.b5", "♫ Note B5 (Ti)", 2.27, (0.34, 0.52, 0.96), 987.77, "B5"),
    ]
    for pad_id, pad_name, pos_x, rgb, freq, note in chord_pads:
        zone_objects.append({
            "objectID": pad_id,
            "shapeKind": 0,
            "geometryType": 0,
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(pos_x, 0.88, -0.35, (0.28, 0.14, 0.54)),
            "center": [pos_x, 0.88, -0.35],
            "materialId": f"material.studio.{note}",
            "faceColors": make_face_colors(rgb),
            "authoredProperties": {
                "displayName": pv("string", pad_name),
                "controlLabel": pv("string", f"Play Note {note}"),
                "noteName": pv("string", note),
                "acoustic.frequency": pv("double", freq),
                "acoustic.amplitude": pv("double", 0.85),
                "acoustic.waveType": pv("string", "triangle"),
                "isChordPad": pv("bool", True),
                "restY": pv("double", 0.88),
            },
        })
        relations.append(instance_rel(pad_id, "category.control.button"))

    # Labeled Plaque over Scale Pads
    zone_objects.append({
        "objectID": "studio.sign.music",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(1.28, 1.12, -0.35, (2.2, 0.12, 0.04)),
        "center": [1.28, 1.12, -0.35],
        "materialId": "material.studio.sign.dark",
        "faceColors": make_face_colors((0.17, 0.20, 0.27)),
        "authoredProperties": {"displayName": pv("string", "[ ♫ MAJOR SCALE: C5 · D5 · E5 · F5 · G5 · A5 · B5 ]")},
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
        "faceColors": make_face_colors((0.20, 0.22, 0.28)),
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
        # x = controlValue - 1.6, the same mapping law-studio-slider-sync
        # writes every tick. Authored at x = 0 it used to TELEPORT to -0.6 on
        # the first frame after load (audit §B3): the placement and the law
        # disagreed, and the law wins immediately. They now agree.
        "transform": mat4_translate(-0.6, 0.9, 0.55, (0.38, 0.14, 0.34)),
        "center": [-0.6, 0.9, 0.55],
        "materialId": "material.studio.handle",
        "faceColors": make_face_colors((0.92, 0.94, 0.98)),
        "authoredProperties": {
            "displayName": pv("string", "Pulse Rate Handle"),
            "controlLabel": pv("string", "Pulse Rate"),
            "controlValue": pv("double", 1.0),
            "controlMin": pv("double", 0.2),
            "controlMax": pv("double", 3.0),
            # dcontrolValue/dt = dragX·controlStep and dragX is a PIXEL delta,
            # so this is "units per pixel per second". 0.1 swept the whole range
            # in a flick; 0.02 gives the track about a screen-width of travel.
            "controlStep": pv("double", 0.02),
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
        "faceColors": make_face_colors((0.17, 0.20, 0.27)),
        "authoredProperties": {"displayName": pv("string", "[ ⟷ RESONANCE SLIDER ]")},
    })

    # -----------------------------------------------------------------------
    # The screen-space surface: a legend that says what this place is, and a
    # dock of controls along the bottom.
    #
    # Laid out in WINDOW POINTS against the 1280x720 the engine opens with
    # (Engine.cpp glfwCreateWindow) — the same space glfwGetCursorPos reports
    # and, since the repair to Renderer::begin2D's contract, the same space
    # these rectangles are drawn in. They used to be drawn in framebuffer
    # pixels and picked in window points, so on a Retina display the dock
    # appeared at half position with its hit region at double, and no control
    # here could be clicked at all.
    #
    # Text2D beings carry no width, which is what keeps a caption out of the
    # pick: getRect2D collapses to a zero-width strip, so a legend cannot
    # swallow a click meant for the world behind it.
    # -----------------------------------------------------------------------
    INK = (0.93, 0.94, 0.97)
    INK_DIM = (0.55, 0.60, 0.70)
    GOLD = (1.0, 0.78, 0.32)

    def text2d(object_id, text, x, y, size, rgb, z=30):
        return {
            "objectID": object_id,
            "shapeKind": 13,  # Text2D
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

    zone_objects.append(text2d("hud.title", "SYNTHESIS STUDIO", 30, 26, 24, GOLD))
    zone_objects.append(
        text2d("hud.title.sub", "a studio authored entirely as laws, beings and relations",
               30, 54, 13, INK_DIM))

    # The manual, in the world. Zach's play-test: "no tooltip visible, no
    # manual, no button or lever-like widget indicators... what am I supposed
    # to do with this". Nothing here was discoverable by looking, and the
    # signs that were meant to say so were blank cubes.
    for row, line in enumerate([
        "CLICK a dock button below, or the matching block on the desk ahead.",
        "SPAWN ORB   mints a harmonic orb; each one lands further around the ring.",
        "DAY / NIGHT flips the studio's ambient. C5 to B5 play the major scale.",
        "DRAW        turns drawing on, then hold and drag across the white easel.",
        "The slider on the desk sets the pulse rate the crystal breathes at.",
    ]):
        zone_objects.append(
            text2d(f"hud.legend.{row}", line, 30, 92 + row * 19, 13,
                   INK if row == 0 else INK_DIM))

    # --- the dock ---------------------------------------------------------
    DOCK_Y = 638.0
    BTN_Y = 650.0
    BTN_H = 40.0

    def hud_plate(object_id, label, x, w, rgb, props=None, z=20, y=BTN_Y, h=BTN_H):
        authored = {
            "displayName": pv("string", label),
            "controlLabel": pv("string", label),
            "shape.width2D": pv("double", float(w)),
            "shape.height2D": pv("double", float(h)),
            "restY2D": pv("double", float(y)),
        }
        authored.update(props or {})
        return {
            "objectID": object_id,
            "shapeKind": 12,
            "geometryType": 12,
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, float(w), float(h)],
            "transform": mat4_translate(0.0, 0.0, 0.0),
            "center": [0.0, 0.0, 0.0],
            "x2D": float(x),
            "y2D": float(y),
            "zOrder2D": z,
            "materialId": "",
            "faceColors": make_face_colors(rgb),
            "authoredProperties": authored,
        }

    dock = hud_plate("hud.dock.bg", "HUD Control Dock", 230, 820, (0.07, 0.08, 0.11),
                     z=10, y=DOCK_Y, h=64.0)
    dock["authoredProperties"].pop("controlLabel")   # a plate is not a control
    zone_objects.append(dock)

    hud_btn_spawn = "hud.btn.spawn-orb"
    zone_objects.append(hud_plate(
        hud_btn_spawn, "SPAWN ORB", 246, 132, (0.16, 0.55, 0.92), {
            "buttonRole": pv("string", "spawn-orb"),
            "acoustic.frequency": pv("double", 587.33),
            "acoustic.amplitude": pv("double", 0.55),
        }))
    relations.append(instance_rel(hud_btn_spawn, "category.control.button"))

    hud_btn_theme = "hud.btn.toggle-theme"
    zone_objects.append(hud_plate(
        hud_btn_theme, "DAY / NIGHT", 386, 126, (0.95, 0.68, 0.20), {
            "buttonRole": pv("string", "toggle-theme"),
            "acoustic.frequency": pv("double", 392.0),
            "acoustic.amplitude": pv("double", 0.55),
        }))
    relations.append(instance_rel(hud_btn_theme, "category.control.toggle"))

    hud_pads = [
        ("hud.pad.c5", "C5", 522.0, (0.90, 0.28, 0.36), 523.25),
        ("hud.pad.d5", "D5", 572.0, (0.95, 0.50, 0.20), 587.33),
        ("hud.pad.e5", "E5", 622.0, (0.96, 0.70, 0.18), 659.25),
        ("hud.pad.f5", "F5", 672.0, (0.38, 0.78, 0.26), 698.46),
        ("hud.pad.g5", "G5", 722.0, (0.24, 0.82, 0.48), 783.99),
        ("hud.pad.a5", "A5", 772.0, (0.20, 0.62, 0.94), 880.00),
        ("hud.pad.b5", "B5", 822.0, (0.34, 0.52, 0.96), 987.77),
    ]
    for hpad_id, note, x_pos, rgb, freq in hud_pads:
        zone_objects.append(hud_plate(hpad_id, note, x_pos, 44, rgb, {
            "noteName": pv("string", note),
            "acoustic.frequency": pv("double", freq),
            "acoustic.amplitude": pv("double", 0.85),
            "acoustic.waveType": pv("string", "triangle"),
            "isChordPad": pv("bool", True),
        }))
        relations.append(instance_rel(hpad_id, "category.control.button"))

    hud_btn_draw = "hud.btn.draw-stroke"
    zone_objects.append(hud_plate(
        hud_btn_draw, "DRAW: OFF", 876, 152, (0.62, 0.36, 0.90), {
            "buttonRole": pv("string", "draw-mode"),
            "acoustic.frequency": pv("double", 493.88),
            "acoustic.amplitude": pv("double", 0.55),
        }))
    relations.append(instance_rel(hud_btn_draw, "category.control.button"))

    # The studio's ambient state — the being every law addresses as
    # "@state.studio.*". It holds state, it is not a control, and it must not
    # look like one.
    #
    # It was authored as Shape2D (12) with no size or position, which meant the
    # engine's defaults applied: a 100x100 quad at screen (100,100), in the
    # default red nobody chose. That is the red square in the corner of Zach's
    # play-test (audit §A3) — and because a 2D hit unconditionally occludes the
    # 3D pick, it also ate every click inside its rectangle. A pure state holder
    # is a Cube parked below the floor: still a being, still addressable by law
    # text, no longer a shape a Person can mistake for a button. (There is no
    # save-side way to clear physicalObject the way CategoryManager does in C++;
    # a comment saying "this is deliberately out of sight" is the honest
    # alternative to a black box.)
    state_id = "state.studio"
    zone_objects.append({
        "objectID": state_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -4.0, 0.0, (0.02, 0.02, 0.02)),
        "center": [0.0, -4.0, 0.0],
        "materialId": "material.studio.slate",
        "faceColors": make_face_colors((0.13, 0.145, 0.185)),
        "authoredProperties": {
            "displayName": pv("string", "Studio Ambient State"),
            "themeNight": pv("bool", False),
            "drawMode": pv("bool", False),
            "pulseRate": pv("double", 1.0),
            "spawnCount": pv("int", 0),
            "soundFreq": pv("double", 440.0),
            "soundAmp": pv("double", 0.8),
        },
    })

    # -----------------------------------------------------------------------
    # Author Laws
    # -----------------------------------------------------------------------

    # -----------------------------------------------------------------------
    # THE ARCHETYPES ARE NOT HERE, AND THAT IS THE POINT.
    #
    # This file used to author `law-control-button-archetype` and
    # `law-control-toggle-archetype` — byte-for-byte the same condition and
    # (nearly) the same action as `control-button-law` and `control-toggle-law`,
    # which EngineInit registers as FIRST MOVERS on every boot and which
    # LawManager::loadFromJson deliberately preserves across a load. So both
    # pairs loaded, the Law Graph showed a Person two button archetypes and two
    # toggle archetypes, and disabling either changed nothing (audit §B4).
    #
    # The toggle pair was worse than redundant: the first mover writes
    # `controlOn := 1 - o` and the save wrote `o + 1`, so which one a click left
    # behind depended on evaluation order.
    #
    # A world that wants a DIFFERENT archetype overrides by REUSING the id —
    # syncRegisterControlPatterns is first-wins precisely so a Person's revision
    # of a law they were handed survives re-seeding. It does not need a second
    # law wearing a different name.
    #
    # What remains below is only what is the studio's own: what activation
    # MEANS here.
    # -----------------------------------------------------------------------

    # 1. Spawn a Harmonic Orb.
    add_law(
        "law-studio-spawn-orb",
        "Studio: Spawn Harmonic Orb on Button Click",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "spawn-orb")),
        seq(
            # The note comes FIRST. A Create node hands its children the newborn
            # as their subject, and a sibling after it no longer reads the
            # button — so a PlayAudio placed below the Create read the orb's
            # slot on the button and sounded nothing at all.
            play_audio("acoustic.frequency", "acoustic.amplitude", "triangle"),
            map_path("@state.studio.spawnCount", {"c": "@state.studio.spawnCount"}, offset_terms("c", 1.0)),
            create_object(
                # ShapeKind 2 is Sphere. This said 1 and the comment said
                # "Sphere" — 1 is POLYHEDRON, which draws one mesh PER FACE
                # (ObjectRender's drawPolyhedron), so every orb and every
                # stroke segment was costing a fistful of draw calls instead of
                # one. `shape.r` is a sphere parameter and did nothing on it.
                2,
                "interactive.harmonic.orb",
                children=[
                    # `scale` is not a registered property on Object — the
                    # original set("scale", …) wrote nowhere, so every orb came
                    # out at the default radius. `shape.r` is what a Sphere's
                    # size actually is (ObjectProperties.cpp:512).
                    set_path("shape.r", pv("double", 0.22)),
                    set_path("color", pv("vec3", [1.0, 0.85, 0.22])),
                    set_path("position", pv("vec3", [0.0, 1.75, 1.25])),
                    # Every orb used to spawn on the SAME point (audit §B7) while
                    # spawnCount was incremented and read by nobody. The counter
                    # is now the ring's parameter: orb n lands at angle 0.8n on a
                    # circle in front of the easel, which is what makes the count
                    # something a Person can see rather than a number in a file.
                    map_path("position.x", {"n": "@state.studio.spawnCount"},
                             [wave_term(1.35, {}, [sin_factor("n", 0.8)])]),
                    map_path("position.z", {"n": "@state.studio.spawnCount"},
                             [wave_term(0.55, {}, [cos_factor("n", 0.8)]),
                              {"c": 1.25, "factors": {}}]),
                    map_path("position.y", {"n": "@state.studio.spawnCount"},
                             [wave_term(0.28, {}, [sin_factor("n", 1.7)]),
                              {"c": 1.75, "factors": {}}]),
                    add_property("acoustic.frequency", pv("double", 880.0)),
                    add_property("acoustic.amplitude", pv("double", 0.5)),
                    add_property("acoustic.waveType", pv("string", "triangle")),
                    add_property("displayName", pv("string", "Harmonic Orb")),
                    add_relation("", "category.art.stroke", "instance-of"),
                    add_relation("", "category.interactive.orb", "instance-of"),
                ],
            ),
            map_path("position.y", {"ry": "restY"}, offset_terms("ry", -0.04)),
            map_path("y2D", {"r": "restY2D"}, offset_terms("r", 3.0)),
            publish("orb-spawned", "state.studio"),
        ),
        scope=0,
    )

    # 2. Flip the ambient theme.
    add_law(
        "law-studio-theme-toggle",
        "Studio: Switch Ambient Theme on Toggle Click",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "toggle-theme")),
        seq(
            map_path("@state.studio.themeNight", {"tn": "@state.studio.themeNight"}, flip_terms("tn")),
            play_audio("acoustic.frequency", "acoustic.amplitude", "triangle"),
            map_path("position.y", {"ry": "restY"}, offset_terms("ry", -0.04)),
            map_path("y2D", {"r": "restY2D"}, offset_terms("r", 3.0)),
            publish("theme-toggled", "state.studio"),
        ),
        scope=0,
    )

    # 2a/2b. …and what flipping it MEANS.
    #
    # `themeNight` was written by the law above and read by NOTHING (audit §B2):
    # the toggle flipped a boolean and the world did not change, which from the
    # Person's side is indistinguishable from a dead button. These two laws are
    # the missing consequence.
    #
    # Two laws, not one, and they are a BRANCH rather than the loop
    # ControlPatterns.cpp warns about: each reads `@state.studio.themeNight` and
    # writes `color` on the studio's surfaces, so neither one's action can
    # satisfy or invalidate the other's condition. The cascade rule is about
    # actions that feed conditions; these do not.
    for suffix, night, rgb in (("night", True, [0.07, 0.08, 0.13]),
                               ("day", False, [0.38, 0.42, 0.50])):
        add_law(
            f"law-studio-theme-{suffix}",
            f"Studio: Surfaces take the {suffix} ambient",
            1,  # WhileTrue — a level, not an edge
            [],
            all_of(
                compare("isStudioSurface", 0, pv("bool", True)),
                compare("@state.studio.themeNight", 0, pv("bool", night)),
            ),
            set_path("color", pv("vec3", rgb)),
            scope=1,  # Scope::Everyone — every surface that carries the mark
        )

    # 3. Draw mode, as a real toggle.
    #
    # Was `active3DMode := "Draw"`, unconditionally, with no inverse: there was
    # no way out of draw mode from inside the studio, and "Draw" is not one of
    # the ten modes kCreatorTools knows, so the Creator Console's mode display
    # and the channel's mode silently diverged (audit §B5). The studio keeps its
    # own `drawMode` flag instead — one being, one flip, and the drawing law
    # reads it directly.
    add_law(
        "law-studio-draw-mode-toggle",
        "Studio: Toggle Canvas Drawing Mode",
        0,
        ["control-activated"],
        compare("buttonRole", 0, pv("string", "draw-mode")),
        seq(
            map_path("@state.studio.drawMode", {"d": "@state.studio.drawMode"}, flip_terms("d")),
            play_audio("acoustic.frequency", "acoustic.amplitude", "triangle"),
            map_path("y2D", {"r": "restY2D"}, offset_terms("r", 3.0)),
            publish("draw-mode-toggled", "state.studio"),
        ),
        scope=0,
    )

    # 3a/3b. The draw button shows its own state.
    # A mode a Person cannot see they are in is a mode they will fight.
    for suffix, on, rgb, label in (("on", True, [0.45, 0.92, 0.55], "DRAW: ON"),
                                   ("off", False, [0.72, 0.32, 0.95], "DRAW: OFF")):
        add_law(
            f"law-studio-draw-indicator-{suffix}",
            f"Studio: Draw button reads {label}",
            1,
            [],
            all_of(
                compare("buttonRole", 0, pv("string", "draw-mode")),
                compare("@state.studio.drawMode", 0, pv("bool", on)),
            ),
            seq(
                set_path("color", pv("vec3", rgb)),
                set_path("controlLabel", pv("string", label)),
            ),
            scope=1,
        )

    # 4. The slider, clamped to the bounds it always carried.
    #
    # `controlMin` 0.2 and `controlMax` 3.0 were authored and read by nothing —
    # neither the archetype nor this law — so the handle slid off its track in
    # both directions and pulseRate went negative (audit §B3). The clamp is a
    # three-piece Piecewise: constant below the floor, the value between, and
    # constant above the ceiling. It writes controlValue back so the archetype's
    # Flow cannot integrate past the end, which is the only place the runaway
    # could be stopped without teaching the archetype about this slider.
    add_law(
        "law-studio-slider-sync",
        "Studio: Sync Pulse Rate from Slider Value",
        1,
        [],
        related("instance-of", "category.control.slider"),
        seq(
            map_path("controlValue", {"v": "controlValue"},
                     pieces=clamp_pieces("v", 0.2, 3.0), input_var="v"),
            map_path("@state.studio.pulseRate", {"v": "controlValue"},
                     pieces=clamp_pieces("v", 0.2, 3.0), input_var="v"),
            map_path("position.x", {"v": "controlValue"},
                     pieces=clamp_pieces("v", 0.2, 3.0, offset=-1.6), input_var="v"),
        ),
        scope=0,
    )

    # 4a. …and what the pulse rate MEANS.
    #
    # The other half of §B2: `pulseRate` was written every tick and read by
    # nothing. The crystal breathes it. Frequency is authored (a TransFactor's
    # scale is a constant); AMPLITUDE is what the slider moves, so the crystal
    # sits still at the bottom of the track and swells at the top.
    add_law(
        "law-studio-crystal-pulse",
        "Studio: Crystal breathes at the authored pulse rate",
        1,
        [],
        compare("isPulseCrystal", 0, pv("bool", True)),
        seq(
            map_path("color.r", {"p": "@state.studio.pulseRate", "t": "time"},
                     [{"c": 0.40, "factors": {}},
                      wave_term(0.13, {"p": 1.0}, [sin_factor("t", 2.2)])]),
            map_path("color.g", {"p": "@state.studio.pulseRate", "t": "time"},
                     [{"c": 0.78, "factors": {}},
                      wave_term(0.06, {"p": 1.0}, [sin_factor("t", 2.2)])]),
        ),
        scope=1,
    )

    # 5. The chord pads.
    add_law(
        "law-studio-pad-play",
        "Studio: Play Musical Note on Chord Pad Click",
        0,
        ["control-activated"],
        compare("isChordPad", 0, pv("bool", True)),
        seq(
            play_audio("acoustic.frequency", "acoustic.amplitude", "triangle"),
            map_path("position.y", {"ry": "restY"}, offset_terms("ry", -0.04)),
            map_path("y2D", {"r": "restY2D"}, offset_terms("r", 3.0)),
            publish("note-played"),
        ),
        scope=0,
    )

    # 6. The spring, for both bodies a control can have.
    #
    # A 3D control rises on `position.y`; a 2D one on `y2D`. The law used to
    # write only the first, and the five HUD controls carried no `restY` at all,
    # so the entire press-and-release affordance was 3D-only and the HUD gave no
    # click feedback whatsoever (audit §B8). Both writes are here; each is a Map
    # over a binding the other kind of control does not have, and an unread
    # binding makes the node undefined, so each control gets exactly the one
    # that applies to it. Undefined is not failure here — it is the branch.
    add_law(
        "law-studio-button-spring",
        "Studio: Restore Button Elevation",
        0,
        ["object-released"],
        related("instance-of", "category.control.button"),
        seq(
            map_path("position.y", {"ry": "restY"}, copy_terms("ry")),
            map_path("y2D", {"r": "restY2D"}, copy_terms("r")),
        ),
        scope=0,
    )

    # 6a. Toggles spring too — they are not buttons and the law above will
    # never see them.
    add_law(
        "law-studio-toggle-spring",
        "Studio: Restore Toggle Elevation",
        0,
        ["object-released"],
        related("instance-of", "category.control.toggle"),
        seq(
            map_path("position.y", {"ry": "restY"}, copy_terms("ry")),
            map_path("y2D", {"r": "restY2D"}, copy_terms("r")),
        ),
        scope=0,
    )

    # 7. Drawing, on the canvas and only on the canvas.
    #
    # Two repairs (audit §B6). The condition now reads the studio's own
    # `drawMode` rather than a creation-channel mode string nothing recognised.
    # And it reads `isCanvas` — a property the easel has carried since the first
    # draft while NO LAW read it, so strokes could be laid anywhere: on the sky,
    # where `pointerWorld` is (0,0,0) and every segment piled on the origin, or
    # across the HUD, where `pointerWorld` is screen pixels reinterpreted as
    # world coordinates. Scope::Everyone sweeps, `isCanvas` selects, and the
    # pointer decides where — so the authored constraint finally constrains.
    add_law(
        "law-art-stroke-draw",
        "Art Tool: Draw Stroke Singulars along Pointer",
        1,
        [],
        all_of(
            compare("@interaction-channel.leftDown", 0, pv("bool", True)),
            compare("@interaction-channel.dragging", 0, pv("bool", True)),
            compare("@state.studio.drawMode", 0, pv("bool", True)),
            compare("isCanvas", 0, pv("bool", True)),
            compare("@world.pointerOver", 0, pv("bool", True)),
            # THE POINTER MUST HAVE MOVED. Without this the law is a level
            # with no gate: WhileTrue fires every tick the button is held, so
            # holding still on the canvas laid 60 spheres a second on the same
            # spot. At the ~0.2 ms of render per object measured in Sanctum,
            # ten seconds of that is a 7 fps slideshow — and a slideshow is
            # what "the 2D buttons stop working after a certain point" was.
            # Six pixels of travel is about a deliberate stroke and nothing
            # else.
            any_of(
                compare("@interaction-channel.dragX", 4, pv("double", 6.0)),
                compare("@interaction-channel.dragX", 2, pv("double", -6.0)),
                compare("@interaction-channel.dragY", 4, pv("double", 6.0)),
                compare("@interaction-channel.dragY", 2, pv("double", -6.0)),
            ),
        ),
        create_object(
            2,  # Sphere — see the note on the orb above; this said 1 too
            "art.stroke.segment",
            placement_path="@interaction-channel.pointerWorld",
            children=[
                set_path("shape.r", pv("double", 0.045)),
                set_path("color", pv("vec3", [1.0, 0.85, 0.15])),
                add_property("displayName", pv("string", "Stroke")),
                # A stroke that could not be heard: law-stroke-hover-sound reads
                # exactly these two paths, and the segments were created without
                # them (audit §B7), so hovering a stroke was silent even with a
                # working audio channel behind it.
                add_property("acoustic.frequency", pv("double", 1046.5)),
                add_property("acoustic.amplitude", pv("double", 0.32)),
                add_property("acoustic.waveType", pv("string", "triangle")),
                add_relation("", "category.art.stroke", "instance-of"),
            ],
        ),
        scope=1,
    )

    # 8. A stroke chimes when the pointer finds it.
    add_law(
        "law-stroke-hover-sound",
        "Art Stroke: Sound Chime on Hover",
        0,
        ["object-hover-entered"],
        related("instance-of", "category.art.stroke"),
        play_audio("acoustic.frequency", "acoustic.amplitude", "triangle"),
        scope=0,
    )

    # 9. …and lights while it is held.
    add_law(
        "law-stroke-hover-glow",
        "Art Stroke: Illuminate on Pointer Hover",
        1,
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
        "injected_by": "Gemini Spark (authored) / Claude Opus 5 (repaired) / Antigravity (major scale 2026-09-02)",
        "authors": ["Zach"],
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
        "injected_by": "Gemini Spark (authored) / Claude Opus 5 (repaired) / Antigravity (major scale 2026-09-02)",
        "authors": ["Zach"],
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
            # All seven notes of the major scale, plus the three sign variants:
            {"name": "material.studio.C5", "baseColor": [0.95, 0.22, 0.32], "opacity": 1.0},
            {"name": "material.studio.D5", "baseColor": [0.98, 0.50, 0.18], "opacity": 1.0},
            {"name": "material.studio.E5", "baseColor": [1.0, 0.72, 0.12], "opacity": 1.0},
            {"name": "material.studio.F5", "baseColor": [0.35, 0.80, 0.25], "opacity": 1.0},
            {"name": "material.studio.G5", "baseColor": [0.18, 0.88, 0.45], "opacity": 1.0},
            {"name": "material.studio.A5", "baseColor": [0.18, 0.62, 0.98], "opacity": 1.0},
            {"name": "material.studio.B5", "baseColor": [0.28, 0.48, 1.0], "opacity": 1.0},
            {"name": "material.studio.sign.blue", "baseColor": [0.16, 0.34, 0.52], "opacity": 1.0},
            {"name": "material.studio.sign.gold", "baseColor": [0.42, 0.32, 0.12], "opacity": 1.0},
            {"name": "material.studio.sign.dark", "baseColor": [0.20, 0.23, 0.30], "opacity": 1.0},
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
    """Write the Synthesis Studio's two save files.

    THE ZONE FILE IS NOT OURS TO OVERWRITE. `saves/zones/SynthesisStudio/zone.json`
    is an identity-stable Zone: once the app has loaded this world even once, the
    engine owns that file and it holds whatever a Person did in there — paint,
    positions, spawned orbs, strokes. Re-running this script used to clobber it
    unconditionally, with no backup and no merge (audit §C1), which is how the
    on-disk copy came to be missing three authored taxonomy edges that only the
    world file still carried.

    So: refuse by default, say exactly what is in the way, and take a backup even
    under --force. Save files are sacred.
    """
    parser = argparse.ArgumentParser(description="Author the Synthesis Studio save files.")
    parser.add_argument("--force", action="store_true",
                        help="overwrite an existing Zone identity file (a .bak is kept)")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    session, zone = build_world()
    world_path = root / "saves" / "worlds" / "synthesis_studio.json"
    zone_path = root / "saves" / "zones" / ZONE_ID / "zone.json"

    if zone_path.exists() and not args.force:
        print(f"REFUSED: {zone_path} already exists.")
        print("  That file is the live Zone — it holds whatever a Person did in the studio.")
        print("  Re-run with --force to replace it (a .bak is kept), or delete it yourself")
        print("  if you are certain there is nothing in it you want.")
        return 1

    world_path.parent.mkdir(parents=True, exist_ok=True)
    zone_path.parent.mkdir(parents=True, exist_ok=True)

    backups = []
    for existing in (world_path, zone_path):
        if existing.exists():
            backup = existing.with_suffix(existing.suffix + ".bak")
            backup.write_text(existing.read_text())
            backups.append(backup)

    # The engine writes a .ecform beside a legacy JSON world on first load, and
    # SaveSystem::listWorlds then PREFERS it over the .json for the same stem —
    # so a stale .ecform silently shadows everything written here (audit §C2).
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
        print(f"  Removed stale  {sh} (it would have shadowed the .json on load)")
    print(f"  Zone Objects: {len(zone['world']['objects'])}")
    print(f"  Relations: {len(zone['formationRelations'])}")
    print(f"  Laws: {len(LAWS)}")
    print(f"  Author: {AUTHOR}")
    print(f"  Injected by: Antigravity (major scale 2026-09-02), on Zach's authority")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
