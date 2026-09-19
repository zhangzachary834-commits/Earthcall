import json

with open("saves/worlds/donut chaos.json", "r") as f:
    data = json.load(f)

for obj in data.get("objects", []):
    obj["renderMode"] = 2  # Mesh

with open("saves/worlds/donut chaos_meshed.json", "w") as f:
    json.dump(data, f, indent=2)
