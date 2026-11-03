import json
import os

for voice in ["root", "fifth", "ninth"]:
    path = f"saves/laws/law-ambient-pan-{voice}/law.json"
    if not os.path.exists(path):
        continue
    with open(path, "r") as f:
        data = json.load(f)
    
    law = data.get("law", {})
    if "lawId" in law:
        law["id"] = law.pop("lawId")
    
    # Needs to match the 'ambient.creator' object we just added to the zone
    law["authors"] = ["ambient.creator"]
    
    root_data = {
        "identifier": f"law-ambient-pan-{voice}",
        "authors": ["ambient.creator"],
        "law": law
    }
    
    with open(path, "w") as f:
        json.dump(root_data, f, indent=4)

print("Fixed law JSON schemas!")
