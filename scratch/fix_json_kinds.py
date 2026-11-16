import json
import glob

condition_kinds = {
    "Compare": 0, "InRegion": 1, "Related": 2, "All": 3, "Any": 4, "Not": 5
}
action_kinds = {
    "Set": 0, "Add": 1, "Scale": 2, "Lerp": 3, "Drive": 4, "Sequence": 5, "Parallel": 6,
    "Spawn": 7, "Map": 8, "Flow": 9, "Publish": 10, "Destroy": 11, "Relate": 12,
    "Create": 13, "AddRelation": 14, "AuthorZone": 15, "CreateMaterial": 16,
    "SetVolume": 17, "PlayAudio": 18, "WritePixel": 19, "ElevatePixels": 20,
    "FileRead": 23, "FileWrite": 24, "CodecTransform": 25
}

def fix_conditions(conds):
    for c in conds:
        if "kind" in c and isinstance(c["kind"], str):
            c["kind"] = condition_kinds.get(c["kind"], c["kind"])
        if "children" in c:
            fix_conditions(c["children"])

def fix_action(act):
    if "kind" in act and isinstance(act["kind"], str):
        act["kind"] = action_kinds.get(act["kind"], act["kind"])
    if "children" in act:
        for c in act["children"]:
            fix_action(c)

for filepath in glob.glob("saves/laws/*/law.json"):
    with open(filepath, "r") as f:
        data = json.load(f)
    
    if "conditions" in data:
        fix_conditions(data["conditions"])
    if "actionModel" in data:
        fix_action(data["actionModel"])
        
    with open(filepath, "w") as f:
        json.dump(data, f, indent=4)

print("Fixed kinds in all JSON laws.")
