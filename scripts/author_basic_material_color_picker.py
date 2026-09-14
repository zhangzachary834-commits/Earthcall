#!/usr/bin/env python3
"""Author the Enhanced 2D Pixel Creator & Material Color Studio.

This is a First-Mover authoring tool, not runtime behavior. It augments the
BasicPixelChanger Zone into a professional 2D Pixel Creator adhering to
professional UI/UX design principles (clear visual hierarchy, dedicated studio
toolbar, artboard matte/frame, status feedback, quick palette swatches, and
deep OntoMath HSV+RGB color inspection) while strictly abiding by AGENTS.md
and the Seven Refusals.

All UI controls are authored Shape2D/Text2D objects, Materials, Relations,
and Formations governed by pure Event-Condition-Action Laws.
"""

from __future__ import annotations

import base64
import colorsys
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
ZONE_PATH = ROOT / "saves/zones/BasicPixelChanger/zone.json"
LAW_DIR = ROOT / "saves/laws"
AUTHOR = "Zach"
INJECTED_BY = "Gemini Spark, session 2026-09-13-21-30"


def rgba_texture(size: int, sample) -> str:
    pixels = bytearray()
    for y in range(size):
        v = y / (size - 1) if size > 1 else 0.0
        for x in range(size):
            u = x / (size - 1) if size > 1 else 0.0
            r, g, b = sample(u, v)
            pixels.extend((round(max(0.0, min(1.0, r)) * 255),
                           round(max(0.0, min(1.0, g)) * 255),
                           round(max(0.0, min(1.0, b)) * 255), 255))
    return base64.b64encode(bytes(pixels)).decode("ascii")


def solid_texture(color=(1.0, 1.0, 1.0), size: int = 4) -> str:
    return rgba_texture(size, lambda u, v: color)


def material(name: str, texture: str | None = None, color=(1.0, 1.0, 1.0),
             texture_size: int = 32) -> dict[str, Any]:
    out: dict[str, Any] = {
        "ambient": 0.2,
        "baseColor": list(color),
        "diffuse": 0.8,
        "name": name,
        "opacity": 1.0,
        "shininess": 32.0,
        "specular": 1.0,
    }
    if texture is not None:
        out["faceTextures"] = [{"size": texture_size, "pixelsB64": texture}]
    return out


def object_2d(object_id: str, x: float, y: float, width: float, height: float,
              material_id: str, label: str = "", color=(1.0, 1.0, 1.0),
              z: int = 10, properties: dict[str, Any] | None = None,
              text: bool = False) -> dict[str, Any]:
    shape = [1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, width, height]
    props = dict(properties or {})
    if label:
        props.setdefault("displayName", {"t": "string", "v": label})
        props.setdefault("label.size2D", {"t": "double", "v": min(18.0, max(11.0, height * 0.38))})
    if text:
        props["label2D"] = {"t": "string", "v": label}
    return {
        "authoredProperties": props,
        "authoritativeAxis": [0.0, 1.0, 0.0],
        "center": [0.0, 0.0, 0.0],
        "faceColors": [list(color) for _ in range(6)],
        "geometryType": 12 if not text else 13,
        "materialId": material_id,
        "objectID": object_id,
        "renderMode": 0,
        "rotationResponsiveness": 10.0,
        "shapeKind": 12 if not text else 13,
        "shapeParams": shape,
        "targetRotation": [0.0, -0.0, 0.0],
        "transform": [1.0, 0.0, 0.0, 0.0,
                      0.0, 1.0, 0.0, 0.0,
                      0.0, 0.0, 1.0, 0.0,
                      0.0, 0.0, 0.0, 1.0],
        "x2D": x,
        "y2D": y,
        "zOrder2D": z,
    }


def scalar_node(variable: str) -> dict[str, Any]:
    return {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {variable: 1.0}}]}}


def scalar_terms(*terms: tuple[float, dict[str, float]]) -> dict[str, Any]:
    return {
        "op": 0,
        "scalarForm": {
            "terms": [{"c": coefficient, "factors": factors}
                      for coefficient, factors in terms]
        },
    }


def vector_node(variable: str) -> dict[str, Any]:
    return {"op": 1, "var": variable}


def vector_construct(*components: dict[str, Any]) -> dict[str, Any]:
    return {"op": 2, "children": list(components)}


def everywhere(node: dict[str, Any], variable: str) -> dict[str, Any]:
    return {"input": variable, "pieces": [{"mathNode": node}]}


def map_action(path: str, node: dict[str, Any], variable: str,
               sources: str | dict[str, str]) -> dict[str, Any]:
    return {
        "kind": 8,
        "path": path,
        "function": everywhere(node, variable),
        "bindings": {variable: sources} if isinstance(sources, str) else sources,
    }


def hsv_piecewise() -> dict[str, Any]:
    """Exact HSV→RGB as six authored OntoMath pieces over h in [0,1]."""
    v = scalar_terms((1.0, {"v": 1.0}))
    m = scalar_terms((1.0, {"v": 1.0}), (-1.0, {"v": 1.0, "s": 1.0}))
    rising = lambda offset: scalar_terms(
        (1.0, {"v": 1.0}), (-1.0, {"v": 1.0, "s": 1.0}),
        (-offset, {"v": 1.0, "s": 1.0}),
        (6.0, {"h": 1.0, "v": 1.0, "s": 1.0}))
    falling = lambda offset: scalar_terms(
        (1.0, {"v": 1.0}), (-1.0, {"v": 1.0, "s": 1.0}),
        (offset, {"v": 1.0, "s": 1.0}),
        (-6.0, {"h": 1.0, "v": 1.0, "s": 1.0}))
    vectors = [
        vector_construct(v, rising(0.0), m),
        vector_construct(falling(2.0), v, m),
        vector_construct(m, v, rising(2.0)),
        vector_construct(m, falling(4.0), v),
        vector_construct(rising(4.0), m, v),
        vector_construct(v, m, falling(6.0)),
    ]
    pieces = []
    for sector, node in enumerate(vectors):
        pieces.append({
            "lo": sector / 6.0,
            "includeLo": True,
            "hi": (sector + 1) / 6.0,
            "includeHi": sector == 5,
            "mathNode": node,
        })
    return {"input": "h", "pieces": pieces}


def hsv_color_action() -> dict[str, Any]:
    return {
        "kind": 8,
        "path": "@material-color-picker.selectedColor",
        "function": hsv_piecewise(),
        "bindings": {
            "h": "@material-color-picker.hue",
            "s": "@material-color-picker.saturation",
            "v": "@material-color-picker.value",
        },
    }


def identity_condition(object_id: str) -> dict[str, Any]:
    return {"kind": 8, "otherId": object_id}


def picker_law(law_id: str, name: str, strip_id: str, component: str) -> dict[str, Any]:
    actions = [
        map_action(
            f"@material-color-picker.selectedColor.{component}",
            scalar_node("u"), "u", "@interaction-channel.hoveredU"),
        map_action("@creation-channel.activeColor", vector_node("c"), "c",
                   "@material-color-picker.selectedColor"),
        {"kind": 10, "eventType": "color-selection-changed",
         "publishSubject": "material-color-picker"},
    ]
    return {
        "authors": [AUTHOR],
        "identifier": law_id,
        "injected_by": INJECTED_BY,
        "law": {
            "actionModel": {"kind": 5, "children": actions},
            "activation": 0,
            "applicationLog": [],
            "authors": [AUTHOR],
            "authority": 0,
            "conditionMode": "all",
            "conditionModel": identity_condition(strip_id),
            "conditionSubjects": [],
            "drives": False,
            "enabled": True,
            "id": law_id,
            "name": name,
            "provenance": [{"directed": True, "entityA": law_id,
                            "entityB": AUTHOR, "events": [],
                            "type": "authored-by", "weight": 1.0}],
            "retrigger": 0,
            "scope": 0,
            "targets": [],
        },
        "triggers": ["object-clicked"],
    }


def chromatic_law(law_id: str, name: str, control_id: str,
                  two_dimensional: bool) -> dict[str, Any]:
    actions = []
    if two_dimensional:
        actions.extend([
            map_action("@material-color-picker.hue", scalar_node("u"), "u",
                       "@interaction-channel.hoveredU"),
            map_action(
                "@material-color-picker.saturation",
                scalar_terms((1.0, {}), (-1.0, {"v": 1.0})),
                "v", "@interaction-channel.hoveredV"),
        ])
    else:
        actions.append(
            map_action("@material-color-picker.value", scalar_node("u"), "u",
                       "@interaction-channel.hoveredU"))
    actions.extend([
        hsv_color_action(),
        map_action("@creation-channel.activeColor", vector_node("c"), "c",
                   "@material-color-picker.selectedColor"),
        {"kind": 10, "eventType": "color-selection-changed",
         "publishSubject": "material-color-picker"},
    ])
    law = picker_law(law_id, name, control_id, "r")
    law["law"]["actionModel"] = {"kind": 5, "children": actions}
    return law


def material_apply_law() -> dict[str, Any]:
    law_id = "law-material-color-picker-apply"
    actions = [
        # Hand the authored selection to the canvas as its own writable
        # Property. The pixel Law can then read its subject locally instead
        # of depending on a global named-being lookup during the click edge.
        map_action("@basic-pixel-canvas.paintColor",
                   vector_node("c"), "c", "@material-color-picker.selectedColor"),
        map_action("@material.material-color-picker-preview.baseColor",
                   vector_node("c"), "c", "@material-color-picker.selectedColor"),
        map_action("@material.authored-color-target.baseColor",
                   vector_node("c"), "c", "@material-color-picker.selectedColor"),
    ]
    return {
        "authors": [AUTHOR],
        "identifier": law_id,
        "injected_by": INJECTED_BY,
        "law": {
            "actionModel": {"kind": 5, "children": actions},
            "activation": 0,
            "applicationLog": [],
            "authors": [AUTHOR],
            "authority": 0,
            "conditionMode": "all",
            "conditionModel": identity_condition("material-color-picker"),
            "conditionSubjects": [],
            "drives": False,
            "enabled": True,
            "id": law_id,
            "name": "Material Color Picker — apply to Material",
            "provenance": [{"directed": True, "entityA": law_id,
                            "entityB": AUTHOR, "events": [],
                            "type": "authored-by", "weight": 1.0}],
            "retrigger": 0,
            "scope": 0,
            "targets": [],
        },
        "triggers": ["color-selection-changed"],
    }


def relation(a: str, b: str, relation_type: str = "part-of") -> dict[str, Any]:
    return {
        "attachment": {"childAnchor": [0.0, 0.0, 0.0], "enabled": False,
                        "inheritRotation": True, "inheritScale": True,
                        "inheritTranslation": True,
                        "localOffset": [1.0, 0.0, 0.0, 0.0,
                                         0.0, 1.0, 0.0, 0.0,
                                         0.0, 0.0, 1.0, 0.0,
                                         0.0, 0.0, 0.0, 1.0],
                        "parentAnchor": [0.0, 0.0, 0.0]},
        "directed": True,
        "entityA": a,
        "entityB": b,
        "events": [],
        "type": relation_type,
        "weight": 1.0,
    }


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


def main() -> None:
    zone = json.loads(ZONE_PATH.read_text())
    zone.setdefault("lawRefs", [])
    law_ids = ["law-material-color-picker-red", "law-material-color-picker-green",
               "law-material-color-picker-blue", "law-material-color-picker-chromatic",
               "law-material-color-picker-value", "law-material-color-picker-apply"]
    # The Zone maintains exactly the canonical 7-law closure required by the
    # runtime and verified by basic_pixel_changer_test.
    all_law_ids = ["law-basic-pixel-changer"] + law_ids
    zone["lawRefs"] = [ref for ref in zone["lawRefs"] if ref in set(all_law_ids)]
    for lid in all_law_ids:
        if lid not in zone["lawRefs"]:
            zone["lawRefs"].append(lid)

    # 1. Textures & Palettes
    gradient_red = rgba_texture(32, lambda u, v: (u, 0.0, 0.0))
    gradient_green = rgba_texture(32, lambda u, v: (0.0, u, 0.0))
    gradient_blue = rgba_texture(32, lambda u, v: (0.0, 0.0, u))
    gradient_value = rgba_texture(32, lambda u, v: (u, u, u))
    chromatic = rgba_texture(
        64, lambda u, v: colorsys.hsv_to_rgb(u, 1.0 - v, 1.0))
    white = solid_texture((1.0, 1.0, 1.0))
    dark_panel = solid_texture((0.065, 0.075, 0.10))
    tool_panel = solid_texture((0.055, 0.062, 0.082))
    card_bg = solid_texture((0.11, 0.13, 0.17))
    artboard_mat = solid_texture((0.09, 0.105, 0.14))
    header_mat = solid_texture((0.07, 0.08, 0.11))
    btn_pen_mat = solid_texture((0.15, 0.22, 0.35))
    btn_eraser_mat = solid_texture((0.20, 0.23, 0.30))

    # Curated 12-color pixel art palette
    swatches = [
        ("swatch-black", (0.04, 0.04, 0.04), "BLACK"),
        ("swatch-white", (1.0, 1.0, 1.0), "WHITE"),
        ("swatch-slate", (0.35, 0.38, 0.45), "SLATE"),
        ("swatch-silver", (0.72, 0.76, 0.82), "SILVER"),
        ("swatch-crimson", (0.92, 0.15, 0.18), "RED"),
        ("swatch-amber", (0.96, 0.52, 0.10), "ORANGE"),
        ("swatch-lemon", (0.98, 0.85, 0.12), "YELLOW"),
        ("swatch-emerald", (0.15, 0.76, 0.32), "GREEN"),
        ("swatch-cyan", (0.10, 0.80, 0.88), "CYAN"),
        ("swatch-cobalt", (0.18, 0.38, 0.92), "BLUE"),
        ("swatch-purple", (0.58, 0.20, 0.88), "PURPLE"),
        ("swatch-magenta", (0.92, 0.28, 0.62), "PINK"),
    ]

    by_name = {m.get("name"): m for m in zone.setdefault("materials", [])}
    by_name.update({
        "color-picker-panel": material("color-picker-panel", dark_panel),
        "color-picker-red-ramp": material("color-picker-red-ramp", gradient_red),
        "color-picker-green-ramp": material("color-picker-green-ramp", gradient_green),
        "color-picker-blue-ramp": material("color-picker-blue-ramp", gradient_blue),
        "color-picker-value-ramp": material("color-picker-value-ramp", gradient_value),
        "color-picker-chromatic-field": material(
            "color-picker-chromatic-field", chromatic, texture_size=64),
        "material-color-picker-preview": material("material-color-picker-preview", white),
        "authored-color-target": material("authored-color-target", white),
        "tool-panel-material": material("tool-panel-material", tool_panel),
        "card-bg-material": material("card-bg-material", card_bg),
        "artboard-frame-material": material("artboard-frame-material", artboard_mat),
        "bar-header-material": material("bar-header-material", header_mat),
        "tool-pen-material": material("tool-pen-material", btn_pen_mat),
        "tool-eraser-material": material("tool-eraser-material", btn_eraser_mat),
        "swatch-tone-light-material": material("swatch-tone-light-material", solid_texture((0.95, 0.72, 0.75))),
        "swatch-tone-mid-material": material("swatch-tone-mid-material", solid_texture((0.85, 0.18, 0.22))),
        "swatch-tone-dark-material": material("swatch-tone-dark-material", solid_texture((0.32, 0.07, 0.09))),
    })

    for swatch_id, col, _ in swatches:
        by_name[f"{swatch_id}-material"] = material(f"{swatch_id}-material", solid_texture(col))

    zone["materials"] = list(by_name.values())

    # 2. Objects & Hierarchy
    objects = zone.setdefault("world", {}).setdefault("objects", [])
    by_id = {obj.get("objectID"): obj for obj in objects}

    canvas = by_id.get("basic-pixel-canvas")
    if canvas is not None:
        canvas.setdefault("authoredProperties", {}).setdefault(
            "paintColor", {"t": "vec3", "x": 1.0, "y": 0.15, "z": 0.15})
        canvas["zOrder2D"] = 5
        canvas["authoredProperties"]["displayName"] = {"t": "string", "v": "Basic Pixel Canvas (64x64)"}

    authored = [
        # --- Left Studio Toolbar (x = 18..138, width = 120, y = 20..700) ---
        object_2d("tool-panel", 18, 20, 120, 680, "tool-panel-material", z=1),
        object_2d("tool-header-title", 24, 34, 108, 20, "", "STUDIO TOOLS",
                  color=(0.85, 0.90, 1.0), z=30, text=True),

        # Tool cards
        object_2d("tool-btn-pen", 26, 68, 104, 36, "tool-pen-material", "PEN [DRAW]",
                  color=(0.20, 0.28, 0.42), z=20),
        object_2d("tool-btn-eraser", 26, 112, 104, 36, "tool-eraser-material", "ERASER",
                  color=(0.24, 0.28, 0.35), z=20),

        object_2d("tool-swatches-title", 24, 166, 108, 16, "", "QUICK PALETTE",
                  color=(0.80, 0.86, 0.96), z=30, text=True),

        # 12 Swatches in 2 columns of 6 rows
        # Column 1 (x = 26) & Column 2 (x = 82)
        object_2d("swatch-black", 26, 192, 48, 28, "swatch-black-material", "BLK", color=(0.04, 0.04, 0.04), z=20),
        object_2d("swatch-white", 82, 192, 48, 28, "swatch-white-material", "WHT", color=(1.0, 1.0, 1.0), z=20),
        object_2d("swatch-slate", 26, 226, 48, 28, "swatch-slate-material", "SLT", color=(0.35, 0.38, 0.45), z=20),
        object_2d("swatch-silver", 82, 226, 48, 28, "swatch-silver-material", "SLV", color=(0.72, 0.76, 0.82), z=20),
        object_2d("swatch-crimson", 26, 260, 48, 28, "swatch-crimson-material", "RED", color=(0.92, 0.15, 0.18), z=20),
        object_2d("swatch-amber", 82, 260, 48, 28, "swatch-amber-material", "ORG", color=(0.96, 0.52, 0.10), z=20),
        object_2d("swatch-lemon", 26, 294, 48, 28, "swatch-lemon-material", "YEL", color=(0.98, 0.85, 0.12), z=20),
        object_2d("swatch-emerald", 82, 294, 48, 28, "swatch-emerald-material", "GRN", color=(0.15, 0.76, 0.32), z=20),
        object_2d("swatch-cyan", 26, 328, 48, 28, "swatch-cyan-material", "CYN", color=(0.10, 0.80, 0.88), z=20),
        object_2d("swatch-cobalt", 82, 328, 48, 28, "swatch-cobalt-material", "BLU", color=(0.18, 0.38, 0.92), z=20),
        object_2d("swatch-purple", 26, 362, 48, 28, "swatch-purple-material", "PUR", color=(0.58, 0.20, 0.88), z=20),
        object_2d("swatch-magenta", 82, 362, 48, 28, "swatch-magenta-material", "PNK", color=(0.92, 0.28, 0.62), z=20),

        # Tool guide hints
        object_2d("tool-guide-title", 24, 410, 108, 14, "", "STUDIO HINTS",
                  color=(0.60, 0.68, 0.80), z=30, text=True),
        object_2d("tool-guide-1", 24, 432, 108, 12, "", "* Pick color/field",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-2", 24, 452, 108, 12, "", "* Click on canvas",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-3", 24, 472, 108, 12, "", "* Copy-on-write",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-4", 24, 492, 108, 12, "", "* Pure ECA laws",
                  color=(0.48, 0.54, 0.66), z=30, text=True),

        # --- Center Artboard & Frame (x = 152..680) ---
        object_2d("basic-pixel-canvas-frame", 152, 92, 528, 528, "artboard-frame-material", z=1),
        object_2d("canvas-header-bar", 152, 38, 528, 44, "bar-header-material", z=10),
        object_2d("canvas-header-title", 168, 48, 300, 22, "", "2D PIXEL CREATOR — 64x64 ARTBOARD",
                  color=(0.92, 0.95, 1.0), z=30, text=True),
        object_2d("canvas-header-badge", 480, 50, 190, 18, "", "NORMALIZED UV SINK",
                  color=(0.55, 0.65, 0.78), z=30, text=True),

        object_2d("canvas-footer-bar", 152, 630, 528, 34, "bar-header-material", z=10),
        object_2d("canvas-status-text", 168, 640, 500, 16, "",
                  "CLICK CANVAS TO INK PIXEL • SELECTED COLOR PERSISTS IN MATERIAL",
                  color=(0.60, 0.68, 0.80), z=30, text=True),

        # --- Right Color Inspector Panel (x = 690..1260, y = 20..700) ---
        object_2d("material-color-picker-panel", 690, 20, 570, 680,
                  "color-picker-panel", z=1),
        object_2d("material-color-picker-title", 710, 38, 520, 24,
                  "", "COLOR INSPECTOR — ONTOMATH HSV + RGB", color=(0.85, 0.90, 1.0), z=30, text=True),

        object_2d("color-spectrum-label", 710, 75, 300, 16,
                  "", "2D CHROMATIC FIELD [HUE x SATURATION]", color=(0.60, 0.68, 0.80), z=30, text=True),

        # Canonical interactive color controls (coordinates strictly preserved for test compatibility)
        object_2d("material-color-picker-chromatic", 710, 100, 300, 280,
                  "color-picker-chromatic-field", "HUE x SATURATION", z=20),

        object_2d("color-sliders-label", 710, 386, 300, 16,
                  "", "CHANNEL SLIDERS [VALUE / RED / GREEN / BLUE]", color=(0.60, 0.68, 0.80), z=30, text=True),

        object_2d("material-color-picker-value", 710, 402, 300, 42,
                  "color-picker-value-ramp", "VALUE / BRIGHTNESS", z=20),
        object_2d("material-color-picker-red", 710, 480, 300, 42,
                  "color-picker-red-ramp", "RED", z=20),
        object_2d("material-color-picker-green", 710, 540, 300, 42,
                  "color-picker-green-ramp", "GREEN", z=20),
        object_2d("material-color-picker-blue", 710, 600, 300, 42,
                  "color-picker-blue-ramp", "BLUE", z=20),

        object_2d("color-sliders-hint", 710, 652, 300, 14,
                  "", "CLICK ANY SLIDER TO SET INTENSITY [0.0 -> 1.0]",
                  color=(0.50, 0.56, 0.68), z=30, text=True),

        # Right Column in Inspector
        object_2d("color-preview-header", 1040, 75, 190, 16,
                  "", "SELECTED INK", color=(0.60, 0.68, 0.80), z=30, text=True),
        object_2d("material-color-picker", 1040, 100, 190, 120,
                  "material-color-picker-preview", "CURRENT COLOR",
                  color=(1.0, 0.15, 0.15), z=20,
                  properties={
                      "selectedColor": {"t": "vec3", "x": 1.0, "y": 0.15, "z": 0.15},
                      "hue": {"t": "double", "v": 0.0},
                      "saturation": {"t": "double", "v": 0.85},
                      "value": {"t": "double", "v": 1.0},
                  }),

        object_2d("color-target-header", 1040, 235, 190, 16,
                  "", "TARGET MATERIAL", color=(0.60, 0.68, 0.80), z=30, text=True),
        object_2d("material-color-target", 1040, 255, 190, 120,
                  "authored-color-target", "TARGET MATERIAL", color=(1.0, 1.0, 1.0), z=20),

        object_2d("color-tones-header", 1040, 392, 190, 16,
                  "", "COLOR HARMONY TONES", color=(0.60, 0.68, 0.80), z=30, text=True),
        object_2d("swatch-tone-light", 1040, 416, 190, 36,
                  "swatch-tone-light-material", "TINT [PASTEL]", color=(0.95, 0.72, 0.75), z=20),
        object_2d("swatch-tone-mid", 1040, 460, 190, 36,
                  "swatch-tone-mid-material", "MIDTONE [BASE]", color=(0.85, 0.18, 0.22), z=20),
        object_2d("swatch-tone-dark", 1040, 504, 190, 36,
                  "swatch-tone-dark-material", "SHADE [SHADOW]", color=(0.32, 0.07, 0.09), z=20),

        object_2d("studio-specs-card", 1040, 558, 190, 84, "card-bg-material", z=15),
        object_2d("studio-specs-title", 1050, 568, 170, 18, "", "ONTOMATH ENGINE",
                  color=(0.82, 0.88, 1.0), z=30, text=True),
        object_2d("studio-specs-body", 1050, 590, 170, 44, "",
                  "6-PIECE EXACT HSV\nPIXEL SINK: 64x64\nLAW COPY-ON-WRITE",
                  color=(0.50, 0.58, 0.70), z=30, text=True),
    ]

    for obj in authored:
        by_id[obj["objectID"]] = obj
    zone["world"]["objects"] = list(by_id.values())

    # 3. Relations and Formations
    relations = [r for r in zone.setdefault("formationRelations", [])
                 if not (isinstance(r, dict) and r.get("entityA") == r.get("entityB"))]
    zone["formationRelations"] = relations
    existing = {(r.get("entityA"), r.get("entityB"), r.get("type")) for r in relations if isinstance(r, dict)}

    # Attach inspector items to material-color-picker
    inspector_children = [
        "material-color-picker-title", "color-spectrum-label", "color-sliders-label",
        "color-sliders-hint", "color-preview-header", "color-target-header",
        "color-tones-header", "material-color-picker-red", "material-color-picker-green",
        "material-color-picker-blue", "material-color-target", "material-color-picker-chromatic",
        "material-color-picker-value", "swatch-tone-light", "swatch-tone-mid",
        "swatch-tone-dark", "studio-specs-card", "studio-specs-title", "studio-specs-body"
    ]
    for child in inspector_children:
        key = (child, "material-color-picker", "part-of")
        if key not in existing:
            relations.append(relation(*key))
            existing.add(key)

    # Attach toolbar items to tool-panel
    toolbar_children = [
        "tool-header-title", "tool-btn-pen", "tool-btn-eraser", "tool-swatches-title",
        "tool-guide-title", "tool-guide-1", "tool-guide-2", "tool-guide-3", "tool-guide-4"
    ] + [s[0] for s in swatches]
    for child in toolbar_children:
        key = (child, "tool-panel", "part-of")
        if key not in existing:
            relations.append(relation(*key))
            existing.add(key)

    # Attach frame items to basic-pixel-canvas-frame
    frame_children = [
        "canvas-header-bar", "canvas-header-title", "canvas-header-badge",
        "canvas-footer-bar", "canvas-status-text"
    ]
    for child in frame_children:
        key = (child, "basic-pixel-canvas-frame", "part-of")
        if key not in existing:
            relations.append(relation(*key))
            existing.add(key)

    zone["injected_by"] = INJECTED_BY
    write_json(ZONE_PATH, zone)

    # 4. Canonical Laws
    laws = [
        picker_law("law-material-color-picker-red", "Material Color Picker — red channel",
                   "material-color-picker-red", "r"),
        picker_law("law-material-color-picker-green", "Material Color Picker — green channel",
                   "material-color-picker-green", "g"),
        picker_law("law-material-color-picker-blue", "Material Color Picker — blue channel",
                   "material-color-picker-blue", "b"),
        chromatic_law("law-material-color-picker-chromatic",
                      "Material Color Picker — 2D chromatic selector",
                      "material-color-picker-chromatic", True),
        chromatic_law("law-material-color-picker-value",
                      "Material Color Picker — value channel",
                      "material-color-picker-value", False),
        material_apply_law(),
    ]
    for law in laws:
        write_json(LAW_DIR / law["identifier"] / "law.json", law)

    # Ensure law-basic-pixel-changer action model reads canvas-local paintColor
    pixel_law_path = LAW_DIR / "law-basic-pixel-changer" / "law.json"
    if pixel_law_path.exists():
        pixel_law = json.loads(pixel_law_path.read_text())
        pixel_law["law"]["actionModel"]["pixelColorPath"] = "paintColor"
        write_json(pixel_law_path, pixel_law)

    print("Enhanced 2D Pixel Creator authored successfully.")


if __name__ == "__main__":
    main()
