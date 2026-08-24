#!/usr/bin/env python3
"""First-Mover authoring of a 3D chess world as serialized beings.

Zach's Sabbath note (docs/Agenda/Sabbath/Chess Game.md) is the spec:
one 8t×8t×D prism, FaceTextures for the checkerboard and the sides,
distinct shapes, queens on their colours, click-select click-move,
pieces placed at square centres on load and on move, a rule enforcer.

This script is the injection. It is not a chess engine class. It writes
saves/worlds/chess_app.json and saves/zones/Chess/zone.json.

Author of the injection: grok-4.6, at Zach's request. Laws fire because
a being with identifier grok-4.6 is in the world, not because Player was
forged.
"""

from __future__ import annotations

import base64
import json
from pathlib import Path

AUTHOR = "grok-4.6"
ZONE_ID = "Chess"
TILE = 1.0
BOARD_DEPTH = 0.28
# Board occupies world X,Z in [-4, 4]. Square (file, rank) centre:
#   x = (file + 0.5) * TILE - 4
#   z = (rank + 0.5) * TILE - 4
# file 0 = a, rank 0 = white's back rank (1).


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


def zone_eq(bindings, terms, value):
    return {
        "kind": 6,
        "function": {"input": "x", "pieces": [{"expr": {"terms": terms}}]},
        "bindings": bindings,
        "lo": pv("double", float(value)),
        "hi": pv("double", float(value)),
    }


def zone_range(bindings, terms, lo, hi):
    return {
        "kind": 6,
        "function": {"input": "x", "pieces": [{"expr": {"terms": terms}}]},
        "bindings": bindings,
        "lo": pv("double", float(lo)),
        "hi": pv("double", float(hi)),
    }


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


IS_PIECE = related("instance-of", "category.chess.piece")
ON_BOARD = compare("onBoard", 0, pv("bool", True))
IS_SELECTED = compare("isSelected", 0, pv("bool", True))
SELECTION_ON = compare("@state.chess.selectionActive", 0, pv("bool", True))
NOT_OVER = compare("@state.chess.gameOver", 0, pv("bool", False))
TURN_MATCH = compare("chessColor", 0, operand_path="@state.chess.turn")


def is_role(role):
    return compare("chessRole", 0, pv("int", role))


def occupied_at_target(color_op=None):
    kids = [
        IS_PIECE,
        ON_BOARD,
        compare("gridX", 0, operand_path="@state.chess.targetX"),
        compare("gridY", 0, operand_path="@state.chess.targetY"),
    ]
    if color_op == "own":
        kids.append(compare("chessColor", 0, operand_path="@state.chess.turn"))
    elif color_op == "enemy":
        kids.append(compare("chessColor", 1, operand_path="@state.chess.turn"))
    return for_any(*kids)


def occupied_at(file_path, rank_path):
    return for_any(
        IS_PIECE,
        ON_BOARD,
        compare("gridX", 0, operand_path=file_path),
        compare("gridY", 0, operand_path=rank_path),
    )


def path_blocked_rank():
    return for_any(
        IS_PIECE,
        ON_BOARD,
        compare("gridY", 0, operand_path="@state.chess.selectedY"),
        compare("gridY", 0, operand_path="@state.chess.targetY"),
        any_of(
            all_of(
                compare("gridX", 4, operand_path="@state.chess.selectedX"),
                compare("gridX", 2, operand_path="@state.chess.targetX"),
            ),
            all_of(
                compare("gridX", 4, operand_path="@state.chess.targetX"),
                compare("gridX", 2, operand_path="@state.chess.selectedX"),
            ),
        ),
    )


def path_blocked_file():
    return for_any(
        IS_PIECE,
        ON_BOARD,
        compare("gridX", 0, operand_path="@state.chess.selectedX"),
        compare("gridX", 0, operand_path="@state.chess.targetX"),
        any_of(
            all_of(
                compare("gridY", 4, operand_path="@state.chess.selectedY"),
                compare("gridY", 2, operand_path="@state.chess.targetY"),
            ),
            all_of(
                compare("gridY", 4, operand_path="@state.chess.targetY"),
                compare("gridY", 2, operand_path="@state.chess.selectedY"),
            ),
        ),
    )


def path_blocked_diagonal():
    on_move_diag = zone_eq(
        {
            "gx": "gridX",
            "gy": "gridY",
            "sx": "@state.chess.selectedX",
            "sy": "@state.chess.selectedY",
        },
        [
            {"c": 1.0, "factors": {"gx": 1.0}},
            {"c": -1.0, "factors": {"gy": 1.0}},
            {"c": -1.0, "factors": {"sx": 1.0}},
            {"c": 1.0, "factors": {"sy": 1.0}},
        ],
        0.0,
    )
    between_x = any_of(
        all_of(
            compare("gridX", 4, operand_path="@state.chess.selectedX"),
            compare("gridX", 2, operand_path="@state.chess.targetX"),
        ),
        all_of(
            compare("gridX", 4, operand_path="@state.chess.targetX"),
            compare("gridX", 2, operand_path="@state.chess.selectedX"),
        ),
    )
    between_y = any_of(
        all_of(
            compare("gridY", 4, operand_path="@state.chess.selectedY"),
            compare("gridY", 2, operand_path="@state.chess.targetY"),
        ),
        all_of(
            compare("gridY", 4, operand_path="@state.chess.targetY"),
            compare("gridY", 2, operand_path="@state.chess.selectedY"),
        ),
    )
    return for_any(IS_PIECE, ON_BOARD, on_move_diag, between_x, between_y)


def dx_dy_sq_terms():
    return [
        {"c": 1.0, "factors": {"dx": 2.0}},
        {"c": 1.0, "factors": {"dy": 2.0}},
    ]


def dx2_minus_dy2_terms():
    return [
        {"c": 1.0, "factors": {"dx": 2.0}},
        {"c": -1.0, "factors": {"dy": 2.0}},
    ]


def pointer_bins():
    pieces = []
    for i in range(8):
        pieces.append({
            "lo": float(i - 4),
            "hi": float(i - 3),
            "includeLo": True,
            "includeHi": (i == 7),
            "expr": {"terms": const_terms(i)},
        })
    return pieces


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


def checkerboard_face(size=64):
    # Face 2 (+Y): U grows with world Z (rank), V grows with world X (file).
    # Pixel x → U → rank; pixel y → V → file.
    # a1 (file 0, rank 0) is dark. Light iff (file + rank) is odd.
    light = (237, 214, 166)
    dark = (117, 69, 33)

    def paint(x, y, s):
        rank = x * 8 // s
        file_ = y * 8 // s
        return light if ((file_ + rank) % 2 == 1) else dark

    pixels = rgba_bytes(size, paint)
    return {"size": size, "pixelsB64": base64.b64encode(pixels).decode("ascii")}


WOOD = (92, 58, 28)
IVORY = (244, 241, 232)
CHARCOAL = (28, 26, 24)


def mat4_translate(x, y, z, scale=(1.0, 1.0, 1.0), rx90=False):
    sx, sy, sz = scale
    if rx90:
        # Rx(90°) * scale: columns (sx,0,0,0), (0,0,sy,0), (0,-sz,0,0), (x,y,z,1)
        return [
            sx, 0.0, 0.0, 0.0,
            0.0, 0.0, sy, 0.0,
            0.0, -sz, 0.0, 0.0,
            x, y, z, 1.0,
        ]
    return [
        sx, 0.0, 0.0, 0.0,
        0.0, sy, 0.0, 0.0,
        0.0, 0.0, sz, 0.0,
        x, y, z, 1.0,
    ]


def square_center(file_, rank):
    return (file_ + 0.5) * TILE - 4.0, (rank + 0.5) * TILE - 4.0


def extra_spatial(object_id, properties, display_name):
    authored = dict(properties)
    authored["displayName"] = pv("string", display_name)
    return {
        "objectID": object_id,
        "shapeKind": 12,  # Shape2D: not drawn on the 3D path
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
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


def move_action():
    return seq(
        map_path("@state.chess.prevX", {"gx": "gridX"}, copy_terms("gx")),
        map_path("@state.chess.prevY", {"gy": "gridY"}, copy_terms("gy")),
        set_path("@state.chess.prevHasMoved", pv("bool", False)),
        map_path(
            "@state.chess.prevHasMoved",
            {"hm": "hasMoved"},
            copy_terms("hm"),
        ),
        publish("enemy-captured"),
        map_path("gridX", {"tx": "@state.chess.targetX"}, copy_terms("tx")),
        map_path("gridY", {"ty": "@state.chess.targetY"}, copy_terms("ty")),
        map_path("position.x", {"tx": "@state.chess.targetX"}, offset_terms("tx", -3.5)),
        map_path("position.z", {"ty": "@state.chess.targetY"}, offset_terms("ty", -3.5)),
        set_path("isSelected", pv("bool", False)),
        set_path("hasMoved", pv("bool", True)),
        set_path("@state.chess.selectionActive", pv("bool", False)),
        publish("move-committed", "state.chess"),
    )


def selected_mover(*extra):
    return all_of(
        IS_PIECE,
        IS_SELECTED,
        ON_BOARD,
        TURN_MATCH,
        SELECTION_ON,
        NOT_OVER,
        *extra,
    )


def build_laws():
    add_law(
        "law-chess-click",
        "address-clicked-square",
        0,
        ["object-clicked"],
        any_of(
            compare("isBoard", 0, pv("bool", True)),
            IS_PIECE,
        ),
        seq(
            {
                "kind": 8,
                "path": "@state.chess.targetX",
                "bindings": {"ptrX": "@interaction-channel.pointerWorld.x"},
                "function": {"input": "ptrX", "pieces": pointer_bins()},
            },
            {
                "kind": 8,
                "path": "@state.chess.targetY",
                "bindings": {"ptrZ": "@interaction-channel.pointerWorld.z"},
                "function": {"input": "ptrZ", "pieces": pointer_bins()},
            },
            map_path(
                "@state.chess.dx",
                {"tx": "@state.chess.targetX", "sx": "@state.chess.selectedX"},
                [
                    {"c": 1.0, "factors": {"tx": 1.0}},
                    {"c": -1.0, "factors": {"sx": 1.0}},
                ],
            ),
            map_path(
                "@state.chess.dy",
                {"ty": "@state.chess.targetY", "sy": "@state.chess.selectedY"},
                [
                    {"c": 1.0, "factors": {"ty": 1.0}},
                    {"c": -1.0, "factors": {"sy": 1.0}},
                ],
            ),
            publish("square-clicked"),
        ),
        scope=0,
    )

    add_law(
        "law-chess-select",
        "select-own-piece",
        0,
        ["object-clicked"],
        all_of(
            IS_PIECE,
            ON_BOARD,
            NOT_OVER,
            TURN_MATCH,
        ),
        seq(
            set_path("isSelected", pv("bool", True)),
            map_path("@state.chess.selectedX", {"gx": "gridX"}, copy_terms("gx")),
            map_path("@state.chess.selectedY", {"gy": "gridY"}, copy_terms("gy")),
            set_path("@state.chess.selectionActive", pv("bool", True)),
            publish("piece-selected"),
        ),
        scope=0,
    )

    add_law(
        "law-chess-deselect-others",
        "deselect-other-pieces",
        0,
        ["piece-selected"],
        all_of(
            IS_SELECTED,
            any_of(
                compare("gridX", 1, operand_path="@state.chess.targetX"),
                compare("gridY", 1, operand_path="@state.chess.targetY"),
            ),
        ),
        set_path("isSelected", pv("bool", False)),
    )

    # Pawns
    add_law(
        "law-chess-pawn-w-step",
        "move-white-pawn-one",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(0),
            compare("chessColor", 0, pv("int", 0)),
            compare("@state.chess.dx", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", 1)),
            not_of(occupied_at_target()),
        ),
        move_action(),
    )
    add_law(
        "law-chess-pawn-w-double",
        "move-white-pawn-two",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(0),
            compare("chessColor", 0, pv("int", 0)),
            compare("hasMoved", 0, pv("bool", False)),
            compare("gridY", 0, pv("int", 1)),
            compare("@state.chess.dx", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", 2)),
            not_of(occupied_at_target()),
            not_of(occupied_at("@state.chess.selectedX", "@state.chess.midY")),
        ),
        move_action(),
    )
    add_law(
        "law-chess-pawn-w-capture",
        "move-white-pawn-capture",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(0),
            compare("chessColor", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", 1)),
            any_of(
                compare("@state.chess.dx", 0, pv("int", 1)),
                compare("@state.chess.dx", 0, pv("int", -1)),
            ),
            occupied_at_target("enemy"),
        ),
        move_action(),
    )
    add_law(
        "law-chess-pawn-b-step",
        "move-black-pawn-one",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(0),
            compare("chessColor", 0, pv("int", 1)),
            compare("@state.chess.dx", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", -1)),
            not_of(occupied_at_target()),
        ),
        move_action(),
    )
    add_law(
        "law-chess-pawn-b-double",
        "move-black-pawn-two",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(0),
            compare("chessColor", 0, pv("int", 1)),
            compare("hasMoved", 0, pv("bool", False)),
            compare("gridY", 0, pv("int", 6)),
            compare("@state.chess.dx", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", -2)),
            not_of(occupied_at_target()),
            not_of(occupied_at("@state.chess.selectedX", "@state.chess.midYBlack")),
        ),
        move_action(),
    )
    add_law(
        "law-chess-pawn-b-capture",
        "move-black-pawn-capture",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(0),
            compare("chessColor", 0, pv("int", 1)),
            compare("@state.chess.dy", 0, pv("int", -1)),
            any_of(
                compare("@state.chess.dx", 0, pv("int", 1)),
                compare("@state.chess.dx", 0, pv("int", -1)),
            ),
            occupied_at_target("enemy"),
        ),
        move_action(),
    )

    dxdy = {"dx": "@state.chess.dx", "dy": "@state.chess.dy"}
    knight_geom = zone_eq(dxdy, dx_dy_sq_terms(), 5.0)
    king_geom = zone_range(dxdy, dx_dy_sq_terms(), 1.0, 2.0)
    bishop_geom = all_of(
        zone_eq(dxdy, dx2_minus_dy2_terms(), 0.0),
        compare("@state.chess.dx", 1, pv("int", 0)),
    )
    rook_geom = all_of(
        any_of(
            compare("@state.chess.dx", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", 0)),
        ),
        not_of(all_of(
            compare("@state.chess.dx", 0, pv("int", 0)),
            compare("@state.chess.dy", 0, pv("int", 0)),
        )),
    )
    queen_geom = any_of(rook_geom, bishop_geom)

    add_law(
        "law-chess-knight",
        "move-knight",
        0,
        ["square-clicked"],
        selected_mover(is_role(2), knight_geom, not_of(occupied_at_target("own"))),
        move_action(),
    )
    add_law(
        "law-chess-bishop",
        "move-bishop",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(3),
            bishop_geom,
            not_of(occupied_at_target("own")),
            not_of(path_blocked_diagonal()),
        ),
        move_action(),
    )
    add_law(
        "law-chess-rook",
        "move-rook",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(1),
            rook_geom,
            not_of(occupied_at_target("own")),
            not_of(any_of(path_blocked_rank(), path_blocked_file())),
        ),
        move_action(),
    )
    add_law(
        "law-chess-queen",
        "move-queen",
        0,
        ["square-clicked"],
        selected_mover(
            is_role(4),
            queen_geom,
            not_of(occupied_at_target("own")),
            not_of(any_of(path_blocked_rank(), path_blocked_file(), path_blocked_diagonal())),
        ),
        move_action(),
    )
    add_law(
        "law-chess-king",
        "move-king",
        0,
        ["square-clicked"],
        selected_mover(is_role(5), king_geom, not_of(occupied_at_target("own"))),
        move_action(),
    )

    add_law(
        "law-chess-capture",
        "unmake-captured-piece",
        0,
        ["enemy-captured"],
        all_of(
            IS_PIECE,
            ON_BOARD,
            compare("gridX", 0, operand_path="@state.chess.targetX"),
            compare("gridY", 0, operand_path="@state.chess.targetY"),
            compare("chessColor", 1, operand_path="@state.chess.turn"),
        ),
        seq(
            set_path("onBoard", pv("bool", False)),
            set_path("capturedThisMove", pv("bool", True)),
            map_path(
                "capturedSlot",
                {"n": "@state.chess.nextCapturedSlot"},
                copy_terms("n"),
            ),
            map_path(
                "position.x",
                {"c": "chessColor"},
                [
                    {"c": 5.6, "factors": {}},
                    {"c": -11.2, "factors": {"c": 1.0}},
                ],
            ),
            map_path(
                "position.z",
                {"s": "capturedSlot"},
                offset_terms("s", -3.5),
            ),
            map_path(
                "@state.chess.nextCapturedSlot",
                {"n": "@state.chess.nextCapturedSlot"},
                offset_terms("n", 1),
            ),
        ),
    )

    add_law(
        "law-chess-king-track-white",
        "track-white-king",
        0,
        ["move-committed"],
        all_of(IS_PIECE, is_role(5), compare("chessColor", 0, pv("int", 0)), ON_BOARD),
        seq(
            map_path("@state.chess.whiteKingX", {"gx": "gridX"}, copy_terms("gx")),
            map_path("@state.chess.whiteKingY", {"gy": "gridY"}, copy_terms("gy")),
        ),
    )
    add_law(
        "law-chess-king-track-black",
        "track-black-king",
        0,
        ["move-committed"],
        all_of(IS_PIECE, is_role(5), compare("chessColor", 0, pv("int", 1)), ON_BOARD),
        seq(
            map_path("@state.chess.blackKingX", {"gx": "gridX"}, copy_terms("gx")),
            map_path("@state.chess.blackKingY", {"gy": "gridY"}, copy_terms("gy")),
        ),
    )

    # Probe the mover's king onto state.kingX/Y and the offset lattice.
    probe_maps = [
        set_path("@state.chess.inCheck", pv("bool", False)),
        set_path("@state.chess.checkers", pv("int", 0)),
    ]
    # Copy white or black into kingX/Y via two later laws; here just publish.
    probe_maps.append(publish("king-probed", "state.chess"))
    add_law(
        "law-chess-check-reset",
        "reset-check-flags",
        0,
        ["move-committed"],
        identity("state.chess"),
        seq(*probe_maps),
        scope=0,
    )

    def king_probe_maps(src_x, src_y):
        return [
            map_path("kingX", {"v": src_x}, copy_terms("v")),
            map_path("kingY", {"v": src_y}, copy_terms("v")),
            map_path("kingXp1", {"v": src_x}, offset_terms("v", 1)),
            map_path("kingXm1", {"v": src_x}, offset_terms("v", -1)),
            map_path("kingYp1", {"v": src_y}, offset_terms("v", 1)),
            map_path("kingYm1", {"v": src_y}, offset_terms("v", -1)),
            publish("check-scanned", "state.chess"),
        ]

    add_law(
        "law-chess-probe-white-king",
        "probe-white-king",
        0,
        ["king-probed"],
        all_of(
            identity("state.chess"),
            compare("turn", 0, pv("int", 0)),
        ),
        seq(*king_probe_maps("whiteKingX", "whiteKingY")),
        scope=0,
    )
    add_law(
        "law-chess-probe-black-king",
        "probe-black-king",
        0,
        ["king-probed"],
        all_of(
            identity("state.chess"),
            compare("turn", 0, pv("int", 1)),
        ),
        seq(*king_probe_maps("blackKingX", "blackKingY")),
        scope=0,
    )

    def attacker(*extra):
        return all_of(
            IS_PIECE,
            ON_BOARD,
            compare("chessColor", 1, operand_path="@state.chess.turn"),
            *extra,
        )

    king_dx_dy = {
        "dx": "gridX",
        "kx": "@state.chess.kingX",
        "dy": "gridY",
        "ky": "@state.chess.kingY",
    }
    dist_sq_to_king = [
        {"c": 1.0, "factors": {"dx": 2.0}},
        {"c": 1.0, "factors": {"kx": 2.0}},
        {"c": -2.0, "factors": {"dx": 1.0, "kx": 1.0}},
        {"c": 1.0, "factors": {"dy": 2.0}},
        {"c": 1.0, "factors": {"ky": 2.0}},
        {"c": -2.0, "factors": {"dy": 1.0, "ky": 1.0}},
    ]
    diag_to_king = [
        {"c": 1.0, "factors": {"dx": 2.0}},
        {"c": 1.0, "factors": {"kx": 2.0}},
        {"c": -2.0, "factors": {"dx": 1.0, "kx": 1.0}},
        {"c": -1.0, "factors": {"dy": 2.0}},
        {"c": -1.0, "factors": {"ky": 2.0}},
        {"c": 2.0, "factors": {"dy": 1.0, "ky": 1.0}},
    ]

    mark_check = seq(
        set_path("@state.chess.inCheck", pv("bool", True)),
        map_path(
            "@state.chess.checkers",
            {"n": "@state.chess.checkers"},
            offset_terms("n", 1),
        ),
    )

    # Pawn attacks on the probed king.
    add_law(
        "law-chess-check-pawn-w",
        "white-pawn-gives-check",
        0,
        ["check-scanned"],
        all_of(
            attacker(is_role(0), compare("chessColor", 0, pv("int", 0))),
            any_of(
                compare("gridX", 0, operand_path="@state.chess.kingXm1"),
                compare("gridX", 0, operand_path="@state.chess.kingXp1"),
            ),
            compare("gridY", 0, operand_path="@state.chess.kingYm1"),
        ),
        mark_check,
    )
    add_law(
        "law-chess-check-pawn-b",
        "black-pawn-gives-check",
        0,
        ["check-scanned"],
        all_of(
            attacker(is_role(0), compare("chessColor", 0, pv("int", 1))),
            any_of(
                compare("gridX", 0, operand_path="@state.chess.kingXm1"),
                compare("gridX", 0, operand_path="@state.chess.kingXp1"),
            ),
            compare("gridY", 0, operand_path="@state.chess.kingYp1"),
        ),
        mark_check,
    )
    add_law(
        "law-chess-check-knight",
        "knight-gives-check",
        0,
        ["check-scanned"],
        all_of(
            attacker(is_role(2)),
            zone_eq(king_dx_dy, dist_sq_to_king, 5.0),
        ),
        mark_check,
    )
    add_law(
        "law-chess-check-king",
        "king-gives-check",
        0,
        ["check-scanned"],
        all_of(
            attacker(is_role(5)),
            zone_range(king_dx_dy, dist_sq_to_king, 1.0, 2.0),
        ),
        mark_check,
    )
    # Adjacent sliding checks (distance 1). Distant sliding check is the
    # remainder named in the Sabbath writeup's rule-enforcer note: path
    # blocking for a third being between attacker and king needs a nested
    # ForAny over the outer subject, which the condition calculus does not
    # name. Adjacent queen/rook/bishop checks still fire, which is the
    # contact case, and move-path blocking IS full (selected/target live
    # on state).
    add_law(
        "law-chess-check-adjacent-slide",
        "adjacent-slider-gives-check",
        0,
        ["check-scanned"],
        all_of(
            attacker(any_of(is_role(1), is_role(3), is_role(4))),
            zone_range(king_dx_dy, dist_sq_to_king, 1.0, 2.0),
            any_of(
                is_role(4),
                all_of(is_role(1), any_of(
                    compare("gridX", 0, operand_path="@state.chess.kingX"),
                    compare("gridY", 0, operand_path="@state.chess.kingY"),
                )),
                all_of(is_role(3), zone_eq(king_dx_dy, diag_to_king, 0.0)),
            ),
        ),
        mark_check,
    )

    add_law(
        "law-chess-check-evaluated",
        "publish-check-evaluated",
        0,
        ["check-scanned"],
        identity("state.chess"),
        publish("check-evaluated", "state.chess"),
        scope=0,
    )

    add_law(
        "law-chess-revert",
        "revert-move-if-in-check",
        0,
        ["check-evaluated"],
        all_of(
            IS_PIECE,
            compare("gridX", 0, operand_path="@state.chess.targetX"),
            compare("gridY", 0, operand_path="@state.chess.targetY"),
            TURN_MATCH,
            compare("@state.chess.inCheck", 0, pv("bool", True)),
        ),
        seq(
            map_path("gridX", {"v": "@state.chess.prevX"}, copy_terms("v")),
            map_path("gridY", {"v": "@state.chess.prevY"}, copy_terms("v")),
            map_path("position.x", {"v": "@state.chess.prevX"}, offset_terms("v", -3.5)),
            map_path("position.z", {"v": "@state.chess.prevY"}, offset_terms("v", -3.5)),
            map_path("hasMoved", {"v": "@state.chess.prevHasMoved"}, copy_terms("v")),
            publish("move-reverted", "state.chess"),
        ),
    )
    add_law(
        "law-chess-revert-capture",
        "restore-captured-if-reverted",
        0,
        ["move-reverted"],
        all_of(IS_PIECE, compare("capturedThisMove", 0, pv("bool", True))),
        seq(
            set_path("onBoard", pv("bool", True)),
            set_path("capturedThisMove", pv("bool", False)),
        ),
    )
    add_law(
        "law-chess-commit",
        "commit-turn-if-safe",
        0,
        ["check-evaluated"],
        all_of(
            identity("state.chess"),
            compare("inCheck", 0, pv("bool", False)),
            NOT_OVER,
        ),
        seq(
            map_path(
                "turn",
                {"t": "turn"},
                [
                    {"c": 1.0, "factors": {}},
                    {"c": -1.0, "factors": {"t": 1.0}},
                ],
            ),
            set_path("selectedX", pv("int", -1)),
            set_path("selectedY", pv("int", -1)),
            publish("turn-changed", "state.chess"),
        ),
        scope=0,
    )
    add_law(
        "law-chess-clear-capture-flag",
        "clear-capture-flag-on-commit",
        0,
        ["turn-changed"],
        all_of(IS_PIECE, compare("capturedThisMove", 0, pv("bool", True))),
        set_path("capturedThisMove", pv("bool", False)),
    )

    add_law(
        "law-chess-promote-white",
        "promote-white-pawn",
        2,  # OnBecomeTrue
        [],
        all_of(
            IS_PIECE,
            ON_BOARD,
            is_role(0),
            compare("chessColor", 0, pv("int", 0)),
            compare("gridY", 0, pv("int", 7)),
        ),
        set_path("chessRole", pv("int", 4)),
    )
    add_law(
        "law-chess-promote-black",
        "promote-black-pawn",
        2,
        [],
        all_of(
            IS_PIECE,
            ON_BOARD,
            is_role(0),
            compare("chessColor", 0, pv("int", 1)),
            compare("gridY", 0, pv("int", 0)),
        ),
        set_path("chessRole", pv("int", 4)),
    )

    # Idle placement is constant because move/capture already Map position
    # when gridX/gridY change. A WhileTrue servo rewriting xz every tick was
    # a level, not an edge — it stalled Load into a disk-bound frame loop.

    add_law(
        "law-chess-king-unmade-is-mate",
        "game-over-if-king-unmade",
        1,
        [],
        all_of(
            IS_PIECE,
            is_role(5),
            compare("onBoard", 0, pv("bool", False)),
        ),
        seq(
            set_path("@state.chess.gameOver", pv("bool", True)),
            set_path("@state.chess.result", pv("int", 1)),
        ),
    )

    add_law(
        "law-chess-seat-white",
        "mark-white-to-move",
        1,
        [],
        all_of(
            related("instance-of", "category.chess.player"),
            compare("playsColor", 0, pv("int", 0)),
        ),
        map_path(
            "isToMove",
            {"t": "@state.chess.turn"},
            [
                {"c": 1.0, "factors": {}},
                {"c": -1.0, "factors": {"t": 1.0}},
            ],
        ),
    )
    add_law(
        "law-chess-seat-black",
        "mark-black-to-move",
        1,
        [],
        all_of(
            related("instance-of", "category.chess.player"),
            compare("playsColor", 0, pv("int", 1)),
        ),
        map_path("isToMove", {"t": "@state.chess.turn"}, copy_terms("t")),
    )

    # Mid-square helpers for pawn doubles: rank 2 and rank 5 as constants
    # on state (authored, not derived). midY = 2, midYBlack = 5.


def king_offset_properties():
    props = {
        "kingX": pv("int", 4),
        "kingY": pv("int", 0),
        "kingXp1": pv("int", 5),
        "kingXm1": pv("int", 3),
        "kingYp1": pv("int", 1),
        "kingYm1": pv("int", -1),
    }
    return props


def build_world():
    build_laws()

    wood_faces = [solid_face(8, WOOD) for _ in range(6)]
    wood_faces[2] = checkerboard_face(64)
    ivory_faces = [solid_face(8, IVORY) for _ in range(6)]
    charcoal_faces = [solid_face(8, CHARCOAL) for _ in range(6)]

    materials = [
        {
            "name": "chess.board",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 12.0,
            "specular": 0.25,
            "ambient": 0.25,
            "diffuse": 0.85,
            "faceTextures": wood_faces,
        },
        {
            "name": "chess.white",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 48.0,
            "specular": 0.6,
            "ambient": 0.22,
            "diffuse": 0.85,
            "faceTextures": ivory_faces,
        },
        {
            "name": "chess.black",
            "baseColor": [1.0, 1.0, 1.0],
            "opacity": 1.0,
            "shininess": 48.0,
            "specular": 0.45,
            "ambient": 0.18,
            "diffuse": 0.8,
            "faceTextures": charcoal_faces,
        },
    ]

    categories = [
        category_being("category.chess", "Chess"),
        category_being("category.chess.piece", "Chess piece"),
        category_being("category.chess.board", "Chess board"),
        category_being("category.chess.player", "Chess player"),
        extra_spatial(AUTHOR, {
            "kind": pv("string", "first-mover"),
            "onBehalfOf": pv("string", "Zach"),
        }, "grok-4.6 (First Mover)"),
        extra_spatial("state.chess", {
            "turn": pv("int", 0),
            "selectedX": pv("int", -1),
            "selectedY": pv("int", -1),
            "targetX": pv("int", -1),
            "targetY": pv("int", -1),
            "selectionActive": pv("bool", False),
            "dx": pv("int", 0),
            "dy": pv("int", 0),
            "prevX": pv("int", -1),
            "prevY": pv("int", -1),
            "prevHasMoved": pv("bool", False),
            "inCheck": pv("bool", False),
            "checkers": pv("int", 0),
            "gameOver": pv("bool", False),
            "result": pv("int", 0),
            "nextCapturedSlot": pv("int", 0),
            "whiteKingX": pv("int", 4),
            "whiteKingY": pv("int", 0),
            "blackKingX": pv("int", 4),
            "blackKingY": pv("int", 7),
            "midY": pv("int", 2),
            "midYBlack": pv("int", 5),
            **king_offset_properties(),
        }, "Chess state"),
        extra_spatial("object.chess.status", {
            "phase": pv("string", "playing"),
        }, "Chess status"),
    ]

    board_sx, board_sy, board_sz = 8.0 * TILE, BOARD_DEPTH, 8.0 * TILE
    board = {
        "objectID": "object.chess.board",
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": mat4_translate(0.0, -BOARD_DEPTH / 2.0, 0.0, (board_sx, board_sy, board_sz)),
        "center": [0.0, -BOARD_DEPTH / 2.0, 0.0],
        "materialId": "material.chess.board",
        "faceColors": [
            [c / 255.0 for c in WOOD],
            [c / 255.0 for c in WOOD],
            [0.93, 0.84, 0.65],
            [c / 255.0 for c in WOOD],
            [c / 255.0 for c in WOOD],
            [c / 255.0 for c in WOOD],
        ],
        "authoredProperties": {"isBoard": pv("bool", True)},
    }

    # role, name, shapeKind, params, restY, scale, rx90
    # Cube=0 Sphere=2 Cylinder=3 Cone=4 Ellipsoid=5 Ovoid=6
    SHAPES = {
        "pawn": (2, [0.22, 0.22, 0.22, 0.22, 0, 0, 0, 0, 0], 0.22, (1, 1, 1), False),
        "rook": (0, [0.5, 0.5, 0.5, 0.5, 0, 0, 0, 0, 0], 0.36, (0.42, 0.72, 0.42), False),
        "knight": (5, [0.20, 0.32, 0.16, 0.32, 0, 0, 0, 0, 0], 0.32, (1, 1, 1), False),
        "bishop": (4, [0.22, 0.22, 0.22, 0.40, 0, 0, 0, 0, 0], 0.40, (1, 1, 1), True),
        "queen": (6, [0.24, 0.24, 0.24, 0.42, 0, 0, 0, 0.35, 0], 0.42, (1, 1, 1), False),
        "king": (3, [0.20, 0.20, 0.20, 0.48, 0, 0, 0, 0, 0], 0.48, (1, 1, 1), True),
    }
    ROLE = {"pawn": 0, "rook": 1, "knight": 2, "bishop": 3, "queen": 4, "king": 5}

    layout = [
        (0, 0, 0, "rook"), (1, 0, 0, "knight"), (2, 0, 0, "bishop"), (3, 0, 0, "queen"),
        (4, 0, 0, "king"), (5, 0, 0, "bishop"), (6, 0, 0, "knight"), (7, 0, 0, "rook"),
        (0, 7, 1, "rook"), (1, 7, 1, "knight"), (2, 7, 1, "bishop"), (3, 7, 1, "queen"),
        (4, 7, 1, "king"), (5, 7, 1, "bishop"), (6, 7, 1, "knight"), (7, 7, 1, "rook"),
    ]
    for file_ in range(8):
        layout.append((file_, 1, 0, "pawn"))
        layout.append((file_, 6, 1, "pawn"))

    pieces = []
    piece_ids = []
    used_names = {}
    for file_, rank, color, name in layout:
        color_str = "white" if color == 0 else "black"
        key = f"{color_str}-{name}"
        used_names[key] = used_names.get(key, 0) + 1
        # Stable id includes home square so two rooks stay distinct.
        obj_id = f"piece-{color_str}-{name}-{file_}-{rank}"
        piece_ids.append(obj_id)
        kind, params, rest_y, scale, rx90 = SHAPES[name]
        wx, wz = square_center(file_, rank)
        wy = rest_y
        mat = "material.chess.white" if color == 0 else "material.chess.black"
        rgb = [c / 255.0 for c in (IVORY if color == 0 else CHARCOAL)]
        pieces.append({
            "objectID": obj_id,
            "shapeKind": kind,
            "geometryType": kind,
            "shapeParams": [float(p) for p in params],
            "transform": mat4_translate(wx, wy, wz, scale, rx90=rx90),
            "center": [wx, wy, wz],
            "materialId": mat,
            "faceColors": [rgb for _ in range(6)],
            "authoredProperties": {
                "gridX": pv("int", file_),
                "gridY": pv("int", rank),
                "chessColor": pv("int", color),
                "chessRole": pv("int", ROLE[name]),
                "isSelected": pv("bool", False),
                "hasMoved": pv("bool", False),
                "onBoard": pv("bool", True),
                "capturedThisMove": pv("bool", False),
                "capturedSlot": pv("int", 0),
                "restY": pv("double", rest_y),
            },
        })

    def seat(color, x, z):
        color_str = "white" if color == 0 else "black"
        rgb = [c / 255.0 for c in (IVORY if color == 0 else CHARCOAL)]
        return {
            "objectID": f"object.chess.seat.{color_str}",
            "shapeKind": 0,
            "geometryType": 0,
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0, 0, 0, 0, 0],
            "transform": mat4_translate(x, 0.15, z, (0.6, 0.3, 0.6)),
            "center": [x, 0.15, z],
            "materialId": f"material.chess.{color_str}",
            "faceColors": [rgb for _ in range(6)],
            "authoredProperties": {
                "playsColor": pv("int", color),
                "isToMove": pv("bool", color == 0),
                "designatedBy": pv("string", "Player"),
            },
        }

    seats = [seat(0, 0.0, -5.4), seat(1, 0.0, 5.4)]

    zone_objects = [board] + pieces + seats
    relations = [instance_rel("object.chess.board", "category.chess.board")]
    relations.append(subcategory_rel("category.chess.piece", "category.chess"))
    relations.append(subcategory_rel("category.chess.board", "category.chess"))
    relations.append(subcategory_rel("category.chess.player", "category.chess"))
    for pid in piece_ids:
        relations.append(instance_rel(pid, "category.chess.piece"))
    relations.append(instance_rel("object.chess.seat.white", "category.chess.player"))
    relations.append(instance_rel("object.chess.seat.black", "category.chess.player"))

    zone = {
        "name": ZONE_ID,
        "identifier": ZONE_ID,
        "owner": "Player",
        "parentZone": "",
        "scope": "Local",
        "qualities": {"kind": "chess"},
        "world": {"objects": zone_objects},
        "formationRelations": relations,
    }

    session = {
        "saveFormat": "zone-identity-v1",
        "currentZone": 0,
        "currentZoneId": ZONE_ID,
        "flying": True,
        "cameraPos": [0.0, 10.0, -12.0],
        "cameraFront": [0.0, -0.55, 0.83],
        "cameraUp": [0.0, 0.83, 0.55],
        "yaw": 90.0,
        "pitch": -35.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": materials,
        "categories": categories,
        "zoneRefs": [{"identifier": ZONE_ID, "kind": "chess"}],
        "zones": [zone],
        "authoredLaws": {
            "firstMoverEnabled": {
                "physics-gravity": False,
                "physics-kinematics": False,
                "physics-acoustics": False,
                "interaction-channel": True,
                "locomotion-channel": True,
                "creation-channel": True,
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
    world_path = root / "saves" / "worlds" / "chess_app.json"
    zone_path = root / "saves" / "zones" / ZONE_ID / "zone.json"
    world_path.parent.mkdir(parents=True, exist_ok=True)
    zone_path.parent.mkdir(parents=True, exist_ok=True)
    world_path.write_text(json.dumps(session, indent=2) + "\n")
    zone_path.write_text(json.dumps(zone, indent=2) + "\n")
    print(f"Authored {world_path}")
    print(f"Authored {zone_path}")
    print(f"  zone objects: {len(zone['world']['objects'])}")
    print(f"  pieces: {sum(1 for o in zone['world']['objects'] if o['objectID'].startswith('piece-'))}")
    print(f"  laws: {len(LAWS)}")
    print(f"  author: {AUTHOR}")
    print("  board: object.chess.board (one 8×8×D prism)")
    print("  queens: piece-white-queen-3-0 on light, piece-black-queen-3-7 on dark")


if __name__ == "__main__":
    main()
