import json
import copy

def create_base_world():
    return {
        "authoredLaws": {
            "firstMoverEnabled": {},
            "formationMembers": [],
            "laws": [],
            "triggers": {}
        },
        "cameraFront": [0.0, 0.0, -1.0],
        "cameraPos": [0.0, 5.0, 5.0],
        "cameraUp": [0.0, 1.0, 0.0],
        "categories": [],
        "concepts": [],
        "currentColor": [1.0, 1.0, 1.0, 1.0],
        "currentZone": "zone.chess",
        "flying": True,
        "materials": [],
        "mathFunctions": [],
        "objects": [],
        "physicsLaws": [],
        "pitch": -45.0,
        "playerBody": None,
        "transferPolicy": {},
        "worldTime": 0.0,
        "zones": [
            {
                "id": "zone.chess",
                "name": "Chess Game",
                "authoredLaws": {
                    "firstMoverEnabled": {},
                    "formationMembers": [],
                    "laws": [],
                    "triggers": {}
                },
                "objects": [],
                "formationRelations": [],
                "physics": {
                    "gravity": [0.0, 0.0, 0.0]
                }
            }
        ]
    }

def add_category(world, category_id, display_name):
    world["objects"].append({
        "objectID": category_id,
        "shapeKind": 0,
        "materialId": "material.default",
        "authoredProperties": {
            "displayName": {"t": "string", "v": display_name}
        },
        "authors": ["Gemini"]
    })

def add_object_to_zone(world, zone_id, obj):
    for z in world["zones"]:
        if z["id"] == zone_id:
            z["objects"].append(obj)
            return

def add_relation(world, zone_id, type_str, entityA, entityB, directed=True):
    for z in world["zones"]:
        if z["id"] == zone_id:
            z["formationRelations"].append({
                "type": type_str,
                "entityA": entityA,
                "entityB": entityB,
                "directed": directed,
                "weight": 1.0,
                "authors": ["Gemini"]
            })
            return

def add_law(world, law_id, name, activation, condition, action, triggers):
    law = {
        "id": law_id,
        "name": name,
        "enabled": True,
        "authority": 0,
        "activation": activation,
        "scope": 1,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": ["Gemini"],
        "conditionModel": condition,
        "actionModel": action
    }
    world["authoredLaws"]["laws"].append(law)
    world["authoredLaws"]["formationMembers"].append(law_id)
    world["authoredLaws"]["triggers"][law_id] = triggers

def build_chess():
    world = create_base_world()
    zone = "zone.chess"

    # State object
    world["objects"].append({
        "objectID": "state.chess",
        "shapeKind": 0,
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
    })

    # Categories
    add_category(world, "category.chess.board", "Chess Board")
    add_category(world, "category.chess.piece", "Chess Piece")
    add_category(world, "category.chess.pawn", "Pawn")
    add_relation(world, zone, "subcategory-of", "category.chess.pawn", "category.chess.piece")

    # The Board
    board = {
        "objectID": "object.chess.board",
        "shapeKind": 2, # Square
        "position": [0.0, -0.05, 0.0],
        "extents": [8.0, 0.1, 8.0],
        "faceColors": [[1.0, 1.0, 1.0, 1.0]] * 6,
        "authors": ["Gemini"]
    }
    add_object_to_zone(world, zone, board)
    add_relation(world, zone, "instance-of", "object.chess.board", "category.chess.board")

    # Pieces (Simplified to just Pawns for this MVP to satisfy Grok's requirement)
    for i in range(8):
        for color, z_pos, row in [(0, -2.5, 1), (1, 2.5, 6)]:
            piece_id = f"piece-{'white' if color == 0 else 'black'}-pawn-{i}"
            piece = {
                "objectID": piece_id,
                "shapeKind": 5, # Cone
                "position": [i - 3.5, 0.5, z_pos],
                "extents": [0.4, 1.0, 0.4],
                "faceColors": [[1.0, 1.0, 1.0, 1.0] if color == 0 else [0.2, 0.2, 0.2, 1.0]] * 6,
                "authoredProperties": {
                    "gridX": {"t": "int", "v": i},
                    "gridY": {"t": "int", "v": row},
                    "chessColor": {"t": "int", "v": color},
                    "chessRole": {"t": "int", "v": 0},
                    "isSelected": {"t": "bool", "v": False}
                },
                "authors": ["Gemini"]
            }
            add_object_to_zone(world, zone, piece)
            add_relation(world, zone, "instance-of", piece_id, "category.chess.pawn")

    # We will need laws to:
    # 1. law-chess-click: trigger on `object-clicked`. 
    #    Condition: Any(Related("instance-of", "category.chess.board"), Related("instance-of", "category.chess.piece"))
    #    Action: Map pointerWorld to targetX/Y, then emit `board-clicked`
    # 2. law-chess-select: trigger on `board-clicked`
    #    Condition: Piece at targetX/Y has chessColor == turn
    #    Action: Set selectedX/Y, set selectionActive=true, emit `piece-selected`
    # 3. law-chess-move: trigger on `board-clicked`
    #    Condition: selectionActive==true, AND valid move.
    #    Action: emit `piece-moved`

    # To keep this first script simple, I'll just write it and refine iteratively.
    
    with open("saves/worlds/chess.json", "w") as f:
        json.dump(world, f, indent=2)

if __name__ == "__main__":
    build_chess()
