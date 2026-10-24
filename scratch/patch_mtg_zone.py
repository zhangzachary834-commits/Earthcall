import json
import os

path = "saves/zones/MTG/zone.json"
with open(path, "r") as f:
    data = json.load(f)

data["identifier"] = "MTG"
data["name"] = "Magic: The Gathering"

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Added identifier and name to", path)
