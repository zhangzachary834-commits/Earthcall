import json

def get_mapping_pieces():
    pieces = []
    # board is at x,y = 0, size = 8.
    # so x runs from -4 to 4, y runs from -4 to 4
    # gridX = 0 => x in [-4, -3], gridX = 1 => [-3, -2], ...
    for i in range(8):
        pieces.append({
            "lo": {"t": "double", "v": float(i - 4)},
            "hi": {"t": "double", "v": float(i - 3)},
            "expr": {"sum": [{"c": float(i), "factors": {}}]}
        })
    return pieces

def rewrite():
    with open('scripts/generate_chess_v2.py', 'r') as f:
        code = f.read()
    
    # 1. Replace 64 squares with 1 board
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
            (0.0, 0.0, -0.5),
            (8.0, 8.0, 0.1),
            (1.0, 1.0, 1.0, 1.0)
        )
        board["properties"]["isBoard"] = {"t": "bool", "v": True}
        # In a real engine we would set faceTexture here to a checkerboard texture
        objects.append(board)"""
    
    code = code.replace(squares_gen, board_gen)

    # 2. Fix the click law to use bindings and Piecewise mapping
    old_click = """    add_law("handle-click", "object-clicked", 0, 
        Cmp("gridX", 5, -100),
        {"kind": 5, "children": [
            {"kind": 8, "path": "@chess-state.targetX", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.targetY", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.dx", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedX": 1.0}}]}}]}},
            {"kind": 8, "path": "@chess-state.dy", "function": {"pieces": [{"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedY": 1.0}}]}}]}},
            {"kind": 10, "eventType": "process-click", "publishSubject": "@chess-state", "publishObject": ""}
        ]}
    )"""
            
    new_click = f"""    add_law("handle-click", "object-clicked", 0, 
        Cmp("isBoard", 0, True),
        {{"kind": 5, "children": [
            {{"kind": 8, "path": "@chess-state.targetX", "bindings": {{"ptrX": "@interaction-channel.pointerWorld.x"}}, "function": {{"input": "ptrX", "pieces": {json.dumps(get_mapping_pieces())}}}}},
            {{"kind": 8, "path": "@chess-state.targetY", "bindings": {{"ptrY": "@interaction-channel.pointerWorld.y"}}, "function": {{"input": "ptrY", "pieces": {json.dumps(get_mapping_pieces())}}}}},
            {{"kind": 8, "path": "@chess-state.dx", "function": {{"pieces": [{{"expr": {{"sum": [{{"c": 1.0, "factors": {{"@chess-state.targetX": 1.0}}}}, {{"c": -1.0, "factors": {{"@chess-state.selectedX": 1.0}}}}]}}}}]}}}},
            {{"kind": 8, "path": "@chess-state.dy", "function": {{"pieces": [{{"expr": {{"sum": [{{"c": 1.0, "factors": {{"@chess-state.targetY": 1.0}}}}, {{"c": -1.0, "factors": {{"@chess-state.selectedY": 1.0}}}}]}}}}]}}}},
            {{"kind": 10, "eventType": "process-click", "publishSubject": "@chess-state", "publishObject": ""}}
        ]}}
    )"""
            
    code = code.replace(old_click, new_click)
    
    with open('scripts/generate_chess_v3.py', 'w') as f:
        f.write(code)

rewrite()
