import json

with open("saves/zones/BasicPixelChanger/zone.json", "r") as f:
    head_json = json.load(f)
    
import subprocess
subprocess.run(["git", "checkout", "2708bf4a", "--", "saves/zones/BasicPixelChanger/zone.json"])

with open("saves/zones/BasicPixelChanger/zone.json", "r") as f:
    gemini_json = json.load(f)

# The new spatialRoot from c1f31e78 (which is in head_json)
if "spatialRoot" in head_json:
    gemini_json["spatialRoot"] = head_json["spatialRoot"]

# Save it back
with open("saves/zones/BasicPixelChanger/zone.json", "w") as f:
    json.dump(gemini_json, f, indent=2)

print("Merged spatialRoot into Gemini Spark's zone.json!")
