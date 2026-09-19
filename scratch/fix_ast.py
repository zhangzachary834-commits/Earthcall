import re
with open("scripts/generate_chess_v2.py", "r") as f:
    text = f.read()

text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}, {"coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.selectedX"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedX": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}, {"coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.selectedY"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -1.0, "factors": {"@chess-state.selectedY": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "@chess-state.targetX"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetX": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "@chess-state.targetY"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"@chess-state.targetY": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": []}, {"coefficient": -1.0, "factors": [{"type": "var", "name": "@chess-state.turn"}]}]}', '"expr": {"sum": [{"c": 1.0, "factors": {}}, {"c": -1.0, "factors": {"@chess-state.turn": 1.0}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridX"}]}, {"coefficient": -3.5, "factors": []}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"gridX": 1.0}}, {"c": -3.5, "factors": {}}]}')
text = text.replace('"value": {"sum": [{"coefficient": 1.0, "factors": [{"type": "var", "name": "gridY"}]}, {"coefficient": -3.5, "factors": []}]}', '"expr": {"sum": [{"c": 1.0, "factors": {"gridY": 1.0}}, {"c": -3.5, "factors": {}}]}')

with open("scripts/generate_chess_v2.py", "w") as f:
    f.write(text)
