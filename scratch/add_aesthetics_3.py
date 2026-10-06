import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

# Define materials
materials = data.get("materials", [])

materials.append({
    "id": "material.mtg.card.green",
    "colorExpr": {
        "input": "x",
        "pieces": [ { "mathNode": { "op": 2, "children": [
            {"op": 0, "scalarForm": {"terms": [{"c": 0.2}]}},
            {"op": 0, "scalarForm": {"terms": [{"c": 0.8}]}},
            {"op": 0, "scalarForm": {"terms": [{"c": 0.2}]}}
        ]}}]
    }
})

materials.append({
    "id": "material.mtg.card.red",
    "colorExpr": {
        "input": "x",
        "pieces": [ { "mathNode": { "op": 2, "children": [
            {"op": 0, "scalarForm": {"terms": [{"c": 0.9}]}},
            {"op": 0, "scalarForm": {"terms": [{"c": 0.2}]}},
            {"op": 0, "scalarForm": {"terms": [{"c": 0.2}]}}
        ]}}]
    }
})

data["materials"] = materials

# Update objects
for obj in data["objects"]:
    if "grizzly_bears" in obj["objectID"]:
        obj["materialId"] = "material.mtg.card.green"
    elif "lightning_bolt" in obj["objectID"]:
        obj["materialId"] = "material.mtg.card.red"

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Card materials customized!")
