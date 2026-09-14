from __future__ import annotations

import base64
import colorsys
import json
from pathlib import Path
from typing import Any

ROOT = Path("/Users/zacharyzhang/Documents/GitHub/Earthcall")
ZONE_PATH = ROOT / "saves/zones/BasicPixelChanger/zone.json"
LAW_DIR = ROOT / "saves/laws"
AUTHOR = "Zach"
INJECTED_BY = "Gemini Spark, session 2026-09-14-02-45"


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
    """Exact HSV->RGB as six authored OntoMath pieces over h in [0,1]."""
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
        # 1. Swatch / Tool Selection: if clicked subject carries swatchColor, set selectedColor
        map_action(
            "@material-color-picker.selectedColor",
            vector_node("c"), "c", "@event.subject.swatchColor"),
        # 2. Immediate Click / Press / Drag Paint: if clicked subject is basic-pixel-canvas, write pixel immediately
        {
            "kind": 21,
            "pixelColorPath": "paintColor",
            "pixelFacePath": "@interaction-channel.hoveredFace",
            "pixelUPath": "@interaction-channel.hoveredU",
            "pixelVPath": "@interaction-channel.hoveredV"
        },
        # 3. Tool Brush Radius: sync brush radius if clicking a tool button
        map_action(
            "@basic-pixel-canvas.brushRadius",
            scalar_node("r"), "r", "@event.subject.swatchBrushRadius"),
        # 4. Canvas Inking: synchronize canvas paintColor with active selectedColor
        map_action(
            "@basic-pixel-canvas.paintColor",
            vector_node("c"), "c", "@material-color-picker.selectedColor"),
        # 5. Preview and Target Material live updates
        map_action(
            "@material.material-color-picker-preview.baseColor",
            vector_node("c"), "c", "@material-color-picker.selectedColor"),
        map_action(
            "@material.authored-color-target.baseColor",
            vector_node("c"), "c", "@material-color-picker.selectedColor"),
        map_action(
            "@creation-channel.activeColor",
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
            "conditionModel": {
                "kind": 4,  # Any
                "children": [
                    {"kind": 8, "otherId": "material-color-picker"},
                    {"kind": 2, "relationType": "part-of", "otherId": "tool-panel"},
                    {"kind": 2, "relationType": "part-of", "otherId": "material-color-picker"},
                    {"kind": 8, "otherId": "basic-pixel-canvas"}
                ]
            },
            "conditionSubjects": [],
            "drives": False,
            "enabled": True,
            "id": law_id,
            "name": "Material Color Picker — apply to Material & Canvas",
            "provenance": [{"directed": True, "entityA": law_id,
                            "entityB": AUTHOR, "events": [],
                            "type": "authored-by", "weight": 1.0}],
            "retrigger": 0,
            "scope": 0,
            "targets": [],
        },
        "triggers": [
            "color-selection-changed",
            "object-clicked",
            "object-pressed",
            "object-drag-started",
            "object-dragged",
            "object-drag-ended"
        ],
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
    card_bg = solid_texture((0.105, 0.125, 0.165))
    artboard_mat = solid_texture((0.085, 0.10, 0.135))
    header_mat = solid_texture((0.07, 0.08, 0.11))
    btn_pen_mat = solid_texture((0.15, 0.22, 0.35))
    btn_brush_mat = solid_texture((0.16, 0.28, 0.45))
    btn_eraser_mat = solid_texture((0.20, 0.23, 0.30))
    btn_fill_mat = solid_texture((0.12, 0.24, 0.38))
    btn_clear_mat = solid_texture((0.26, 0.11, 0.14))
    btn_swap_mat = solid_texture((0.18, 0.18, 0.24))

    # Curated 16-color professional pixel art palette (8 rows x 2 columns)
    swatches = [
        # Neutrals
        ("swatch-black", (0.04, 0.04, 0.04), "BLK"),
        ("swatch-white", (1.0, 1.0, 1.0), "WHT"),
        ("swatch-slate", (0.35, 0.38, 0.45), "SLT"),
        ("swatch-silver", (0.72, 0.76, 0.82), "SLV"),
        # Warm / Sunset
        ("swatch-crimson", (0.92, 0.15, 0.18), "RED"),
        ("swatch-coral", (0.96, 0.38, 0.22), "CRL"),
        ("swatch-amber", (0.96, 0.55, 0.10), "ORG"),
        ("swatch-lemon", (0.98, 0.85, 0.12), "YEL"),
        # Nature / Foliage
        ("swatch-lime", (0.55, 0.82, 0.18), "LIM"),
        ("swatch-emerald", (0.15, 0.76, 0.32), "GRN"),
        ("swatch-teal", (0.12, 0.65, 0.68), "TEA"),
        ("swatch-cyan", (0.10, 0.80, 0.88), "CYN"),
        # Cool / Vivid
        ("swatch-cobalt", (0.18, 0.38, 0.92), "BLU"),
        ("swatch-indigo", (0.38, 0.22, 0.88), "IND"),
        ("swatch-purple", (0.65, 0.18, 0.85), "PUR"),
        ("swatch-magenta", (0.92, 0.28, 0.62), "PNK"),
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
        "tool-brush-material": material("tool-brush-material", btn_brush_mat),
        "tool-eraser-material": material("tool-eraser-material", btn_eraser_mat),
        "tool-fill-material": material("tool-fill-material", btn_fill_mat),
        "tool-clear-material": material("tool-clear-material", btn_clear_mat),
        "tool-swap-material": material("tool-swap-material", btn_swap_mat),
        "swatch-tone-light-material": material("swatch-tone-light-material", solid_texture((0.95, 0.72, 0.75))),
        "swatch-tone-mid-material": material("swatch-tone-mid-material", solid_texture((0.85, 0.18, 0.22))),
        "swatch-tone-dark-material": material("swatch-tone-dark-material", solid_texture((0.32, 0.07, 0.09))),
        "swatch-tone-accent-material": material("swatch-tone-accent-material", solid_texture((0.15, 0.72, 0.88))),
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
        canvas.setdefault("authoredProperties", {}).setdefault(
            "brushRadius", {"t": "double", "v": 1.0})
        canvas["zOrder2D"] = 5
        canvas["authoredProperties"]["displayName"] = {"t": "string", "v": "Basic Pixel Canvas (64x64)"}
        full_canvas_selector = {
            "input": "u",
            "pieces": [
                {
                    "lo": 0.0,
                    "includeLo": True,
                    "hi": 1.0,
                    "includeHi": True,
                    "mathNode": {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {}}]}}
                }
            ]
        }
        canvas["authoredProperties"]["surface.selection.authored.full-canvas"] = {
            "t": "string",
            "v": json.dumps({"face": 0, "selector": full_canvas_selector})
        }

    authored = [
        # --- Left Studio Toolbar (x = 18..138, width = 120, y = 20..700) ---
        object_2d("tool-panel", 18, 20, 120, 680, "tool-panel-material", z=1),
        object_2d("tool-header-title", 24, 34, 108, 20, "", "STUDIO TOOLS",
                  color=(0.58, 0.77, 0.99), z=30, text=True),

        # GIMP / Clip Studio Primary Tool Rack (6 Action Cards)
        object_2d("tool-btn-pen", 26, 58, 104, 26, "tool-pen-material", "PEN [1px]",
                  color=(0.20, 0.28, 0.42), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.10, "y": 0.70, "z": 0.25},
                              "swatchBrushRadius": {"t": "double", "v": 1.0}}),
        object_2d("tool-btn-brush", 26, 88, 104, 26, "tool-brush-material", "BRUSH [2px]",
                  color=(0.18, 0.32, 0.48), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.18, "y": 0.38, "z": 0.92},
                              "swatchBrushRadius": {"t": "double", "v": 2.0}}),
        object_2d("tool-btn-eraser", 26, 118, 104, 26, "tool-eraser-material", "ERASER",
                  color=(0.24, 0.28, 0.35), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 1.0, "y": 1.0, "z": 1.0},
                              "swatchBrushRadius": {"t": "double", "v": 2.0}}),
        object_2d("tool-btn-fill", 26, 148, 104, 26, "tool-fill-material", "FILL INK",
                  color=(0.18, 0.32, 0.50), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.10, "y": 0.70, "z": 0.25}}),
        object_2d("tool-btn-clear", 26, 178, 104, 26, "tool-clear-material", "CLEAR [WHT]",
                  color=(0.35, 0.15, 0.18), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 1.0, "y": 1.0, "z": 1.0}}),
        object_2d("tool-btn-swap", 26, 208, 104, 26, "tool-swap-material", "SWAP [BLK]",
                  color=(0.18, 0.20, 0.26), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.04, "y": 0.04, "z": 0.04}}),

        object_2d("tool-swatches-title", 24, 240, 108, 16, "", "QUICK PALETTE",
                  color=(0.80, 0.86, 0.96), z=30, text=True),

        # 16 Swatches in 2 columns of 8 rows (x = 26 & x = 82)
        object_2d("swatch-black", 26, 258, 48, 22, "swatch-black-material", "BLK", color=(0.04, 0.04, 0.04), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.04, "y": 0.04, "z": 0.04}}),
        object_2d("swatch-white", 82, 258, 48, 22, "swatch-white-material", "WHT", color=(1.0, 1.0, 1.0), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 1.0, "y": 1.0, "z": 1.0}}),
        object_2d("swatch-slate", 26, 284, 48, 22, "swatch-slate-material", "SLT", color=(0.35, 0.38, 0.45), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.35, "y": 0.38, "z": 0.45}}),
        object_2d("swatch-silver", 82, 284, 48, 22, "swatch-silver-material", "SLV", color=(0.72, 0.76, 0.82), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.72, "y": 0.76, "z": 0.82}}),
        object_2d("swatch-crimson", 26, 310, 48, 22, "swatch-crimson-material", "RED", color=(0.92, 0.15, 0.18), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.92, "y": 0.15, "z": 0.18}}),
        object_2d("swatch-coral", 82, 310, 48, 22, "swatch-coral-material", "CRL", color=(0.96, 0.38, 0.22), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.96, "y": 0.38, "z": 0.22}}),
        object_2d("swatch-amber", 26, 336, 48, 22, "swatch-amber-material", "ORG", color=(0.96, 0.55, 0.10), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.96, "y": 0.55, "z": 0.10}}),
        object_2d("swatch-lemon", 82, 336, 48, 22, "swatch-lemon-material", "YEL", color=(0.98, 0.85, 0.12), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.98, "y": 0.85, "z": 0.12}}),
        object_2d("swatch-lime", 26, 362, 48, 22, "swatch-lime-material", "LIM", color=(0.55, 0.82, 0.18), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.55, "y": 0.82, "z": 0.18}}),
        object_2d("swatch-emerald", 82, 362, 48, 22, "swatch-emerald-material", "GRN", color=(0.15, 0.76, 0.32), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.15, "y": 0.76, "z": 0.32}}),
        object_2d("swatch-teal", 26, 388, 48, 22, "swatch-teal-material", "TEA", color=(0.12, 0.65, 0.68), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.12, "y": 0.65, "z": 0.68}}),
        object_2d("swatch-cyan", 82, 388, 48, 22, "swatch-cyan-material", "CYN", color=(0.10, 0.80, 0.88), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.10, "y": 0.80, "z": 0.88}}),
        object_2d("swatch-cobalt", 26, 414, 48, 22, "swatch-cobalt-material", "BLU", color=(0.18, 0.38, 0.92), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.18, "y": 0.38, "z": 0.92}}),
        object_2d("swatch-indigo", 82, 414, 48, 22, "swatch-indigo-material", "IND", color=(0.38, 0.22, 0.88), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.38, "y": 0.22, "z": 0.88}}),
        object_2d("swatch-purple", 26, 440, 48, 22, "swatch-purple-material", "PUR", color=(0.65, 0.18, 0.85), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.65, "y": 0.18, "z": 0.85}}),
        object_2d("swatch-magenta", 82, 440, 48, 22, "swatch-magenta-material", "PNK", color=(0.92, 0.28, 0.62), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.92, "y": 0.28, "z": 0.62}}),

        # GIMP / Clip Studio Shortcuts & Guide
        object_2d("tool-guide-title", 24, 474, 108, 14, "", "STUDIO GUIDE",
                  color=(0.60, 0.68, 0.80), z=30, text=True),
        object_2d("tool-guide-1", 24, 494, 108, 12, "", "* Pen/Brush: Draw",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-2", 24, 512, 108, 12, "", "* Eraser: White ink",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-3", 24, 530, 108, 12, "", "* Fill: Flood board",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-4", 24, 548, 108, 12, "", "* Swap: Quick black",
                  color=(0.48, 0.54, 0.66), z=30, text=True),
        object_2d("tool-guide-5", 24, 566, 108, 12, "", "* Drag: Smooth line",
                  color=(0.48, 0.54, 0.66), z=30, text=True),

        # --- Center Artboard & Frame (x = 152..680) ---
        object_2d("basic-pixel-canvas-frame", 152, 92, 528, 528, "artboard-frame-material", z=1),
        object_2d("canvas-header-bar", 152, 38, 528, 44, "bar-header-material", z=10),
        object_2d("canvas-header-title", 168, 48, 300, 22, "", "2D PIXEL CREATOR — 64x64 MATRIX",
                  color=(0.92, 0.95, 1.0), z=30, text=True),
        object_2d("canvas-header-badge", 470, 50, 200, 18, "", "UV SINK • 8px/TEXEL",
                  color=(0.55, 0.65, 0.78), z=30, text=True),

        object_2d("canvas-footer-bar", 152, 630, 528, 34, "bar-header-material", z=10),
        object_2d("canvas-status-text", 168, 640, 500, 16, "",
                  "CLICK/DRAG TO INK • CHOOSE TOOLS OR PALETTE ON LEFT • COLOR LAB ON RIGHT",
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
                  "", "CHANNEL CONTROLS [VALUE / RED / GREEN / BLUE]", color=(0.60, 0.68, 0.80), z=30, text=True),

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

        # Right Column in Inspector (x = 1040..1230)
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

        object_2d("color-tones-header", 1040, 390, 190, 16,
                  "", "HARMONY TONES (CLICK TO INK)", color=(0.60, 0.68, 0.80), z=30, text=True),
        object_2d("swatch-tone-light", 1040, 410, 190, 24,
                  "swatch-tone-light-material", "TINT [PASTEL]", color=(0.95, 0.72, 0.75), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.95, "y": 0.72, "z": 0.75}}),
        object_2d("swatch-tone-mid", 1040, 438, 190, 24,
                  "swatch-tone-mid-material", "MIDTONE [BASE]", color=(0.85, 0.18, 0.22), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.85, "y": 0.18, "z": 0.22}}),
        object_2d("swatch-tone-dark", 1040, 466, 190, 24,
                  "swatch-tone-dark-material", "SHADE [SHADOW]", color=(0.32, 0.07, 0.09), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.32, "y": 0.07, "z": 0.09}}),
        object_2d("swatch-tone-accent", 1040, 494, 190, 24,
                  "swatch-tone-accent-material", "ACCENT [CONTRAST]", color=(0.15, 0.72, 0.88), z=20,
                  properties={"swatchColor": {"t": "vec3", "x": 0.15, "y": 0.72, "z": 0.88}}),

        # GIMP / Clip Studio Layers & Engine Deck
        object_2d("layers-deck-card", 1040, 526, 190, 116, "card-bg-material", z=15),
        object_2d("layers-deck-title", 1050, 536, 170, 16, "", "LAYERS & ENGINE",
                  color=(0.82, 0.88, 1.0), z=30, text=True),
        object_2d("layers-deck-l1", 1050, 556, 170, 13, "", "L1: INK [COPY-ON-WRITE]",
                  color=(0.60, 0.75, 0.95), z=30, text=True),
        object_2d("layers-deck-l0", 1050, 574, 170, 13, "", "L0: BASE [WHITE RGBA8]",
                  color=(0.55, 0.62, 0.74), z=30, text=True),
        object_2d("layers-deck-blend", 1050, 592, 170, 13, "", "BLEND: NORMAL • 100%",
                  color=(0.48, 0.54, 0.65), z=30, text=True),
        object_2d("layers-deck-scale", 1050, 610, 170, 13, "", "ZOOM: 8x • GRID: 64x64",
                  color=(0.48, 0.54, 0.65), z=30, text=True),
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
        "swatch-tone-dark", "swatch-tone-accent", "layers-deck-card", "layers-deck-title",
        "layers-deck-l1", "layers-deck-l0", "layers-deck-blend", "layers-deck-scale"
    ]
    for child in inspector_children:
        key = (child, "material-color-picker", "part-of")
        if key not in existing:
            relations.append(relation(*key))
            existing.add(key)

    # Attach toolbar items to tool-panel
    toolbar_children = [
        "tool-header-title", "tool-btn-pen", "tool-btn-brush", "tool-btn-eraser",
        "tool-btn-fill", "tool-btn-clear", "tool-btn-swap", "tool-swatches-title",
        "tool-guide-title", "tool-guide-1", "tool-guide-2", "tool-guide-3",
        "tool-guide-4", "tool-guide-5"
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

    pixel_law_path = LAW_DIR / "law-basic-pixel-changer" / "law.json"
    if pixel_law_path.exists():
        pixel_law = json.loads(pixel_law_path.read_text())
        pixel_law["law"]["actionModel"]["pixelColorPath"] = "paintColor"
        write_json(pixel_law_path, pixel_law)

    print("GIMP/Clip Studio style 2D Studio authored successfully.")


if __name__ == "__main__":
    main()
