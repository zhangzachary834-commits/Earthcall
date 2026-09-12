import json
with open("saves/worlds/synthesis_studio_living.json", "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {}).get("laws", [])
while_true = [L for L in laws if L.get("activation") == 1]

def collect_kinds(node):
    if not node: return []
    out = [node.get("kind")]
    for child in node.get("children", []):
        out.extend(collect_kinds(child))
    return out

for i, L in enumerate(while_true):
    c = L.get("conditionModel", {})
    kinds = collect_kinds(c)
    print(f"Law {i}: {kinds}")

