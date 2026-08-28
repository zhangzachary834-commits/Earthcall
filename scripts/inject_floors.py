import json
import glob
import os

for zone_path in glob.glob("saves/zones/*/zone.json"):
    with open(zone_path, "r") as f:
        data = json.load(f)
    
    # Check if there's already a floor
    has_floor = False
    for obj in data.get("objects", []):
        if obj.get("injected_by") == "Antigravity":
            # Fix it!
            obj["baseline"] = "ground"
            if "baseline" in obj.get("authoredProperties", {}):
                del obj["authoredProperties"]["baseline"]
            has_floor = True
            
    with open(zone_path, "w") as f:
        json.dump(data, f, indent=2)

print("Done.")
