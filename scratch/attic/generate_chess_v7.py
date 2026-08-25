import json
import os

BASE_SAVE_PATH = "saves/worlds/my_second_world.json"
OUT_SAVE_PATH = "saves/worlds/chess.json"

def main():
    with open(BASE_SAVE_PATH, "r") as f:
        world = json.load(f)
    
    world["zones"][0]["world"]["objects"] = []
    world["zones"][0]["formationRelations"] = []
    world["authoredLaws"]["laws"] = []
    world["authoredLaws"]["triggers"] = {}
    world["authoredLaws"]["formationMembers"] = []
    
    world["authoredLaws"]["rete"]["alphaNodes"] = []
    world["authoredLaws"]["rete"]["betaNodes"] = []
    world["authoredLaws"]["rete"]["facts"] = []
    
    objects = []
    
    game_state = {
        "objectID": "chess-state",
        "shapeKind": 0,
        "shapeParams": [0.1, 0.1, 0.1],
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, -4.5, 0, 0, 1],
        "center": [-4.5, 0, 0],
        "faceColors": [[0.5,0.5,0.5]] * 6,
        "authoredProperties": {
            "turn": {"t": "int", "v": 0},
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
            "id": law_id, "name": name, "enabled": True, "authority": 0,
            "activation": activation, "scope": 1, "drives": False,
            "retrigger": 0, "conditionMode": "all", "authors": ["Player"],
            "conditionSubjects": [], "targets": [],
            "conditionModel": condition, "actionModel": action,
            "conditionDescriptions": [name], "actionDescriptions": [name],
            "provenance": [], "applicationLog": []
        }
        laws.append(law)
        formationMembers.append(law_id)
        if trigger:
            if law_id not in triggers: triggers[law_id] = []
            triggers[law_id].append(trigger)
            
    for y in range(8):
        for x in range(8):
            color = [0.9, 0.9, 0.8] if (x + y) % 2 == 1 else [0.4, 0.5, 0.4]
            sq = {
                "objectID": f"board-{x}-{y}",
                "shapeKind": 0, "shapeParams": [0.5, 0.05, 0.5],
                "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, x - 3.5, 0, y - 3.5, 1],
                "center": [x - 3.5, 0, y - 3.5], "faceColors": [color] * 6,
                "authoredProperties": {
                    "gridX": {"t": "int", "v": x},
                    "gridY": {"t": "int", "v": y},
                    "isSquare": {"t": "bool", "v": True}
                }
            }
            objects.append(sq)
            
    piece_roles = {"pawn": 0, "knight": 1, "bishop": 2, "rook": 3, "queen": 4, "king": 5}
    def create_piece(x, y, role, color_id, color_val):
        pid = f"piece-{color_id}-{role}-{x}-{y}"
        shape = 3
        if role == "pawn": shape = 3; params = [0.3, 0.4, 0.3]
        elif role == "rook": shape = 0; params = [0.35, 0.5, 0.35]
        elif role == "knight": shape = 2; params = [0.35, 0.35, 0.35]
        elif role == "bishop": shape = 4; params = [0.3, 0.6, 0.3]
        elif role == "queen": shape = 3; params = [0.4, 0.7, 0.4]
        elif role == "king": shape = 0; params = [0.4, 0.8, 0.4]
        color = [0.95, 0.95, 0.95] if color_id == "white" else [0.2, 0.2, 0.2]
        
        obj = {
            "objectID": pid, "shapeKind": shape, "shapeParams": params,
            "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, x - 3.5, params[1], y - 3.5, 1],
            "center": [x - 3.5, params[1], y - 3.5], "faceColors": [color] * 6,
            "authoredProperties": {
                "gridX": {"t": "int", "v": x}, "gridY": {"t": "int", "v": y},
                "chessRole": {"t": "int", "v": piece_roles[role]},
                "chessColor": {"t": "int", "v": color_val},
                "isSelected": {"t": "bool", "v": False},
                "hasMoved": {"t": "bool", "v": False}
            }
        }
        objects.append(obj)
        world["zones"][0]["formationRelations"].append({
            "type": "instance-of", "entityA": pid, "entityB": "category.chess.piece",
            "directed": True, "weight": 1.0, "events": []
        })

    for x in range(8):
        create_piece(x, 1, "pawn", "white", 0)
        create_piece(x, 6, "pawn", "black", 1)
        
    order = ["rook", "knight", "bishop", "queen", "king", "bishop", "knight", "rook"]
    for x in range(8):
        create_piece(x, 0, order[x], "white", 0)
        create_piece(x, 7, order[x], "black", 1)

    def Cmp(path, op, val):
        return {"kind": 0, "path": path, "op": op, "operand": {"t": "int", "v": val}}
    def CmpPath(path, op, path2):
        return {"kind": 0, "path": path, "op": op, "operandPath": path2}
    def All(*conds):
        return {"kind": 3, "children": list(conds)}
    def Any(*conds):
        return {"kind": 4, "children": list(conds)}
    def Not(cond):
        return {"kind": 5, "children": [cond]}
        
    def AbsEq(path, val):
        return Any(Cmp(path, 0, val), Cmp(path, 0, -val))
        
    is_piece = {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"}
    add_law("handle-click", "object-clicked", 0, 
        Any(Cmp("isBoard", 0, True), is_piece),
        {"kind": 5, "children": [
            {"kind": 8, "path": "@chess-state.targetX", "bindings": {"ptrX": "@interaction-channel.pointerWorld.x"}, "function": {"input": "ptrX", "pieces": [{"lo": {"t": "double", "v": -4.0}, "hi": {"t": "double", "v": -3.0}, "expr": {"sum": [{"c": 0.0, "factors": {}}]}}, {"lo": {"t": "double", "v": -3.0}, "hi": {"t": "double", "v": -2.0}, "expr": {"sum": [{"c": 1.0, "factors": {}}]}}, {"lo": {"t": "double", "v": -2.0}, "hi": {"t": "double", "v": -1.0}, "expr": {"sum": [{"c": 2.0, "factors": {}}]}}, {"lo": {"t": "double", "v": -1.0}, "hi": {"t": "double", "v": 0.0}, "expr": {"sum": [{"c": 3.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 0.0}, "hi": {"t": "double", "v": 1.0}, "expr": {"sum": [{"c": 4.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 1.0}, "hi": {"t": "double", "v": 2.0}, "expr": {"sum": [{"c": 5.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 2.0}, "hi": {"t": "double", "v": 3.0}, "expr": {"sum": [{"c": 6.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 3.0}, "hi": {"t": "double", "v": 4.0}, "expr": {"sum": [{"c": 7.0, "factors": {}}]}}]}},
            {"kind": 8, "path": "@chess-state.targetY", "bindings": {"ptrY": "@interaction-channel.pointerWorld.z"}, "function": {"input": "ptrY", "pieces": [{"lo": {"t": "double", "v": -4.0}, "hi": {"t": "double", "v": -3.0}, "expr": {"sum": [{"c": 0.0, "factors": {}}]}}, {"lo": {"t": "double", "v": -3.0}, "hi": {"t": "double", "v": -2.0}, "expr": {"sum": [{"c": 1.0, "factors": {}}]}}, {"lo": {"t": "double", "v": -2.0}, "hi": {"t": "double", "v": -1.0}, "expr": {"sum": [{"c": 2.0, "factors": {}}]}}, {"lo": {"t": "double", "v": -1.0}, "hi": {"t": "double", "v": 0.0}, "expr": {"sum": [{"c": 3.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 0.0}, "hi": {"t": "double", "v": 1.0}, "expr": {"sum": [{"c": 4.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 1.0}, "hi": {"t": "double", "v": 2.0}, "expr": {"sum": [{"c": 5.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 2.0}, "hi": {"t": "double", "v": 3.0}, "expr": {"sum": [{"c": 6.0, "factors": {}}]}}, {"lo": {"t": "double", "v": 3.0}, "hi": {"t": "double", "v": 4.0}, "expr": {"sum": [{"c": 7.0, "factors": {}}]}}]}},
            {"kind": 8, "path": "@chess-state.dx", "bindings": {"@chess-state.targetX": "@chess-state.targetX", "@chess-state.selectedX": "@chess-state.selectedX"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetX": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.dy", "bindings": {"@chess-state.targetY": "@chess-state.targetY", "@chess-state.selectedY": "@chess-state.selectedY"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetY": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedY": 1.0}}]}}]}},
            {"kind": 10, "eventType": "process-click", "publishSubject": "@chess-state", "publishObject": ""}
        ]}
    )
    
    add_law("select-piece", "process-click", 0,
        All(
            {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"},
            CmpPath("gridX", 0, "@chess-state.targetX"),
            CmpPath("gridY", 0, "@chess-state.targetY"),
            CmpPath("chessColor", 0, "@chess-state.turn")
        ),
        {"kind": 5, "children": [
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": True}},
            {"kind": 8, "path": "@chess-state.selectedX", "bindings": {"gridX": "gridX"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.selectedY", "bindings": {"gridY": "gridY"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}]}}]}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": True}},
            {"kind": 10, "eventType": "piece-selected", "publishSubject": "@event.subject", "publishObject": ""}
        ]}
    )
    
    add_law("deselect-others", "piece-selected", 0,
        All(
            Cmp("isSelected", 0, True),
            Not({"kind": 8, "identityId": "@event.subject"})
        ),
        {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": False}}
    )
    
    is_piece = {"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"}
    
    target_empty = Not({"kind": 9, "children": [All(is_piece, CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "@chess-state.targetY"))]})
    target_enemy = {"kind": 9, "children": [All(is_piece, CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "@chess-state.targetY"), CmpPath("chessColor", 1, "@chess-state.turn"))]}
    
    # MATHEMATICS DOCUMENTATION:
    # We use ConditionMode::Zone to perform algebraic multiplication!
    # Because 'Compare' nodes only support linear addition, we use 'Zone' (f <= 0) to check bounds and curves.
    # To check if an arbitrary square (gridX, gridY) lies ON the line segment between selected(X,Y) and target(X,Y):
    # We enforce (gridX - selectedX) * (gridX - targetX) <= 0.
    # Expanded: gridX^2 - gridX*targetX - gridX*selectedX + selectedX*targetX <= 0
    def on_segment():
        boundsX = {"kind": 6, "lo": {"t": "int", "v": -999999}, "hi": {"t": "int", "v": 0}, "function": {"pieces": [{"expr": {"sum": [
            { "c": 1.0, "factors": {"gridX": 2.0} },
            { "c": -1.0, "factors": {"gridX": 1.0, "@chess-state.targetX": 1.0} },
            { "c": -1.0, "factors": {"gridX": 1.0, "@chess-state.selectedX": 1.0} },
            { "c": 1.0, "factors": {"@chess-state.selectedX": 1.0, "@chess-state.targetX": 1.0} }
        ]}}]}}
        boundsY = {"kind": 6, "lo": {"t": "int", "v": -999999}, "hi": {"t": "int", "v": 0}, "function": {"pieces": [{"expr": {"sum": [
            { "c": 1.0, "factors": {"gridY": 2.0} },
            { "c": -1.0, "factors": {"gridY": 1.0, "@chess-state.targetY": 1.0} },
            { "c": -1.0, "factors": {"gridY": 1.0, "@chess-state.selectedY": 1.0} },
            { "c": 1.0, "factors": {"@chess-state.selectedY": 1.0, "@chess-state.targetY": 1.0} }
        ]}}]}}
        # To check if an arbitrary square (gridX, gridY) lies on the same exact vector line:
        # Cross product of (gridX - selectedX, gridY - selectedY) and (dx, dy) must be 0!
        line = {"kind": 6, "lo": {"t": "int", "v": 0}, "hi": {"t": "int", "v": 0}, "function": {"pieces": [{"expr": {"sum": [
            { "c": 1.0, "factors": {"gridX": 1.0, "@chess-state.dy": 1.0} },
            { "c": -1.0, "factors": {"@chess-state.selectedX": 1.0, "@chess-state.dy": 1.0} },
            { "c": -1.0, "factors": {"gridY": 1.0, "@chess-state.dx": 1.0} },
            { "c": 1.0, "factors": {"@chess-state.selectedY": 1.0, "@chess-state.dx": 1.0} }
        ]}}]}}
        not_start = Not(All(CmpPath("gridX", 0, "@chess-state.selectedX"), CmpPath("gridY", 0, "@chess-state.selectedY")))
        not_end = Not(All(CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "@chess-state.targetY")))
        return All(is_piece, boundsX, boundsY, line, not_start, not_end)
        
    path_clear = Not({"kind": 9, "children": [on_segment()]})
    
    valid_pawn_w = All(
        Cmp("chessColor", 0, 0),
        Any(
            All(CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "gridY"), Cmp("@chess-state.dy", 0, 1), target_empty),
            All(CmpPath("gridX", 0, "@chess-state.targetX"), Cmp("gridY", 0, 1), Cmp("@chess-state.dy", 0, 2), target_empty, path_clear),
            All(AbsEq("@chess-state.dx", 1), Cmp("@chess-state.dy", 0, 1), target_enemy)
        )
    )
    valid_pawn_b = All(
        Cmp("chessColor", 0, 1),
        Any(
            All(CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "gridY"), Cmp("@chess-state.dy", 0, -1), target_empty),
            All(CmpPath("gridX", 0, "@chess-state.targetX"), Cmp("gridY", 0, 6), Cmp("@chess-state.dy", 0, -2), target_empty, path_clear),
            All(AbsEq("@chess-state.dx", 1), Cmp("@chess-state.dy", 0, -1), target_enemy)
        )
    )
    valid_pawn = All(Cmp("chessRole", 0, 0), Any(valid_pawn_w, valid_pawn_b))
    
    valid_knight = All(Cmp("chessRole", 0, 1), Any(
        All(AbsEq("@chess-state.dx", 1), AbsEq("@chess-state.dy", 2)),
        All(AbsEq("@chess-state.dx", 2), AbsEq("@chess-state.dy", 1))
    ), Any(target_empty, target_enemy))
    
    valid_bishop = All(Cmp("chessRole", 0, 2), Any(
        {"kind": 6, "lo": 0, "hi": 0, "function": {"pieces": [{"value": {"sum": [
            { "coefficient": 1.0, "factors": [{"type": "var", "name": "@chess-state.dx"}, {"type": "var", "name": "@chess-state.dx"}] },
            { "coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.dy"}, {"type": "var", "name": "@chess-state.dy"}] }
        ]}}]}}
    ), Not(Cmp("@chess-state.dx", 0, 0)), Any(target_empty, target_enemy), path_clear)
    
    valid_rook = All(Cmp("chessRole", 0, 3), Any(
        All(Cmp("@chess-state.dx", 0, 0), Not(Cmp("@chess-state.dy", 0, 0))),
        All(Cmp("@chess-state.dy", 0, 0), Not(Cmp("@chess-state.dx", 0, 0)))
    ), Any(target_empty, target_enemy), path_clear)
    
    valid_queen = All(Cmp("chessRole", 0, 4), Any(
        Any(
            {"kind": 6, "lo": 0, "hi": 0, "function": {"pieces": [{"value": {"sum": [
                { "coefficient": 1.0, "factors": [{"type": "var", "name": "@chess-state.dx"}, {"type": "var", "name": "@chess-state.dx"}] },
                { "coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.dy"}, {"type": "var", "name": "@chess-state.dy"}] }
            ]}}]}},
            All(Cmp("@chess-state.dx", 0, 0), Not(Cmp("@chess-state.dy", 0, 0))),
            All(Cmp("@chess-state.dy", 0, 0), Not(Cmp("@chess-state.dx", 0, 0)))
        )
    ), Any(target_empty, target_enemy), path_clear)
    
    valid_king = All(Cmp("chessRole", 0, 5), Any(
        All(AbsEq("@chess-state.dx", 1), Cmp("@chess-state.dy", 0, 0)),
        All(AbsEq("@chess-state.dy", 1), Cmp("@chess-state.dx", 0, 0)),
        All(AbsEq("@chess-state.dx", 1), AbsEq("@chess-state.dy", 1))
    ), Any(target_empty, target_enemy))
    
    add_law("validate-move", "process-click", 0,
        All(
            Cmp("isSelected", 0, True),
            Not(All(CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "@chess-state.targetY"))),
            Not({"kind": 9, "children": [All(is_piece, CmpPath("gridX", 0, "@chess-state.targetX"), CmpPath("gridY", 0, "@chess-state.targetY"), CmpPath("chessColor", 0, "@chess-state.turn"))]}),
            Any(valid_pawn, valid_knight, valid_bishop, valid_rook, valid_queen, valid_king)
        ),
        {"kind": 10, "eventType": "execute-move", "publishSubject": "@event.subject", "publishObject": ""}
    )
    
    add_law("execute-move", "execute-move", 0,
        Cmp("isSelected", 0, True),
        {"kind": 5, "children": [
            {"kind": 10, "eventType": "capture-enemy", "publishSubject": "@chess-state", "publishObject": ""},
            {"kind": 8, "path": "gridX", "bindings": {"@chess-state.targetX": "@chess-state.targetX"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetX": 1.0}}]}}]}},
            {"kind": 8, "path": "gridY", "bindings": {"@chess-state.targetY": "@chess-state.targetY"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetY": 1.0}}]}}]}},
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "hasMoved", "operand": {"t": "bool", "v": True}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "@chess-state.selectedX", "operand": {"t": "int", "v": -1}},
            {"kind": 0, "path": "@chess-state.selectedY", "operand": {"t": "int", "v": -1}},
            {"kind": 8, "path": "@chess-state.turn", "bindings": {"@chess-state.turn": "@chess-state.turn"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {}}, {"c": -1.0, "factors": {"@chess-state.turn": 1.0}}]}}]}},
            {"kind": 10, "eventType": "update-positions", "publishSubject": "@event.subject", "publishObject": ""}
        ]}
    )
    
    add_law("capture-enemy", "capture-enemy", 0,
        All(
            is_piece,
            CmpPath("gridX", 0, "@chess-state.targetX"),
            CmpPath("gridY", 0, "@chess-state.targetY"),
            CmpPath("chessColor", 1, "@chess-state.turn")
        ),
        {"kind": 0, "path": "position.y", "operand": {"t": "double", "v": -100.0}} 
    )
    
    add_law("update-positions", "update-positions", 0,
        is_piece,
        {"kind": 5, "children": [
            {"kind": 8, "path": "position.x", "bindings": {"gridX": "gridX"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -3.5, "factors": {}}]}}]}},
            {"kind": 8, "path": "position.z", "bindings": {"gridY": "gridY"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -3.5, "factors": {}}]}}]}}
        ]}
    )
    
    add_law("highlight-selected", "", 1, 
        Cmp("isSelected", 0, True),
        {"kind": 0, "path": "faceColors[0].y", "operand": {"t": "double", "v": 1.0}} 
    )
    add_law("remove-highlight-white", "", 1,
        All(Cmp("isSelected", 0, False), Cmp("chessColor", 0, 0)),
        {"kind": 5, "children": [
            {"kind": 0, "path": "faceColors[0].x", "operand": {"t": "double", "v": 0.95}},
            {"kind": 0, "path": "faceColors[0].y", "operand": {"t": "double", "v": 0.95}},
            {"kind": 0, "path": "faceColors[0].z", "operand": {"t": "double", "v": 0.95}}
        ]}
    )
    add_law("remove-highlight-black", "", 1,
        All(Cmp("isSelected", 0, False), Cmp("chessColor", 0, 1)),
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
