import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

order = 0
for obj in data.get("objects", []):
    obj_id = obj.get("objectID", "")
    if obj_id.startswith("card."):
        obj.setdefault("authoredProperties", {})
        obj["authoredProperties"]["mtg.zone"] = {"t": "string", "v": "hand.1"}
        obj["authoredProperties"]["mtg.handOrder"] = {"t": "number", "v": float(order)}
        order += 1

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Patched cards!")
