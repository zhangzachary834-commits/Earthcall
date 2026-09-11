#!/usr/bin/env python3
"""Author the Law/Singular-driven material color picker demo.

This is a First-Mover authoring tool, not runtime behavior. It augments the
BasicPixelChanger Zone with ordinary Shape2D/Text2D beings and writes shared
Law roots whose models are the same models a Person can author in the Law
Author. Existing canvas paint is preserved: objects and materials are merged
by stable identifier/name instead of replacing the Zone snapshot.
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
INJECTED_BY = "Codex (GPT-5), session 01a07d15-f266-7902-bc11-cf7b06b0b343"


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
        props.setdefault("label.size2D", {"t": "double", "v": min(18.0, max(11.0, height * 0.35))})
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
        # Keep the legacy Creator Console selection synchronized as a bridge;
        # the pixel writer itself reads selectedColor below.
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
    # Keep every unrelated authored reference in place, but make this
    # instrument's closure deterministic and dependency-readable on reruns.
    zone["lawRefs"] = [ref for ref in zone["lawRefs"] if ref not in set(law_ids)] + law_ids

    gradient_red = rgba_texture(32, lambda u, v: (u, 0.0, 0.0))
    gradient_green = rgba_texture(32, lambda u, v: (0.0, u, 0.0))
    gradient_blue = rgba_texture(32, lambda u, v: (0.0, 0.0, u))
    gradient_value = rgba_texture(32, lambda u, v: (u, u, u))
    chromatic = rgba_texture(
        64, lambda u, v: colorsys.hsv_to_rgb(u, 1.0 - v, 1.0))
    white = rgba_texture(32, lambda u, v: (1.0, 1.0, 1.0))
    panel = rgba_texture(8, lambda u, v: (0.055, 0.065, 0.09))

    by_name = {m.get("name"): m for m in zone.setdefault("materials", [])}
    by_name.update({
        "color-picker-panel": material("color-picker-panel", panel),
        "color-picker-red-ramp": material("color-picker-red-ramp", gradient_red),
        "color-picker-green-ramp": material("color-picker-green-ramp", gradient_green),
        "color-picker-blue-ramp": material("color-picker-blue-ramp", gradient_blue),
        "color-picker-value-ramp": material("color-picker-value-ramp", gradient_value),
        "color-picker-chromatic-field": material(
            "color-picker-chromatic-field", chromatic, texture_size=64),
        "material-color-picker-preview": material("material-color-picker-preview", white),
        "authored-color-target": material("authored-color-target", white),
    })
    zone["materials"] = list(by_name.values())

    objects = zone.setdefault("world", {}).setdefault("objects", [])
    by_id = {obj.get("objectID"): obj for obj in objects}
    authored = [
        object_2d("material-color-picker-panel", 690, 20, 570, 680,
                  "color-picker-panel", z=1),
        object_2d("material-color-picker-title", 710, 38, 520, 28,
                  "", "MATERIAL COLOR — LAW + ONTOMATH", color=(0.82, 0.88, 1.0), z=30, text=True),
        object_2d("material-color-picker", 1040, 110, 190, 120,
                  "material-color-picker-preview", "SELECTED COLOR",
                  color=(1.0, 0.15, 0.15), z=20,
                  properties={
                      "selectedColor": {"t": "vec3", "x": 1.0, "y": 0.15, "z": 0.15},
                      "hue": {"t": "double", "v": 0.0},
                      "saturation": {"t": "double", "v": 0.85},
                      "value": {"t": "double", "v": 1.0},
                  }),
        object_2d("material-color-picker-chromatic", 710, 100, 300, 280,
                  "color-picker-chromatic-field", "HUE × SATURATION", z=20),
        object_2d("material-color-picker-value", 710, 402, 300, 42,
                  "color-picker-value-ramp", "VALUE / BRIGHTNESS", z=20),
        object_2d("material-color-picker-red", 710, 480, 300, 42,
                  "color-picker-red-ramp", "RED", z=20),
        object_2d("material-color-picker-green", 710, 540, 300, 42,
                  "color-picker-green-ramp", "GREEN", z=20),
        object_2d("material-color-picker-blue", 710, 600, 300, 42,
                  "color-picker-blue-ramp", "BLUE", z=20),
        object_2d("material-color-target", 1040, 260, 190, 120,
                  "authored-color-target", "TARGET MATERIAL", color=(1.0, 1.0, 1.0), z=20),
    ]
    for obj in authored:
        by_id[obj["objectID"]] = obj
    zone["world"]["objects"] = list(by_id.values())

    relations = [r for r in zone.setdefault("formationRelations", [])
                 if not (isinstance(r, dict) and r.get("entityA") == r.get("entityB"))]
    zone["formationRelations"] = relations
    existing = {(r.get("entityA"), r.get("entityB"), r.get("type")) for r in relations if isinstance(r, dict)}
    for object_id in [obj["objectID"] for obj in authored
                      if obj["objectID"] not in {"material-color-picker-panel",
                                                   "material-color-picker"}]:
        key = (object_id, "material-color-picker", "part-of")
        if key not in existing:
            relations.append(relation(*key))

    zone["injected_by"] = INJECTED_BY
    write_json(ZONE_PATH, zone)

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

    # The pixel writer now consumes the same authored color Singular as the
    # material picker. Keep the old CreationChannel path out of the Person
    # path; the RGB bars still mirror it as a compatibility bridge.
    pixel_law_path = LAW_DIR / "law-basic-pixel-changer" / "law.json"
    if pixel_law_path.exists():
        pixel_law = json.loads(pixel_law_path.read_text())
        pixel_law["law"]["actionModel"]["pixelColorPath"] = "@material-color-picker.selectedColor"
        write_json(pixel_law_path, pixel_law)


if __name__ == "__main__":
    main()
