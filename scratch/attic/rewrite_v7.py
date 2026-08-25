import json

def rewrite():
    with open('scripts/generate_chess_v2.py', 'r') as f:
        code = f.read()

    squares_gen = """        # Board squares
        for x in range(8):
            for y in range(8):
                is_white = (x + y) % 2 != 0
                obj = create_object(f"square_{x}_{y}", "Square", 
                    (x - 3.5, y - 3.5, -0.5), # z = -0.5 to be under the pieces
                    (0.95, 0.95, 0.1), 
                    (1, 1, 1, 1) if is_white else (0.2, 0.2, 0.2, 1)
                )
                obj["properties"]["gridX"] = {"t": "int", "v": x}
                obj["properties"]["gridY"] = {"t": "int", "v": y}
                objects.append(obj)"""
    
    board_gen = """        # Chess Board
        board = create_object("chess_board", "Square",
            (0.0, -0.5, 0.0),
            (8.0, 0.1, 8.0),
            (1.0, 1.0, 1.0, 1.0)
        )
        # Wait, the board originally was placed at (x - 3.5, 0, y - 3.5)
        # So we should put it at (0, -0.5, 0) and scale (8, 0.1, 8)!
        # Let's fix that!
        board["transform"] = [1,0,0,0, 0,1,0,0, 0,0,1,0, 0, -0.5, 0, 1]
        board["center"] = [0, -0.5, 0]
        board["shapeParams"] = [4.0, 0.05, 4.0] # shapeParams are half-extents for Square!
        board["authoredProperties"]["isBoard"] = {"t": "bool", "v": True}
        objects.append(board)"""
    
    code = code.replace(squares_gen, board_gen)

    def get_mapping_pieces():
        pieces = []
        for i in range(8):
            pieces.append({
                "lo": {"t": "double", "v": float(i - 4)},
                "hi": {"t": "double", "v": float(i - 3)},
                "expr": {"sum": [{"c": float(i), "factors": {}}]}
            })
        return json.dumps(pieces)

    old_click = """    add_law("handle-click", "object-clicked", 0, 
        Cmp("gridX", 5, -100),
        {"kind": 5, "children": [
            {"kind": 8, "path": "@chess-state.targetX", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.targetY", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.dx", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.dy", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedY": 1.0}}]}}]}},
            {"kind": 10, "event": "process-click", "subjectId": "@chess-state"}
        ]}
    )"""

    new_click = f"""    is_piece = {{"kind": 2, "relationType": "instance-of", "otherId": "category.chess.piece"}}
    add_law("handle-click", "object-clicked", 0, 
        Any(Cmp("isBoard", 0, True), is_piece),
        {{"kind": 5, "children": [
            {{"kind": 8, "path": "@chess-state.targetX", "bindings": {{"ptrX": "@interaction-channel.pointerWorld.x"}}, "function": {{"input": "ptrX", "pieces": {get_mapping_pieces()}}}}},
            {{"kind": 8, "path": "@chess-state.targetY", "bindings": {{"ptrY": "@interaction-channel.pointerWorld.z"}}, "function": {{"input": "ptrY", "pieces": {get_mapping_pieces()}}}}},
            {{"kind": 8, "path": "@chess-state.dx", "bindings": {{"@chess-state.targetX": "@chess-state.targetX", "@chess-state.selectedX": "@chess-state.selectedX"}}, "function": {{"pieces": [{{"expr": {{"sum": [{{"c": 1.0, "factors": {{"@chess-state.targetX": 1.0}}}}, {{"c": -1.0, "factors": {{"@chess-state.selectedX": 1.0}}}}]}}}}]}}}},
            {{"kind": 8, "path": "@chess-state.dy", "bindings": {{"@chess-state.targetY": "@chess-state.targetY", "@chess-state.selectedY": "@chess-state.selectedY"}}, "function": {{"pieces": [{{"expr": {{"sum": [{{"c": 1.0, "factors": {{"@chess-state.targetY": 1.0}}}}, {{"c": -1.0, "factors": {{"@chess-state.selectedY": 1.0}}}}]}}}}]}}}},
            {{"kind": 10, "eventType": "process-click", "publishSubject": "@chess-state", "publishObject": ""}}
        ]}}
    )"""
    code = code.replace(old_click, new_click)

    old_select = """        {"kind": 5, "children": [
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": True}},
            {"kind": 8, "path": "@chess-state.selectedX", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.selectedY", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}]}}]}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": True}},
            {"kind": 10, "event": "piece-selected"}
        ]}"""
    new_select = """        {"kind": 5, "children": [
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": True}},
            {"kind": 8, "path": "@chess-state.selectedX", "bindings": {"gridX": "gridX"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.selectedY", "bindings": {"gridY": "gridY"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}]}}]}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": True}},
            {"kind": 10, "eventType": "piece-selected", "publishSubject": "@event.subject", "publishObject": ""}
        ]}"""
    code = code.replace(old_select, new_select)

    old_val = """        {"kind": 10, "event": "execute-move"}"""
    new_val = """        {"kind": 10, "eventType": "execute-move", "publishSubject": "@event.subject", "publishObject": ""}"""
    code = code.replace(old_val, new_val)

    old_exec = """        {"kind": 5, "children": [
            {"kind": 10, "event": "capture-enemy", "subjectId": "@chess-state"},
            {"kind": 8, "path": "gridX", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetX": 1.0}}]}}]}},
            {"kind": 8, "path": "gridY", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetY": 1.0}}]}}]}},
            {"kind": 0, "path": "isSelected", "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "hasMoved", "operand": {"t": "bool", "v": True}},
            {"kind": 0, "path": "@chess-state.selectionActive", "operand": {"t": "bool", "v": False}},
            {"kind": 0, "path": "@chess-state.selectedX", "operand": {"t": "int", "v": -1}},
            {"kind": 0, "path": "@chess-state.selectedY", "operand": {"t": "int", "v": -1}},
            {"kind": 8, "path": "@chess-state.turn", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {}}, {"c": -1.0, "factors": {"@chess-state.turn": 1.0}}]}}]}},
            {"kind": 10, "event": "update-positions"}
        ]}"""
    new_exec = """        {"kind": 5, "children": [
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
        ]}"""
    code = code.replace(old_exec, new_exec)

    old_up = """    add_law("update-positions", "update-positions", 0,
        is_piece,
        {"kind": 5, "children": [
            {"kind": 8, "path": "position.x", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -3.5, "factors": {}}]}}]}},
            {"kind": 8, "path": "position.z", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -3.5, "factors": {}}]}}]}}
        ]}
    )"""
    new_up = """    add_law("update-positions", "update-positions", 0,
        is_piece,
        {"kind": 5, "children": [
            {"kind": 8, "path": "position.x", "bindings": {"gridX": "gridX"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -3.5, "factors": {}}]}}]}},
            {"kind": 8, "path": "position.z", "bindings": {"gridY": "gridY"}, "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -3.5, "factors": {}}]}}]}}
        ]}
    )"""
    code = code.replace(old_up, new_up)

    with open('scratch/generate_chess_v7.py', 'w') as f:
        f.write(code)

rewrite()
