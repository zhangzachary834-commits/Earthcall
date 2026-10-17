import json

path = "saves/zones/MTG/zone.json"
with open(path, "r") as f:
    data = json.load(f)

for obj in data.get("objects", []):
    # If it's a card, make it a Cube (shapeKind: 0) instead of Cylinder (3)
    if obj["objectID"].startswith("card."):
        obj["shapeKind"] = 0
        obj["shapeParams"] = [0.3, 0.01, 0.4, 0,0,0, 0,0,0]
    
    # If it's a category, formation, or state... YEET it under the map
    if obj["objectID"].startswith("category.") or \
       obj["objectID"].startswith("mtg.formation.") or \
       obj["objectID"] == "state.mtg":
        obj["shapeKind"] = 0
        obj["transform"] = [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-100,0, 1]

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Patched shapes and YEETED categories to Y = -100")
