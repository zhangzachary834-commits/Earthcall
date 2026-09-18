import json

with open("saves/worlds/synthesis_studio.json", "r") as f:
    data = json.load(f)

for law in data["authoredLaws"]["laws"]:
    if law.get("id") == "law-art-stroke-draw":
        # The conditionModel has a children array. The 6th child is the Any node.
        # Let's find it safely.
        for child in law["conditionModel"]["children"]:
            if child.get("kind") == 4: # Any
                for cmp_node in child["children"]:
                    if cmp_node["operand"]["v"] == 6.0:
                        cmp_node["operand"]["v"] = 0.9
                    elif cmp_node["operand"]["v"] == -6.0:
                        cmp_node["operand"]["v"] = -0.9

with open("saves/worlds/synthesis_studio.json", "w") as f:
    json.dump(data, f, indent=2)
