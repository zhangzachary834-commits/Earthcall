import json
import os

BASE_SAVE_PATH = "saves/worlds/my_second_world.json"
OUT_SAVE_PATH = "saves/worlds/chess.json"

def main():
    with open(BASE_SAVE_PATH, "r") as f:
        world = json.load(f)
    
    # Clear existing objects and laws
    world["zones"][0]["world"]["objects"] = []
    world["zones"][0]["formationRelations"] = []
    world["authoredLaws"]["laws"] = []
    world["authoredLaws"]["triggers"] = {}
    world["authoredLaws"]["formationMembers"] = []
    
    # Preserve first-mover laws
    world["authoredLaws"]["rete"]["alphaNodes"] = []
    world["authoredLaws"]["rete"]["betaNodes"] = []
    world["authoredLaws"]["rete"]["facts"] = []
    
    objects = []
    
    # Create Game State Object
    game_state = {
        "objectID": "chess-state",
        "shapeKind": 0, # Cube
        "shapeParams": [0.1, 0.1, 0.1],
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, -4.5, 0, 0, 1],
        "center": [-4.5, 0, 0],
        "faceColors": [[0.5,0.5,0.5]] * 6,
        "authoredProperties": {
            "turn": {"t": "int", "v": 0}, # 0 = white, 1 = black
            "targetX": {"t": "int", "v": -1},
            "targetY": {"t": "int", "v": -1},
            "selectionActive": {"t": "bool", "v": False},
            "dx": {"t": "int", "v": 0},
            "dy": {"t": "int", "v": 0},
            "selectedX": {"t": "int", "v": -1},
            "selectedY": {"t": "int", "v": -1}
        }
    }
    objects.append(game_state)
    
    # Create Category Being for Pieces
    category_piece = {
        "objectID": "category.chess.piece",
        "shapeKind": 0,
        "shapeParams": [0.01, 0.01, 0.01],
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0, -2, 0, 1],
        "center": [0, -2, 0],
        "faceColors": [[0,0,0]] * 6,
        "authoredProperties": {}
    }
    objects.append(category_piece)

    laws = []
    triggers = {}
    formationMembers = []
    
    law_counter = 100
    
    def add_law(name, trigger, activation, condition, action):
        nonlocal law_counter
        law_id = f"law-{law_counter}"
        law_counter += 1
        
        law = {
            "id": law_id,
            "name": name,
            "enabled": True,
            "authority": 0,
            "activation": activation,
            "scope": 1, # Everyone
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Player"],
            "conditionSubjects": [],
            "targets": [],
            "conditionModel": condition,
            "actionModel": action,
            "conditionDescriptions": [name + " condition"],
            "actionDescriptions": [name + " action"],
            "provenance": [],
            "applicationLog": []
        }
        laws.append(law)
        formationMembers.append(law_id)
        if trigger:
            if law_id not in triggers:
                triggers[law_id] = []
            triggers[law_id].append(trigger)
            
    # Board Generation
    for y in range(8):
        for x in range(8):
            color = [0.9, 0.9, 0.8] if (x + y) % 2 == 1 else [0.4, 0.5, 0.4]
            sq_id = f"board-{x}-{y}"
            sq = {
                "objectID": sq_id,
                "shapeKind": 0, # Cube
                "shapeParams": [0.5, 0.05, 0.5],
                "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, x - 3.5, 0, y - 3.5, 1],
                "center": [x - 3.5, 0, y - 3.5],
                "faceColors": [color] * 6,
                "authoredProperties": {
                    "gridX": {"t": "int", "v": x},
                    "gridY": {"t": "int", "v": y},
                    "isSquare": {"t": "bool", "v": True}
                }
            }
            objects.append(sq)
            
    # Pieces Generation
    piece_roles = {
        "pawn": 0, "knight": 1, "bishop": 2, "rook": 3, "queen": 4, "king": 5
    }
    
    def create_piece(x, y, role, color_id, color_val):
        pid = f"piece-{color_id}-{role}-{x}-{y}"
        
        shape = 3 # Cylinder for generic
        if role == "pawn": shape = 3; params = [0.3, 0.4, 0.3]
        elif role == "rook": shape = 0; params = [0.35, 0.5, 0.35]
        elif role == "knight": shape = 2; params = [0.35, 0.35, 0.35] # Sphere
        elif role == "bishop": shape = 4; params = [0.3, 0.6, 0.3] # Cone
        elif role == "queen": shape = 3; params = [0.4, 0.7, 0.4]
        elif role == "king": shape = 0; params = [0.4, 0.8, 0.4]
        
        color = [0.95, 0.95, 0.95] if color_id == "white" else [0.2, 0.2, 0.2]
        
        obj = {
            "objectID": pid,
            "shapeKind": shape,
            "shapeParams": params,
            "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, x - 3.5, params[1], y - 3.5, 1],
            "center": [x - 3.5, params[1], y - 3.5],
            "faceColors": [color] * 6,
            "authoredProperties": {
                "gridX": {"t": "int", "v": x},
                "gridY": {"t": "int", "v": y},
                "chessRole": {"t": "int", "v": piece_roles[role]},
                "chessColor": {"t": "int", "v": color_val},
                "isSelected": {"t": "bool", "v": False},
                "hasMoved": {"t": "bool", "v": False}
            }
        }
        objects.append(obj)
        world["zones"][0]["formationRelations"].append({
            "type": "instance-of",
            "entityA": pid,
            "entityB": "category.chess.piece",
            "directed": True,
            "weight": 1.0,
            "events": []
        })

    # Pawns
    for x in range(8):
        create_piece(x, 1, "pawn", "white", 0)
        create_piece(x, 6, "pawn", "black", 1)
        
    order = ["rook", "knight", "bishop", "queen", "king", "bishop", "knight", "rook"]
    for x in range(8):
        create_piece(x, 0, order[x], "white", 0)
        create_piece(x, 7, order[x], "black", 1)

    # Core Interaction Logic
    # 1. Any object with a gridX clicked -> store its coordinates in chess-state
    add_law("handle-click", "object-clicked", 0, 
        {"kind": 0, "path": "gridX", "op": 5, "operand": {"t": "int", "v": -100}}, # Ge -100
        {"kind": 5, "children": [
            {"kind": 8, "path": "@chess-state.targetX", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}]}}]}},
            {"kind": 8, "path": "@chess-state.targetY", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}]}}]}},
            # Compute dx and dy relative to selectedX, selectedY
            {"kind": 8, "path": "@chess-state.dx", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}, {"coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.selectedX"}]}]}}]}},
            {"kind": 8, "path": "@chess-state.dy", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}, {"coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.selectedY"}]}]}}]}},
            {"kind": 10, "event": "process-click", "subjectId": "@chess-state"}
        ]}
    )
    
    # 2. Select Piece (if click on our own piece)
    add_law("select-piece", "process-click", 0,
        {"kind": 3, "children": [
            {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
            {"kind": 0, "path": "gridX", "op": 0, "operandPath": "@chess-state.targetX"},
            {"kind": 0, "path": "gridY", "op": 0, "operandPath": "@chess-state.targetY"},
            {"kind": 0, "path": "chessColor", "op": 0, "operandPath": "@chess-state.turn"}
        ]},
        {"kind": 5, "children": [
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": True}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": True}},
            {"kind": 8, "path": "@chess-state.selectedX", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}]}}]}},
            {"kind": 8, "path": "@chess-state.selectedY", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}]}}]}},
            {"kind": 10, "event": "piece-selected"}
        ]}
    )
    
    # 3. Deselect Others
    add_law("deselect-others", "piece-selected", 0,
        {"kind": 3, "children": [
            {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
            {"kind": 5, "children": [ # Not
                {"kind": 8, "identityId": "@event.subject"}
            ]}
        ]},
        {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": False}}
    )
    
    # 4. Dummy Move Validation: Allow ANY move for selected piece if clicked on empty square or enemy
    add_law("validate-move-dummy", "process-click", 0,
        {"kind": 3, "children": [
            {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
            {"kind": 5, "children": [ # Not clicking on itself
                {"kind": 3, "children": [
                    {"kind": 0, "path": "gridX", "op": 0, "operandPath": "@chess-state.targetX"},
                    {"kind": 0, "path": "gridY", "op": 0, "operandPath": "@chess-state.targetY"}
                ]}
            ]},
            # Not clicking on our own piece
            {"kind": 5, "children": [
                {"kind": 9, "children": [ # ForAny
                    {"kind": 3, "children": [
                        {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
                        {"kind": 0, "path": "gridX", "op": 0, "operandPath": "@chess-state.targetX"},
                        {"kind": 0, "path": "gridY", "op": 0, "operandPath": "@chess-state.targetY"},
                        {"kind": 0, "path": "chessColor", "op": 0, "operandPath": "@chess-state.turn"}
                    ]}
                ]}
            ]}
        ]},
        {"kind": 10, "event": "execute-move"}
    )
    
    # 5. Execute Move
    add_law("execute-move", "execute-move", 0,
        {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
        {"kind": 5, "children": [
            {"kind": 8, "path": "gridX", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "@chess-state.targetX"}]}]}}]}},
            {"kind": 8, "path": "gridY", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "@chess-state.targetY"}]}]}}]}},
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "hasMoved", "operand": {"t": "bool", "v": True}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "@chess-state.selectedX", "operand": {"t": "int", "v": -1}},
            {"kind": 0, "path": "@chess-state.selectedY", "operand": {"t": "int", "v": -1}},
            {"kind": 8, "path": "@chess-state.turn", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": []}, {"coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.turn"}]}]}}]}}, # 1 - turn
            {"kind": 10, "event": "update-positions"},
            {"kind": 10, "event": "capture-enemy", "subjectId": "@chess-state"}
        ]}
    )
    
    # 6. Capture Enemy
    add_law("capture-enemy", "capture-enemy", 0,
        {"kind": 3, "children": [
            {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
            {"kind": 0, "path": "gridX", "op": 0, "operandPath": "@chess-state.targetX"},
            {"kind": 0, "path": "gridY", "op": 0, "operandPath": "@chess-state.targetY"},
            {"kind": 0, "path": "chessColor", "op": 0, "operandPath": "@chess-state.turn"} # Because turn was just flipped!
        ]},
        {"kind": 0, "path": "position.y", "operand": {"t": "double", "v": -100.0}} # Move off board instead of destroy to avoid nullptrs
    )
    
    # 7. Update Positions (Drive over time)
    # Actually, let's just snap position for now to keep it simple.
    add_law("update-positions", "update-positions", 0,
        {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
        {"kind": 5, "children": [
            {"kind": 8, "path": "position.x", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}, {"coefficient": -3.5, "factors": []}]}}]}},
            {"kind": 8, "path": "position.z", "function": {"pieces": [{"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}, {"coefficient": -3.5, "factors": []}]}}]}}
        ]}
    )
    
    # Highlight Selected Piece
    add_law("highlight-selected", "", 1, # WhileTrue
        {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": True}},
        {"kind": 0, "path": "faceColors[0].y", "operand": {"t": "double", "v": 1.0}} # Make it glow green?
    )
    # Remove Highlight when deselected
    add_law("remove-highlight-white", "", 1,
        {"kind": 3, "children": [
            {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 0}}
        ]},
        {"kind": 5, "children": [
            {"kind": 0, "path": "faceColors[0].x", "operand": {"t": "double", "v": 0.95}},
            {"kind": 0, "path": "faceColors[0].y", "operand": {"t": "double", "v": 0.95}},
            {"kind": 0, "path": "faceColors[0].z", "operand": {"t": "double", "v": 0.95}}
        ]}
    )
    add_law("remove-highlight-black", "", 1,
        {"kind": 3, "children": [
            {"kind": 0, "path": "isSelected", "op": 0, "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "chessColor", "op": 0, "operand": {"t": "int", "v": 1}}
        ]},
        {"kind": 5, "children": [
            {"kind": 0, "path": "faceColors[0].x", "operand": {"t": "double", "v": 0.2}},
            {"kind": 0, "path": "faceColors[0].y", "operand": {"t": "double", "v": 0.2}},
            {"kind": 0, "path": "faceColors[0].z", "operand": {"t": "double", "v": 0.2}}
        ]}
    )
    
    world["zones"][0]["world"]["objects"] = objects
    world["authoredLaws"]["laws"] = laws
    world["authoredLaws"]["triggers"] = triggers
    world["authoredLaws"]["formationMembers"] = formationMembers
    
    with open(OUT_SAVE_PATH, "w") as f:
        json.dump(world, f, indent=2)

if __name__ == "__main__":
    main()
