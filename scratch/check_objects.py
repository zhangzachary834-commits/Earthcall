import json

path = "saves/zones/Ambient Zone/zone.json"
with open(path) as f:
    zone = json.load(f)

for obj in zone["world"]["objects"]:
    print(obj.get("objectID"), obj.get("authoredProperties"))

