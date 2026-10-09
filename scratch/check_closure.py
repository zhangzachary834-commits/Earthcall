import json

zone_path = "saves/zones/Ambient Zone/zone.json"
zone = json.load(open(zone_path))

owned_objects = []
if "world" in zone and "objects" in zone["world"]:
    owned_objects = zone["world"]["objects"]
elif "objects" in zone:
    owned_objects = zone["objects"]

object_ids = set()
for o in owned_objects:
    if "objectID" in o:
        object_ids.add(o["objectID"])
    elif "id" in o:
        object_ids.add(o["id"])

print("Owned objects:", object_ids)

law_refs = zone.get("lawRefs", [])
print("Law refs:", law_refs)

for ref in law_refs:
    law_path = f"saves/laws/{ref}/law.json"
    try:
        with open(law_path) as f:
            root = json.load(f)
    except Exception as e:
        print(f"[{ref}] Error reading: {e}")
        continue
    
    if "identifier" not in root or root["identifier"] != ref:
        print(f"[{ref}] root identifier mismatch")
        continue
        
    if "law" not in root or not isinstance(root["law"], dict):
        print(f"[{ref}] no law object")
        continue
        
    law = root["law"]
    if "id" not in law or law["id"] != ref:
        print(f"[{ref}] law id mismatch")
        continue
        
    if "authors" not in law or not isinstance(law["authors"], list) or len(law["authors"]) == 0:
        print(f"[{ref}] no authors array")
        continue
        
    for a in law["authors"]:
        if not isinstance(a, str):
            print(f"[{ref}] author is not string")
            continue
        
        # Check if in objects
        if a not in object_ids:
            print(f"[{ref}] AUTHOR NOT FOUND IN ZONE: {a}")
            
    print(f"[{ref}] ALL OK!")

