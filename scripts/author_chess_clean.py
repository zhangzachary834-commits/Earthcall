#!/usr/bin/env python3
import json

def get_mapping_pieces():
    pieces = []
    for i in range(8):
        pieces.append({
            "lo": float(i - 4),
            "hi": float(i - 3),
            "includeLo": True,
            "includeHi": (i == 7),
            "expr": {
                "terms": [
                    {"c": float(i), "factors": {}}
                ]
            }
        })
    return pieces

def build_world():
    world = {
        "currentZone": 0,
        "flying": True,
        "cameraPos": [0.0, 8.0, 8.0],
        "cameraFront": [0.0, -0.7071, -0.7071],
        "cameraUp": [0.0, 0.7071, -0.7071],
        "yaw": -90.0,
        "pitch": -45.0,
        "currentColor": [1.0, 1.0, 1.0],
        "materials": [],
        "zones": [
            {
                "name": "Chess Ground",
                "artStyle": "default",
                "world": {
                    "objects": []
                },
                "formationRelations": []
            }
        ],
        "authoredLaws": {
            "formationMembers": [],
            "laws": [],
            "triggers": {}
        }
    }

    zone_objects = world["zones"][0]["world"]["objects"]
    relations = world["zones"][0]["formationRelations"]
    laws = world["authoredLaws"]["laws"]
    triggers = world["authoredLaws"]["triggers"]
    formation_members = world["authoredLaws"]["formationMembers"]

    # 0. First Mover Identity
    first_mover_obj = {
        "objectID": "Gemini",
        "shapeKind": 0, # extra-spatial
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Gemini (First Mover)"}
        },
        "authors": ["Gemini"]
    }
    zone_objects.append(first_mover_obj)

    # 1. State object
    state_obj = {
        "objectID": "state.chess",
        "shapeKind": 0, # extra-spatial
        "authoredProperties": {
            "turn": {"t": "int", "v": 0},
            "selectedX": {"t": "int", "v": -1},
            "selectedY": {"t": "int", "v": -1},
            "targetX": {"t": "int", "v": -1},
            "targetY": {"t": "int", "v": -1},
            "selectionActive": {"t": "bool", "v": False},
            "dx": {"t": "int", "v": 0},
            "dy": {"t": "int", "v": 0}
        },
        "authors": ["Gemini"]
    }
    zone_objects.append(state_obj)

    # 2. Categories
    cat_board = {
        "objectID": "category.chess.board",
        "shapeKind": 0,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Chess Board"}
        },
        "authors": ["Gemini"]
    }
    cat_piece = {
        "objectID": "category.chess.piece",
        "shapeKind": 0,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Chess Piece"}
        },
        "authors": ["Gemini"]
    }
    zone_objects.append(cat_board)
    zone_objects.append(cat_piece)

    # 3. Single Board Object
    board_obj = {
        "objectID": "object.chess.board",
        "shapeKind": 1, # Cube
        "geometryType": 0,
        "shapeParams": [8.0, 0.2, 8.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "transform": [
            8.0, 0.0, 0.0, 0.0,
            0.0, 0.2, 0.0, 0.0,
            0.0, 0.0, 8.0, 0.0,
            0.0, -0.1, 0.0, 1.0
        ],
        "center": [0.0, -0.1, 0.0],
        "materialId": "",
        "faceColors": [
            [0.85, 0.75, 0.65],
            [0.85, 0.75, 0.65],
            [0.82, 0.70, 0.55], # top face
            [0.85, 0.75, 0.65],
            [0.85, 0.75, 0.65],
            [0.85, 0.75, 0.65]
        ],
        "authoredProperties": {
            "isBoard": {"t": "bool", "v": True}
        },
        "authors": ["Gemini"]
    }
    zone_objects.append(board_obj)
    relations.append({
        "type": "instance-of",
        "entityA": "object.chess.board",
        "entityB": "category.chess.board",
        "directed": True,
        "weight": 1.0,
        "authors": ["Gemini"]
    })

    # 4. Chess Pieces Setup
    # Roles: 0=Pawn, 1=Rook, 2=Knight, 3=Bishop, 4=Queen, 5=King
    pieces_layout = [
        # White back rank (y=0)
        (0, 0, 0, 1, "rook", [0.5, 0.9, 0.5]),
        (1, 0, 0, 2, "knight", [0.45, 1.0, 0.45]),
        (2, 0, 0, 3, "bishop", [0.45, 1.1, 0.45]),
        (3, 0, 0, 4, "queen", [0.5, 1.2, 0.5]),
        (4, 0, 0, 5, "king", [0.5, 1.3, 0.5]),
        (5, 0, 0, 3, "bishop", [0.45, 1.1, 0.45]),
        (6, 0, 0, 2, "knight", [0.45, 1.0, 0.45]),
        (7, 0, 0, 1, "rook", [0.5, 0.9, 0.5]),
        # Black back rank (y=7)
        (0, 7, 1, 1, "rook", [0.5, 0.9, 0.5]),
        (1, 7, 1, 2, "knight", [0.45, 1.0, 0.45]),
        (2, 7, 1, 3, "bishop", [0.45, 1.1, 0.45]),
        (3, 7, 1, 4, "queen", [0.5, 1.2, 0.5]),
        (4, 7, 1, 5, "king", [0.5, 1.3, 0.5]),
        (5, 7, 1, 3, "bishop", [0.45, 1.1, 0.45]),
        (6, 7, 1, 2, "knight", [0.45, 1.0, 0.45]),
        (7, 7, 1, 1, "rook", [0.5, 0.9, 0.5]),
    ]
    # White pawns (y=1) & Black pawns (y=6)
    for col in range(8):
        pieces_layout.append((col, 1, 0, 0, "pawn", [0.4, 0.8, 0.4]))
        pieces_layout.append((col, 6, 1, 0, "pawn", [0.4, 0.8, 0.4]))

    for x, y, color, role, name, dims in pieces_layout:
        color_str = "white" if color == 0 else "black"
        obj_id = f"piece-{color_str}-{name}-{x}-{y}"
        world_x = float(x) - 3.5
        world_z = float(y) - 3.5
        world_y = dims[1] / 2.0
        
        piece_color = [0.95, 0.95, 0.90] if color == 0 else [0.15, 0.15, 0.15]
        
        piece_obj = {
            "objectID": obj_id,
            "shapeKind": 3 if role in [0, 1, 4, 5] else 1, # Cylinder or Cube
            "geometryType": 0,
            "shapeParams": [dims[0], dims[1], dims[2], 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": [
                dims[0], 0.0, 0.0, 0.0,
                0.0, dims[1], 0.0, 0.0,
                0.0, 0.0, dims[2], 0.0,
                world_x, world_y, world_z, 1.0
            ],
            "center": [world_x, world_y, world_z],
            "materialId": "",
            "faceColors": [piece_color for _ in range(6)],
            "authoredProperties": {
                "gridX": {"t": "int", "v": x},
                "gridY": {"t": "int", "v": y},
                "chessColor": {"t": "int", "v": color},
                "chessRole": {"t": "int", "v": role},
                "isSelected": {"t": "bool", "v": False},
                "hasMoved": {"t": "bool", "v": False}
            },
            "authors": ["Gemini"]
        }
        zone_objects.append(piece_obj)
        relations.append({
            "type": "instance-of",
            "entityA": obj_id,
            "entityB": "category.chess.piece",
            "directed": True,
            "weight": 1.0,
            "authors": ["Gemini"]
        })

    # Helper to add laws
    def add_law(law_id, name, activation, triggers_list, cond_node, action_node, scope=1):
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
            "authors": ["Gemini"],
            "conditionModel": cond_node,
            "actionModel": action_node
        }
        laws.append(law)
        formation_members.append(law_id)
        if triggers_list:
            triggers[law_id] = triggers_list

    # -----------------------------------------------------------------------
    # LAW 1: law-chess-click
    # Triggers on object-clicked (InteractionChannel click edge)
    # Condition: Subject is either the Board or a Chess Piece
    # Action: Map pointer coordinates to board grid targetX/Y, compute dx/dy, emit board-clicked
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-click",
        name="handle-click",
        activation=0, # OnEvent
        triggers_list=["object-clicked"],
        cond_node={
            "kind": 4, # Any
            "children": [
                {
                    "kind": 0, # PropertyComparison
                    "path": "isBoard",
                    "op": 0, # Eq
                    "operand": {"t": "bool", "v": True}
                },
                {
                    "kind": 2, # Related
                    "relationType": "instance-of",
                    "otherId": "category.chess.piece"
                }
            ]
        },
        action_node={
            "kind": 5, # Sequence
            "children": [
                {
                    "kind": 8, # Map
                    "path": "@state.chess.targetX",
                    "bindings": {
                        "ptrX": "@interaction-channel.pointerWorld.x"
                    },
                    "function": {
                        "input": "ptrX",
                        "pieces": get_mapping_pieces()
                    }
                },
                {
                    "kind": 8, # Map
                    "path": "@state.chess.targetY",
                    "bindings": {
                        "ptrY": "@interaction-channel.pointerWorld.z"
                    },
                    "function": {
                        "input": "ptrY",
                        "pieces": get_mapping_pieces()
                    }
                },
                {
                    "kind": 8, # Map
                    "path": "@state.chess.dx",
                    "bindings": {
                        "tx": "@state.chess.targetX",
                        "sx": "@state.chess.selectedX"
                    },
                    "function": {
                        "pieces": [{
                            "expr": {
                                "terms": [
                                    {"c": 1.0, "factors": {"tx": 1.0}},
                                    {"c": -1.0, "factors": {"sx": 1.0}}
                                ]
                            }
                        }]
                    }
                },
                {
                    "kind": 8, # Map
                    "path": "@state.chess.dy",
                    "bindings": {
                        "ty": "@state.chess.targetY",
                        "sy": "@state.chess.selectedY"
                    },
                    "function": {
                        "pieces": [{
                            "expr": {
                                "terms": [
                                    {"c": 1.0, "factors": {"ty": 1.0}},
                                    {"c": -1.0, "factors": {"sy": 1.0}}
                                ]
                            }
                        }]
                    }
                },
                {
                    "kind": 10, # Publish
                    "eventType": "board-clicked",
                    "publishSubject": "state.chess",
                    "publishObject": ""
                }
            ]
        }
    )

    # -----------------------------------------------------------------------
    # LAW 2: law-chess-select
    # Triggers on board-clicked
    # Condition: Piece whose (gridX, gridY) == (targetX, targetY) and chessColor == turn
    # Action: Set isSelected=True, set selectedX/Y, selectionActive=True, emit piece-selected
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-select",
        name="select-piece",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3, # All
            "children": [
                {
                    "kind": 2, # Related
                    "relationType": "instance-of",
                    "otherId": "category.chess.piece"
                },
                {
                    "kind": 0,
                    "path": "gridX",
                    "op": 0,
                    "operandPath": "@state.chess.targetX"
                },
                {
                    "kind": 0,
                    "path": "gridY",
                    "op": 0,
                    "operandPath": "@state.chess.targetY"
                },
                {
                    "kind": 0,
                    "path": "chessColor",
                    "op": 0,
                    "operandPath": "@state.chess.turn"
                }
            ]
        },
        action_node={
            "kind": 5, # Sequence
            "children": [
                {
                    "kind": 0, # Set
                    "path": "isSelected",
                    "operand": {"t": "bool", "v": True}
                },
                {
                    "kind": 8, # Map
                    "path": "@state.chess.selectedX",
                    "bindings": {"gx": "gridX"},
                    "function": {
                        "pieces": [{
                            "expr": {"terms": [{"c": 1.0, "factors": {"gx": 1.0}}]}
                        }]
                    }
                },
                {
                    "kind": 8, # Map
                    "path": "@state.chess.selectedY",
                    "bindings": {"gy": "gridY"},
                    "function": {
                        "pieces": [{
                            "expr": {"terms": [{"c": 1.0, "factors": {"gy": 1.0}}]}
                        }]
                    }
                },
                {
                    "kind": 0, # Set
                    "path": "@state.chess.selectionActive",
                    "operand": {"t": "bool", "v": True}
                },
                {
                    "kind": 10, # Publish
                    "eventType": "piece-selected",
                    "publishSubject": "", # emits on THIS piece
                    "publishObject": ""
                }
            ]
        }
    )

    # -----------------------------------------------------------------------
    # LAW 3: law-chess-deselect
    # Triggers on piece-selected
    # Condition: isSelected == True AND (gridX != targetX OR gridY != targetY)
    # Action: isSelected = False
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-deselect",
        name="deselect-others",
        activation=0,
        triggers_list=["piece-selected"],
        cond_node={
            "kind": 3, # All
            "children": [
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
                            "operandPath": "@state.chess.targetX"
                        },
                        {
                            "kind": 0,
                            "path": "gridY",
                            "op": 1, # Ne
                            "operandPath": "@state.chess.targetY"
                        }
                    ]
                }
            ]
        },
        action_node={
            "kind": 0, # Set
            "path": "isSelected",
            "operand": {"t": "bool", "v": False}
        }
    )

    # Common move execution action node
    def make_move_action():
        return {
            "kind": 5, # Sequence
            "children": [
                # 1. Publish enemy-captured to destroy any enemy sitting at (targetX, targetY)
                {
                    "kind": 10, # Publish
                    "eventType": "enemy-captured",
                    "publishSubject": "",
                    "publishObject": ""
                },
                # 2. Update moving piece's grid coordinates
                {
                    "kind": 8, # Map
                    "path": "gridX",
                    "bindings": {"tx": "@state.chess.targetX"},
                    "function": {
                        "pieces": [{
                            "expr": {"terms": [{"c": 1.0, "factors": {"tx": 1.0}}]}
                        }]
                    }
                },
                {
                    "kind": 8, # Map
                    "path": "gridY",
                    "bindings": {"ty": "@state.chess.targetY"},
                    "function": {
                        "pieces": [{
                            "expr": {"terms": [{"c": 1.0, "factors": {"ty": 1.0}}]}
                        }]
                    }
                },
                {
                    "kind": 0, # Set
                    "path": "isSelected",
                    "operand": {"t": "bool", "v": False}
                },
                {
                    "kind": 0, # Set
                    "path": "hasMoved",
                    "operand": {"t": "bool", "v": True}
                },
                # 3. Clear selection in global state
                {
                    "kind": 0, # Set
                    "path": "@state.chess.selectionActive",
                    "operand": {"t": "bool", "v": False}
                },
                {
                    "kind": 0, # Set
                    "path": "@state.chess.selectedX",
                    "operand": {"t": "int", "v": -1}
                },
                {
                    "kind": 0, # Set
                    "path": "@state.chess.selectedY",
                    "operand": {"t": "int", "v": -1}
                },
                # 4. Flip turn (turn = 1 - turn)
                {
                    "kind": 8, # Map (flip turn: turn = 1 - turn)
                    "path": "@state.chess.turn",
                    "bindings": {"t": "@state.chess.turn"},
                    "function": {
                        "pieces": [{
                            "expr": {
                                "terms": [
                                    {"c": 1.0, "factors": {}},
                                    {"c": -1.0, "factors": {"t": 1.0}}
                                ]
                            }
                        }]
                    }
                },
                # 5. Tell all pieces to sync their 3D transforms
                {
                    "kind": 10, # Publish
                    "eventType": "positions-updated",
                    "publishSubject": "",
                    "publishObject": ""
                }
            ]
        }

    # -----------------------------------------------------------------------
    # LAW 4a: White Pawn Advance (1 step forward)
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-pawn-w-1",
        name="move-white-pawn-step",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "@state.chess.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "@state.chess.dy", "op": 0, "operand": {"t": "int", "v": 1}}
            ]
        },
        action_node=make_move_action()
    )

    # -----------------------------------------------------------------------
    # LAW 4b: White Pawn Double Advance (2 steps forward from rank 1)
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-pawn-w-2",
        name="move-white-pawn-double",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "hasMoved", "op": 0, "operand": {"t": "bool", "v": False}},
                {"kind": 0, "path": "@state.chess.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "@state.chess.dy", "op": 0, "operand": {"t": "int", "v": 2}}
            ]
        },
        action_node=make_move_action()
    )

    # -----------------------------------------------------------------------
    # LAW 4c: White Pawn Diagonal Capture
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-pawn-w-cap",
        name="move-white-pawn-capture",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "@state.chess.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "@state.chess.dy", "op": 0, "operand": {"t": "int", "v": 1}},
                {
                    "kind": 4, # Any: dx == 1 or dx == -1
                    "children": [
                        {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": 1}},
                        {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": -1}}
                    ]
                }
            ]
        },
        action_node=make_move_action()
    )

    # -----------------------------------------------------------------------
    # LAW 5a: Black Pawn Advance (1 step forward)
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-pawn-b-1",
        name="move-black-pawn-step",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}},
                {"kind": 0, "path": "@state.chess.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "@state.chess.dy", "op": 0, "operand": {"t": "int", "v": -1}}
            ]
        },
        action_node=make_move_action()
    )

    # -----------------------------------------------------------------------
    # LAW 5b: Black Pawn Double Advance (2 steps forward from rank 6)
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-pawn-b-2",
        name="move-black-pawn-double",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}},
                {"kind": 0, "path": "hasMoved", "op": 0, "operand": {"t": "bool", "v": False}},
                {"kind": 0, "path": "@state.chess.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "@state.chess.dy", "op": 0, "operand": {"t": "int", "v": -2}}
            ]
        },
        action_node=make_move_action()
    )

    # -----------------------------------------------------------------------
    # LAW 5c: Black Pawn Diagonal Capture
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-pawn-b-cap",
        name="move-black-pawn-capture",
        activation=0,
        triggers_list=["board-clicked"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "chessRole", "op": 0, "operand": {"t": "int", "v": 0}},
                {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}},
                {"kind": 0, "path": "@state.chess.selectionActive", "op": 0, "operand": {"t": "bool", "v": True}},
                {"kind": 0, "path": "@state.chess.dy", "op": 0, "operand": {"t": "int", "v": -1}},
                {
                    "kind": 4, # Any: dx == 1 or dx == -1
                    "children": [
                        {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": 1}},
                        {"kind": 0, "path": "@state.chess.dx", "op": 0, "operand": {"t": "int", "v": -1}}
                    ]
                }
            ]
        },
        action_node=make_move_action()
    )

    # -----------------------------------------------------------------------
    # LAW 6: Enemy Capture
    # Triggers on enemy-captured
    # Condition: Piece at (targetX, targetY) whose chessColor == turn (since turn is already flipped to victim's color!)
    # Action: Destroy victim
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-capture",
        name="capture-enemy",
        activation=0,
        triggers_list=["enemy-captured"],
        cond_node={
            "kind": 3,
            "children": [
                {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                {"kind": 0, "path": "gridX", "op": 0, "operandPath": "@state.chess.targetX"},
                {"kind": 0, "path": "gridY", "op": 0, "operandPath": "@state.chess.targetY"},
                {"kind": 0, "path": "chessColor", "op": 0, "operandPath": "@state.chess.turn"}
            ]
        },
        action_node={
            "kind": 16, # Destroy
            "elementToken": "" # destroys THIS subject
        }
    )

    # -----------------------------------------------------------------------
    # LAW 7: Update 3D World Positions
    # Triggers on positions-updated
    # Action: position.x = gridX - 3.5, position.z = gridY - 3.5
    # -----------------------------------------------------------------------
    add_law(
        law_id="law-chess-update-pos",
        name="sync-piece-transform",
        activation=0,
        triggers_list=["positions-updated"],
        cond_node={
            "kind": 2,
            "relationType": "instance-of",
            "otherId": "category.chess.piece"
        },
        action_node={
            "kind": 5,
            "children": [
                {
                    "kind": 8, # Map
                    "path": "position.x",
                    "bindings": {"gx": "gridX"},
                    "function": {
                        "pieces": [{
                            "expr": {
                                "terms": [
                                    {"c": 1.0, "factors": {"gx": 1.0}},
                                    {"c": -3.5, "factors": {}}
                                ]
                            }
                        }]
                    }
                },
                {
                    "kind": 8, # Map
                    "path": "position.z",
                    "bindings": {"gy": "gridY"},
                    "function": {
                        "pieces": [{
                            "expr": {
                                "terms": [
                                    {"c": 1.0, "factors": {"gy": 1.0}},
                                    {"c": -3.5, "factors": {}}
                                ]
                            }
                        }]
                    }
                }
            ]
        }
    )

    return world

def main():
    world = build_world()
    save_path = "saves/worlds/chess_first_mover.json"
    with open(save_path, "w") as f:
        json.dump(world, f, indent=2)
    print(f"Authored clean First Mover chess world to {save_path}")

if __name__ == "__main__":
    main()
