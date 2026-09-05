#!/usr/bin/env python3
"""First-Mover authoring of a 3D Go (Weiqi / Baduk) world as serialized beings.

Implements the Go board game in Earthcall:
- One single rectangular prism board (Goban) with 19x19 grid spacing,
  custom Kaya wood FaceTexture with 19x19 grid lines and 9 star points (hoshi).
- 361 properly positioned intersection beings with world coordinates,
  geometry, and state properties.
- Black and white stone bowls (Goke) and stones.
- Player seats for Black and White (hotseat).
- Go state tracking (turn, moveCount, phase).
- Core interaction laws for clicking intersections and placing stones.

Writes:
- saves/worlds/go_app.json
- saves/worlds/go_app.ecform
- saves/zones/Go Game/zone.json
- saves/zones/Go/zone.json
"""

from __future__ import annotations

import base64
import json
import math
from pathlib import Path

AUTHOR = "grok-4.6"
ZONE_ID = "Go Game"
ZONE_NAME = "Go Game"

SPACING = 0.5  # Distance between adjacent grid lines
BOARD_WIDTH = 10.0
BOARD_LENGTH = 10.0
BOARD_DEPTH = 0.5

# Board colors
KAYA = (218, 175, 105)
WOOD = (165, 120, 65)
CHARCOAL = (24, 24, 26)       # Black stone
IVORY = (242, 240, 235)       # White stone
BOWL_WOOD = (110, 68, 32)
LINE_COLOR = (32, 26, 20)     # Ink for grid lines & hoshi

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

def identity(other_id):
    return {"kind": 8, "otherId": other_id}

def for_any(*inner, being_kind=1):
    body = inner[0] if len(inner) == 1 else all_of(*inner)
    return {"kind": 9, "beingKind": being_kind, "children": [body]}

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
        "events": [{"description": "authored-by", "deltaWeight": 1.0, "timestamp": 1787390000}],
    }]

def mat4_translate(x, y, z, scale=(1.0, 1.0, 1.0)):
    sx, sy, sz = scale
    return [
        sx, 0.0, 0.0, 0.0,
        0.0, sy, 0.0, 0.0,
        0.0, 0.0, sz, 0.0,
        x, y, z, 1.0,
    ]

def extra_spatial(object_id, properties, display_name):
    authored = dict(properties)
    authored["displayName"] = pv("string", display_name)
    return {
        "objectID": object_id,
        "shapeKind": 12,  # Shape2D: not drawn on the 3D path
        "geometryType": 12,
        "shapeParams": [0.0] * 10,
        "transform": mat4_translate(80.0, 80.0, 80.0, (0.01, 0.01, 0.01)),
        "center": [80.0, 80.0, 80.0],
        "materialId": "",
        "authoredProperties": authored,
    }

def category_being(object_id, display_name):
    return {
        "objectID": object_id,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0] * 10,
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
        "events": [{"description": "instance-of", "deltaWeight": 1.0, "timestamp": 1787390000}],
    }

def subcategory_rel(a, b):
    return {
        "type": "subcategory-of",
        "entityA": a,
        "entityB": b,
        "directed": True,
        "weight": 1.0,
        "events": [{"description": "subcategory-of", "deltaWeight": 1.0, "timestamp": 1787390000}],
    }

# ---------------------------------------------------------------------------
# Texture Generation
# ---------------------------------------------------------------------------

def rgba_bytes(size, painter):
    buf = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            r, g, b = painter(x, y, size)
            i = (y * size + x) * 4
            buf[i] = r
            buf[i + 1] = g
            buf[i + 2] = b
            buf[i + 3] = 255
    return bytes(buf)

def solid_face(size, rgb):
    r, g, b = rgb
    pixels = rgba_bytes(size, lambda x, y, s: (r, g, b))
    return {"size": size, "pixelsB64": base64.b64encode(pixels).decode("ascii")}

def goban_grid_face(size=128):
    """Generates the top face of a 19x19 Goban with wood grain, 19 lines, and 9 star points (hoshi)."""
    # Grid margins: 5% border on all sides
    margin_ratio = 0.05
    grid_start = margin_ratio * (size - 1)
    grid_span = (1.0 - 2.0 * margin_ratio) * (size - 1)
    step = grid_span / 18.0

    line_coords = [grid_start + i * step for i in range(19)]
    hoshi_indices = [3, 9, 15]
    hoshi_coords = [(line_coords[xi], line_coords[yi]) for xi in hoshi_indices for yi in hoshi_indices]

    def paint(px, py, s):
        # Subtle organic wood grain along X/Y
        grain = int(3.5 * math.sin(py * 0.25) + 1.5 * math.cos((px + py) * 0.12))
        r = max(0, min(255, KAYA[0] + grain))
        g = max(0, min(255, KAYA[1] + grain))
        b = max(0, min(255, KAYA[2] + grain))

        # Check for hoshi (star point) dots (radius ~2.2 pixels)
        for hx, hy in hoshi_coords:
            dist_sq = (px - hx)**2 + (py - hy)**2
            if dist_sq <= 4.8:
                return LINE_COLOR

        # Check for grid lines
        on_grid_x = any(abs(px - lx) <= 0.65 for lx in line_coords)
        on_grid_y = any(abs(py - ly) <= 0.65 for ly in line_coords)

        in_bounds_x = (line_coords[0] - 0.65) <= px <= (line_coords[-1] + 0.65)
        in_bounds_y = (line_coords[0] - 0.65) <= py <= (line_coords[-1] + 0.65)

        if (on_grid_x and in_bounds_y) or (on_grid_y and in_bounds_x):
            return LINE_COLOR

        return (r, g, b)

    pixels = rgba_bytes(size, paint)
    return {"size": size, "pixelsB64": base64.b64encode(pixels).decode("ascii")}

# ---------------------------------------------------------------------------
# Laws
# ---------------------------------------------------------------------------

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

def go_pointer_bins():
    pieces = []
    for i in range(19):
        lo = (i - 9.0) * SPACING - (SPACING / 2.0)
        hi = (i - 9.0) * SPACING + (SPACING / 2.0)
        pieces.append({
            "lo": float(lo),
            "hi": float(hi),
            "includeLo": True,
            "includeHi": (i == 18),
            "expr": {"terms": const_terms(i)},
        })
    return pieces

def build_laws():
    # Click addressing law: when board or intersection is clicked, compute grid coordinates
    add_law(
        "law-go-click",
        "address-clicked-intersection",
        0,
        ["object-clicked"],
        any_of(
            compare("isBoard", 0, pv("bool", True)),
            compare("isIntersection", 0, pv("bool", True)),
        ),
        seq(
            {
                "kind": 8,
                "path": "@go_state.targetX",
                "bindings": {"ptrX": "@interaction-channel.pointerWorld.x"},
                "function": {"input": "ptrX", "pieces": go_pointer_bins()},
            },
            {
                "kind": 8,
                "path": "@go_state.targetY",
                "bindings": {"ptrZ": "@interaction-channel.pointerWorld.z"},
                "function": {"input": "ptrZ", "pieces": go_pointer_bins()},
            },
            map_path(
                "@state.go.targetX",
                {"tx": "@go_state.targetX"},
                copy_terms("tx"),
            ),
            map_path(
                "@state.go.targetY",
                {"ty": "@go_state.targetY"},
                copy_terms("ty"),
            ),
            publish("intersection-clicked", "go_state"),
        ),
        scope=0,
    )

    # Place Black Stone law
    add_law(
        "law-go-place-black",
        "place-black-stone",
        0,
        ["object-clicked"],
        all_of(
            compare("isIntersection", 0, pv("bool", True)),
            compare("is_empty", 0, pv("bool", True)),
            compare("@go_state.current_turn", 0, pv("string", "black")),
        ),
        seq(
            set_path("is_empty", pv("bool", False)),
            set_path("isEmpty", pv("bool", False)),
            set_path("stone_color", pv("string", "black")),
            set_path("stoneColor", pv("int", 1)),
            set_path("shape", pv("string", "Sphere")),
            set_path("@go_state.current_turn", pv("string", "white")),
            set_path("@state.go.current_turn", pv("string", "white")),
            publish("stone-placed", "go_state"),
        ),
        scope=0,
    )

    # Place White Stone law
    add_law(
        "law-go-place-white",
        "place-white-stone",
        0,
        ["object-clicked"],
        all_of(
            compare("isIntersection", 0, pv("bool", True)),
            compare("is_empty", 0, pv("bool", True)),
            compare("@go_state.current_turn", 0, pv("string", "white")),
        ),
        seq(
            set_path("is_empty", pv("bool", False)),
            set_path("isEmpty", pv("bool", False)),
            set_path("stone_color", pv("string", "white")),
            set_path("stoneColor", pv("int", 2)),
            set_path("shape", pv("string", "Sphere")),
            set_path("@go_state.current_turn", pv("string", "black")),
            set_path("@state.go.current_turn", pv("string", "black")),
            publish("stone-placed", "go_state"),
        ),
        scope=0,
    )

# ---------------------------------------------------------------------------
# World Building
# ---------------------------------------------------------------------------

def build_world():
    build_laws()

    # Materials
    board_faces = [solid_face(8, WOOD) for _ in range(6)]
    board_faces[2] = goban_grid_face(128)  # Top face: 19x19 grid with hoshi

    black_faces = [solid_face(8, CHARCOAL) for _ in range(6)]
    white_faces = [solid_face(8, IVORY) for _ in range(6)]
    bowl_faces = [solid_face(8, BOWL_WOOD) for _ in range(6)]
    intersection_faces = [solid_face(8, KAYA) for _ in range(6)]

    materials = [
        {
            "name": "go.board",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 16.0,
            "specular": 0.2,
            "ambient": 0.25,
            "diffuse": 0.85,
            "faceTextures": board_faces,
        },
        {
            "name": "go.black",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 48.0,
            "specular": 0.5,
            "ambient": 0.15,
            "diffuse": 0.8,
            "faceTextures": black_faces,
        },
        {
            "name": "go.white",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 48.0,
            "specular": 0.6,
            "ambient": 0.22,
            "diffuse": 0.85,
            "faceTextures": white_faces,
        },
        {
            "name": "go.bowl",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 24.0,
            "specular": 0.3,
            "ambient": 0.2,
            "diffuse": 0.8,
            "faceTextures": bowl_faces,
        },
        {
            "name": "go.intersection",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 16.0,
            "specular": 0.2,
            "ambient": 0.25,
            "diffuse": 0.85,
            "faceTextures": intersection_faces,
        },
    ]

    # Categories
    categories = [
        category_being("category.go", "Go Game"),
        category_being("category.go.board", "Go Board"),
        category_being("category.go.intersection", "Go Intersection"),
        category_being("category.go.stone", "Go Stone"),
        category_being("category.go.player", "Go Player"),
        category_being("category.go.bowl", "Go Bowl"),
        category_being("grok-4.6", "grok-4.6 First Mover"),
    ]

    # Board Being
    wood_rgb = [c / 255.0 for c in WOOD]
    kaya_rgb = [c / 255.0 for c in KAYA]
    board = {
        "objectID": "object.go.board",
        "baseline": "ground",
        "shapeKind": 0,  # Cube
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -BOARD_DEPTH / 2.0, 0.0, (BOARD_WIDTH, BOARD_DEPTH, BOARD_LENGTH)),
        "center": [0.0, -BOARD_DEPTH / 2.0, 0.0],
        "materialId": "material.go.board",
        "faceColors": [wood_rgb, wood_rgb, kaya_rgb, wood_rgb, wood_rgb, wood_rgb],
        "authoredProperties": {
            "isBoard": pv("bool", True),
            "displayName": pv("string", "Goban"),
        },
    }

    # Intersections
    intersections = []
    intersection_ids = []
    for x in range(19):
        for y in range(19):
            ix_id = f"intersection_{x}_{y}"
            intersection_ids.append(ix_id)
            wx = (x - 9.0) * SPACING
            wz = (y - 9.0) * SPACING
            wy = 0.005  # Resting flush on top of the board
            intersections.append({
                "objectID": ix_id,
                "shapeKind": 3,  # Cylinder pad at the intersection
                "geometryType": 3,
                "shapeParams": [0.08, 0.08, 0.08, 0.005, 0.0, 0.0, 0.0, 0.0, 0.0],
                "transform": mat4_translate(wx, wy, wz, (0.16, 0.01, 0.16)),
                "center": [wx, wy, wz],
                "materialId": "material.go.intersection",
                "faceColors": [kaya_rgb for _ in range(6)],
                "authoredProperties": {
                    "grid_x": pv("double", float(x)),
                    "grid_y": pv("double", float(y)),
                    "gridX": pv("int", x),
                    "gridY": pv("int", y),
                    "is_empty": pv("bool", True),
                    "isEmpty": pv("bool", True),
                    "stone_color": pv("string", "none"),
                    "stoneColor": pv("int", 0),
                    "shape": pv("string", "Intersection"),
                    "restY": pv("double", wy),
                    "isIntersection": pv("bool", True),
                    "displayName": pv("string", f"Intersection ({x}, {y})"),
                },
            })

    # Bowls
    bowl_rgb = [c / 255.0 for c in BOWL_WOOD]
    bowls = [
        {
            "objectID": "object.go.bowl.black",
            "shapeKind": 3,  # Cylinder bowl
            "geometryType": 3,
            "shapeParams": [0.65, 0.65, 0.65, 0.22, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(5.8, 0.22, -5.2, (1.3, 0.44, 1.3)),
            "center": [5.8, 0.22, -5.2],
            "materialId": "material.go.bowl",
            "faceColors": [bowl_rgb for _ in range(6)],
            "authoredProperties": {
                "isBowl": pv("bool", True),
                "bowlColor": pv("string", "black"),
                "displayName": pv("string", "Black Stone Bowl (Goke)"),
            },
        },
        {
            "objectID": "object.go.bowl.white",
            "shapeKind": 3,  # Cylinder bowl
            "geometryType": 3,
            "shapeParams": [0.65, 0.65, 0.65, 0.22, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(-5.8, 0.22, 5.2, (1.3, 0.44, 1.3)),
            "center": [-5.8, 0.22, 5.2],
            "materialId": "material.go.bowl",
            "faceColors": [bowl_rgb for _ in range(6)],
            "authoredProperties": {
                "isBowl": pv("bool", True),
                "bowlColor": pv("string", "white"),
                "displayName": pv("string", "White Stone Bowl (Goke)"),
            },
        },
    ]

    # Starter supply stones displayed neatly in the bowls
    black_rgb = [c / 255.0 for c in CHARCOAL]
    white_rgb = [c / 255.0 for c in IVORY]
    stones = []
    stone_ids = []
    for i in range(5):
        s_id = f"stone-black-supply-{i}"
        stone_ids.append(s_id)
        bx = 5.8 + 0.15 * math.cos(i * 1.25)
        bz = -5.2 + 0.15 * math.sin(i * 1.25)
        by = 0.46 + i * 0.04
        stones.append({
            "objectID": s_id,
            "shapeKind": 5,  # Ellipsoid (lenticular Go stone)
            "geometryType": 5,
            "shapeParams": [0.20, 0.07, 0.20, 0.07, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(bx, by, bz, (0.42, 0.14, 0.42)),
            "center": [bx, by, bz],
            "materialId": "material.go.black",
            "faceColors": [black_rgb for _ in range(6)],
            "authoredProperties": {
                "isStone": pv("bool", True),
                "stoneColor": pv("string", "black"),
                "displayName": pv("string", f"Black Stone {i+1}"),
            },
        })

    for i in range(5):
        s_id = f"stone-white-supply-{i}"
        stone_ids.append(s_id)
        bx = -5.8 + 0.15 * math.cos(i * 1.25)
        bz = 5.2 + 0.15 * math.sin(i * 1.25)
        by = 0.46 + i * 0.04
        stones.append({
            "objectID": s_id,
            "shapeKind": 5,  # Ellipsoid (lenticular Go stone)
            "geometryType": 5,
            "shapeParams": [0.20, 0.07, 0.20, 0.07, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(bx, by, bz, (0.42, 0.14, 0.42)),
            "center": [bx, by, bz],
            "materialId": "material.go.white",
            "faceColors": [white_rgb for _ in range(6)],
            "authoredProperties": {
                "isStone": pv("bool", True),
                "stoneColor": pv("string", "white"),
                "displayName": pv("string", f"White Stone {i+1}"),
            },
        })

    # Seats
    seats = [
        {
            "objectID": "object.go.seat.black",
            "shapeKind": 0,
            "geometryType": 0,
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(0.0, 0.15, -6.2, (0.8, 0.3, 0.8)),
            "center": [0.0, 0.15, -6.2],
            "materialId": "material.go.black",
            "faceColors": [black_rgb for _ in range(6)],
            "authoredProperties": {
                "playsColor": pv("string", "black"),
                "color": pv("int", 0),
                "isToMove": pv("bool", True),
                "designatedBy": pv("string", "Player"),
                "displayName": pv("string", "Black Player Seat"),
            },
        },
        {
            "objectID": "object.go.seat.white",
            "shapeKind": 0,
            "geometryType": 0,
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": mat4_translate(0.0, 0.15, 6.2, (0.8, 0.3, 0.8)),
            "center": [0.0, 0.15, 6.2],
            "materialId": "material.go.white",
            "faceColors": [white_rgb for _ in range(6)],
            "authoredProperties": {
                "playsColor": pv("string", "white"),
                "color": pv("int", 1),
                "isToMove": pv("bool", False),
                "designatedBy": pv("string", "Player"),
                "displayName": pv("string", "White Player Seat"),
            },
        },
    ]

    # Extra-spatial state beings
    state_beings = [
        extra_spatial("go_state", {
            "current_turn": pv("string", "black"),
            "turn": pv("string", "black"),
            "turnColor": pv("int", 0),
            "moveCount": pv("int", 0),
            "targetX": pv("int", 9),
            "targetY": pv("int", 9),
            "phase": pv("string", "playing"),
        }, "Go Game State"),
        extra_spatial("state.go", {
            "current_turn": pv("string", "black"),
            "turn": pv("string", "black"),
            "turnColor": pv("int", 0),
            "moveCount": pv("int", 0),
            "targetX": pv("int", 9),
            "targetY": pv("int", 9),
            "phase": pv("string", "playing"),
        }, "Go State"),
    ]

    zone_objects = [board] + intersections + bowls + stones + seats + state_beings

    # Relations
    relations = [
        instance_rel("object.go.board", "category.go.board"),
        subcategory_rel("category.go.board", "category.go"),
        subcategory_rel("category.go.intersection", "category.go"),
        subcategory_rel("category.go.stone", "category.go"),
        subcategory_rel("category.go.player", "category.go"),
        subcategory_rel("category.go.bowl", "category.go"),
        instance_rel("object.go.seat.black", "category.go.player"),
        instance_rel("object.go.seat.white", "category.go.player"),
        instance_rel("object.go.bowl.black", "category.go.bowl"),
        instance_rel("object.go.bowl.white", "category.go.bowl"),
    ]
    for ix_id in intersection_ids:
        relations.append(instance_rel(ix_id, "category.go.intersection"))
    for s_id in stone_ids:
        relations.append(instance_rel(s_id, "category.go.stone"))

    zone = {
        "name": ZONE_NAME,
        "identifier": ZONE_ID,
        "owner": "Player",
        "parentZone": "",
        "scope": "Local",
        "qualities": {"kind": "go"},
        "world": {"objects": zone_objects},
        "formationRelations": relations,
    }

    session = {
        "saveFormat": "zone-identity-v1",
        "currentZone": 0,
        "currentZoneId": ZONE_ID,
        "flying": True,
        "cameraPos": [0.0, 11.0, -11.0],
        "cameraFront": [0.0, -0.65, 0.75],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": 90.0,
        "pitch": -42.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": materials,
        "categories": categories,
        "zoneRefs": [{"identifier": ZONE_ID, "kind": "go"}],
        "zones": [zone],
        "authoredLaws": {
            "firstMoverEnabled": {
                "physics-gravity": False,
                "physics-kinematics": False,
                "physics-acoustics": False,
                "interaction-channel": True,
                "locomotion-channel": True,
                "creation-channel": False,
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

    world_json_path = root / "saves" / "worlds" / "go_app.json"
    world_ecform_path = root / "saves" / "worlds" / "go_app.ecform"
    zone_game_path = root / "saves" / "zones" / "Go Game" / "zone.json"
    zone_go_path = root / "saves" / "zones" / "Go" / "zone.json"

    world_json_path.parent.mkdir(parents=True, exist_ok=True)
    zone_game_path.parent.mkdir(parents=True, exist_ok=True)
    zone_go_path.parent.mkdir(parents=True, exist_ok=True)

    json_text = json.dumps(session, indent=2) + "\n"
    zone_text = json.dumps(zone, indent=2) + "\n"

    # For zone "Go", clone zone with identifier "Go"
    zone_go = dict(zone)
    zone_go["name"] = "Go"
    zone_go["identifier"] = "Go"
    zone_go_text = json.dumps(zone_go, indent=2) + "\n"

    world_json_path.write_text(json_text)
    world_ecform_path.write_text(json_text)
    zone_game_path.write_text(zone_text)
    zone_go_path.write_text(zone_go_text)

    # Also update scratch/author_go.py so it remains in sync
    scratch_script = root / "scratch" / "author_go.py"
    if scratch_script.parent.exists():
        scratch_script.write_text(Path(__file__).read_text())

    print(f"Authored {world_json_path}")
    print(f"Authored {world_ecform_path}")
    print(f"Authored {zone_game_path}")
    print(f"Authored {zone_go_path}")
    print(f"  zone objects: {len(zone['world']['objects'])}")
    print(f"  intersections: {len([o for o in zone['world']['objects'] if o['objectID'].startswith('intersection_')])}")
    print(f"  laws: {len(LAWS)}")
    print(f"  author: {AUTHOR}")

if __name__ == "__main__":
    main()
