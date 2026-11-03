import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

# Define materials
materials = data.get("materials", [])
# Card material (white/off-white)
materials.append({
    "id": "material.mtg.card",
    "colorExpr": {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 2, # VectorConstruct
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.95}]}}, # R
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.95}]}}, # G
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.9}]}}   # B
                    ]
                }
            }
        ]
    }
})

# Button material (green/blue)
materials.append({
    "id": "material.mtg.button",
    "colorExpr": {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.2}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.8}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.4}]}}
                    ]
                }
            }
        ]
    }
})

# Table material (dark brown wood color)
materials.append({
    "id": "material.mtg.table",
    "colorExpr": {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.3}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.15}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.05}]}}
                    ]
                }
            }
        ]
    }
})

data["materials"] = materials

# Update existing objects to use materials
for obj in data["objects"]:
    if obj["objectID"].startswith("card."):
        obj["materialId"] = "material.mtg.card"
    elif obj["objectID"].startswith("button."):
        obj["materialId"] = "material.mtg.button"

# Add a table object!
table = {
    "objectID": "object.mtg.table",
    "shapeKind": 0, # Box
    "geometryType": 0,
    "shapeParams": [15.0, 0.2, 10.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    "position": [0.0, 0.0, 0.0], # Under the cards
    "materialId": "material.mtg.table"
}
data["objects"].append(table)

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Aesthetics added!")
