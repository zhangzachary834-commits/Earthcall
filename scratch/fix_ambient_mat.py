import json

path = "saves/zones/Ambient Zone/zone.json"
with open(path, "r") as f:
    data = json.load(f)

if "materials" in data and len(data["materials"]) > 0:
    data["materials"][0]["name"] = "default"

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Fixed Ambient Zone material!")
