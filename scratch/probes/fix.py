import json
import os

with open("saves/worlds/basic_2d_button.json", "r") as f:
    data = json.load(f)

# The fix: Make name identical to identifier so Zone::getIdentifier() == identifier
data["zones"][0]["name"] = "Basic2DButtonZone"

with open("saves/worlds/basic_2d_button.json", "w") as f:
    json.dump(data, f, indent=2)

print("Fixed JSON.")
