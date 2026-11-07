import json

path = "saves/zones/Ambient Zone/zone.json"
with open(path, "r") as f:
    data = json.load(f)

if "lawRefs" not in data:
    data["lawRefs"] = []

if "law-ambient-pan" not in data["lawRefs"]:
    data["lawRefs"].append("law-ambient-pan")

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Added lawRefs to zone.json")
