import json

path = "saves/zones/Ambient Zone/zone.json"
with open(path, "r") as f:
    data = json.load(f)

data["identifier"] = "Ambient Zone"
if "spatialRoot" in data and "id" in data["spatialRoot"]:
    data["spatialRoot"]["id"] = "Ambient Zone_spatialRoot"

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Fixed Ambient Zone ID!")
