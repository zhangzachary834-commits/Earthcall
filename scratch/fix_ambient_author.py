import json

zone_path = "saves/zones/Ambient Zone/zone.json"
with open(zone_path, "r") as f:
    zone = json.load(f)

# check if it already exists
found = False
for o in zone['world']['objects']:
    if o.get('objectID') == 'ambient.creator':
        found = True
        break

if not found:
    zone['world']['objects'].append({
        "objectID": "ambient.creator",
        "conceptIdentifier": "",
        "shapeKind": "box",
        "materialId": "default",
        "transform": [1.0, 0.0, 0.0, 0.0, 
                      0.0, 1.0, 0.0, 0.0, 
                      0.0, 0.0, 1.0, 0.0, 
                      0.0, 0.0, 0.0, 1.0],
        "x2D": 0.0,
        "y2D": 0.0,
        "zOrder2D": 0,
        "authoredProperties": {}
    })

with open(zone_path, "w") as f:
    json.dump(zone, f, indent=2)

print("Added ambient.creator object to Ambient Zone!")
