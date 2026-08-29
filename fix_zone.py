import json

path = "saves/zones/Perlin Noise Floor Zone/zone.json"
with open(path, "r") as f:
    zone = json.load(f)

for obj in zone["world"]["objects"]:
    if obj["objectID"] == "perlin-ground-plane":
        obj["renderMode"] = 2

with open(path, "w") as f:
    json.dump(zone, f, indent=2)
print("Zone updated")
