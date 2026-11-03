#!/usr/bin/env python3
"""First-Mover authoring script for 2D Chess in Earthcall.

Authors the 'Chess2D' Zone with true screen-space 2D representation (ShapeKind::Shape2D),
authored materials, full 64-square planar board, 32 distinct 2D pieces with typography glyphs,
side capture trays, and discrete authored Laws governing selection, movement, capture, and turn state.

Authors: Zachary Zhang & Gemini (First Mover)
"""

import json
import os
import shutil

X0 = 140.0
Y0 = 100.0
SQ_SIZE = 60.0
PIECE_SIZE = 48.0
PIECE_PAD = (SQ_SIZE - PIECE_SIZE) / 2.0  # 6.0

def square_pos(gx, gy):
    """Screen-space top-left for board square (gx, gy). Rank 0 at bottom, rank 7 at top."""
    x = X0 + gx * SQ_SIZE
    y = Y0 + (7 - gy) * SQ_SIZE
    return x, y

def piece_pos(gx, gy):
    """Screen-space top-left for piece on (gx, gy)."""
    sx, sy = square_pos(gx, gy)
    return sx + PIECE_PAD, sy + PIECE_PAD

def make_linear_expr(var_name, coeff, const):
    """OntoMath linear polynomial: coeff * var + const"""
    terms = []
    if coeff != 0.0:
        terms.append({"c": float(coeff), "factors": {var_name: 1.0}})
    if const != 0.0 or not terms:
        terms.append({"c": float(const), "factors": {}})
    return {
        "input": "x",
        "pieces": [{
            "mathNode": {
                "op": 0,
                "scalarForm": {"terms": terms}
            }
        }]
    }

def build_chess2d():
    # -----------------------------------------------------------------------
    # Materials
    # -----------------------------------------------------------------------
    materials = [
        {
            "id": "material.chess2d.frame",
            "name": "Mahogany Frame",
            "baseColor": [0.22, 0.15, 0.10],
            "opacity": 1.0,
            "roughness": 0.5,
            "metallic": 0.0
        },
        {
            "id": "material.chess2d.light_square",
            "name": "Parchment Light Square",
            "baseColor": [0.91, 0.85, 0.75],
            "opacity": 1.0,
            "roughness": 0.6,
            "metallic": 0.0
        },
        {
            "id": "material.chess2d.dark_square",
            "name": "Walnut Dark Square",
            "baseColor": [0.52, 0.38, 0.26],
            "opacity": 1.0,
            "roughness": 0.6,
            "metallic": 0.0
        },
        {
            "id": "material.chess2d.piece_white",
            "name": "Ivory White Piece",
            "baseColor": [0.96, 0.94, 0.90],
            "opacity": 1.0,
            "roughness": 0.3,
            "metallic": 0.0
        },
        {
            "id": "material.chess2d.piece_black",
            "name": "Obsidian Black Piece",
            "baseColor": [0.20, 0.20, 0.24],
            "opacity": 1.0,
            "roughness": 0.3,
            "metallic": 0.0
        },
        {
            "id": "material.chess2d.tray",
            "name": "Captured Piece Tray",
            "baseColor": [0.18, 0.14, 0.12],
            "opacity": 0.95,
            "roughness": 0.7,
            "metallic": 0.0
        }
    ]

    objects = []
    relations = []

    # -----------------------------------------------------------------------
    # 0. Authors (Person and First Mover)
    # -----------------------------------------------------------------------
    objects.append({
        "objectID": "Zach",
        "shapeKind": 0,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Zachary Zhang"}
        },
        "authors": ["Zach"]
    })
    objects.append({
        "objectID": "Gemini",
        "shapeKind": 0,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Gemini 3.8 Flash (First Mover)"},
            "onBehalfOf": {"t": "string", "v": "Zach"}
        },
        "authors": ["Zach", "Gemini"]
    })

    # -----------------------------------------------------------------------
    # 1. Extra-spatial Game State
    # -----------------------------------------------------------------------
    objects.append({
        "objectID": "state.chess2d",
        "shapeKind": 0,
        "authoredProperties": {
            "turn": {"t": "int", "v": 0},
            "selectedX": {"t": "int", "v": -1},
            "selectedY": {"t": "int", "v": -1},
            "targetX": {"t": "int", "v": -1},
            "targetY": {"t": "int", "v": -1},
            "dx": {"t": "int", "v": 0},
            "dy": {"t": "int", "v": 0},
            "selectionActive": {"t": "bool", "v": False},
            "nextWhiteCapturedSlot": {"t": "int", "v": 0},
            "nextBlackCapturedSlot": {"t": "int", "v": 0},
            "gameOver": {"t": "bool", "v": False}
        },
        "authors": ["Zach", "Gemini"]
    })

    # -----------------------------------------------------------------------
    # 2. Categories
    # -----------------------------------------------------------------------
    categories = [
        ("category.chess2d.board", "Chess 2D Board"),
        ("category.chess2d.square", "Chess 2D Square"),
        ("category.chess2d.piece", "Chess 2D Piece"),
        ("category.chess2d.tray", "Chess 2D Tray"),
        ("category.chess2d.ui", "Chess 2D UI")
    ]
    for cat_id, cat_name in categories:
        objects.append({
            "objectID": cat_id,
            "shapeKind": 0,
            "authoredProperties": {
                "displayName": {"t": "string", "v": cat_name}
            },
            "authors": ["Zach", "Gemini"]
        })

    # -----------------------------------------------------------------------
    # 3. UI Header and Turn Banner
    # -----------------------------------------------------------------------
    title_obj = {
        "objectID": "ui.chess2d.title",
        "shapeKind": 12, # Shape2D
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 480.0, 40.0],
        "x2D": 140.0,
        "y2D": 35.0,
        "zOrder2D": 2,
        "materialId": "material.chess2d.frame",
        "faceColors": [[0.26, 0.18, 0.12] for _ in range(6)],
        "authoredProperties": {
            "label2D": {"t": "string", "v": "EARTHCALL 2D CHESS"}
        },
        "authors": ["Zach", "Gemini"]
    }
    objects.append(title_obj)
    relations.append({
        "type": "instance-of",
        "entityA": "ui.chess2d.title",
        "entityB": "category.chess2d.ui",
        "directed": True,
        "weight": 1.0,
        "authors": ["Zach", "Gemini"]
    })

    turn_obj = {
        "objectID": "ui.chess2d.turn",
        "shapeKind": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 200.0, 36.0],
        "x2D": 280.0,
        "y2D": 600.0,
        "zOrder2D": 2,
        "materialId": "material.chess2d.piece_white",
        "faceColors": [[0.92, 0.90, 0.86] for _ in range(6)],
        "authoredProperties": {
            "label2D": {"t": "string", "v": "TURN: WHITE"}
        },
        "authors": ["Zach", "Gemini"]
    }
    objects.append(turn_obj)
    relations.append({
        "type": "instance-of",
        "entityA": "ui.chess2d.turn",
        "entityB": "category.chess2d.ui",
        "directed": True,
        "weight": 1.0,
        "authors": ["Zach", "Gemini"]
    })

    # -----------------------------------------------------------------------
    # 4. Board Frame & Side Trays
    # -----------------------------------------------------------------------
    board_frame = {
        "objectID": "object.chess2d.frame",
        "shapeKind": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 504.0, 504.0],
        "x2D": 128.0,
        "y2D": 88.0,
        "zOrder2D": 0,
        "materialId": "material.chess2d.frame",
        "faceColors": [[0.22, 0.15, 0.10] for _ in range(6)],
        "authoredProperties": {
            "isBoard": {"t": "bool", "v": True}
        },
        "authors": ["Zach", "Gemini"]
    }
    objects.append(board_frame)
    relations.append({
        "type": "instance-of",
        "entityA": "object.chess2d.frame",
        "entityB": "category.chess2d.board",
        "directed": True,
        "weight": 1.0,
        "authors": ["Zach", "Gemini"]
    })

    # Trays for captured pieces
    tray_white = {
        "objectID": "object.chess2d.tray.white",
        "shapeKind": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 56.0, 480.0],
        "x2D": 60.0,
        "y2D": 100.0,
        "zOrder2D": 0,
        "materialId": "material.chess2d.tray",
        "faceColors": [[0.18, 0.14, 0.12] for _ in range(6)],
        "authoredProperties": {
            "isTray": {"t": "bool", "v": True}
        },
        "authors": ["Zach", "Gemini"]
    }
    objects.append(tray_white)
    relations.append({
        "type": "instance-of",
        "entityA": "object.chess2d.tray.white",
        "entityB": "category.chess2d.tray",
        "directed": True,
        "weight": 1.0,
        "authors": ["Zach", "Gemini"]
    })

    tray_black = {
        "objectID": "object.chess2d.tray.black",
        "shapeKind": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 56.0, 480.0],
        "x2D": 644.0,
        "y2D": 100.0,
        "zOrder2D": 0,
        "materialId": "material.chess2d.tray",
        "faceColors": [[0.18, 0.14, 0.12] for _ in range(6)],
        "authoredProperties": {
            "isTray": {"t": "bool", "v": True}
        },
        "authors": ["Zach", "Gemini"]
    }
    objects.append(tray_black)
    relations.append({
        "type": "instance-of",
        "entityA": "object.chess2d.tray.black",
        "entityB": "category.chess2d.tray",
        "directed": True,
        "weight": 1.0,
        "authors": ["Zach", "Gemini"]
    })

    # -----------------------------------------------------------------------
    # 5. 64 Board Squares
    # -----------------------------------------------------------------------
    for gx in range(8):
        for gy in range(8):
            sq_id = f"square-{gx}-{gy}"
            sx, sy = square_pos(gx, gy)
            is_light = ((gx + gy) % 2 == 1)
            color = [0.91, 0.85, 0.75] if is_light else [0.52, 0.38, 0.26]
            mat_id = "material.chess2d.light_square" if is_light else "material.chess2d.dark_square"

            sq_obj = {
                "objectID": sq_id,
                "shapeKind": 12, # Shape2D
                "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SQ_SIZE, SQ_SIZE],
                "x2D": sx,
                "y2D": sy,
                "zOrder2D": 1,
                "materialId": mat_id,
                "faceColors": [color for _ in range(6)],
                "authoredProperties": {
                    "gridX": {"t": "int", "v": gx},
                    "gridY": {"t": "int", "v": gy},
                    "isSquare": {"t": "bool", "v": True}
                },
                "authors": ["Zach", "Gemini"]
            }
            objects.append(sq_obj)
            relations.append({
                "type": "instance-of",
                "entityA": sq_id,
                "entityB": "category.chess2d.square",
                "directed": True,
                "weight": 1.0,
                "authors": ["Zach", "Gemini"]
            })

    # -----------------------------------------------------------------------
    # 6. 32 Chess Pieces
    # Roles: 0=Pawn, 1=Rook, 2=Knight, 3=Bishop, 4=Queen, 5=King
    # -----------------------------------------------------------------------
    pieces_setup = [
        # White back rank (gy = 0)
        (0, 0, 0, 1, "rook", "R"),
        (1, 0, 0, 2, "knight", "N"),
        (2, 0, 0, 3, "bishop", "B"),
        (3, 0, 0, 4, "queen", "Q"),
        (4, 0, 0, 5, "king", "K"),
        (5, 0, 0, 3, "bishop", "B"),
        (6, 0, 0, 2, "knight", "N"),
        (7, 0, 0, 1, "rook", "R"),
        # Black back rank (gy = 7)
        (0, 7, 1, 1, "rook", "R"),
        (1, 7, 1, 2, "knight", "N"),
        (2, 7, 1, 3, "bishop", "B"),
        (3, 7, 1, 4, "queen", "Q"),
        (4, 7, 1, 5, "king", "K"),
        (5, 7, 1, 3, "bishop", "B"),
        (6, 7, 1, 2, "knight", "N"),
        (7, 7, 1, 1, "rook", "R"),
    ]
    # Pawns
    for col in range(8):
        pieces_setup.append((col, 1, 0, 0, "pawn", "P"))
        pieces_setup.append((col, 6, 1, 0, "pawn", "P"))

    for gx, gy, color, role, name, label in pieces_setup:
        color_str = "white" if color == 0 else "black"
        piece_id = f"piece-{color_str}-{name}-{gx}-{gy}"
        px, py = piece_pos(gx, gy)
        face_col = [0.96, 0.94, 0.90] if color == 0 else [0.20, 0.20, 0.24]
        mat_id = "material.chess2d.piece_white" if color == 0 else "material.chess2d.piece_black"

        piece_obj = {
            "objectID": piece_id,
            "shapeKind": 12, # Shape2D
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, PIECE_SIZE, PIECE_SIZE],
            "x2D": px,
            "y2D": py,
            "zOrder2D": 10,
            "materialId": mat_id,
            "faceColors": [face_col for _ in range(6)],
            "authoredProperties": {
                "gridX": {"t": "int", "v": gx},
                "gridY": {"t": "int", "v": gy},
                "chessColor": {"t": "int", "v": color},
                "chessRole": {"t": "int", "v": role},
                "onBoard": {"t": "bool", "v": True},
                "isSelected": {"t": "bool", "v": False},
                "hasMoved": {"t": "bool", "v": False},
                "label2D": {"t": "string", "v": label}
            },
            "authors": ["Zach", "Gemini"]
        }
        objects.append(piece_obj)
        relations.append({
            "type": "instance-of",
            "entityA": piece_id,
            "entityB": "category.chess2d.piece",
            "directed": True,
            "weight": 1.0,
            "authors": ["Zach", "Gemini"]
        })

    # -----------------------------------------------------------------------
    # 7. Authored Laws
    # -----------------------------------------------------------------------
    laws_dict = {}

    def make_move_action(can_capture=False):
        """Standard sequence of actions executed when a move is validated."""
        children = []
        if can_capture:
            # Publish enemy-captured with moving piece as subject
            children.append({
                "kind": 10, # Publish
                "eventType": "enemy-captured",
                "publishSubject": "",
                "publishObject": ""
            })
        children.extend([
            # Update piece's logical grid position
                {
                    "kind": 8, # Map
                    "path": "gridX",
                    "bindings": {"tx": "@state.chess2d.targetX"},
                    "function": make_linear_expr("tx", 1.0, 0.0)
                },
                {
                    "kind": 8, # Map
                    "path": "gridY",
                    "bindings": {"ty": "@state.chess2d.targetY"},
                    "function": make_linear_expr("ty", 1.0, 0.0)
                },
                # 3. Direct screen manifestation update:
                #    x2D := 146.0 + 60.0 * targetX
                #    y2D := 526.0 - 60.0 * targetY
                {
                    "kind": 8, # Map
                    "path": "x2D",
                    "bindings": {"tx": "@state.chess2d.targetX"},
                    "function": make_linear_expr("tx", 60.0, 146.0)
                },
                {
                    "kind": 8, # Map
                    "path": "y2D",
                    "bindings": {"ty": "@state.chess2d.targetY"},
                    "function": make_linear_expr("ty", -60.0, 526.0)
                },
                # 4. Deselect and mark moved
                {
                    "kind": 0, # Set
                    "path": "isSelected",
                    "operand": {"t": "bool", "v": False}
                },
                {
                    "kind": 0, # Set
                    "path": "zOrder2D",
                    "operand": {"t": "int", "v": 10}
                },
                {
                    "kind": 0, # Set
                    "path": "hasMoved",
                    "operand": {"t": "bool", "v": True}
                },
                # 5. Clear selection in state
                {
                    "kind": 0, # Set
                    "path": "@state.chess2d.selectionActive",
                    "operand": {"t": "bool", "v": False}
                },
                {
                    "kind": 0, # Set
                    "path": "@state.chess2d.selectedX",
                    "operand": {"t": "int", "v": -1}
                },
                {
                    "kind": 0, # Set
                    "path": "@state.chess2d.selectedY",
                    "operand": {"t": "int", "v": -1}
                },
                # 6. Flip turn: turn := 1 - turn
                {
                    "kind": 8, # Map
                    "path": "@state.chess2d.turn",
                    "bindings": {"t": "@state.chess2d.turn"},
                    "function": {
                        "input": "x",
                        "pieces": [{
                            "mathNode": {
                                "op": 0,
                                "scalarForm": {
                                    "terms": [
                                        {"c": 1.0, "factors": {}},
                                        {"c": -1.0, "factors": {"t": 1.0}}
                                    ]
                                }
                            }
                        }]
                    }
                },
                # 7. Publish positions-updated
                {
                    "kind": 10, # Publish
                    "eventType": "positions-updated",
                    "publishSubject": "",
                    "publishObject": ""
                }
        ])
        return {
            "kind": 5, # Sequence
            "children": children
        }

    # LAW 1: law-chess2d-click
    # Triggers on object-clicked when clicking a Square or Piece
    laws_dict["law-chess2d-click"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-click",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-click",
            "name": "handle-chess2d-click",
            "enabled": True,
            "activation": 0, # OnEvent
            "scope": 0, # Subject
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 4, # Any
                "children": [
                    {
                        "kind": 2, # Related
                        "relationType": "instance-of",
                        "otherId": "category.chess2d.square"
                    },
                    {
                        "kind": 3, # All: unselected piece
                        "children": [
                            {
                                "kind": 2, # Related
                                "relationType": "instance-of",
                                "otherId": "category.chess2d.piece"
                            },
                            {
                                "kind": 0,
                                "path": "isSelected",
                                "op": 0, # Eq
                                "operand": {"t": "bool", "v": False}
                            }
                        ]
                    }
                ]
            },
            "actionModel": {
                "kind": 5, # Sequence
                "children": [
                    {
                        "kind": 8, # Map
                        "path": "@state.chess2d.targetX",
                        "bindings": {"gx": "gridX"},
                        "function": make_linear_expr("gx", 1.0, 0.0)
                    },
                    {
                        "kind": 8, # Map
                        "path": "@state.chess2d.targetY",
                        "bindings": {"gy": "gridY"},
                        "function": make_linear_expr("gy", 1.0, 0.0)
                    },
                    {
                        "kind": 8, # Map: dx = selectionActive * (targetX - selectedX)
                        "path": "@state.chess2d.dx",
                        "bindings": {
                            "tx": "gridX",
                            "sx": "@state.chess2d.selectedX",
                            "sa": "@state.chess2d.selectionActive"
                        },
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {"sa": 1.0, "tx": 1.0}},
                                            {"c": -1.0, "factors": {"sa": 1.0, "sx": 1.0}}
                                        ]
                                    }
                                }
                            }]
                        }
                    },
                    {
                        "kind": 8, # Map: dy = selectionActive * (targetY - selectedY)
                        "path": "@state.chess2d.dy",
                        "bindings": {
                            "ty": "gridY",
                            "sy": "@state.chess2d.selectedY",
                            "sa": "@state.chess2d.selectionActive"
                        },
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {"sa": 1.0, "ty": 1.0}},
                                            {"c": -1.0, "factors": {"sa": 1.0, "sy": 1.0}}
                                        ]
                                    }
                                }
                            }]
                        }
                    },
                    {
                        "kind": 10, # Publish
                        "eventType": "board-clicked",
                        "publishSubject": "state.chess2d",
                        "publishObject": ""
                    }
                ]
            },
            "targets": []
        },
        "triggers": ["object-clicked"]
    }

    # LAW 2: law-chess2d-select
    # Triggers on board-clicked: selects friendly piece at target
    laws_dict["law-chess2d-select"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-select",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-select",
            "name": "select-chess2d-piece",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3, # All
                "children": [
                    {
                        "kind": 2, # Related
                        "relationType": "instance-of",
                        "otherId": "category.chess2d.piece"
                    },
                    {
                        "kind": 0,
                        "path": "onBoard",
                        "op": 0, # Eq
                        "operand": {"t": "bool", "v": True}
                    },
                    {
                        "kind": 0,
                        "path": "gridX",
                        "op": 0,
                        "operandPath": "@state.chess2d.targetX"
                    },
                    {
                        "kind": 0,
                        "path": "gridY",
                        "op": 0,
                        "operandPath": "@state.chess2d.targetY"
                    },
                    {
                        "kind": 0,
                        "path": "chessColor",
                        "op": 0,
                        "operandPath": "@state.chess2d.turn"
                    },
                    {
                        "kind": 0,
                        "path": "isSelected",
                        "op": 0,
                        "operand": {"t": "bool", "v": False}
                    }
                ]
            },
            "actionModel": {
                "kind": 5, # Sequence
                "children": [
                    {
                        "kind": 0, # Set
                        "path": "isSelected",
                        "operand": {"t": "bool", "v": True}
                    },
                    {
                        "kind": 0, # Set
                        "path": "zOrder2D",
                        "operand": {"t": "int", "v": 20}
                    },
                    {
                        "kind": 8, # Map
                        "path": "@state.chess2d.selectedX",
                        "bindings": {"gx": "gridX"},
                        "function": make_linear_expr("gx", 1.0, 0.0)
                    },
                    {
                        "kind": 8, # Map
                        "path": "@state.chess2d.selectedY",
                        "bindings": {"gy": "gridY"},
                        "function": make_linear_expr("gy", 1.0, 0.0)
                    },
                    {
                        "kind": 0, # Set
                        "path": "@state.chess2d.selectionActive",
                        "operand": {"t": "bool", "v": True}
                    },
                    {
                        "kind": 10, # Publish
                        "eventType": "piece-selected",
                        "publishSubject": "",
                        "publishObject": ""
                    }
                ]
            },
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 3: law-chess2d-deselect-others
    laws_dict["law-chess2d-deselect-others"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-deselect-others",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-deselect-others",
            "name": "deselect-other-pieces",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3, # All
                "children": [
                    {
                        "kind": 2,
                        "relationType": "instance-of",
                        "otherId": "category.chess2d.piece"
                    },
                    {
                        "kind": 0,
                        "path": "isSelected",
                        "op": 0,
                        "operand": {"t": "bool", "v": True}
                    },
                    {
                        "kind": 4, # Any
                        "children": [
                            {
                                "kind": 0,
                                "path": "gridX",
                                "op": 1, # Ne
                                "operandPath": "@state.chess2d.targetX"
                            },
                            {
                                "kind": 0,
                                "path": "gridY",
                                "op": 1, # Ne
                                "operandPath": "@state.chess2d.targetY"
                            }
                        ]
                    }
                ]
            },
            "actionModel": {
                "kind": 5,
                "children": [
                    {
                        "kind": 0,
                        "path": "isSelected",
                        "operand": {"t": "bool", "v": False}
                    },
                    {
                        "kind": 0,
                        "path": "zOrder2D",
                        "operand": {"t": "int", "v": 10}
                    }
                ]
            },
            "targets": []
        },
        "triggers": ["piece-selected"]
    }

    # LAW 4: law-chess2d-deselect-self
    laws_dict["law-chess2d-deselect-self"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-deselect-self",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-deselect-self",
            "name": "deselect-self",
            "enabled": True,
            "activation": 0, # OnEvent
            "scope": 0, # Subject
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3, # All
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}}
                ]
            },
            "actionModel": {
                "kind": 5,
                "children": [
                    {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": False}},
                    {"kind": 0, "path": "zOrder2D", "operand": {"t": "int", "v": 10}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "operand": {"t": "bool", "v": False}},
                    {"kind": 0, "path": "@state.chess2d.selectedX", "operand": {"t": "int", "v": -1}},
                    {"kind": 0, "path": "@state.chess2d.selectedY", "operand": {"t": "int", "v": -1}},
                    {"kind": 0, "path": "@state.chess2d.dx", "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.dy", "operand": {"t": "int", "v": 0}},
                    {
                        "kind": 10, # Publish
                        "eventType": "piece-deselected",
                        "publishSubject": "",
                        "publishObject": ""
                    }
                ]
            },
            "targets": []
        },
        "triggers": ["object-clicked"]
    }

    # LAW 5a: law-chess2d-pawn-w-step
    laws_dict["law-chess2d-pawn-w-step"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-pawn-w-step",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-pawn-w-step",
            "name": "white-pawn-step",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 1}}
                ]
            },
            "actionModel": make_move_action(can_capture=False),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 5b: law-chess2d-pawn-w-double
    laws_dict["law-chess2d-pawn-w-double"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-pawn-w-double",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-pawn-w-double",
            "name": "white-pawn-double",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "hasMoved", "op": 0, "operand": {"t": "bool", "v": False}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 2}}
                ]
            },
            "actionModel": make_move_action(can_capture=False),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 5c: law-chess2d-pawn-w-capture
    laws_dict["law-chess2d-pawn-w-capture"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-pawn-w-capture",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-pawn-w-capture",
            "name": "white-pawn-capture",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 1}},
                    {
                        "kind": 4, # Any: dx == 1 or dx == -1
                        "children": [
                            {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 1}},
                            {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": -1}}
                        ]
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 6a: law-chess2d-pawn-b-step
    laws_dict["law-chess2d-pawn-b-step"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-pawn-b-step",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-pawn-b-step",
            "name": "black-pawn-step",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": -1}}
                ]
            },
            "actionModel": make_move_action(can_capture=False),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 6b: law-chess2d-pawn-b-double
    laws_dict["law-chess2d-pawn-b-double"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-pawn-b-double",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-pawn-b-double",
            "name": "black-pawn-double",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}},
                    {"kind": 0, "path": "hasMoved", "op": 0, "operand": {"t": "bool", "v": False}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": -2}}
                ]
            },
            "actionModel": make_move_action(can_capture=False),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 6c: law-chess2d-pawn-b-capture
    laws_dict["law-chess2d-pawn-b-capture"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-pawn-b-capture",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-pawn-b-capture",
            "name": "black-pawn-capture",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                    {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": -1}},
                    {
                        "kind": 4, # Any: dx == 1 or dx == -1
                        "children": [
                            {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 1}},
                            {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": -1}}
                        ]
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 7: law-chess2d-knight (Role 2)
    # dx^2 + dy^2 == 5
    laws_dict["law-chess2d-knight"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-knight",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-knight",
            "name": "move-knight",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 2}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {
                        "kind": 4, # Any valid (dx, dy) pair for knight
                        "children": [
                            # (|dx| == 1 and |dy| == 2)
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 4, "children": [
                                        {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 1}},
                                        {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": -1}}
                                    ]},
                                    {"kind": 4, "children": [
                                        {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 2}},
                                        {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": -2}}
                                    ]}
                                ]
                            },
                            # (|dx| == 2 and |dy| == 1)
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 4, "children": [
                                        {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 2}},
                                        {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": -2}}
                                    ]},
                                    {"kind": 4, "children": [
                                        {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 1}},
                                        {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": -1}}
                                    ]}
                                ]
                            }
                        ]
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 8: law-chess2d-rook (Role 1)
    # (dx == 0 && dy != 0) || (dy == 0 && dx != 0)
    laws_dict["law-chess2d-rook"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-rook",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-rook",
            "name": "move-rook",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 1}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {
                        "kind": 4, # Any
                        "children": [
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                                    {"kind": 0, "path": "@state.chess2d.dy", "op": 1, "operand": {"t": "int", "v": 0}}
                                ]
                            },
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 0}},
                                    {"kind": 0, "path": "@state.chess2d.dx", "op": 1, "operand": {"t": "int", "v": 0}}
                                ]
                            }
                        ]
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 9: law-chess2d-bishop (Role 3)
    # dx^2 - dy^2 == 0 and dx != 0
    laws_dict["law-chess2d-bishop"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-bishop",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-bishop",
            "name": "move-bishop",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 3}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 1, "operand": {"t": "int", "v": 0}},
                    {
                        "kind": 6, # OntoMath InRange: dx^2 - dy^2 == 0
                        "bindings": {
                            "dx": "@state.chess2d.dx",
                            "dy": "@state.chess2d.dy"
                        },
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {"dx": 2.0}},
                                            {"c": -1.0, "factors": {"dy": 2.0}}
                                        ]
                                    }
                                }
                            }]
                        },
                        "lo": {"t": "double", "v": -0.1},
                        "hi": {"t": "double", "v": 0.1}
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 10: law-chess2d-queen (Role 4)
    # Rook move OR Bishop move
    laws_dict["law-chess2d-queen"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-queen",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-queen",
            "name": "move-queen",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 4}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {
                        "kind": 4, # Any (orthogonal or diagonal)
                        "children": [
                            # Orthogonal: dx == 0 && dy != 0
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 0, "path": "@state.chess2d.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                                    {"kind": 0, "path": "@state.chess2d.dy", "op": 1, "operand": {"t": "int", "v": 0}}
                                ]
                            },
                            # Orthogonal: dy == 0 && dx != 0
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 0, "path": "@state.chess2d.dy", "op": 0, "operand": {"t": "int", "v": 0}},
                                    {"kind": 0, "path": "@state.chess2d.dx", "op": 1, "operand": {"t": "int", "v": 0}}
                                ]
                            },
                            # Diagonal: dx^2 - dy^2 == 0 && dx != 0
                            {
                                "kind": 3,
                                "children": [
                                    {"kind": 0, "path": "@state.chess2d.dx", "op": 1, "operand": {"t": "int", "v": 0}},
                                    {
                                        "kind": 6,
                                        "bindings": {
                                            "dx": "@state.chess2d.dx",
                                            "dy": "@state.chess2d.dy"
                                        },
                                        "function": {
                                            "input": "x",
                                            "pieces": [{
                                                "mathNode": {
                                                    "op": 0,
                                                    "scalarForm": {
                                                        "terms": [
                                                            {"c": 1.0, "factors": {"dx": 2.0}},
                                                            {"c": -1.0, "factors": {"dy": 2.0}}
                                                        ]
                                                    }
                                                }
                                            }]
                                        },
                                        "lo": {"t": "double", "v": -0.1},
                                        "hi": {"t": "double", "v": 0.1}
                                    }
                                ]
                            }
                        ]
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 11: law-chess2d-king (Role 5)
    # dx in [-1, 1] && dy in [-1, 1] && (dx != 0 || dy != 0)
    laws_dict["law-chess2d-king"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-king",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-king",
            "name": "move-king",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 5}},
                    {"kind": 0, "path": "@state.chess2d.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 5, "operand": {"t": "int", "v": -1}}, # Ge -1
                    {"kind": 0, "path": "@state.chess2d.dx", "op": 3, "operand": {"t": "int", "v": 1}},  # Le 1
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 5, "operand": {"t": "int", "v": -1}}, # Ge -1
                    {"kind": 0, "path": "@state.chess2d.dy", "op": 3, "operand": {"t": "int", "v": 1}},  # Le 1
                    {
                        "kind": 4, # Any: dx != 0 || dy != 0
                        "children": [
                            {"kind": 0, "path": "@state.chess2d.dx", "op": 1, "operand": {"t": "int", "v": 0}},
                            {"kind": 0, "path": "@state.chess2d.dy", "op": 1, "operand": {"t": "int", "v": 0}}
                        ]
                    }
                ]
            },
            "actionModel": make_move_action(can_capture=True),
            "targets": []
        },
        "triggers": ["board-clicked"]
    }

    # LAW 12: law-chess2d-capture
    # Triggers on enemy-captured: unmake victim sitting at target square
    laws_dict["law-chess2d-capture"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-capture",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-capture",
            "name": "capture-enemy-piece",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "onBoard", "op": 0, "operand": {"t": "bool", "v": True}},
                    {"kind": 0, "path": "gridX", "op": 0, "operandPath": "@state.chess2d.targetX"},
                    {"kind": 0, "path": "gridY", "op": 0, "operandPath": "@state.chess2d.targetY"},
                    {"kind": 0, "path": "chessColor", "op": 1, "operandPath": "@event.subject.chessColor"}
                ]
            },
            "actionModel": {
                "kind": 5, # Sequence
                "children": [
                    {"kind": 0, "path": "onBoard", "operand": {"t": "bool", "v": False}},
                    {"kind": 0, "path": "gridX", "operand": {"t": "int", "v": -1}},
                    {"kind": 0, "path": "gridY", "operand": {"t": "int", "v": -1}},
                    {"kind": 0, "path": "zOrder2D", "operand": {"t": "int", "v": 5}},
                    {"kind": 0, "path": "shape.width2D", "operand": {"t": "float", "v": 36.0}},
                    {"kind": 0, "path": "shape.height2D", "operand": {"t": "float", "v": 36.0}},
                    # Move to left tray if white (slot * 28 + 110, x=70), right tray if black (x=654)
                    {
                        "kind": 8, # Map x2D
                        "path": "x2D",
                        "bindings": {"c": "chessColor"},
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 70.0, "factors": {}},
                                            {"c": 584.0, "factors": {"c": 1.0}} # 70 when c=0, 654 when c=1
                                        ]
                                    }
                                }
                            }]
                        }
                    },
                    {
                        "kind": 8, # Map y2D
                        "path": "y2D",
                        "bindings": {
                            "c": "chessColor",
                            "sw": "@state.chess2d.nextWhiteCapturedSlot",
                            "sb": "@state.chess2d.nextBlackCapturedSlot"
                        },
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 110.0, "factors": {}},
                                            {"c": 26.0, "factors": {"sw": 1.0}},
                                            {"c": -26.0, "factors": {"c": 1.0, "sw": 1.0}},
                                            {"c": 26.0, "factors": {"c": 1.0, "sb": 1.0}}
                                        ]
                                    }
                                }
                            }]
                        }
                    },
                    # Advance slot count on state
                    {
                        "kind": 8,
                        "path": "@state.chess2d.nextWhiteCapturedSlot",
                        "bindings": {
                            "sw": "@state.chess2d.nextWhiteCapturedSlot",
                            "c": "chessColor"
                        },
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {"sw": 1.0}},
                                            {"c": 1.0, "factors": {}},
                                            {"c": -1.0, "factors": {"c": 1.0}}
                                        ]
                                    }
                                }
                            }]
                        }
                    },
                    {
                        "kind": 8,
                        "path": "@state.chess2d.nextBlackCapturedSlot",
                        "bindings": {
                            "sb": "@state.chess2d.nextBlackCapturedSlot",
                            "c": "chessColor"
                        },
                        "function": {
                            "input": "x",
                            "pieces": [{
                                "mathNode": {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {"sb": 1.0}},
                                            {"c": 1.0, "factors": {"c": 1.0}}
                                        ]
                                    }
                                }
                            }]
                        }
                    }
                ]
            },
            "targets": []
        },
        "triggers": ["enemy-captured"]
    }

    # LAW 13: law-chess2d-sync-pos
    # Triggers on positions-updated: syncs all onBoard pieces to their screen coordinates
    laws_dict["law-chess2d-sync-pos"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-sync-pos",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-sync-pos",
            "name": "sync-chess2d-positions",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 2, "relationType": "instance-of", "otherId": "category.chess2d.piece"},
                    {"kind": 0, "path": "onBoard", "op": 0, "operand": {"t": "bool", "v": True}}
                ]
            },
            "actionModel": {
                "kind": 5,
                "children": [
                    {
                        "kind": 8, # Map x2D: 146.0 + 60.0 * gridX
                        "path": "x2D",
                        "bindings": {"gx": "gridX"},
                        "function": make_linear_expr("gx", 60.0, 146.0)
                    },
                    {
                        "kind": 8, # Map y2D: 526.0 - 60.0 * gridY
                        "path": "y2D",
                        "bindings": {"gy": "gridY"},
                        "function": make_linear_expr("gy", -60.0, 526.0)
                    }
                ]
            },
            "targets": []
        },
        "triggers": ["positions-updated"]
    }

    # LAW 14: law-chess2d-turn-banner
    # Triggers on positions-updated: updates HUD turn indicator label and background color
    laws_dict["law-chess2d-turn-white"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-turn-white",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-turn-white",
            "name": "turn-banner-white",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 8, "otherId": "ui.chess2d.turn"},
                    {"kind": 0, "path": "@state.chess2d.turn", "op": 0, "operand": {"t": "int", "v": 0}}
                ]
            },
            "actionModel": {
                "kind": 5,
                "children": [
                    {"kind": 0, "path": "label2D", "operand": {"t": "string", "v": "TURN: WHITE"}},
                    {"kind": 0, "path": "face.0.color", "operand": {"t": "vec3", "v": [0.92, 0.90, 0.86]}}
                ]
            },
            "targets": []
        },
        "triggers": ["positions-updated"]
    }

    laws_dict["law-chess2d-turn-black"] = {
        "authors": ["Zach", "Gemini"],
        "identifier": "law-chess2d-turn-black",
        "injected_by": "Gemini 3.8 Flash, 2026-10-16 2D Chess Zone authoring",
        "law": {
            "id": "law-chess2d-turn-black",
            "name": "turn-banner-black",
            "enabled": True,
            "activation": 0,
            "scope": 1,
            "authority": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach", "Gemini"],
            "conditionModel": {
                "kind": 3,
                "children": [
                    {"kind": 8, "otherId": "ui.chess2d.turn"},
                    {"kind": 0, "path": "@state.chess2d.turn", "op": 0, "operand": {"t": "int", "v": 1}}
                ]
            },
            "actionModel": {
                "kind": 5,
                "children": [
                    {"kind": 0, "path": "label2D", "operand": {"t": "string", "v": "TURN: BLACK"}},
                    {"kind": 0, "path": "face.0.color", "operand": {"t": "vec3", "v": [0.24, 0.24, 0.28]}}
                ]
            },
            "targets": []
        },
        "triggers": ["positions-updated"]
    }

    # -----------------------------------------------------------------------
    # Assemble Zone JSON
    # -----------------------------------------------------------------------
    law_refs = list(laws_dict.keys())

    zone_data = {
        "identifier": "Chess2D",
        "name": "Chess 2D",
        "owner": "Zach",
        "parentZone": "",
        "scope": 1,
        "qualities": ["game", "chess", "2d", "board"],
        "lawRefs": law_refs,
        "materials": materials,
        "world": {
            "objects": objects,
            "relations": [],
            "formations": []
        },
        "formationRelations": relations
    }

    return zone_data, laws_dict

def main():
    zone_data, laws_dict = build_chess2d()

    # Targets: repository checkout saves/ and live data home
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    repo_saves = os.path.join(repo_root, "saves")
    data_home = os.path.expanduser("~/Library/Application Support/Earthcall")

    targets = [repo_saves]
    if os.path.exists(data_home):
        targets.append(data_home)

    for target_base in targets:
        zones_dir = os.path.join(target_base, "zones", "Chess2D")
        os.makedirs(zones_dir, exist_ok=True)
        zone_file = os.path.join(zones_dir, "zone.json")
        with open(zone_file, "w") as f:
            json.dump(zone_data, f, indent=4)
        print(f"Wrote Zone identity to: {zone_file}")

        for law_id, law_content in laws_dict.items():
            law_dir = os.path.join(target_base, "laws", law_id)
            os.makedirs(law_dir, exist_ok=True)
            law_file = os.path.join(law_dir, "law.json")
            with open(law_file, "w") as f:
                json.dump(law_content, f, indent=4)
        print(f"Wrote {len(laws_dict)} discrete Law roots to {os.path.join(target_base, 'laws')}")

    print("Successfully authored clean 2D Chess Zone!")

if __name__ == "__main__":
    main()
