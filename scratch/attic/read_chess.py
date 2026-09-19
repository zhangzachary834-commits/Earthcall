import json
with open("saves/worlds/chess.json", "r") as f: data = json.load(f)
for law in data["authoredLaws"]["laws"]:
    print(f"Law: {law['name']} (id: {law['id']}) activation: {law['activation']}")
