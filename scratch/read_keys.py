import json
with open("saves/worlds/chess.json", "r") as f: data = json.load(f)
print(list(data.keys()))
