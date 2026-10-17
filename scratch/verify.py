import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {})
print(f"Total laws: {len(laws)}")
for name in laws:
    print(f"- {name}")
