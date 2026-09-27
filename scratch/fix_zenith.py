import json
import os

filepath = "saves/zones/Celestial Zenith/zone.json"
with open(filepath, "r") as f:
    j = json.load(f)

j["name"] = "Celestial Zenith"
j["identifier"] = "Celestial Zenith"

with open(filepath, "w") as f:
    json.dump(j, f, indent=2)

print("Fixed!")
